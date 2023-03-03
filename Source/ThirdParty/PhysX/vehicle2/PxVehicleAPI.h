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

#include "vehicle2/PxVehicleLimits.h"
#include "vehicle2/PxVehicleComponent.h"
#include "vehicle2/PxVehicleComponentSequence.h"
#include "vehicle2/PxVehicleParams.h"
#include "vehicle2/PxVehicleFunctions.h"
#include "vehicle2/PxVehicleMaths.h"

#include "vehicle2/braking/PxVehicleBrakingParams.h"
#include "vehicle2/braking/PxVehicleBrakingFunctions.h"

#include "vehicle2/commands/PxVehicleCommandParams.h"
#include "vehicle2/commands/PxVehicleCommandStates.h"
#include "vehicle2/commands/PxVehicleCommandHelpers.h"

#include "vehicle2/drivetrain/PxVehicleDrivetrainParams.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainStates.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainHelpers.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainFunctions.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainComponents.h"

#include "vehicle2/physxActor/PxVehiclePhysXActorStates.h"
#include "vehicle2/physxActor/PxVehiclePhysXActorHelpers.h"
#include "vehicle2/physxActor/PxVehiclePhysXActorFunctions.h"
#include "vehicle2/physxActor/PxVehiclePhysXActorComponents.h"

#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintParams.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintStates.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintHelpers.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintFunctions.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintComponents.h"

#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryState.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryParams.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryHelpers.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryFunctions.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryComponents.h"

#include "vehicle2/pvd/PxVehiclePvdHelpers.h"
#include "vehicle2/pvd/PxVehiclePvdFunctions.h"
#include "vehicle2/pvd/PxVehiclePvdComponents.h"

#include "vehicle2/rigidBody/PxVehicleRigidBodyParams.h"
#include "vehicle2/rigidBody/PxVehicleRigidBodyStates.h"
#include "vehicle2/rigidBody/PxVehicleRigidBodyFunctions.h"
#include "vehicle2/rigidBody/PxVehicleRigidBodyComponents.h"

#include "vehicle2/roadGeometry/PxVehicleRoadGeometryState.h"

#include "vehicle2/steering/PxVehicleSteeringParams.h"
#include "vehicle2/steering/PxVehicleSteeringFunctions.h"

#include "vehicle2/suspension/PxVehicleSuspensionParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionStates.h"
#include "vehicle2/suspension/PxVehicleSuspensionHelpers.h"
#include "vehicle2/suspension/PxVehicleSuspensionFunctions.h"
#include "vehicle2/suspension/PxVehicleSuspensionComponents.h"

#include "vehicle2/tire/PxVehicleTireParams.h"
#include "vehicle2/tire/PxVehicleTireStates.h"
#include "vehicle2/tire/PxVehicleTireHelpers.h"
#include "vehicle2/tire/PxVehicleTireFunctions.h"
#include "vehicle2/tire/PxVehicleTireComponents.h"

#include "vehicle2/wheel/PxVehicleWheelParams.h"
#include "vehicle2/wheel/PxVehicleWheelStates.h"
#include "vehicle2/wheel/PxVehicleWheelHelpers.h"
#include "vehicle2/wheel/PxVehicleWheelFunctions.h"
#include "vehicle2/wheel/PxVehicleWheelComponents.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

	/** \brief Initialize the PhysX Vehicle library.

	This should be called before calling any functions or methods in extensions which may require allocation.
	\note This function does not need to be called before creating a PxDefaultAllocator object.

	\param foundation a PxFoundation object

	@see PxCloseVehicleExtension PxFoundation 
	*/
	PX_FORCE_INLINE bool PxInitVehicleExtension(physx::PxFoundation& foundation)
	{
		PX_UNUSED(foundation);
		PX_CHECK_AND_RETURN_VAL(&PxGetFoundation() == &foundation, "Supplied foundation must match the one that will be used to perform allocations", false);
		PxIncFoundationRefCount();
		return true;
	}

	/** \brief Shut down the PhysX Vehicle library.

	This function should be called to cleanly shut down the PhysX Vehicle library before application exit.

	\note This function is required to be called to release foundation usage.

	@see PxInitVehicleExtension
	*/
	PX_FORCE_INLINE void PxCloseVehicleExtension()
	{
		PxDecFoundationRefCount();
	}


#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
