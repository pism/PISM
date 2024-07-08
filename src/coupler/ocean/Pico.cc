// Copyright (C) 2012-2019, 2021, 2022, 2023, 2024 Constantine Khrulev, Ricarda Winkelmann, Ronja Reese, Torsten
// Albrecht, and Matthias Mengel
//
// This file is part of PISM.
//
// PISM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// PISM is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License
// along with PISM; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
// Please cite this model as:
// 1.
// Antarctic sub-shelf melt rates via PICO
// R. Reese, T. Albrecht, M. Mengel, X. Asay-Davis and R. Winkelmann
// The Cryosphere, 12, 1969-1985, (2018)
// DOI: 10.5194/tc-12-1969-2018
//
// 2.
// A box model of circulation and melting in ice shelf caverns
// D. Olbers & H. Hellmer
// Ocean Dynamics (2010), Volume 60, Issue 1, pp 141–153
// DOI: 10.1007/s10236-009-0252-z

#include <gsl/gsl_math.h> // GSL_NAN

#include "pism/coupler/util/options.hh"
#include "pism/geometry/Geometry.hh"
#include "pism/util/ConfigInterface.hh"
#include "pism/util/Grid.hh"
#include "pism/util/Mask.hh"
#include "pism/util/Time.hh"

#include "pism/coupler/ocean/Pico.hh"
#include "pism/coupler/ocean/PicoGeometry.hh"
#include "pism/coupler/ocean/PicoPhysics.hh"
#include "pism/util/array/Forcing.hh"

namespace pism {
namespace ocean {

Pico::Pico(std::shared_ptr<const Grid> grid)
  : CompleteOceanModel(grid, std::shared_ptr<OceanModel>()),
    m_Soc(m_grid, "pico_salinity"),
    m_Soc_box0(m_grid, "pico_salinity_box0"),
    m_Toc(m_grid, "pico_temperature"),
    m_Toc_box0(m_grid, "pico_temperature_box0"),
    m_T_star(m_grid, "pico_T_star"),
    m_overturning(m_grid, "pico_overturning"),
    m_basal_melt_rate(m_grid, "pico_basal_melt_rate"),
    m_geometry(grid),
    m_n_basins(0),
    m_n_boxes(0),
    m_n_shelves(0) {

  ForcingOptions opt(*m_grid->ctx(), "ocean.pico");

  {
    auto buffer_size = static_cast<int>(m_config->get_number("input.forcing.buffer_size"));

    File file(m_grid->com, opt.filename, io::PISM_NETCDF3, io::PISM_READONLY);

    m_theta_ocean = std::make_shared<array::Forcing>(m_grid,
                                                file,
                                                "theta_ocean",
                                                "", // no standard name
                                                buffer_size,
                                                opt.periodic,
                                                LINEAR);

    m_salinity_ocean = std::make_shared<array::Forcing>(m_grid,
                                                   file,
                                                   "salinity_ocean",
                                                   "", // no standard name
                                                   buffer_size,
                                                   opt.periodic,
                                                   LINEAR);
  }

  m_theta_ocean->metadata(0)
      .long_name("potential temperature of the adjacent ocean")
      .units("kelvin");

  m_salinity_ocean->metadata(0)
      .long_name("salinity of the adjacent ocean")
      .units("g/kg");

  // computed salinity in ocean boxes
  m_Soc.metadata(0).long_name("ocean salinity field").units("g/kg");
  m_Soc.metadata()["_FillValue"] = { 0.0 };

  // salinity input for box 1
  m_Soc_box0.metadata(0).long_name("ocean base salinity field").units("g/kg");
  m_Soc_box0.metadata()["_FillValue"] = { 0.0 };

  // computed temperature in ocean boxes
  m_Toc.metadata(0).long_name("ocean temperature field").units("kelvin");
  m_Toc.metadata()["_FillValue"] = { 0.0 };

  // temperature input for box 1
  m_Toc_box0.metadata(0).long_name("ocean base temperature").units("kelvin");
  m_Toc_box0.metadata()["_FillValue"] = { 0.0 };

  m_T_star.metadata(0).long_name("T_star field").units("degree_Celsius");
  m_T_star.metadata()["_FillValue"] = { 0.0 };

  m_overturning.metadata(0).long_name("cavity overturning").units("m^3 s^-1");
  m_overturning.metadata()["_FillValue"] = { 0.0 };

  m_basal_melt_rate.metadata(0)
      .long_name("PICO sub-shelf melt rate")
      .units("m s^-1")
      .output_units("m year^-1");
  m_basal_melt_rate.metadata()["_FillValue"] = {0.0};

  m_shelf_base_temperature->metadata()["_FillValue"] = {0.0};

  m_n_boxes  = static_cast<int>(m_config->get_number("ocean.pico.number_of_boxes"));

  {
    auto climate_index_file = m_config->get_string("climate_index.file");
    auto climate_snapshots  = m_config->get_string("ocean.climate_index.climate_snapshots.file");
    if (not climate_index_file.empty() and not climate_snapshots.empty()) {
      m_climate_index_forcing.reset(new ClimateIndex(grid));
    } else {
      m_climate_index_forcing = nullptr;
    }
  }
}

void Pico::init_impl(const Geometry &geometry) {
  (void) geometry;

  m_log->message(2, "* Initializing the Potsdam Ice-shelf Cavity mOdel for the ocean ...\n");

  ForcingOptions opt(*m_grid->ctx(), "ocean.pico");

  m_theta_ocean->init(opt.filename, opt.periodic);
  m_salinity_ocean->init(opt.filename, opt.periodic);

  if ((bool)m_climate_index_forcing) {
    m_climate_index_forcing->init_forcing();
  }

  // This initializes the basin_mask
  m_geometry.init();

  // FIXME: m_n_basins is a misnomer
  m_n_basins = static_cast<int>(max(m_geometry.basin_mask())) + 1;

  m_log->message(4, "PICO basin min=%f, max=%f\n",
                 min(m_geometry.basin_mask()),
                 max(m_geometry.basin_mask()));

  PicoPhysics physics(*m_config);

  m_log->message(2, "  -Using %d drainage basins and values: \n"
                    "   gamma_T= %.2e, overturning_coeff = %.2e... \n",
                 m_n_basins - 1, physics.gamma_T(), physics.overturning_coeff());

  m_log->message(2, "  -Depth of continental shelf for computation of temperature and salinity input\n"
                    "   is set for whole domain to continental_shelf_depth=%.0f meter\n",
                 physics.continental_shelf_depth());

  // read time-independent data right away:
  if (m_theta_ocean->buffer_size() == 1 and m_salinity_ocean->buffer_size() == 1) {
    m_theta_ocean->update(time().current(), 0.0);
    m_salinity_ocean->update(time().current(), 0.0);
  }

  double
    ice_density   = m_config->get_number("constants.ice.density"),
    water_density = m_config->get_number("constants.sea_water.density"),
    g             = m_config->get_number("constants.standard_gravity");

  compute_average_water_column_pressure(geometry, ice_density, water_density, g,
                                        *m_water_column_pressure);
}

void Pico::define_model_state_impl(const File &output) const {

  m_geometry.basin_mask().define(output, io::PISM_DOUBLE);
  m_Soc_box0.define(output, io::PISM_DOUBLE);
  m_Toc_box0.define(output, io::PISM_DOUBLE);
  m_overturning.define(output, io::PISM_DOUBLE);

  OceanModel::define_model_state_impl(output);
}

void Pico::write_model_state_impl(const File &output) const {

  m_geometry.basin_mask().write(output);
  m_Soc_box0.write(output);
  m_Toc_box0.write(output);
  m_overturning.write(output);

  OceanModel::write_model_state_impl(output);
}

/*!
* Extend basal melt rates to grounded and ocean neighbors for consitency with subgl_melt.
* Note that melt rates are then simply interpolated into partially floating cells, they
* are not included in the calculations of PICO.
*/
static void extend_basal_melt_rates(const array::CellType1 &cell_type,
                                    array::Scalar1 &basal_melt_rate) {

  auto grid = basal_melt_rate.grid();

  // update ghosts of the basal melt rate so that we can use basal_melt_rate.box(i,j)
  // below
  basal_melt_rate.update_ghosts();

  array::AccessScope list{&cell_type, &basal_melt_rate};

  for (auto p = grid->points(); p; p.next()) {

    const int i = p.i(), j = p.j();

    auto M = cell_type.box(i, j);

    bool potential_partially_filled_cell =
      ((M.c  == MASK_GROUNDED or M.c  == MASK_ICE_FREE_OCEAN) and
       (M.w  == MASK_FLOATING or M.e  == MASK_FLOATING or
        M.s  == MASK_FLOATING or M.n  == MASK_FLOATING or
        M.sw == MASK_FLOATING or M.nw == MASK_FLOATING or
        M.se == MASK_FLOATING or M.ne == MASK_FLOATING));

    if (potential_partially_filled_cell) {
      auto BMR = basal_melt_rate.box(i, j);

      int N = 0;
      double melt_sum = 0.0;

      melt_sum += M.nw == MASK_FLOATING ? (++N, BMR.nw) : 0.0;
      melt_sum += M.n  == MASK_FLOATING ? (++N, BMR.n)  : 0.0;
      melt_sum += M.ne == MASK_FLOATING ? (++N, BMR.ne) : 0.0;
      melt_sum += M.e  == MASK_FLOATING ? (++N, BMR.e)  : 0.0;
      melt_sum += M.se == MASK_FLOATING ? (++N, BMR.se) : 0.0;
      melt_sum += M.s  == MASK_FLOATING ? (++N, BMR.s)  : 0.0;
      melt_sum += M.sw == MASK_FLOATING ? (++N, BMR.sw) : 0.0;
      melt_sum += M.w  == MASK_FLOATING ? (++N, BMR.w)  : 0.0;

      if (N != 0) { // If there are floating neigbors, return average melt rates
        basal_melt_rate(i, j) = melt_sum / N;
      }
    }
  } // end of the loop over grid points
}

void Pico::update_impl(const Geometry &geometry, double t, double dt) {

  if ((bool)m_climate_index_forcing) {
    m_climate_index_forcing->update_forcing(t, dt, *m_theta_ocean, *m_salinity_ocean);
  } else {
    m_theta_ocean->update(t, dt);
    m_salinity_ocean->update(t, dt);
    
    m_theta_ocean->average(t, dt);
    m_salinity_ocean->average(t, dt);
  }

  // set values that will be used outside of floating ice areas
  {
    double T_fill_value   = m_config->get_number("constants.fresh_water.melting_point_temperature");
    double Toc_fill_value = m_Toc.metadata().get_number("_FillValue");
    double Soc_fill_value = m_Soc.metadata().get_number("_FillValue");
    double M_fill_value   = m_basal_melt_rate.metadata().get_number("_FillValue");
    double O_fill_value   = m_overturning.metadata().get_number("_FillValue");

    m_shelf_base_temperature->set(T_fill_value);
    m_basal_melt_rate.set(M_fill_value);
    m_Toc.set(Toc_fill_value);
    m_Soc.set(Soc_fill_value);
    m_overturning.set(O_fill_value);
    m_T_star.set(Toc_fill_value);
  }

  PicoPhysics physics(*m_config);

  const array::Scalar &ice_thickness    = geometry.ice_thickness;
  const auto &cell_type = geometry.cell_type;
  const array::Scalar &bed_elevation    = geometry.bed_elevation;

  // Geometric part of PICO
  m_geometry.update(bed_elevation, cell_type);

  // FIXME: m_n_shelves is not really the number of shelves.
  m_n_shelves = static_cast<int>(max(m_geometry.ice_shelf_mask())) + 1;

  // Physical part of PICO
  {

    // prepare ocean input temperature and salinity
    {
      std::vector<double> basin_temperature(m_n_basins);
      std::vector<double> basin_salinity(m_n_basins);

      compute_ocean_input_per_basin(physics,
                                    m_geometry.basin_mask(),
                                    m_geometry.continental_shelf_mask(),
                                    *m_salinity_ocean,
                                    *m_theta_ocean,
                                    basin_temperature,
                                    basin_salinity); // per basin

      set_ocean_input_fields(physics,
                             ice_thickness,
                             cell_type,
                             m_geometry.basin_mask(),
                             m_geometry.ice_shelf_mask(),
                             basin_temperature,
                             basin_salinity,
                             m_Toc_box0,
                             m_Soc_box0); // per shelf
    }

    // Use the Beckmann-Goosse parameterization to set reasonable values throughout the
    // domain.
    beckmann_goosse(physics,
                    ice_thickness,                // input
                    m_geometry.ice_shelf_mask(), // input
                    cell_type,                    // input
                    m_Toc_box0,                   // input
                    m_Soc_box0,                   // input
                    m_basal_melt_rate,
                    *m_shelf_base_temperature,
                    m_Toc,
                    m_Soc);

    // In ice shelves, replace Beckmann-Goosse values using the Olbers and Hellmer model.
    process_box1(physics,
                 ice_thickness,                             // input
                 m_geometry.ice_shelf_mask(),               // input
                 m_geometry.box_mask(),                     // input
                 m_Toc_box0,                                // input
                 m_Soc_box0,                                // input
                 m_basal_melt_rate,
                 *m_shelf_base_temperature,
                 m_T_star,
                 m_Toc,
                 m_Soc,
                 m_overturning);

    process_other_boxes(physics,
                        ice_thickness,               // input
                        m_geometry.ice_shelf_mask(), // input
                        m_geometry.box_mask(),       // input
                        m_basal_melt_rate,
                        *m_shelf_base_temperature,
                        m_T_star,
                        m_Toc,
                        m_Soc);
  }

  extend_basal_melt_rates(cell_type, m_basal_melt_rate);

  m_shelf_base_mass_flux->copy_from(m_basal_melt_rate);
  m_shelf_base_mass_flux->scale(physics.ice_density());

  double
    ice_density   = m_config->get_number("constants.ice.density"),
    water_density = m_config->get_number("constants.sea_water.density"),
    g             = m_config->get_number("constants.standard_gravity");

  compute_average_water_column_pressure(geometry, ice_density, water_density, g,
                                        *m_water_column_pressure);
}


MaxTimestep Pico::max_timestep_impl(double t) const {
  (void) t;

  return MaxTimestep("ocean pico");
}


//! Compute temperature and salinity input from ocean data by averaging.

//! We average the ocean data over the continental shelf reagion for each basin.
//! We use dummy ocean data if no such average can be calculated.


void Pico::compute_ocean_input_per_basin(const PicoPhysics &physics,
                                         const array::Scalar &basin_mask,
                                         const array::Scalar &continental_shelf_mask,
                                         const array::Scalar &salinity_ocean,
                                         const array::Scalar &theta_ocean,
                                         std::vector<double> &temperature,
                                         std::vector<double> &salinity) const {
  std::vector<int> count(m_n_basins, 0);
  // additional vectors to allreduce efficiently with IntelMPI
  std::vector<int> countr(m_n_basins, 0);
  std::vector<double> salinityr(m_n_basins);
  std::vector<double> temperaturer(m_n_basins);

  temperature.resize(m_n_basins);
  salinity.resize(m_n_basins);
  for (int basin_id = 0; basin_id < m_n_basins; basin_id++) {
    temperature[basin_id] = 0.0;
    salinity[basin_id]    = 0.0;
  }

  array::AccessScope list{ &theta_ocean, &salinity_ocean, &basin_mask, &continental_shelf_mask };

  // compute the sum for each basin for region that intersects with the continental shelf
  // area and is not covered by an ice shelf. (continental shelf mask excludes ice shelf
  // areas)
  for (auto p = m_grid->points(); p; p.next()) {
    const int i = p.i(), j = p.j();

    if (continental_shelf_mask.as_int(i, j) == 2) {
      int basin_id = basin_mask.as_int(i, j);

      count[basin_id] += 1;
      salinity[basin_id] += salinity_ocean(i, j);
      temperature[basin_id] += theta_ocean(i, j);
    }
  }

  // Divide by number of grid cells if more than zero cells belong to the basin. if no
  // ocean_contshelf_mask values intersect with the basin, count is zero. In such case,
  // use dummy temperature and salinity. This could happen, for example, if the ice shelf
  // front advances beyond the continental shelf break.
  GlobalSum(m_grid->com, count.data(), countr.data(), m_n_basins);
  GlobalSum(m_grid->com, salinity.data(), salinityr.data(), m_n_basins);
  GlobalSum(m_grid->com, temperature.data(), temperaturer.data(), m_n_basins);

  // copy values
  count       = countr;
  salinity    = salinityr;
  temperature = temperaturer;

  // "dummy" basin
  {
    temperature[0] = physics.T_dummy();
    salinity[0]    = physics.S_dummy();
  }

  for (int basin_id = 1; basin_id < m_n_basins; basin_id++) {

    if (count[basin_id] != 0) {
      salinity[basin_id] /= count[basin_id];
      temperature[basin_id] /= count[basin_id];

      m_log->message(5, "  %d: temp =%.3f, salinity=%.3f\n", basin_id, temperature[basin_id], salinity[basin_id]);
    } else {
      m_log->message(
          2,
          "PICO WARNING: basin %d contains no cells with ocean data on continental shelf\n"
          "              (no values with ocean_contshelf_mask=2).\n"
          "              Using default temperature (%.3f K) and salinity (%.3f g/kg)\n"
          "              since mean salinity and temperature cannot be computed.\n"
          "              This may bias the basal melt rate estimate.\n"
          "              Please check your input data.\n",
          basin_id, physics.T_dummy(), physics.S_dummy());

      temperature[basin_id] = physics.T_dummy();
      salinity[basin_id]    = physics.S_dummy();
    }
  }
}

//! Set ocean ocean input from box 0 as boundary condition for box 1.

//! Set ocean temperature and salinity (Toc_box0, Soc_box0)
//! from box 0 (in front of the ice shelf) as inputs for
//! box 1, which is the ocean box adjacent to the grounding line.
//!
//! We enforce that Toc_box0 is always at least the local pressure melting point.
void Pico::set_ocean_input_fields(const PicoPhysics &physics,
                                  const array::Scalar &ice_thickness,
                                  const array::CellType1 &mask,
                                  const array::Scalar &basin_mask,
                                  const array::Scalar &shelf_mask,
                                  const std::vector<double> &basin_temperature,
                                  const std::vector<double> &basin_salinity,
                                  array::Scalar &Toc_box0,
                                  array::Scalar &Soc_box0) const {
  
  array::AccessScope list{ &ice_thickness, &basin_mask, &Soc_box0, &Toc_box0, &mask, &shelf_mask };

  std::vector<int> n_shelf_cells_per_basin(m_n_shelves * m_n_basins, 0);
  std::vector<int> n_shelf_cells(m_n_shelves, 0);
  std::vector<int> cfs_in_basins_per_shelf(m_n_shelves * m_n_basins, 0);
  // additional vectors to allreduce efficiently with IntelMPI
  std::vector<int> n_shelf_cells_per_basinr(m_n_shelves * m_n_basins, 0);
  std::vector<int> n_shelf_cellsr(m_n_shelves, 0);
  std::vector<int> cfs_in_basins_per_shelfr(m_n_shelves * m_n_basins, 0);

  // 1) count the number of cells in each shelf
  // 2) count the number of cells in the intersection of each shelf with all the basins
  {
    for (auto p = m_grid->points(); p; p.next()) {
      const int i = p.i(), j = p.j();
      int s = shelf_mask.as_int(i, j);
      int b = basin_mask.as_int(i, j);
      n_shelf_cells_per_basin[s*m_n_basins+b]++;
      n_shelf_cells[s]++;

      // find all basins b, in which the ice shelf s has a calving front with potential ocean water intrusion
      if (mask.as_int(i, j) == MASK_FLOATING) {
        auto M = mask.star(i, j);
        if (M.n == MASK_ICE_FREE_OCEAN or
            M.e == MASK_ICE_FREE_OCEAN or
            M.s == MASK_ICE_FREE_OCEAN or
            M.w == MASK_ICE_FREE_OCEAN) {
          if (cfs_in_basins_per_shelf[s * m_n_basins + b] != b) {
            cfs_in_basins_per_shelf[s * m_n_basins + b] = b;
          }
        }
      }
    }
    
    GlobalSum(m_grid->com, n_shelf_cells.data(),
              n_shelf_cellsr.data(), m_n_shelves);
    GlobalSum(m_grid->com, n_shelf_cells_per_basin.data(),
              n_shelf_cells_per_basinr.data(), m_n_shelves*m_n_basins);
    GlobalSum(m_grid->com, cfs_in_basins_per_shelf.data(),
              cfs_in_basins_per_shelfr.data(), m_n_shelves*m_n_basins);
    // copy data
    n_shelf_cells = n_shelf_cellsr;
    n_shelf_cells_per_basin = n_shelf_cells_per_basinr;
    cfs_in_basins_per_shelf = cfs_in_basins_per_shelfr;

    for (int s = 0; s < m_n_shelves; s++) {
      for (int b = 0; b < m_n_basins; b++) {
        int sb = s * m_n_basins + b;
        // remove ice shelf parts from the count that do not have a calving front in that basin
        if (n_shelf_cells_per_basin[sb] > 0 and cfs_in_basins_per_shelf[sb] == 0) {
          n_shelf_cells[s] -= n_shelf_cells_per_basin[sb];
          n_shelf_cells_per_basin[sb] = 0;
        }
      }
    }
  }

  // now set potential temperature and salinity box 0:

  int low_temperature_counter = 0;
  for (auto p = m_grid->points(); p; p.next()) {
    const int i = p.i(), j = p.j();

    // make sure all temperatures are zero at the beginning of each time step
    Toc_box0(i, j) = 0.0; // in K
    Soc_box0(i, j) = 0.0; // in psu

    int s = shelf_mask.as_int(i, j);

    if (mask.as_int(i, j) == MASK_FLOATING and s > 0) {
      // note: shelf_mask = 0 in lakes

      assert(n_shelf_cells[s] > 0);
      double N = std::max(n_shelf_cells[s], 1); // protect from division by zero

      // weighted input depending on the number of shelf cells in each basin
      for (int b = 1; b < m_n_basins; b++) { //Note: b=0 yields nan
        int sb = s * m_n_basins + b;
        Toc_box0(i, j) += basin_temperature[b] * n_shelf_cells_per_basin[sb] / N;
        Soc_box0(i, j) += basin_salinity[b] * n_shelf_cells_per_basin[sb] / N;
      }

      double theta_pm = physics.theta_pm(Soc_box0(i, j), physics.pressure(ice_thickness(i, j)));

      // temperature input for grounding line box should not be below pressure melting point
      if (Toc_box0(i, j) < theta_pm) {
        const double eps = 0.001;
        // Setting Toc_box0 a little higher than theta_pm ensures that later equations are
        // well solvable.
        Toc_box0(i, j) = theta_pm + eps;
        low_temperature_counter += 1;
      }
    }
  }

  low_temperature_counter = GlobalSum(m_grid->com, low_temperature_counter);
  if (low_temperature_counter > 0) {
    m_log->message(
        2,
        "PICO WARNING: temperature has been below pressure melting temperature in %d cases,\n"
        "              setting it to pressure melting temperature\n",
        low_temperature_counter);
  }
}

/*!
 * Use the simpler parameterization due to [@ref BeckmannGoosse2003] to set default
 * sub-shelf temperature and melt rate values.
 *
 * At grid points containing floating ice not connected to the ocean, set the basal melt
 * rate to zero and set basal temperature to the pressure melting point.
 */
void Pico::beckmann_goosse(const PicoPhysics &physics,
                           const array::Scalar &ice_thickness,
                           const array::Scalar &shelf_mask,
                           const array::CellType &cell_type,
                           const array::Scalar &Toc_box0,
                           const array::Scalar &Soc_box0,
                           array::Scalar &basal_melt_rate,
                           array::Scalar &basal_temperature,
                           array::Scalar &Toc,
                           array::Scalar &Soc) {

  const double T0          = m_config->get_number("constants.fresh_water.melting_point_temperature"),
               beta_CC     = m_config->get_number("constants.ice.beta_Clausius_Clapeyron"),
               g           = m_config->get_number("constants.standard_gravity"),
               ice_density = m_config->get_number("constants.ice.density");

  array::AccessScope list{ &ice_thickness, &cell_type, &shelf_mask,      &Toc_box0,          &Soc_box0,
                                &Toc,           &Soc,       &basal_melt_rate, &basal_temperature };

  for (auto p = m_grid->points(); p; p.next()) {
    const int i = p.i(), j = p.j();

    if (cell_type.floating_ice(i, j)) {
      if (shelf_mask.as_int(i, j) > 0) {
        double pressure = physics.pressure(ice_thickness(i, j));

        basal_melt_rate(i, j) =
            physics.melt_rate_beckmann_goosse(physics.theta_pm(Soc_box0(i, j), pressure), Toc_box0(i, j));
        basal_temperature(i, j) = physics.T_pm(Soc_box0(i, j), pressure);

        // diagnostic outputs
        Toc(i, j) = Toc_box0(i, j); // in kelvin
        Soc(i, j) = Soc_box0(i, j); // in psu
      } else {
        // Floating ice cells not connected to the ocean.
        const double pressure = ice_density * g * ice_thickness(i, j); // FIXME issue #15

        basal_temperature(i, j) = T0 - beta_CC * pressure;
        basal_melt_rate(i, j)   = 0.0;
      }
    }
  }
}


void Pico::process_box1(const PicoPhysics &physics,
                        const array::Scalar &ice_thickness,
                        const array::Scalar &shelf_mask,
                        const array::Scalar &box_mask,
                        const array::Scalar &Toc_box0,
                        const array::Scalar &Soc_box0,
                        array::Scalar &basal_melt_rate,
                        array::Scalar &basal_temperature,
                        array::Scalar &T_star,
                        array::Scalar &Toc,
                        array::Scalar &Soc,
                        array::Scalar &overturning) {

  std::vector<double> box1_area(m_n_shelves);

  compute_box_area(1, shelf_mask, box_mask, box1_area);

  array::AccessScope list{ &ice_thickness, &shelf_mask, &box_mask,    &T_star,          &Toc_box0,          &Toc,
                                &Soc_box0,      &Soc,        &overturning, &basal_melt_rate, &basal_temperature };

  int n_Toc_failures = 0;

  // basal melt rate, ambient temperature and salinity and overturning calculation
  // for each box1 grid cell.
  for (auto p = m_grid->points(); p; p.next()) {
    const int i = p.i(), j = p.j();

    int shelf_id = shelf_mask.as_int(i, j);

    if (box_mask.as_int(i, j) == 1 and shelf_id > 0) {

      const double pressure = physics.pressure(ice_thickness(i, j));

      T_star(i, j) = physics.T_star(Soc_box0(i, j), Toc_box0(i, j), pressure);

      auto Toc_box1 = physics.Toc_box1(box1_area[shelf_id], T_star(i, j), Soc_box0(i, j), Toc_box0(i, j));

      // This can only happen if T_star > 0.25*p_coeff, in particular T_star > 0
      // which can only happen for values of Toc_box0 close to the local pressure melting point
      if (Toc_box1.failed) {
        m_log->message(5,
                       "PICO WARNING: negative square root argument at %d, %d\n"
                       "              probably because of positive T_star=%f \n"
                       "              Not aborting, but setting square root to 0... \n",
                       i, j, T_star(i, j));

        n_Toc_failures += 1;
      }

      Toc(i, j) = Toc_box1.value;
      Soc(i, j) = physics.Soc_box1(Toc_box0(i, j), Soc_box0(i, j), Toc(i, j)); // in psu

      overturning(i, j) = physics.overturning(Soc_box0(i, j), Soc(i, j), Toc_box0(i, j), Toc(i, j));

      // main outputs
      basal_melt_rate(i, j)    = physics.melt_rate(physics.theta_pm(Soc(i, j), pressure), Toc(i, j));
      basal_temperature(i, j) = physics.T_pm(Soc(i, j), pressure);
    }
  }

  n_Toc_failures = GlobalSum(m_grid->com, n_Toc_failures);
  if (n_Toc_failures > 0) {
    m_log->message(2,
                   "PICO WARNING: square-root argument for temperature calculation\n"
                   "              has been negative in %d cases.\n",
                   n_Toc_failures);
  }
}

void Pico::process_other_boxes(const PicoPhysics &physics,
                               const array::Scalar &ice_thickness,
                               const array::Scalar &shelf_mask,
                               const array::Scalar &box_mask,
                               array::Scalar &basal_melt_rate,
                               array::Scalar &basal_temperature,
                               array::Scalar &T_star,
                               array::Scalar &Toc,
                               array::Scalar &Soc) const {

  std::vector<double> overturning(m_n_shelves, 0.0);
  std::vector<double> salinity(m_n_shelves, 0.0);
  std::vector<double> temperature(m_n_shelves, 0.0);

  // get average overturning from box 1 that is used as input later
  compute_box_average(1, m_overturning, shelf_mask, box_mask, overturning);

  std::vector<bool> use_beckmann_goosse(m_n_shelves);

  array::AccessScope list{ &ice_thickness, &shelf_mask,      &box_mask,           &T_star,   &Toc,
                                &Soc,           &basal_melt_rate, &basal_temperature };

  // Iterate over all boxes i for i > 1
  for (int box = 2; box <= m_n_boxes; ++box) {

    compute_box_average(box - 1, Toc, shelf_mask, box_mask, temperature);
    compute_box_average(box - 1, Soc, shelf_mask, box_mask, salinity);

    // find all the shelves where we should fall back to the Beckmann-Goosse
    // parameterization
    for (int s = 1; s < m_n_shelves; ++s) {
      use_beckmann_goosse[s] = (salinity[s] == 0.0 or
                                temperature[s] == 0.0 or
                                overturning[s] == 0.0);
    }

    std::vector<double> box_area;
    compute_box_area(box, shelf_mask, box_mask, box_area);

    int n_beckmann_goosse_cells = 0;

    for (auto p = m_grid->points(); p; p.next()) {
      const int i = p.i(), j = p.j();

      int shelf_id = shelf_mask.as_int(i, j);

      if (box_mask.as_int(i, j) == box and shelf_id > 0) {

        if (use_beckmann_goosse[shelf_id]) {
          n_beckmann_goosse_cells += 1;
          continue;
        }

        // Get the input from previous box
        const double
          S_previous       = salinity[shelf_id],
          T_previous       = temperature[shelf_id],
          overturning_box1 = overturning[shelf_id];

        {
          double pressure = physics.pressure(ice_thickness(i, j));

          // diagnostic outputs
          T_star(i, j) = physics.T_star(S_previous, T_previous, pressure);
          Toc(i, j)    = physics.Toc(box_area[shelf_id], T_previous, T_star(i, j), overturning_box1, S_previous);
          Soc(i, j)    = physics.Soc(S_previous, T_previous, Toc(i, j));

          // main outputs: basal melt rate and temperature
          basal_melt_rate(i, j)   = physics.melt_rate(physics.theta_pm(Soc(i, j), pressure), Toc(i, j));
          basal_temperature(i, j) = physics.T_pm(Soc(i, j), pressure);
        }
      }
    } // loop over grid points

    n_beckmann_goosse_cells = GlobalSum(m_grid->com, n_beckmann_goosse_cells);
    if (n_beckmann_goosse_cells > 0) {
      m_log->message(
          2,
          "PICO WARNING: [box %d]: switched to the Beckmann Goosse (2003) model at %d locations\n"
          "              (no boundary data from the previous box)\n",
          box, n_beckmann_goosse_cells);
    }

  } // loop over boxes
}


// Write diagnostic variables to extra files if requested
DiagnosticList Pico::diagnostics_impl() const {

  DiagnosticList result = {
    { "basins",                 Diagnostic::wrap(m_geometry.basin_mask()) },
    { "pico_overturning",       Diagnostic::wrap(m_overturning) },
    { "pico_salinity_box0",     Diagnostic::wrap(m_Soc_box0) },
    { "pico_temperature_box0",  Diagnostic::wrap(m_Toc_box0) },
    { "pico_box_mask",          Diagnostic::wrap(m_geometry.box_mask()) },
    { "pico_shelf_mask",        Diagnostic::wrap(m_geometry.ice_shelf_mask()) },
    { "pico_ice_rise_mask",     Diagnostic::wrap(m_geometry.ice_rise_mask()) },
    { "pico_basal_melt_rate",   Diagnostic::wrap(m_basal_melt_rate) },
    { "pico_contshelf_mask",    Diagnostic::wrap(m_geometry.continental_shelf_mask()) },
    { "pico_salinity",          Diagnostic::wrap(m_Soc) },
    { "pico_temperature",       Diagnostic::wrap(m_Toc) },
    { "pico_T_star",            Diagnostic::wrap(m_T_star) },
    { "pico_basal_temperature", Diagnostic::wrap(*m_shelf_base_temperature) },
  };

  return combine(result, OceanModel::diagnostics_impl());
}

/*!
 * For each shelf, compute average of a given field over the box with id `box_id`.
 *
 * This method is used to get inputs from a previous box for the next one.
 */
void Pico::compute_box_average(int box_id,
                               const array::Scalar &field,
                               const array::Scalar &shelf_mask,
                               const array::Scalar &box_mask,
                               std::vector<double> &result) const {

  array::AccessScope list{ &field, &shelf_mask, &box_mask };

  // fill results with zeros
  result.resize(m_n_shelves);
  for (int s = 0; s < m_n_shelves; ++s) {
    result[s] = 0.0;
  }

  // compute the sum of field in each shelf's box box_id
  std::vector<int> n_cells(m_n_shelves);
  {
    std::vector<int> n_cells_per_box(m_n_shelves, 0);
    for (auto p = m_grid->points(); p; p.next()) {
      const int i = p.i(), j = p.j();

      int shelf_id = shelf_mask.as_int(i, j);

      if (box_mask.as_int(i, j) == box_id) {
        n_cells_per_box[shelf_id] += 1;
        result[shelf_id] += field(i, j);
      }
    }
    GlobalSum(m_grid->com, n_cells_per_box.data(), n_cells.data(), m_n_shelves);
  }

  {
    std::vector<double> tmp(m_n_shelves);
    GlobalSum(m_grid->com, result.data(), tmp.data(), m_n_shelves);
    // copy data
    result = tmp;
  }

  for (int s = 0; s < m_n_shelves; ++s) {
    if (n_cells[s] > 0) {
      result[s] /= static_cast<double>(n_cells[s]);
    }
  }
}

/*!
 * For all shelves compute areas of boxes with id `box_id`.
 *
 * @param[in] box_mask box index mask
 * @param[out] result resulting box areas.
 *
 * Note: shelf and box indexes start from 1.
 */
void Pico::compute_box_area(int box_id,
                            const array::Scalar &shelf_mask,
                            const array::Scalar &box_mask,
                            std::vector<double> &result) const {
  result.resize(m_n_shelves);
  array::AccessScope list{ &shelf_mask, &box_mask };

  auto cell_area = m_grid->cell_area();

  for (auto p = m_grid->points(); p; p.next()) {
    const int i = p.i(), j = p.j();

    int shelf_id = shelf_mask.as_int(i, j);

    if (shelf_id > 0 and box_mask.as_int(i, j) == box_id) {
      result[shelf_id] += cell_area;
    }
  }
  
  // compute GlobalSum from index 1 to index m_n_shelves-1
  std::vector<double> result1(m_n_shelves);
  GlobalSum(m_grid->com, &result[1], &result1[1], m_n_shelves-1);
  // copy data
  for (int i = 1; i < m_n_shelves; i++) {
    result[i] = result1[i];
  }
}

} // end of namespace ocean
} // end of namespace pism
