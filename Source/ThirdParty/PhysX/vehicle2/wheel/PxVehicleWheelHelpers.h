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

#include "vehicle2/PxVehicleFunctions.h"

#include "vehicle2/suspension/PxVehicleSuspensionParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionStates.h"

#include "PxVehicleWheelParams.h"
#include "PxVehicleWheelStates.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Compute the quaternion of a wheel in the rigid body frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] camberAngle is the camber angle in radian sinduced by suspension compliance.
\param[in] toeAngle is the toe angle in radians induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] rotationAngle is the angle around the wheel's lateral axis.
\return The quaterion of the wheel in the rigid body frame.
@see PxVehicleComputeWheelOrientation
*/
PX_FORCE_INLINE PxQuat PxVehicleComputeWheelLocalOrientation
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxReal camberAngle, const PxReal toeAngle, const PxReal steerAngle,
 const PxReal rotationAngle)
{
	const PxQuat wheelLocalOrientation =
		(suspensionParams.suspensionAttachment.q * PxVehicleComputeRotation(frame, camberAngle, 0.0f, steerAngle + toeAngle))*
		(suspensionParams.wheelAttachment.q * PxVehicleComputeRotation(frame, 0.0f, rotationAngle, 0.0f));
	return wheelLocalOrientation;
}

/**
\brief Compute the quaternion of a wheel in the world frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] camberAngle is the camber angle in radian induced by suspension compliance.
\param[in] toeAngle is the toe angle in radians induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] rigidBodyOrientation is the quaterion of the rigid body in the world frame.
\param[in] rotationAngle is the angle around the wheel's lateral axis.
\return The quaterion of the wheel in the world frame.
@see PxVehicleComputeWheelLocalOrientation
*/
PX_FORCE_INLINE PxQuat PxVehicleComputeWheelOrientation
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxReal camberAngle, const PxReal toeAngle, const PxReal steerAngle,
 const PxQuat& rigidBodyOrientation, const PxReal rotationAngle)
{
	const PxQuat wheelOrientation = rigidBodyOrientation * PxVehicleComputeWheelLocalOrientation(frame, suspensionParams,
		camberAngle, toeAngle, steerAngle, rotationAngle);
	return wheelOrientation;
}

/**
\brief Compute the pose of the wheel in the rigid body frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] suspensionState is the compression state of the suspenson.
\param[in] camberAngle is the camber angle in radian induced by suspension compliance.
\param[in] toeAngle is the toe angle in radians induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] rotationAngle is the angle around the wheel's lateral axis.
\return The pose of the wheel in the rigid body frame.
*/
PX_FORCE_INLINE PxTransform PxVehicleComputeWheelLocalPose
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionState& suspensionState,
 const PxReal camberAngle, const PxReal toeAngle, const PxReal steerAngle,
 const PxReal rotationAngle)
{
	//Full equation:
		//PxTransform(suspAttachment.p + suspParams.suspensionTravelDir*suspDist, suspAttachment.q) * 
		//PxTransform(PxVec3(0), PxQuat(camber,  0, steer+toe)) *
		//wheelAttachment *
		//PxTransform(PxVec3(0), PxQuat(0, rotation, 0))
	//Reduces to:
		//PxTransform(suspAttachment.p + suspParams.suspensionTravelDir*suspDist, suspAttachment.q * PxQuat(camber,  0, steer+toe)) *
		//PxTranfsorm(wheelAttachment.p, wheelAttachment.q * PxQuat(0, rotation, 0))
	const PxF32 suspDist = (suspensionState.jounce != PX_VEHICLE_UNSPECIFIED_JOUNCE) ? (suspensionParams.suspensionTravelDist - suspensionState.jounce) : 0.0f;
	const PxTransform wheelLocalPose =
		PxTransform(
			suspensionParams.suspensionAttachment.p + suspensionParams.suspensionTravelDir*suspDist,
			suspensionParams.suspensionAttachment.q*PxVehicleComputeRotation(frame, camberAngle, 0.0f, steerAngle + toeAngle))*
		PxTransform(
			suspensionParams.wheelAttachment.p,
			suspensionParams.wheelAttachment.q*PxVehicleComputeRotation(frame, 0.0f, rotationAngle, 0.0f));
	return wheelLocalPose;
}


/**
\brief Compute the pose of the wheel in the rigid body frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] suspensionState is the compression state of the suspenson.
\param[in] suspensionComplianceState is the camber and toe angles induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] wheelState is angle around the wheel's lateral axis.
\return The pose of the wheel in the rigid body frame.
*/
PX_FORCE_INLINE PxTransform PxVehicleComputeWheelLocalPose
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionState& suspensionState, const PxVehicleSuspensionComplianceState& suspensionComplianceState,
 const PxReal steerAngle,
 const PxVehicleWheelRigidBody1dState& wheelState)
{
	return PxVehicleComputeWheelLocalPose(frame, suspensionParams, suspensionState,
		suspensionComplianceState.camber, suspensionComplianceState.toe, steerAngle,
		wheelState.rotationAngle);
}

/**
\brief Compute the pose of the wheel in the world frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] suspensionState is the compression state of the suspenson.
\param[in] camberAngle is the camber angle in radian induced by suspension compliance.
\param[in] toeAngle is the toe angle in radians induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] rigidBodyPose is the pose of the rigid body in the world frame.
\param[in] rotationAngle is the angle around the wheel's lateral axis.
\return The pose of the wheel in the world frame.
*/
PX_FORCE_INLINE PxTransform PxVehicleComputeWheelPose
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionState& suspensionState,
 const PxReal camberAngle, const PxReal toeAngle, const PxReal steerAngle,
 const PxTransform& rigidBodyPose, const PxReal rotationAngle)
{
	const PxTransform wheelPose = rigidBodyPose * PxVehicleComputeWheelLocalPose(frame, suspensionParams, suspensionState,
		camberAngle, toeAngle, steerAngle, rotationAngle);
	return wheelPose;
}

/**
\brief Compute the pose of the wheel in the world frame.
\param[in] frame describes the longitudinal and lateral axes of the vehicle.
\param[in] suspensionParams describes the suspension and wheel frames.
\param[in] suspensionState is the compression state of the suspenson.
\param[in] suspensionComplianceState is the camber and toe angles induced by suspension compliance.
\param[in] steerAngle is the steer angle in radians applied to the wheel.
\param[in] rigidBodyPose is the pose of the rigid body in the world frame.
\param[in] wheelState is angle around the wheel's lateral axis.
\return The pose of the wheel in the world frame.
*/

PX_FORCE_INLINE PxTransform PxVehicleComputeWheelPose
(const PxVehicleFrame& frame,
 const PxVehicleSuspensionParams& suspensionParams,
 const PxVehicleSuspensionState& suspensionState, const PxVehicleSuspensionComplianceState& suspensionComplianceState, const PxReal steerAngle,
 const PxTransform& rigidBodyPose, const PxVehicleWheelRigidBody1dState& wheelState)
{
	return PxVehicleComputeWheelPose(frame, suspensionParams, suspensionState,
		suspensionComplianceState.camber, suspensionComplianceState.toe, steerAngle,
		rigidBodyPose, wheelState.rotationAngle);
}

/**
\brief Check if the suspension could place the wheel on the ground or not.

\param[in] suspState The state of the suspension to check.
\return True if the wheel connects to the ground, else false.

@see PxVehicleSuspensionState
*/
PX_FORCE_INLINE bool PxVehicleIsWheelOnGround(const PxVehicleSuspensionState& suspState)
{
	return (suspState.separation <= 0.0f);
}

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
