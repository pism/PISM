// Copyright (C) 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2021, 2023, 2024 PISM Authors
//
// This file is part of PISM.
//
// PISM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 3 of the License, or (at your option) any later
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

#include "ClimateIndex.hh"
#include "pism/util/Time.hh"
#include "pism/util/Grid.hh"

namespace pism {
namespace atmosphere {

ClimateIndex::ClimateIndex(std::shared_ptr<const Grid> g)
    : AtmosphereModel(g),
      // Reference fields for the mean annual and mean summer near-surface air temperature
      m_air_temp_annual(m_grid, "air_temp_annual_ref"),
      m_air_temp_annual_ref(m_grid, "air_temp_annual_ref"),
      m_air_temp_summer(m_grid, "air_temp_summer_ref"),
      m_air_temp_summer_ref(m_grid, "air_temp_summer_ref"),

      // Anaomaly temperature fields for Climate index 0 (e.g. LGM), interglacial index 1 (e.g. LIG) and interglacial index 1X (e.g. mPWP)
      m_air_temp_anomaly_annual_0(m_grid, "air_temp_anomaly_annual_0"),
      m_air_temp_anomaly_annual_1(m_grid, "air_temp_anomaly_annual_1"),
      m_air_temp_anomaly_annual_1X(m_grid, "air_temp_anomaly_annual_1X"),

      m_air_temp_anomaly_summer_0(m_grid, "air_temp_anomaly_summer_0"),
      m_air_temp_anomaly_summer_1(m_grid, "air_temp_anomaly_summer_1"),
      m_air_temp_anomaly_summer_1X(m_grid, "air_temp_anomaly_summer_1X"),

      //Reference precipitation field
      m_precipitation(m_grid, "precipitation_ref"),
      m_precipitation_ref(m_grid, "precipitation_ref"),

      m_precipitation_anomaly_0(m_grid, "precipitation_anomaly_0"),
      m_precipitation_anomaly_1(m_grid, "precipitation_anomaly_1"),
      m_precipitation_anomaly_1X(m_grid, "precipitation_anomaly_1X"),

      // Spatial precipitation scaling factor
      m_spatial_precip_scaling(m_grid, "precip_scaling_factor") {

  auto climate_index_file = m_config->get_string("climate_index.file");
  if (climate_index_file.empty()) {
    throw RuntimeError::formatted(PISM_ERROR_LOCATION, "'climate_index.file' cannot be empty");
  }
  m_climate_index.reset(new ClimateIndexWeights(*g->ctx()));

  auto scaling_file = m_config->get_string("atmosphere.yearly_cycle.scaling.file");
  if (not scaling_file.empty()) {
    m_A.reset(new ScalarForcing(*g->ctx(), "atmosphere.yearly_cycle.scaling", "amplitude_scaling",
                                "1", "1", "temperature amplitude scaling"));
  }

  m_air_temp_annual.metadata(0)
      .long_name(
          "mean annual near-surface air temperature (without sub-year time-dependence or forcing)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_annual_ref.metadata(0)
      .long_name(
          "mean annual near-surface air temperature (without sub-year time-dependence or forcing)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_summer.metadata(0)
      .long_name(
          "mean summer (NH: July/ SH: January) near-surface air temperature (without sub-year time-dependence or forcing)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_summer_ref.metadata(0)
      .long_name(
          "mean summer (NH: July/ SH: January) near-surface air temperature (without sub-year time-dependence or forcing)")
      .units("K")
      .set_time_independent(true);

  m_precipitation.metadata(0)
      .long_name("precipitation rate")
      .units("kg m-2 second-1")
      .output_units("kg m-2 year-1")
      .set_time_independent(true);

  m_precipitation_ref.metadata(0)
      .long_name("precipitation rate")
      .units("kg m-2 second-1")
      .output_units("kg m-2 year-1")
      .set_time_independent(true);

  m_air_temp_anomaly_annual_0.metadata(0)
      .long_name(
          "mean annual near-surface air temperature (without sub-year time-dependence or forcing) for Climate index 0 (e.g. LGM)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_anomaly_annual_1.metadata(0)
      .long_name(
          "mean annual near-surface air temperature (without sub-year time-dependence or forcing) for interglacial index 1 (e.g. LIG)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_anomaly_annual_1X.metadata(0)
      .long_name(
          "mean PD annual near-surface air temperature (without sub-year time-dependence or forcing) for interglacial index 1X (e.g. mPWP)")
      .units("K")
      .set_time_independent(true);

  // Paleo time slice temperature summer
  m_air_temp_anomaly_summer_0.metadata(0)
      .long_name(
          "mean summer (NH: July/ SH: January) near-surface air temperature (without sub-year time-dependence or forcing) for Climate index 0 (e.g. LGM)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_anomaly_summer_1.metadata(0)
      .long_name(
          "mean summer (NH: July/ SH: January) near-surface air temperature (without sub-year time-dependence or forcing) for interglacial index 1 (e.g. LIG)")
      .units("K")
      .set_time_independent(true);

  m_air_temp_anomaly_summer_1X.metadata(0)
      .long_name(
          "mean summer (NH: July/ SH: January) near-surface air temperature (without sub-year time-dependence or forcing) for interglacial index 1X (e.g. mPWP)")
      .units("K")
      .set_time_independent(true);

  m_precipitation_anomaly_0.metadata(0)
      .long_name("precipitation rate")
      .units("kg m-2 second-1")
      .output_units("kg m-2 year-1")
      .set_time_independent(true);

  m_precipitation_anomaly_1.metadata(0)
      .long_name("precipitation rate anomaly")
      .units("kg m-2 second-1")
      .output_units("kg m-2 year-1")
      .set_time_independent(true);

  m_precipitation_anomaly_1X.metadata(0)
      .long_name("precipitation rate")
      .units("kg m-2 second-1")
      .output_units("kg m-2 year-1")
      .set_time_independent(true);

  // Spatial precip scaling factor
  m_spatial_precip_scaling.metadata(0)
      .long_name("spatial scaling factor with temperature for precipitation")
      .units("K-1")
      .set_time_independent(true);

  auto summer_peak_day = m_config->get_number("atmosphere.fausto_air_temp.summer_peak_day");
  m_midsummer_year_fraction = time().day_of_the_year_to_year_fraction((int)summer_peak_day);

  m_use_precip_scaling = m_config->get_flag("atmosphere.climate_index.precip_scaling.use");
  m_spatially_variable_scaling = false;
  m_preciplinfactor = 0.0;
  m_use_cos = m_config->get_flag("atmosphere.climate_index.cosinus_yearly_cycle.use");
  m_use_precip_cos = m_config->get_flag("atmosphere.climate_index.precip_cosinus_yearly_cycle.use");
  m_use_1X = m_config->get_flag("climate_index.super_interglacial.use");
}

void ClimateIndex::init_impl(const Geometry &geometry) {
  (void)geometry;
  m_log->message(2, "**** Initializing the 'Climate Index' atmosphere model...\n");

  auto input_file = m_config->get_string("atmosphere.climate_index.climate_snapshots.file");

  if (input_file.empty()) {
    throw RuntimeError(PISM_ERROR_LOCATION,
                       "'atmosphere.climate_index.climate_snapshots.file' cannot be empty");
  }

  m_log->message(2,
                 "  Reading mean annual air temperature, mean July air temperature, and\n"
                 "  precipitation fields from '%s'...\n",
                 input_file.c_str());

  File input(m_grid->com, input_file, io::PISM_GUESS, io::PISM_READONLY);
  auto None = io::Default::Nil();

  // Reference fields
  m_precipitation.regrid(input, None);
  m_precipitation_ref.regrid(input, None);
  m_air_temp_annual.regrid(input, None);
  m_air_temp_annual_ref.regrid(input, None);

  auto user_provided = [&input](const array::Array &a) {
    return input.find_variable(a.metadata(0).get_name());
  };

  m_air_temp_anomaly_annual_0.regrid(input, None);
  m_air_temp_anomaly_annual_1.regrid(input, None);
  if (m_use_1X) {
    m_air_temp_anomaly_annual_1X.regrid(input, None);
  }

  if (m_use_cos) {
    m_log->message(2, " * -use_cosinus_yearly_cycle, thus will use cosinus function with mean summer anomalies \n"
                      " for representing seasonal variations \n");
    m_air_temp_summer.regrid(input, None);
    m_air_temp_summer_ref.regrid(input, None);
    m_air_temp_anomaly_summer_0.regrid(input, None);
    m_air_temp_anomaly_summer_1.regrid(input, None);
    if (m_use_1X) {
      m_air_temp_anomaly_summer_1X.regrid(input, None);
    }
  }

  auto precip_scaling_file =
      m_config->get_string("atmosphere.climate_index.precip_scaling.spatial_linear_factor.file");

  m_spatially_variable_scaling = not precip_scaling_file.empty();

  // If a file is giving for spatial scaling, then it will use it. Otherwise by default it
  // uses the linear, uniform scaling factor from config

  if (not m_use_precip_scaling) {
    m_log->message(
        2,
        "*  no scaling method used for precipitation\n"
        "   thus it will use the precipitation snapshots from atmosphere.climate_index.climate_snapshots.file if exist\n");
    m_precipitation_anomaly_0.regrid(input, None);
    m_precipitation_anomaly_1.regrid(input, None);
    if (m_use_1X) {
      m_precipitation_anomaly_1X.regrid(input, None);
    } 
  } else if (m_spatially_variable_scaling) {
    m_spatial_precip_scaling.regrid(precip_scaling_file, None);
    m_log->message(
        2,
        "*  - scaling file given for precipitation scaling in -atmosphere.climate_index.precip_scaling.spatial_linear_factor\n"
        "    thus Climate Index forcing is using temperature anomalies to calculate\n"
        "    precipitation anomalies using a spatially distributed scaling factor from '%s'...\n",
        precip_scaling_file.c_str());
  } else {
    m_preciplinfactor =
        m_config->get_number("atmosphere.climate_index.precip_scaling.uniform_linear_factor");
    m_log->message(2,
                   "*  -atmosphere.climate_index.precip_scaling is set to uniform scaling factor,\n"
                   "    thus precipitation anomalies are calculated using linear scaling\n"
                   "    with air temperature anomalies (%.3f percent per degree).\n",
                   m_preciplinfactor * 100.0);
  }
}

void ClimateIndex::update_impl(const Geometry &geometry, double t, double dt) {
  (void)geometry;

  auto weights = m_climate_index->update_weights(t, dt);
  double w_0   = weights[0];
  double w_1   = weights[1];
  double w_1X  = m_use_1X ? weights[2] : 0.0;

  m_log->message(3, "**** atmosphere::ClimateIndex weights: w0 = '%f', w1 = '%f', w1X = '%f' ****\n",
                 w_0, w_1, w_1X);

  // mean annual air temperature
  const auto &Ta_ref = m_air_temp_annual_ref;
  const auto &dTa_0  = m_air_temp_anomaly_annual_0;
  const auto &dTa_1  = m_air_temp_anomaly_annual_1;
  const auto &dTa_1X = m_air_temp_anomaly_annual_1X;

  // mean summer air temperature
  const auto &Ts_ref = m_air_temp_summer_ref;
  const auto &dTs_0  = m_air_temp_anomaly_summer_0;
  const auto &dTs_1  = m_air_temp_anomaly_summer_1;
  const auto &dTs_1X = m_air_temp_anomaly_summer_1X;

  // precipitation
  const auto &P_ref = m_precipitation_ref;
  const auto &dP_0  = m_precipitation_anomaly_0;
  const auto &dP_1  = m_precipitation_anomaly_1;
  const auto &dP_1X = m_precipitation_anomaly_1X;

  array::AccessScope scope{ &m_air_temp_annual, &Ta_ref, &dTa_0, &dTa_1, &dTa_1X, &m_precipitation, &P_ref };

  if (m_use_cos) {
    scope.add({ &m_air_temp_summer, &Ts_ref, &dTs_0, &dTs_1, &dTs_1X });
  }

  if (m_use_precip_scaling and m_spatially_variable_scaling) {
    scope.add(m_spatial_precip_scaling);
  } else {
    scope.add({ &dP_0, &dP_1, &dP_1X });
  }

  for (Points p(*m_grid); p; p.next()) {
    const int i = p.i(), j = p.j();

    double annual_anomaly = 0.0;
    // air temperature
    {
      annual_anomaly = w_0 * dTa_0(i, j) + w_1 * dTa_1(i, j) + w_1X * (dTa_1X(i, j) - dTa_1(i, j));
      m_air_temp_annual(i, j) = Ta_ref(i, j) + annual_anomaly;

      if (m_use_cos) {
        double summer_anomaly = w_0 * dTs_0(i, j) + w_1 * dTs_1(i, j) + w_1X * (dTs_1X(i, j) - dTs_1(i, j));
        m_air_temp_summer(i, j) = Ts_ref(i, j) + summer_anomaly;
      }
    }

    // precipitation
    {
      if (m_use_precip_scaling) {
        double F = m_spatially_variable_scaling ? m_spatial_precip_scaling(i, j) : m_preciplinfactor;

        m_precipitation(i, j) = P_ref(i, j) * (1 + annual_anomaly * F);
      } else {
        m_precipitation(i, j) =
            P_ref(i, j) + w_0 * dP_0(i, j) + w_1 * dP_1(i, j) + w_1X * (dP_1X(i, j) - dP_1(i, j));
      }
    }
  } // end of the loop over grid points
}

//! Copies the stored precipitation field into result.
const array::Scalar& ClimateIndex::precipitation_impl() const {
  return m_precipitation;
}

//! Copies the stored mean annual near-surface air temperature field into result.
const array::Scalar& ClimateIndex::air_temperature_impl() const {
  return m_air_temp_annual;
}

//! Copies the stored mean summer near-surface air temperature field into result.
const array::Scalar& ClimateIndex::mean_summer_temp() const {
  return m_air_temp_summer;
}

// same as YearlyCycle
void ClimateIndex::init_timeseries_impl(const std::vector<double> &ts) const {

  size_t N = ts.size();

  m_ts_times.resize(N);
  m_cosine_cycle.resize(N);
  for (unsigned int k = 0; k < N; k++) {
    double tk = time().year_fraction(ts[k]) - m_midsummer_year_fraction;

    m_ts_times[k] = ts[k];
    m_cosine_cycle[k] = cos(2.0 * M_PI * tk);
  }

  if ((bool)m_A) {
    for (unsigned int k = 0; k < N; ++k) {
      m_cosine_cycle[k] *= m_A->value(ts[k]);
    }
  }
}

void ClimateIndex::precip_time_series_impl(int i, int j, std::vector<double> &result) const {
  result.resize(m_ts_times.size());
  for (unsigned int k = 0; k < m_ts_times.size(); k++) {
    if (m_use_cos and m_use_precip_cos) {
      // If use cosinus for yearly cycle of temperature, precipitation anomalies between annual and summer mean are added to the mean precipitation 
      // Precipitation anomalies between annual and summer are calculated using temperature difference and either the uniform linear precip scaling or spatial from file
      double F = m_spatially_variable_scaling ? m_spatial_precip_scaling(i, j) : m_preciplinfactor;
      result[k] = m_precipitation(i,j) + (1 + F * (m_air_temp_summer(i, j) - m_air_temp_annual(i, j)) * m_cosine_cycle[k]);
    } else {
      result[k] = m_precipitation(i,j);
    }
  }
}

void ClimateIndex::temp_time_series_impl(int i, int j, std::vector<double> &result) const {
  result.resize(m_ts_times.size());
  for (unsigned int k = 0; k < m_ts_times.size(); ++k) {
    if (m_use_cos) {
      result[k] = m_air_temp_annual(i, j) + (m_air_temp_summer(i, j) - m_air_temp_annual(i, j)) * m_cosine_cycle[k];
    } else {
      result[k] = m_air_temp_annual(i, j);
    }
  }
}

void ClimateIndex::begin_pointwise_access_impl() const {
  m_air_temp_annual.begin_access();
  if (m_use_cos) {
    m_air_temp_summer.begin_access();
  }
  m_precipitation.begin_access();
  if (m_use_precip_scaling and m_spatially_variable_scaling) {
    m_spatial_precip_scaling.begin_access();
  }
}

void ClimateIndex::end_pointwise_access_impl() const {
  m_air_temp_annual.end_access();
  if (m_use_cos) {
    m_air_temp_summer.end_access();
  }
  m_precipitation.end_access();
  if (m_use_precip_scaling and m_spatially_variable_scaling) {
    m_spatial_precip_scaling.end_access();
  }
}

namespace diagnostics {

/*! @brief Mean summer near-surface air temperature. */
class MeanSummerTemperature : public Diag<ClimateIndex>
{
public:
  MeanSummerTemperature(const ClimateIndex *m) : Diag<ClimateIndex>(m) {
    m_vars = { { m_sys, "air_temp_summer" } };
    m_vars[0]
        .long_name("mean summer near-surface air temperature used in the cosine yearly cycle")
        .units("Kelvin");
  }
private:
  std::shared_ptr<array::Array> compute_impl() const {
    auto result = allocate<array::Scalar>("air_temp_summer");

    result->copy_from(model->mean_summer_temp());

    return result;
  }
};
} // end of namespace diagnostics

DiagnosticList ClimateIndex::diagnostics_impl() const {
  DiagnosticList result = AtmosphereModel::diagnostics_impl();

  result["air_temp_summer"] = Diagnostic::Ptr(new diagnostics::MeanSummerTemperature(this));

  return result;
}

} // end of namespace atmosphere
} // end of namespace pism
