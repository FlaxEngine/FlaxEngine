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
#include "foundation/PxTransform.h"

#include "vehicle2/wheel/PxVehicleWheelParams.h"
#include "vehicle2/wheel/PxVehicleWheelHelpers.h"

#include "PxVehicleSuspensionParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Compute suspension travel direction in the world frame.
\param[in] suspensionParams is a description of the suspension frame.
\param[in] rigidBodyPose is the current pose of the vehicle's rigid body.
\return The return value is the suspension travel direction in the world frame.
\note The suspension travel direction is used to perform queries against the road geometry.
*/
PX_FORCE_INLINE PxVec3 PxVehicleComputeSuspensionDirection(const PxVehicleSuspensionParams& suspensionParams, const PxTransform& rigidBodyPose)
{
	const PxVec3 suspDir = rigidBodyPose.rotate(suspensionParams.suspensionTravelDir);
	return suspDir;
}

/**
\brief Compute the start pose of a suspension query.
\param[in] frame is a description of the longitudinal, lateral and vertical axes.
\param[in] suspensionParams is a description of the suspension frame.
\param[in] steerAngle is the yaw angle of the wheel in radians.
\param[in] rigidBodyPose is the pose of the rigid body in the world frame.
*/
PX_FORCE_INLINE PxTransform PxVehicleComputeWheelPoseForSuspensionQuery(const PxVehicleFrame& frame, const PxVehicleSuspensionParams& suspensionParams,
	const PxReal steerAngle, const PxTransform& rigidBodyPose)
{
	//Compute the wheel pose with zero travel from attachment point, zero compliance, 
	//zero wheel pitch (ignore due to radial symmetry).
	//Zero travel from attachment point (we want the wheel pose at the top of the suspension, i.e., at max compression)
	PxVehicleSuspensionState suspState;
	suspState.setToDefault();
	//Compute the wheel pose.
	const PxTransform wheelPose = PxVehicleComputeWheelPose(frame, suspensionParams, suspState, 0.0f, 0.0f, steerAngle, rigidBodyPose,
		0.0f);

	return wheelPose;
}

/**
\brief Compute the start point, direction and length of a suspension scene raycast.
\param[in] frame is a description of the longitudinal, lateral and vertical axes.
\param[in] wheelParams describes the radius and halfwidth of the wheel.
\param[in] suspensionParams describes the suspension frame and the maximum suspension travel.
\param[in] steerAngle is the yaw angle of the wheel in radians.
\param[in] rigidBodyPose is the pose of the rigid body in the world frame.
\param[out] start is the starting point of the raycast in the world frame.
\param[out] dir is the direction of the raycast in the world frame.
\param[out] dist is the length of the raycast.
\note start, dir and dist together describe a raycast that begins at the top of wheel at maximum compression
and ends at the bottom of wheel at maximum droop.
*/
PX_FORCE_INLINE void PxVehicleComputeSuspensionRaycast
(const PxVehicleFrame& frame, const PxVehicleWheelParams& wheelParams, const PxVehicleSuspensionParams& suspensionParams,
	const PxF32 steerAngle, const PxTransform& rigidBodyPose,
	PxVec3& start, PxVec3& dir, PxReal& dist)
{
	const PxTransform wheelPose = PxVehicleComputeWheelPoseForSuspensionQuery(frame, suspensionParams, steerAngle, rigidBodyPose);

	//Raycast from top of wheel at max compression to bottom of wheel at max droop.
	dir = PxVehicleComputeSuspensionDirection(suspensionParams, rigidBodyPose);
	start = wheelPose.p - dir * wheelParams.radius;
	dist = suspensionParams.suspensionTravelDist + 2.0f*wheelParams.radius;
}

/**
\brief Compute the start pose, direction and length of a suspension scene sweep.
\param[in] frame is a description of the longitudinal, lateral and vertical axes.
\param[in] suspensionParams describes the suspension frame and the maximum suspension travel.
\param[in] steerAngle is the yaw angle of the wheel in radians.
\param[in] rigidBodyPose is the pose of the rigid body in the world frame.
\param[out] start is the start pose of the sweep in the world frame.
\param[out] dir is the direction of the sweep in the world frame.
\param[out] dist is the length of the sweep.
\note start, dir and dist together describe a sweep that begins with the wheel placed at maximum
compression and ends at the maximum droop pose.
*/
PX_FORCE_INLINE void PxVehicleComputeSuspensionSweep
(const PxVehicleFrame& frame, const PxVehicleSuspensionParams& suspensionParams,
	const PxReal steerAngle, const PxTransform& rigidBodyPose,
	PxTransform& start, PxVec3& dir, PxReal& dist)
{
	start = PxVehicleComputeWheelPoseForSuspensionQuery(frame, suspensionParams, steerAngle, rigidBodyPose);
	dir = PxVehicleComputeSuspensionDirection(suspensionParams, rigidBodyPose);
	dist = suspensionParams.suspensionTravelDist;
}


/**
\brief Compute the sprung masses of the suspension springs given (i) the number of sprung masses,
(ii) coordinates of the sprung masses in the rigid body frame, (iii) the center of mass offset of the rigid body, (iv) the
total mass of the rigid body, and (v) the direction of gravity
\param[in] nbSprungMasses is the number of sprung masses of the vehicle.  This value corresponds to the number of wheels on the vehicle.
\param[in] sprungMassCoordinates are the coordinates of the sprung masses in the rigid body frame. The array sprungMassCoordinates must be of
length nbSprungMasses or greater.
\param[in] totalMass is the total mass of all the sprung masses. 
\param[in] gravityDirection describes the direction of gravitational acceleration.
\param[out] sprungMasses are the masses to set in the associated suspension data with PxVehicleSuspensionData::mSprungMass.  The sprungMasses array must be of length
nbSprungMasses or greater. Each element in the sprungMasses array corresponds to the suspension located at the same array element in sprungMassCoordinates.
The center of mass of the masses in sprungMasses with the coordinates in sprungMassCoordinates satisfy the specified centerOfMass.
\return True if the sprung masses were successfully computed, false if the sprung masses were not successfully computed.
*/
bool PxVehicleComputeSprungMasses(const PxU32 nbSprungMasses, const PxVec3* sprungMassCoordinates, const PxReal totalMass, const PxVehicleAxes::Enum gravityDirection, PxReal* sprungMasses);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
