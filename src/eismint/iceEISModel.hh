// Copyright (C) 2004-2007 Jed Brown and Ed Bueler
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

#ifndef __iceEISModel_hh
#define __iceEISModel_hh

#include <petscda.h>
#include "../base/grid.hh"
#include "../base/materials.hh"
#include "../base/iceModel.hh"

class IceEISModel : public IceModel {
public:
    IceEISModel(IceGrid &g, IceType &i);
    void           setflowlawNumber(PetscInt);
    PetscInt       getflowlawNumber();
    virtual PetscErrorCode setFromOptions();
    virtual PetscErrorCode initFromOptions();
    
private:
    char       expername;
    PetscInt   flowlawNumber;
 
    PetscErrorCode initAccumTs();
    PetscErrorCode fillintemps();
    virtual PetscScalar basalVelocity(const PetscScalar x, const PetscScalar y, 
         const PetscScalar H, const PetscScalar T, const PetscScalar alpha,
         const PetscScalar mu);

    // for experiments I and K (also J,L indirectly): read NetCDF files for bed topography:
    PetscErrorCode getBedTopography(const char* topoFile); 
};

#endif /* __iceEISModel_hh */
