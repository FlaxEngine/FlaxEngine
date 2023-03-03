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
#include "foundation/PxSimpleTypes.h"
#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleRigidBodyParams;
struct PxVehicleSuspensionForce;
struct PxVehicleTireForce;
struct PxVehicleAntiRollTorque;
struct PxVehicleRigidBodyState;

/**
\brief Forward integrate rigid body state.
\param[in] axleDescription is a description of the axles of the vehicle and the wheels on each axle.
\param[in] rigidBodyParams is a description of rigid body mass and moment of inertia.
\param[in] suspensionForces is an array of suspension forces and torques in the world frame to be applied to the rigid body.
\param[in] tireForces is an array of tire forces and torques in the world frame to be applied to the rigid body.
\param[in] antiRollTorque is an optional pointer to a single PxVehicleAntiRollTorque instance that contains the accumulated anti-roll
torque to apply to the rigid body.
\param[in] dt is the timestep of the forward  integration.
\param[in] gravity is gravitational acceleration.
\param[in,out] rigidBodyState is the rigid body state that is to be updated.
\note The suspensionForces array must contain an entry for each wheel listed as an active wheel in axleDescription.
\note The tireForces array must contain an entry for each wheel listed as an active wheel in axleDescription.
\note If antiRollTorque is a null pointer then zero anti-roll torque will be applied to the rigid body.
*/
void PxVehicleRigidBodyUpdate
	(const PxVehicleAxleDescription& axleDescription, const PxVehicleRigidBodyParams& rigidBodyParams, 
	const PxVehicleArrayData<const PxVehicleSuspensionForce>& suspensionForces,
	const PxVehicleArrayData<const PxVehicleTireForce>& tireForces,
	const PxVehicleAntiRollTorque* antiRollTorque,
	const PxReal dt, const PxVec3& gravity,
	PxVehicleRigidBodyState& rigidBodyState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
