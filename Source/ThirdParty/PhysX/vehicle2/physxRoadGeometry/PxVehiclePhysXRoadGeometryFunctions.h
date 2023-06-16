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

#include "foundation/PxSimpleTypes.h"

#if !PX_DOXYGEN
namespace physx
{
class PxScene;
class PxConvexMesh;

namespace vehicle2
{
#endif

struct PxVehicleWheelParams;
struct PxVehicleSuspensionParams;
struct PxVehiclePhysXRoadGeometryQueryParams;
struct PxVehiclePhysXRoadGeometryQueryState;
struct PxVehiclePhysXMaterialFrictionParams;
struct PxVehicleRigidBodyState;
struct PxVehicleFrame;
struct PxVehicleRoadGeometryState;

/**
\brief Compute the plane of the road geometry under a wheel and the tire friction of the contact.
\param[in] wheelParams describes the radius and halfwidth of the wheel.
\param[in] suspParams describes the frame of the suspension and wheel and the maximum suspension travel.
\param[in] roadGeomParams describes the operation of the PhysX scene query.
\param[in] materialFrictionParams describes a mapping between PxMaterial and friction in order to compute a tire friction value.
\param[in] wheelYawAngle is the yaw angle (in radians) of the wheel.
\param[in] rigidBodyState describes the pose of the rigid body.
\param[in] scene is the PhysX scene that will be queried by the scene query.
\param[in] unitCylinderSweepMesh is a convex cylindrical mesh of unit radius and half-width to be used in 
the event that a sweep query is to be used.
\param[in] frame describes the lateral, longitudinal and vertical axes and is used to scale unitCylinderSweepMesh 
by the wheel's radius and half-width.
\param[out] roadGeomState contains the plane and friction of the road geometry under the wheel.
\param[out] physxRoadGeometryState Optional buffer to store additional information about the query (like actor/shape that got hit etc.).
            Set to NULL if not needed.
\note PxVehicleRoadGeometryState::hitState will have value false in the event that the there is no reachable road geometry under the wheel and 
true if there is reachable road geometry under the wheel. Road geometry is considered reachable if the suspension can elongate from its
reference pose far enough to place wheel on the ground.
*/
void PxVehiclePhysXRoadGeometryQueryUpdate
(const PxVehicleWheelParams& wheelParams, const PxVehicleSuspensionParams& suspParams,
 const PxVehiclePhysXRoadGeometryQueryParams& roadGeomParams, const PxVehiclePhysXMaterialFrictionParams& materialFrictionParams,
 const PxReal wheelYawAngle, const PxVehicleRigidBodyState& rigidBodyState,
 const PxScene& scene, const PxConvexMesh* unitCylinderSweepMesh,
 const PxVehicleFrame& frame,
 PxVehicleRoadGeometryState& roadGeomState,
 PxVehiclePhysXRoadGeometryQueryState* physxRoadGeometryState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
