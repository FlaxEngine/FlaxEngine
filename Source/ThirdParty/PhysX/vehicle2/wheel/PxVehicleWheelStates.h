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
#include "foundation/PxTransform.h"
#include "foundation/PxMemory.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief It is useful to know if a brake or drive torque is to be applied to a wheel. 
*/
struct PxVehicleWheelActuationState
{
	bool isBrakeApplied;		//!< True if a brake torque is applied, false if not.
	bool isDriveApplied;		//!< True if a drive torque is applied, false if not.

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleWheelActuationState));
	}
};

struct PxVehicleWheelRigidBody1dState
{
	/**
	\brief The rotation speed of the wheel around the lateral axis.

	<b>Unit:</b> radians / time
	*/
	PxReal rotationSpeed;

	/**
	\brief The corrected rotation speed of the wheel around the lateral axis in radians per second.

	At low forward wheel speed, the wheel rotation speed can get unstable (depending on the tire 
	model used) and, for example, oscillate. To integrate the wheel rotation angle, a (potentially)
	blended rotation speed is used which gets stored in #correctedRotationSpeed.

	<b>Unit:</b> radians / time

	@see PxVehicleSimulationContext::thresholdForwardSpeedForWheelAngleIntegration
	*/
	PxReal correctedRotationSpeed;

	/**
	\brief The accumulated angle of the wheel around the lateral axis in radians in range (-2*Pi,2*Pi)
	*/
	PxReal rotationAngle;

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehicleWheelRigidBody1dState));
	}
};

struct PxVehicleWheelLocalPose
{
	PxTransform localPose;		//!< The pose of the wheel in the rigid body frame.
	
	PX_FORCE_INLINE void setToDefault()
	{
		localPose = PxTransform(PxIdentity);
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
