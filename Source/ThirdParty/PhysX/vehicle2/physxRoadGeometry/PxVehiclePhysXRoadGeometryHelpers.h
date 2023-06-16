// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#pragma once

/** \addtogroup vehicle2
  @{
*/

#include "foundation/PxPreprocessor.h"

#if !PX_DOXYGEN
namespace physx
{

class PxConvexMesh;
class PxPhysics;
struct PxCookingParams;

namespace vehicle2
{
#endif

struct PxVehicleFrame;

/**
\brief Create a cylindrical mesh with unit radius and half-width.
\param[in] vehicleFrame is a description of the lateral and longitudinal axes.
\param[in] physics is a PxPhysics instance.
\param[in] params is a PxCookingParams instance
\return Return a PxConvexMesh instance that represents a convex hull with unit radius and half-width.
@see PxVehicleUnitCylinderSweepMeshDestroy
*/
PxConvexMesh* PxVehicleUnitCylinderSweepMeshCreate(const PxVehicleFrame& vehicleFrame, PxPhysics& physics, const PxCookingParams& params);

/**
\brief Release the mesh created with PxVehicleUnitCylinderSweepMeshCreate.
\param[in] mesh is a PxConvexMesh instance.
@see PxVehicleUnitCylinderSweepMeshCreate
*/
void PxVehicleUnitCylinderSweepMeshDestroy(PxConvexMesh* mesh);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
