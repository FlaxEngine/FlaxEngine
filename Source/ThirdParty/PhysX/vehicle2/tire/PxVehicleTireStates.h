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
#include "foundation/PxMemory.h"

#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif


/**
\brief PxVehicleTireDirectionState stores the world frame lateral and longtidinal axes of the tire after 
projecting the wheel pose in the world frame onto the road geometry plane (also in the world frame).
*/
struct PxVehicleTireDirectionState
{
	PxVec3 directions[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireDirectionState));
	}
};


/**
\brief PxVehicleTireSpeedState stores the components of the instantaneous velocity of the rigid body at the tire contact point projected 
along the lateral and longitudinal axes of the tire.
@see PxVehicleTireDirectionState
*/
struct PxVehicleTireSpeedState
{
	PxReal speedStates[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireSpeedState));
	}
};

/**
\brief The lateral and longitudinal tire slips.
@see PxVehicleTireSpeedState
*/
struct PxVehicleTireSlipState
{
	PxReal slips[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireSlipState));
	}
};

/**
\brief The load and friction experienced by a tire.
*/
struct PxVehicleTireGripState
{
	/**
	\brief The tire load

	<b>Unit:</b> force = mass * length / (time^2)
	*/
	PxReal load;

	/**
	\brief The tire friction is the product of the road geometry friction and a friction response multiplier.
	*/
	PxReal friction;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireGripState));
	}
};

/**
\brief Camber angle of the tire relative to the ground plane.
*/
struct PxVehicleTireCamberAngleState
{
	PxReal camberAngle;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireCamberAngleState));
	}
};

/**
\brief Prolonged low speeds in the lateral and longitudinal directions may be handled with "sticky" velocity constraints that activate after
a speed below a threshold has been recorded for a threshold time.
@see PxVehicleTireStickyParams
@see PxVehicleTireSpeedState
*/
struct PxVehicleTireStickyState
{
	PxReal lowSpeedTime[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];
	bool activeStatus[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireStickyState));
	}
};

/**
\brief The longitudinal/lateral forces/torques that develop on the tire.
*/
struct PxVehicleTireForce
{
	/*
	\brief The tire forces that develop along the tire's longitudinal and lateral directions. Specified in the world frame.
	*/
	PxVec3 forces[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	/*
	\brief The tire torques that develop around the tire's longitudinal and lateral directions. Specified in the world frame.
	*/
	PxVec3 torques[PxVehicleTireDirectionModes::eMAX_NB_PLANAR_DIRECTIONS];

	/**
	\brief The aligning moment may be propagated to a torque-driven steering controller.
	*/
	PxReal aligningMoment;

	/**
	\brief The torque to apply to the wheel's 1d rigid body.
	*/
	PxReal wheelTorque;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleTireForce));
	}
};


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
