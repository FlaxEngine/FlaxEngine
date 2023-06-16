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

#include "foundation/PxVec3.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleSuspensionParams;
struct PxVehiclePhysXSuspensionLimitConstraintParams;
struct PxVehicleSuspensionState;
struct PxVehicleSuspensionComplianceState;
struct PxVehicleTireDirectionState;
struct PxVehicleTireStickyState;
struct PxVehicleRigidBodyState;
struct PxVehiclePhysXConstraintState;

/**
\brief Read constraint data from the vehicle's internal state for a single wheel and write it to a 
structure that will be read by the associated PxScene and used to impose the constraints during the next 
PxScene::simulate() step.
\param[in] suspensionParams describes the suspension frame.
\param[in] suspensionLimitParams describes the restitution value applied to any constraint triggered by 
the suspension travel limit.
\param[in] suspensionState describes the excess suspension compression beyond the suspension travel limit that will be 
resolved with a constraint.
\param[in] suspensionComplianceState describes the effect of suspension compliance on the effective application point
of the suspension force.
\param[in] groundPlaneNormal The normal direction of the ground plane the wheel is driving on. A normalized vector is
           expected.
\param[in] tireStickyDampingLong The damping coefficient to use in the constraint to approach a zero target velocity
           along the longitudinal tire axis.
\param[in] tireStickyDampingLat Same concept as tireStickyDampingLong but for the lateral tire axis.
\param[in] tireDirectionState describes the longitudinal and lateral directions of the tire in the world frame.
\param[in] tireStickyState describes the low speed state of the tire in the longitudinal and lateral directions.
\param[in] rigidBodyState describes the pose of the rigid body.
\param[out] constraintState is the data structure that will be read by the associated PxScene in the next call to 
PxScene::simulate().
\note Constraints include suspension constraints to account for suspension travel limit and sticky 
tire constraints that bring the vehicle to rest at low longitudinal and lateral speed.
*/
void PxVehiclePhysXConstraintStatesUpdate
(const PxVehicleSuspensionParams& suspensionParams,
 const PxVehiclePhysXSuspensionLimitConstraintParams& suspensionLimitParams,
 const PxVehicleSuspensionState& suspensionState, const PxVehicleSuspensionComplianceState& suspensionComplianceState,
 const PxVec3& groundPlaneNormal,
 const PxReal tireStickyDampingLong, const PxReal tireStickyDampingLat, 
 const PxVehicleTireDirectionState& tireDirectionState, const PxVehicleTireStickyState& tireStickyState,
 const PxVehicleRigidBodyState& rigidBodyState,
 PxVehiclePhysXConstraintState& constraintState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
