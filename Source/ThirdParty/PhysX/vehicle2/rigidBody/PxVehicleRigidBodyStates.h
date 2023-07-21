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
#include "foundation/PxVec3.h"
#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleRigidBodyState
{
	PxTransform pose;				//!< the body's pose (in world space)
	PxVec3 linearVelocity;			//!< the body's linear velocity (in world space)
	PxVec3 angularVelocity;			//!< the body's angular velocity (in world space)
	PxVec3 previousLinearVelocity;	//!< the previous linear velocity of the body (in world space)
	PxVec3 previousAngularVelocity;	//!< the previous angular velocity of the body (in world space)
	PxVec3 externalForce;			//!< external force (in world space) affecting the rigid body (usually excluding gravitational force)
	PxVec3 externalTorque;			//!< external torque (in world space) affecting the rigid body

	PX_FORCE_INLINE void setToDefault()
	{
		pose = PxTransform(PxIdentity);
		linearVelocity = PxVec3(PxZero);
		angularVelocity = PxVec3(PxZero);
		externalForce = PxVec3(PxZero);
		externalTorque = PxVec3(PxZero);
	}

	/**
	\brief Compute the vertical speed of the rigid body transformed to the world frame.
	\param[in] frame describes the axes of the vehicle
	*/
	PX_FORCE_INLINE PxReal getVerticalSpeed(const PxVehicleFrame& frame) const
	{
		return linearVelocity.dot(pose.q.rotate(frame.getVrtAxis()));
	}

	/**
	\param[in] frame describes the axes of the vehicle
	\brief Compute the lateral speed of the rigid body transformed to the world frame.
	*/
	PX_FORCE_INLINE PxReal getLateralSpeed(const PxVehicleFrame& frame) const
	{
		return linearVelocity.dot(pose.q.rotate(frame.getLatAxis()));
	}

	/**
	\brief Compute the longitudinal speed of the rigid body transformed to the world frame.
	\param[in] frame describes the axes of the vehicle
	*/
	PX_FORCE_INLINE PxReal getLongitudinalSpeed(const PxVehicleFrame& frame) const
	{
		return linearVelocity.dot(pose.q.rotate(frame.getLngAxis()));
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
