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
#include "foundation/PxVec3.h"
#include "foundation/PxMemory.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

#define PX_VEHICLE_UNSPECIFIED_JOUNCE PX_MAX_F32
#define PX_VEHICLE_UNSPECIFIED_SEPARATION PX_MAX_F32

/**

*/
struct PxVehicleSuspensionState
{
	/**
	\brief jounce is the distance from maximum droop.
	\note jounce is positive semi-definite
	\note A value of 0.0 represents the suspension at maximum droop and zero suspension force.
	\note A value of suspensionTravelDist represents the suspension at maximum compression.
	\note jounce is clamped in range [0, suspensionTravelDist].
	*/
	PxReal jounce;

	/**
	\brief jounceSpeed is the rate of change of jounce.
	*/ 
	PxReal jounceSpeed;

	/**
	\brief separation holds extra information about the contact state of the wheel with the ground.
	
	If the suspension travel range is enough to place the wheel on the ground, then separation will be 0.
	If separation holds a negative value, then the wheel penetrates into the ground at maximum compression
	as well as maximum droop. The suspension would need to go beyond maximum compression (ground normal
	pointing in opposite direction of suspension) or beyond maximum droop (ground normal pointing in same
	direction as suspension) to place the wheel on the ground. In that case the separation value defines
	how much the wheel penetrates into the ground along the ground plane normal. This penetration may be
	resolved by using a constraint that simulates the effect of a bump stop.
	If separation holds a positive value, then the wheel does not penetrate the ground at maximum droop
	but can not touch the ground because the suspension would need to expand beyond max droop to reach it
	or because the suspension could not expand fast enough to reach the ground.
	*/
	PxReal separation;

	PX_FORCE_INLINE void setToDefault(const PxReal _jounce = PX_VEHICLE_UNSPECIFIED_JOUNCE,
		const PxReal _separation = PX_VEHICLE_UNSPECIFIED_SEPARATION)
	{
		jounce = _jounce;
		jounceSpeed = 0;
		separation = _separation;
	}
};

/**
\brief The effect of suspension compliance on toe and camber angle and on the tire and suspension force application points.
*/
struct PxVehicleSuspensionComplianceState
{	
	/**
	\brief The toe angle in radians that arises from suspension compliance.
	\note toe is expressed in the suspension frame.
	*/
	PxReal toe;				

	/**
	\brief The camber angle in radians that arises from suspension compliance.
	\note camber is expressed in the suspension frame.
	*/
	PxReal camber;

	/**
	\brief The tire force application point that arises from suspension compliance.
	\note tireForceAppPoint is expressed in the suspension frame.
	*/
	PxVec3 tireForceAppPoint;

	/**
	\brief The suspension force application point that arises from suspension compliance.
	\note suspForceAppPoint is expressed in the suspension frame.
	*/
	PxVec3 suspForceAppPoint;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleSuspensionComplianceState));
	}
};

/**
\brief The force and torque for a single suspension to apply to the vehicle's rigid body.
*/
struct PxVehicleSuspensionForce
{
	/**
	\brief The force to apply to the rigid body.
	\note force is expressed in the world frame.

	<b>Unit:</b> mass * length / (time^2)
	*/
	PxVec3 force;

	/**
	\brief The torque to apply to the rigid body.
	\note torque is expressed in the world frame.

	<b>Unit:</b> mass * (length^2) / (time^2)
	*/
	PxVec3 torque;

	/**
	\brief The component of force that lies along the normal of the plane under the wheel.
	\note normalForce may be used by the tire model as the tire load.

	<b>Unit:</b> mass * length / (time^2)
	*/
	PxReal normalForce;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleSuspensionForce));
	}
};

/**
\brief The anti-roll torque of all anti-roll bars accumulates in a single torque to apply
to the vehicle's rigid body.
*/
struct PxVehicleAntiRollTorque
{
	/**
	\brief The accumulated torque to apply to the rigid body.
	\note antiRollTorque is expressed in the world frame.

	<b>Unit:</b> mass * (length^2) / (time^2)
	*/
	PxVec3 antiRollTorque;						

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleAntiRollTorque));
	}
};


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
