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

#include "vehicle2/PxVehicleParams.h"
#include "vehicle2/PxVehicleComponent.h"

#include "vehicle2/wheel/PxVehicleWheelStates.h"

#include "PxVehiclePhysXActorFunctions.h"
#include "PxVehiclePhysXActorStates.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Work items at the beginning of an update step for a PhysX actor based vehicle.

Includes:
  - Waking the actor up if it is sleeping and a throttle or steer command is issued.
  - Clearing certain states if the actor is sleeping.
  - Reading the state from the PhysX actor and copy to the vehicle internal state.

@see PxVehiclePhysxActorWakeup PxVehiclePhysxActorSleepCheck PxVehicleReadRigidBodyStateFromPhysXActor
*/
class PxVehiclePhysXActorBeginComponent : public PxVehicleComponent
{
public:

	PxVehiclePhysXActorBeginComponent() : PxVehicleComponent() {}
	virtual ~PxVehiclePhysXActorBeginComponent() {}

	/**
	\brief Provide vehicle data items for this component.

	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] commands are the brake, throttle and steer values that will drive the vehicle.
	\param[out] transmissionCommands are the target gear and clutch values that will control
				the transmission. Can be set to NULL if the vehicle does not have a gearbox. If
				specified, then gearParams and gearState has to be specifed too.
	\param[out] gearParams The gearbox parameters. Can be set to NULL if the vehicle does
				not have a gearbox and transmissionCommands is NULL.
	\param[out] gearState The state of the gearbox. Can be set to NULL if the vehicle does
				not have a gearbox and transmissionCommands is NULL.
	\param[out] engineParams The engine parameters. Can be set to NULL if the vehicle does
				not have an engine. Must be specified, if engineState is specified.
	\param[out] physxActor is the PxRigidBody instance associated with the vehicle.
	\param[out] physxSteerState is the previous state of the steer and is used to determine if the
				steering wheel has changed by comparing with PxVehicleCommandState::steer.
	\param[out] physxConstraints The state of the suspension limit and low speed tire constraints.
				If the vehicle actor is sleeping and constraints are active, they will be
				deactivated and marked as dirty.
	\param[out] rigidBodyState is the state of the rigid body used by the Vehicle SDK.
	\param[out] wheelRigidBody1dStates describes the angular speed of each wheel.
	\param[out] engineState The engine state. Can be set to NULL if the vehicle does
				not have an engine. If specified, then engineParams has to be specifed too.
	*/
	virtual void getDataForPhysXActorBeginComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleCommandState*& commands,
		const PxVehicleEngineDriveTransmissionCommandState*& transmissionCommands,
		const PxVehicleGearboxParams*& gearParams,
		const PxVehicleGearboxState*& gearState,
		const PxVehicleEngineParams*& engineParams,
		PxVehiclePhysXActor*& physxActor,
		PxVehiclePhysXSteerState*& physxSteerState,
		PxVehiclePhysXConstraints*& physxConstraints,
		PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
		PxVehicleEngineState*& engineState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehiclePhysXActorBeginComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleCommandState* commands;
		const PxVehicleEngineDriveTransmissionCommandState* transmissionCommands;
		const PxVehicleGearboxParams* gearParams;
		const PxVehicleGearboxState* gearState;
		const PxVehicleEngineParams* engineParams;
		PxVehiclePhysXActor* physxActor;
		PxVehiclePhysXSteerState* physxSteerState;
		PxVehiclePhysXConstraints* physxConstraints;
		PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState> wheelRigidBody1dStates;
		PxVehicleEngineState* engineState;

		getDataForPhysXActorBeginComponent(axleDescription, commands, transmissionCommands,
			gearParams, gearState, engineParams,
			physxActor, physxSteerState, physxConstraints, 
			rigidBodyState, wheelRigidBody1dStates, engineState);

		if (physxActor->rigidBody->getScene())  // Considering case where actor is not in a scene and constraints get solved via immediate mode
		{
			PxVehiclePhysxActorWakeup(*commands, transmissionCommands, gearParams, gearState, 
				*physxActor->rigidBody, *physxSteerState);

			if (PxVehiclePhysxActorSleepCheck(*axleDescription, *physxActor->rigidBody, engineParams,
				*rigidBodyState, *physxConstraints, wheelRigidBody1dStates, engineState))
			{
				return false;
			}
		}

		PxVehicleReadRigidBodyStateFromPhysXActor(*physxActor->rigidBody, *rigidBodyState);

		return true;
	}
};

/**
\brief Work items at the end of an update step for a PhysX actor based vehicle.

Includes:
  - Writing vehicle internal state to the PhysX actor.
  - Keeping the vehicle awake if certain criteria are met.
*/
class PxVehiclePhysXActorEndComponent : public PxVehicleComponent
{
public:

	PxVehiclePhysXActorEndComponent() : PxVehicleComponent() {}
	virtual ~PxVehiclePhysXActorEndComponent() {}

	/**
	\brief Provide vehicle data items for this component.

	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] rigidBodyState is the state of the rigid body used by the Vehicle SDK.
	\param[out] wheelParams describes the radius, mass etc. of the wheels.
	\param[out] wheelShapeLocalPoses are the local poses in the wheel's frame to apply to the PxShape instances that represent the wheel
	\param[out] wheelRigidBody1dStates describes the angular speed of the wheels.
	\param[out] wheelLocalPoses describes the local poses of the wheels in the rigid body frame.
	\param[out] gearState The gear state. Can be set to NULL if the vehicle does
	            not have gears.
	\param[out] physxActor is the PxRigidBody instance associated with the vehicle.
	*/
	virtual void getDataForPhysXActorEndComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxTransform>& wheelShapeLocalPoses,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates, 
		PxVehicleArrayData<const PxVehicleWheelLocalPose>& wheelLocalPoses,
		const PxVehicleGearboxState*& gearState,
		PxVehiclePhysXActor*& physxActor) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);

		PX_PROFILE_ZONE("PxVehiclePhysXActorEndComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxTransform> wheelShapeLocalPoses;
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState> wheelRigidBody1dStates;
		PxVehicleArrayData<const PxVehicleWheelLocalPose> wheelLocalPoses;
		const PxVehicleGearboxState* gearState;
		PxVehiclePhysXActor* physxActor;

		getDataForPhysXActorEndComponent(axleDescription, rigidBodyState,
			wheelParams, wheelShapeLocalPoses, wheelRigidBody1dStates, wheelLocalPoses, gearState,
			physxActor);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];
			
			PxVehicleWriteWheelLocalPoseToPhysXWheelShape(wheelLocalPoses[wheelId].localPose, wheelShapeLocalPoses[wheelId], 
				physxActor->wheelShapes[wheelId]);
		}

		if (context.getType() == PxVehicleSimulationContextType::ePHYSX)
		{
			const PxVehiclePhysXSimulationContext& physxContext = static_cast<const PxVehiclePhysXSimulationContext&>(context);

			PxVehicleWriteRigidBodyStateToPhysXActor(physxContext.physxActorUpdateMode, *rigidBodyState, dt, *physxActor->rigidBody);

			PxVehiclePhysxActorKeepAwakeCheck(*axleDescription, wheelParams, wheelRigidBody1dStates,
				physxContext.physxActorWakeCounterThreshold, physxContext.physxActorWakeCounterResetValue, gearState,
				*physxActor->rigidBody);
		}
		else
		{
			PX_ALWAYS_ASSERT();
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
