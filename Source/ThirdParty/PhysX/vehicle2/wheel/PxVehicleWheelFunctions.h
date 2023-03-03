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
namespace vehicle2
{
#endif

struct PxVehicleWheelParams;
struct PxVehicleWheelActuationState;
struct PxVehicleSuspensionState;
struct PxVehicleTireSpeedState;
struct PxVehicleScale;
struct PxVehicleWheelRigidBody1dState;

/**
\brief Forward integrate the rotation angle of a wheel
\note The rotation angle of the wheel plays no role in simulation but is important to compute the pose of the wheel for rendering.
\param[in] wheelParams describes the radius and half-width of the wheel
\param[in] actuationState describes whether the wheel has drive or brake torque applied to it.
\param[in] suspensionState describes whether the wheel touches the ground.
\param[in] tireSpeedState describes the components of rigid body velocity at the ground contact point along the tire's lateral and longitudinal directions.
\param[in] thresholdForwardSpeedForWheelAngleIntegration Forward wheel speed below which the wheel rotation speed gets blended with the rolling
           speed (based on the forward wheel speed) which is then used to integrate the wheel rotation angle. At low forward wheel speed, the wheel
		   rotation speed can get unstable (depending on the tire model used) and, for example, oscillate. If brake or throttle is applied, there
		   will be no blending.
\param[in] dt is the simulation time that has lapsed since the last call to PxVehicleWheelRotationAngleUpdate
\param[in,out] wheelRigidBody1dState describes the current angular speed and angle of the wheel.
\note At low speeds and large  timesteps, wheel rotation speed can become noisy due to singularities in the tire slip computations. 
At low speeds, therefore, the wheel speed used for integrating the angle is a blend of current angular speed and rolling angular speed if the 
wheel experiences neither brake nor drive torque and can be placed on the ground. The blended rotation speed gets stored in 
PxVehicleWheelRigidBody1dState::correctedRotationSpeed.
*/
void PxVehicleWheelRotationAngleUpdate
(const PxVehicleWheelParams& wheelParams,
 const PxVehicleWheelActuationState& actuationState, const PxVehicleSuspensionState& suspensionState, const PxVehicleTireSpeedState& tireSpeedState,
 const PxReal thresholdForwardSpeedForWheelAngleIntegration, const PxReal dt,
 PxVehicleWheelRigidBody1dState& wheelRigidBody1dState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */

