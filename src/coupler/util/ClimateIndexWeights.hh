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

#ifndef PISM_CLIMATEINDEXEWEIGHT_H
#define PISM_CLIMATEINDEXEWEIGHT_H

#include <array>

#include "pism/util/ScalarForcing.hh"

namespace pism {

class ClimateIndexWeights {
public:
  ClimateIndexWeights(const Context &ctx);
  virtual ~ClimateIndexWeights() = default;

  std::array<double, 3> update_weights(double t, double dt);

protected:
  // glacial index time series
  std::unique_ptr<ScalarForcing> m_index;

  double m_ref_value, m_max_value;
};

} // end of namespace pism

#endif /* PISM_CLIMATEINDEXEWEIGHT_H */
