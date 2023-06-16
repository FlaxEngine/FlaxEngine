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
#include "vehicle2/PxVehicleParams.h"
#include "vehicle2/PxVehicleComponent.h"

#include "vehicle2/braking/PxVehicleBrakingFunctions.h"
#include "vehicle2/commands/PxVehicleCommandHelpers.h"
#include "vehicle2/rigidBody/PxVehicleRigidBodyStates.h"
#include "vehicle2/steering/PxVehicleSteeringFunctions.h"
#include "vehicle2/steering/PxVehicleSteeringParams.h"
#include "vehicle2/wheel/PxVehicleWheelStates.h"
#include "vehicle2/wheel/PxVehicleWheelParams.h"
#include "vehicle2/tire/PxVehicleTireStates.h"

#include "PxVehicleDrivetrainStates.h"
#include "PxVehicleDrivetrainFunctions.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleBrakeCommandResponseParams;
struct PxVehicleDirectDriveThrottleCommandResponseParams;

/**
\brief Forward the applicable set of control values for a direct drive vehicle to a command response state for each 
applicable control value.  
\note The applicable control values are brake, handbrake, throttle and steer.
@see PxVehicleDirectDriveActuationStateComponent
@see PxVehicleDirectDrivetrainComponent
@see PxVehicleBrakeCommandLinearUpdate
@see PxVehicleDirectDriveThrottleLinearCommandUpdate
@see PxVehicleSteerCommandLinearUpdate
@see PxVehicleAckermannSteerUpdate
*/
class PxVehicleDirectDriveCommandResponseComponent : public PxVehicleComponent
{
public:

	PxVehicleDirectDriveCommandResponseComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleDirectDriveCommandResponseComponent() {}

	/**
	\brief Provide vehicle data items for this component.
	
	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] brakeResponseParams An array of brake response parameters with a brake response for each brake command.
	\param[out] throttleResponseParams The throttle response parameters.
	\param[out] steerResponseParams The steer response parameters.
	\param[out] ackermannParams The parameters defining Ackermann steering. NULL if no Ackermann steering is desired.
	\param[out] commands The throttle, brake, steer etc. command states.
	\param[out] transmissionCommands The transmission command state describing the current gear.
	\param[out] rigidBodyState The state of the vehicle's rigid body.
	\param[out] brakeResponseStates The resulting brake response states given the command input and brake response parameters.
	\param[out] throttleResponseStates The resulting throttle response states given the command input and throttle response parameters.
	\param[out] steerResponseStates The resulting steer response states given the command input, steer response and (optionally) Ackermann parameters.
	*/
	virtual void getDataForDirectDriveCommandResponseComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams, 
		const PxVehicleDirectDriveThrottleCommandResponseParams*& throttleResponseParams,
		const PxVehicleSteerCommandResponseParams*& steerResponseParams,
		PxVehicleSizedArrayData<const PxVehicleAckermannParams>& ackermannParams,
		const PxVehicleCommandState*& commands, const PxVehicleDirectDriveTransmissionCommandState*& transmissionCommands,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<PxReal>& brakeResponseStates, 
		PxVehicleArrayData<PxReal>& throttleResponseStates,
		PxVehicleArrayData<PxReal>& steerResponseStates) = 0;

	/**
	\brief Compute a per wheel response to the input brake/handbrake/throttle/steer commands  
	and determine if there is an intention to accelerate the vehicle.
	*/
	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleDirectDriveCommandResponseComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams> brakeResponseParams;
		const PxVehicleDirectDriveThrottleCommandResponseParams* throttleResponseParams;
		const PxVehicleSteerCommandResponseParams* steerResponseParams;
		PxVehicleSizedArrayData<const PxVehicleAckermannParams> ackermannParams;
		const PxVehicleCommandState* commands;
		const PxVehicleDirectDriveTransmissionCommandState* transmissionCommands;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<PxReal> brakeResponseStates;
		PxVehicleArrayData<PxReal> throttleResponseStates;
		PxVehicleArrayData<PxReal> steerResponseStates;

		getDataForDirectDriveCommandResponseComponent(axleDescription,
			brakeResponseParams,  throttleResponseParams, steerResponseParams, ackermannParams,
			commands, transmissionCommands,
			rigidBodyState,
			brakeResponseStates, throttleResponseStates, steerResponseStates);

		const PxReal longitudinalSpeed = rigidBodyState->getLongitudinalSpeed(context.frame);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			PxVehicleBrakeCommandResponseUpdate(
				commands->brakes, commands->nbBrakes, longitudinalSpeed,
				wheelId, brakeResponseParams,
				brakeResponseStates[wheelId]);

			PxVehicleDirectDriveThrottleCommandResponseUpdate(
				commands->throttle, *transmissionCommands, longitudinalSpeed,
				wheelId, *throttleResponseParams,
				throttleResponseStates[wheelId]);

			PxVehicleSteerCommandResponseUpdate(
				commands->steer, longitudinalSpeed,
				wheelId, *steerResponseParams, 
				steerResponseStates[wheelId]);
		}
		if (ackermannParams.size > 0)
			PxVehicleAckermannSteerUpdate(
				commands->steer, 
				*steerResponseParams, ackermannParams, 
				steerResponseStates);

		return true;
	}
};


/**
\brief Determine the actuation state for each wheel of a direct drive vehicle. 
\note The actuation state for each wheel contains a binary record of whether brake and drive torque are to be applied to the wheel.
@see PxVehicleDirectDriveCommandResponseComponent
@see PxVehicleDirectDrivetrainComponent
@see PxVehicleDirectDriveActuationStateUpdate
*/
class PxVehicleDirectDriveActuationStateComponent : public PxVehicleComponent
{
public:

	PxVehicleDirectDriveActuationStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleDirectDriveActuationStateComponent() {}


	/**
	\brief Provide vehicle data items for this component.

	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] brakeResponseStates The brake response states.
	\param[out] throttleResponseStates The throttle response states.
	\param[out] actuationStates The actuation states.
	*/
	virtual void getDataForDirectDriveActuationStateComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxReal>& brakeResponseStates,
		PxVehicleArrayData<const PxReal>& throttleResponseStates,
		PxVehicleArrayData<PxVehicleWheelActuationState>& actuationStates) = 0;

	/**
	\brief Compute the actuation state for each wheel given the brake, handbrake and throttle states.
	\*/
	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleDirectDriveActuationStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxReal> brakeResponseStates;
		PxVehicleArrayData<const PxReal> throttleResponseStates;
		PxVehicleArrayData<PxVehicleWheelActuationState> actuationStates;

		getDataForDirectDriveActuationStateComponent(
			axleDescription,
			brakeResponseStates, throttleResponseStates,
			actuationStates);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];
			PxVehicleDirectDriveActuationStateUpdate(
				brakeResponseStates[wheelId], throttleResponseStates[wheelId], 
				actuationStates[wheelId]);
		}

		return true;
	}
};

/**
\brief Forward integrate the angular speed of each wheel on a vehicle by integrating the 
brake and drive torque applied to each wheel and the torque that develops on the tire as a response 
to the longitudinal tire force.
@see PxVehicleDirectDriveUpdate
*/
class PxVehicleDirectDrivetrainComponent : public PxVehicleComponent
{
public:

	PxVehicleDirectDrivetrainComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleDirectDrivetrainComponent() {}

	virtual void getDataForDirectDrivetrainComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxReal>& brakeResponseStates,
		PxVehicleArrayData<const PxReal>& throttleResponseStates,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
		PxVehicleArrayData<const PxVehicleTireForce>& tireForces,
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleDirectDrivetrainComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxReal> brakeResponseStates;
		PxVehicleArrayData<const PxReal> throttleResponseStates;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleWheelActuationState> actuationStates;
		PxVehicleArrayData<const PxVehicleTireForce> tireForces;
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState> wheelRigidBody1dStates;

		getDataForDirectDrivetrainComponent(axleDescription, 
			brakeResponseStates, throttleResponseStates,
			wheelParams, actuationStates, tireForces,
			wheelRigidBody1dStates);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			PxVehicleDirectDriveUpdate(
				wheelParams[wheelId], actuationStates[wheelId],
				brakeResponseStates[wheelId], throttleResponseStates[wheelId],
				tireForces[wheelId],
				dt,
				wheelRigidBody1dStates[wheelId]);
		}

		return true;
	}
};

/**
\brief Forward the applicable set of control values for a vehicle driven by an engine to a command response state for each
applicable control value.

If parameters for an autobox are provided, the autobox will determine if a gear change should begin in order to 
maintain a desired engine revs.

@see PxVehicleBrakeCommandLinearUpdate
@see PxVehicleClutchCommandResponseLinearUpdate
@see PxVehicleEngineDriveThrottleCommandResponseUpdate
@see PxVehicleSteerCommandLinearUpdate
@see PxVehicleAckermannSteerUpdate
@see PxVehicleAutoBoxUpdate
@see PxVehicleGearCommandResponseUpdate
*/
class PxVehicleEngineDriveCommandResponseComponent : public PxVehicleComponent
{
public:

	PxVehicleEngineDriveCommandResponseComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleEngineDriveCommandResponseComponent() {}

	/**
	\brief Provide vehicle data items for this component.
	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] brakeResponseParams An array of brake response parameters with a brake response for each brake command.
	\param[out] steerResponseParams The steer response parameters.
	\param[out] ackermannParams The parameters defining Ackermann steering. NULL if no Ackermann steering is desired.
	\param[out] gearboxParams The gearbox parameters.
	\param[out] clutchResponseParams The clutch response parameters.
	\param[out] engineParams The engine parameters. Only needed if an autobox is provided (see autoboxParams), else it can be set to NULL.
	\param[out] engineState The engine state. Only needed if an autobox is provided (see autoboxParams), else it can be set to NULL.
	\param[out] autoboxParams The autobox parameters. If not NULL, the autobox will determine the target gear. Requires the parameters
				engineParams, engineState and autoboxState to be available. If no autobox is desired, NULL can be used in which case
				the aforementioned additional parameters can be set to NULL too.
	\param[out] rigidBodyState The state of the vehicle's rigid body.
	\param[out] commands The throttle, brake, steer etc. command states.
	\param[out] transmissionCommands The clutch, target gear etc. command states. If an autobox is provided (see autoboxParams)
				and the target gear is set to PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR, then the autobox will trigger
				gear shifts.
	\param[out] brakeResponseStates The resulting brake response states given the command input and brake response parameters.
	\param[out] throttleResponseState The resulting throttle response to the input throttle command.
	\param[out] steerResponseStates The resulting steer response states given the command input, steer response and (optionally) Ackermann parameters.
	\param[out] gearboxResponseState The resulting gearbox response state given the command input and gearbox parameters.
	\param[out] clutchResponseState The resulting clutch state given the command input and clutch response parameters.
	\param[out] autoboxState The resulting autobox state given the autobox/engine/gear params and engine state. Only needed if an autobox is
				provided (see autoboxParams), else it can be set to NULL.
	*/
	virtual void getDataForEngineDriveCommandResponseComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams,
		const PxVehicleSteerCommandResponseParams*& steerResponseParams,
		PxVehicleSizedArrayData<const PxVehicleAckermannParams>& ackermannParams,
		const PxVehicleGearboxParams*& gearboxParams,
		const PxVehicleClutchCommandResponseParams*& clutchResponseParams,
		const PxVehicleEngineParams*& engineParams,
		const PxVehicleRigidBodyState*& rigidBodyState,
		const PxVehicleEngineState*& engineState,
		const PxVehicleAutoboxParams*& autoboxParams,
		const PxVehicleCommandState*& commands,
		const PxVehicleEngineDriveTransmissionCommandState*& transmissionCommands,
		PxVehicleArrayData<PxReal>& brakeResponseStates,
		PxVehicleEngineDriveThrottleCommandResponseState*& throttleResponseState,
		PxVehicleArrayData<PxReal>& steerResponseStates,
		PxVehicleGearboxState*& gearboxResponseState,
		PxVehicleClutchCommandResponseState*& clutchResponseState,
		PxVehicleAutoboxState*& autoboxState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleEngineDriveCommandResponseComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams> brakeResponseParams;
		const PxVehicleSteerCommandResponseParams* steerResponseParams;
		PxVehicleSizedArrayData<const PxVehicleAckermannParams> ackermannParams;
		const PxVehicleGearboxParams* gearboxParams;
		const PxVehicleClutchCommandResponseParams* clutchResponseParams;
		const PxVehicleEngineParams* engineParams;
		const PxVehicleRigidBodyState* rigidBodyState;
		const PxVehicleEngineState* engineState;
		const PxVehicleAutoboxParams* autoboxParams;
		const PxVehicleCommandState* commands;
		const PxVehicleEngineDriveTransmissionCommandState* transmissionCommands;
		PxVehicleArrayData<PxReal> brakeResponseStates;
		PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState;
		PxVehicleArrayData<PxReal> steerResponseStates;
		PxVehicleGearboxState* gearboxResponseState;
		PxVehicleClutchCommandResponseState* clutchResponseState;
		PxVehicleAutoboxState* autoboxState;

		getDataForEngineDriveCommandResponseComponent(axleDescription, 
			brakeResponseParams, steerResponseParams, ackermannParams,
			gearboxParams, clutchResponseParams, engineParams, rigidBodyState,
			engineState, autoboxParams,
			commands, transmissionCommands,
			brakeResponseStates, throttleResponseState,
			steerResponseStates,
			gearboxResponseState, clutchResponseState, autoboxState);

		//The autobox can modify commands like throttle and target gear. Since the user defined
		//values should not be overwritten, a copy is used to compute the response.
		PxVehicleCommandState commandsTmp = *commands;
		PxVehicleEngineDriveTransmissionCommandState transmissionCommandsTmp = *transmissionCommands;

		const PxReal longitudinalSpeed = rigidBodyState->getLongitudinalSpeed(context.frame);

		//Let the autobox set the target gear, unless the user defined target gear requests
		//a shift already
		if (autoboxParams)
		{
			PX_ASSERT(engineParams);
			PX_ASSERT(engineState);
			PX_ASSERT(autoboxState);

			PxVehicleAutoBoxUpdate(
				*engineParams, *gearboxParams, *autoboxParams,
				*engineState, *gearboxResponseState, dt,
				transmissionCommandsTmp.targetGear, *autoboxState, commandsTmp.throttle);
		}
		else if (transmissionCommandsTmp.targetGear == PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR)
		{
			//If there is no autobox but eAUTOMATIC_GEAR was specified, use the current target gear
			transmissionCommandsTmp.targetGear = gearboxResponseState->targetGear;
		}

		//Distribute brake torque to the wheels across each axle.
		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];
			PxVehicleBrakeCommandResponseUpdate(
				commandsTmp.brakes, commandsTmp.nbBrakes, longitudinalSpeed,
				wheelId, brakeResponseParams,
				brakeResponseStates[i]);
		}

		//Update target gear as required.
		PxVehicleGearCommandResponseUpdate(
			transmissionCommandsTmp.targetGear,
			*gearboxParams,
			*gearboxResponseState);

		//Compute the response to the clutch command.
		PxVehicleClutchCommandResponseLinearUpdate(
			transmissionCommandsTmp.clutch,
			*clutchResponseParams,
			*clutchResponseState);

		//Compute the response to the throttle command.
		PxVehicleEngineDriveThrottleCommandResponseLinearUpdate(
			commandsTmp,
			*throttleResponseState);

		//Update the steer angles and Ackermann correction.
		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];
			PxVehicleSteerCommandResponseUpdate(commandsTmp.steer, longitudinalSpeed, wheelId, *steerResponseParams, steerResponseStates[i]);
		}
		if (ackermannParams.size > 0)
			PxVehicleAckermannSteerUpdate(commandsTmp.steer, *steerResponseParams, ackermannParams, steerResponseStates);

		return true;
	}
};

/**
\brief Compute the per wheel drive torque split of a multi-wheel drive differential.
@see PxVehicleDifferentialStateUpdate
*/
class PxVehicleMultiWheelDriveDifferentialStateComponent : public PxVehicleComponent
{
public:

	PxVehicleMultiWheelDriveDifferentialStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleMultiWheelDriveDifferentialStateComponent() {}

	virtual void getDataForMultiWheelDriveDifferentialStateComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleMultiWheelDriveDifferentialParams*& differentialParams,
		PxVehicleDifferentialState*& differentialState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleMultiWheelDriveDifferentialStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleMultiWheelDriveDifferentialParams* differentialParams;
		PxVehicleDifferentialState* differentialState;

		getDataForMultiWheelDriveDifferentialStateComponent(axleDescription,
			differentialParams, differentialState);

		PxVehicleDifferentialStateUpdate(
			*axleDescription,
			*differentialParams,
			*differentialState);

		return true;
	}
};

/**
\brief Compute the per wheel drive torque split of a differential delivering torque to multiple wheels 
with limited slip applied to specified wheel pairs.
@see PxVehicleDifferentialStateUpdate
*/
class PxVehicleFourWheelDriveDifferentialStateComponent : public PxVehicleComponent
{
public:

	PxVehicleFourWheelDriveDifferentialStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleFourWheelDriveDifferentialStateComponent() {}

	virtual void getDataForFourWheelDriveDifferentialStateComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleFourWheelDriveDifferentialParams*& differentialParams,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidbody1dStates,
		PxVehicleDifferentialState*& differentialState,
		PxVehicleWheelConstraintGroupState*& wheelConstraintGroupState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleFourWheelDriveDifferentialStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleFourWheelDriveDifferentialParams* differentialParams;
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState> wheelRigidbody1dStates;
		PxVehicleDifferentialState* differentialState;
		PxVehicleWheelConstraintGroupState* wheelConstraintGroupState;

		getDataForFourWheelDriveDifferentialStateComponent(axleDescription, differentialParams,
			wheelRigidbody1dStates,
			differentialState, wheelConstraintGroupState);

		PxVehicleDifferentialStateUpdate(
			*axleDescription, *differentialParams, 
			wheelRigidbody1dStates, dt,
			*differentialState, *wheelConstraintGroupState);

		return true;
	}
};

/**
\brief Compute the per wheel drive torque split of a tank drive differential.
@see PxVehicleDifferentialStateUpdate
*/
class PxVehicleTankDriveDifferentialStateComponent : public PxVehicleComponent
{
public:

	PxVehicleTankDriveDifferentialStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleTankDriveDifferentialStateComponent() {}

	/**
	\brief Provide vehicle data items for this component.

	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] transmissionCommands specifies the values of the thrust controllers that divert torque to the tank tracks.
	\param[out] wheelParams is an array describing the radius of each wheel.
	\param[out] differentialParams describes the operation of the tank differential by 
	specifying the default torque split between all wheels connected to the differential and by 
	specifying the wheels coupled to each tank track.
	\param[out] differentialState stores the instantaneous torque split between all wheels arising from the difference between the thrust controllers.
	\param[out] constraintGroupState stores the groups of wheels that are connected by sharing a tank track.
	*/
	virtual void getDataForTankDriveDifferentialStateComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleTankDriveTransmissionCommandState*& transmissionCommands,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		const PxVehicleTankDriveDifferentialParams*& differentialParams,
		PxVehicleDifferentialState*& differentialState,
		PxVehicleWheelConstraintGroupState*& constraintGroupState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleTankDriveDifferentialStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleTankDriveTransmissionCommandState* transmissionCommands;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		const PxVehicleTankDriveDifferentialParams* differentialParams;
		PxVehicleDifferentialState* differentialState;
		PxVehicleWheelConstraintGroupState* constraintGroupState;

		getDataForTankDriveDifferentialStateComponent(
			axleDescription,
			transmissionCommands,
			wheelParams,
			differentialParams, 
			differentialState, constraintGroupState);

		PxVehicleDifferentialStateUpdate(
			*axleDescription,
			wheelParams, *differentialParams, 
			transmissionCommands->thrusts[0], transmissionCommands->thrusts[1],
			*differentialState, *constraintGroupState);

		return true;
	}
};

/**
@deprecated

\brief Compute the per wheel drive torque split of a four wheel drive differential.
@see PxVehicleDifferentialStateUpdate
*/
class PX_DEPRECATED PxVehicleLegacyFourWheelDriveDifferentialStateComponent : public PxVehicleComponent
{
public:

	PxVehicleLegacyFourWheelDriveDifferentialStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleLegacyFourWheelDriveDifferentialStateComponent() {}

	virtual void getDataForLegacyFourWheelDriveDifferentialStateComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleFourWheelDriveDifferentialLegacyParams*& differentialParams,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidbody1dStates,
		PxVehicleDifferentialState*& differentialState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleLegacyFourWheelDriveDifferentialStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleFourWheelDriveDifferentialLegacyParams* differentialParams;
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState> wheelRigidbody1dStates;
		PxVehicleDifferentialState* differentialState;

		getDataForLegacyFourWheelDriveDifferentialStateComponent(axleDescription, differentialParams,
			wheelRigidbody1dStates,
			differentialState);

		PxVehicleDifferentialStateUpdate(
			*differentialParams, wheelRigidbody1dStates,
			*differentialState);

		return true;
	}
};

/**
\brief Determine the actuation state for each wheel for a vehicle propelled by engine torque.
\note The actuation state for each wheel contains a binary record of whether brake and drive torque are to be applied to the wheel.
@see PxVehicleEngineDriveActuationStateUpdate
*/
class PxVehicleEngineDriveActuationStateComponent : public PxVehicleComponent
{
public:

	PxVehicleEngineDriveActuationStateComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleEngineDriveActuationStateComponent() {}

	virtual void getDataForEngineDriveActuationStateComponent(
		const PxVehicleAxleDescription*& axleDescription, 
		const PxVehicleGearboxParams*& gearboxParams,
		PxVehicleArrayData<const PxReal>& brakeResponseStates,
		const PxVehicleEngineDriveThrottleCommandResponseState*& throttleResponseState,
		const PxVehicleGearboxState*& gearboxState,
		const PxVehicleDifferentialState*& differentialState,
		const PxVehicleClutchCommandResponseState*& clutchResponseState,
		PxVehicleArrayData<PxVehicleWheelActuationState>& actuationStates) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleEngineDriveActuationStateComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleGearboxParams* gearboxParams;
		PxVehicleArrayData<const PxReal> brakeResponseStates;
		const PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState;
		const PxVehicleGearboxState* gearboxState;
		const PxVehicleDifferentialState* differentialState;
		const PxVehicleClutchCommandResponseState* clutchResponseState;
		PxVehicleArrayData<PxVehicleWheelActuationState> actuationStates;

		getDataForEngineDriveActuationStateComponent(axleDescription, gearboxParams,
			brakeResponseStates, throttleResponseState,
			gearboxState, differentialState, clutchResponseState,
			actuationStates);

		PxVehicleEngineDriveActuationStateUpdate(
			*axleDescription, 
			*gearboxParams,
			brakeResponseStates, 
			*throttleResponseState,
			*gearboxState, *differentialState, *clutchResponseState,
			actuationStates);

		return true;
	}
};

/**
\brief Forward integrate the angular speed of each wheel and of the engine, accounting for the 
state of the clutch, gearbox and differential.
@see PxVehicleGearboxUpdate
@see PxVehicleEngineDrivetrainUpdate
*/
class PxVehicleEngineDrivetrainComponent : public PxVehicleComponent
{
public:

	PxVehicleEngineDrivetrainComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleEngineDrivetrainComponent() {}

	/**
	\brief Provide vehicle data items for this component.

	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] wheelParams specifies the radius of each wheel.
	\param[out] engineParams specifies the engine's torque curve, idle revs and max revs.
	\param[out] clutchParams specifies the maximum strength of the clutch.
	\param[out] gearboxParams specifies the gear ratio of each gear.
	\param[out] brakeResponseStates stores the instantaneous brake brake torque to apply to each wheel.
	\param[out] actuationStates stores whether a brake and/or drive torque are to be applied to each wheel.
	\param[out] tireForces stores the lateral and longitudinal tire force that has developed on each tire.
	\param[out] throttleResponseState stores the response of the throttle to the input throttle command.
	\param[out] clutchResponseState stores the instantaneous clutch strength that arises from the input clutch command.
	\param[out] differentialState stores the instantaneous torque split between the wheels.
	\param[out] constraintGroupState stores the groups of wheels that are subject to constraints that require them to have the same angular or linear velocity.
	\param[out] wheelRigidBody1dStates stores the per wheel angular speed to be computed by the component.
	\param[out] engineState stores  the engine rotation speed to be computed  by the component.
	\param[out] gearboxState stores the state of the gearbox to be computed by the component.
	\param[out] clutchState stores the clutch slip to be computed by the component.
 	\note If constraintGroupState is set to NULL it is assumed that there are no requirements for any wheels to have the same angular or linear velocity.
	*/
	virtual void getDataForEngineDrivetrainComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		const PxVehicleEngineParams*& engineParams,
		const PxVehicleClutchParams*& clutchParams,
		const PxVehicleGearboxParams*& gearboxParams, 
		PxVehicleArrayData<const PxReal>& brakeResponseStates,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
		PxVehicleArrayData<const PxVehicleTireForce>& tireForces,
		const PxVehicleEngineDriveThrottleCommandResponseState*& throttleResponseState,
		const PxVehicleClutchCommandResponseState*& clutchResponseState,
		const PxVehicleDifferentialState*& differentialState,
		const PxVehicleWheelConstraintGroupState*& constraintGroupState,
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
		PxVehicleEngineState*& engineState,
		PxVehicleGearboxState*& gearboxState,
		PxVehicleClutchSlipState*& clutchState) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehicleEngineDrivetrainComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		const PxVehicleEngineParams* engineParams;
		const PxVehicleClutchParams* clutchParams;
		const PxVehicleGearboxParams* gearboxParams;
		PxVehicleArrayData<const PxReal> brakeResponseStates;
		PxVehicleArrayData<const PxVehicleWheelActuationState> actuationStates;
		PxVehicleArrayData<const PxVehicleTireForce> tireForces;
		const PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState;
		const PxVehicleClutchCommandResponseState* clutchResponseState;
		const PxVehicleDifferentialState* differentialState;
		const PxVehicleWheelConstraintGroupState* constraintGroupState;
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState> wheelRigidBody1dStates;
		PxVehicleEngineState* engineState;
		PxVehicleGearboxState* gearboxState;
		PxVehicleClutchSlipState* clutchState;

		getDataForEngineDrivetrainComponent(axleDescription, wheelParams,
			engineParams, clutchParams, gearboxParams,
			brakeResponseStates, actuationStates, tireForces,
			throttleResponseState, clutchResponseState, differentialState, constraintGroupState,
			wheelRigidBody1dStates, engineState, gearboxState, clutchState);

		PxVehicleGearboxUpdate(*gearboxParams, dt, *gearboxState);

		PxVehicleEngineDrivetrainUpdate(
			*axleDescription, 
			wheelParams, 
			*engineParams, *clutchParams, *gearboxParams,  
			brakeResponseStates, actuationStates, 
			tireForces,  
			*gearboxState, *throttleResponseState, *clutchResponseState, *differentialState, constraintGroupState,
			dt, 
			wheelRigidBody1dStates, *engineState, *clutchState);

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
