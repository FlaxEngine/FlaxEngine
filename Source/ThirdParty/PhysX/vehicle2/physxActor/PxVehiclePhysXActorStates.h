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

#include "foundation/PxPreprocessor.h"
#include "foundation/PxMemory.h"

#include "PxRigidBody.h"

#include "vehicle2/PxVehicleLimits.h"

#if !PX_DOXYGEN
namespace physx
{

class PxShape;

namespace vehicle2
{
#endif

/**
\brief A description of the PhysX actor and shapes that represent the vehicle in an associated PxScene.
*/
struct PxVehiclePhysXActor
{
	/**
	\brief The PhysX rigid body that represents the vehcle in the associated PhysX scene. 
	\note PxActorFlag::eDISABLE_GRAVITY must be set true on the PxRigidBody
	*/
	PxRigidBody* rigidBody;

	/**
	\brief An array of shapes with one shape pointer (or NULL) for each wheel.
	*/
	PxShape* wheelShapes[PxVehicleLimits::eMAX_NB_WHEELS];	

	PX_FORCE_INLINE void setToDefault()
	{
		PxMemZero(this, sizeof(PxVehiclePhysXActor));
	}
};


#define PX_VEHICLE_UNSPECIFIED_STEER_STATE PX_MAX_F32

/**
\brief A description of the previous steer command applied to the vehicle.
*/
struct PxVehiclePhysXSteerState
{

	/**
	\brief The steer command that was most previously applied to the vehicle.
	*/
	PxReal previousSteerCommand;

	PX_FORCE_INLINE void setToDefault()
	{
		previousSteerCommand = PX_VEHICLE_UNSPECIFIED_STEER_STATE;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
