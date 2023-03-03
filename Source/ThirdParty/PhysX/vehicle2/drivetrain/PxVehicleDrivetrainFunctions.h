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
#include "vehicle2/PxVehicleParams.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

struct PxVehicleCommandState;
struct PxVehicleDirectDriveTransmissionCommandState;
struct PxVehicleEngineDriveTransmissionCommandState;
struct PxVehicleDirectDriveThrottleCommandResponseParams;
struct PxVehicleWheelActuationState;
struct PxVehicleWheelParams;
struct PxVehicleTireForce;
struct PxVehicleWheelRigidBody1dState;
struct PxVehicleEngineParams;
struct PxVehicleGearboxParams;
struct PxVehicleAutoboxParams;
struct PxVehicleEngineState;
struct PxVehicleGearboxState;
struct PxVehicleAutoboxState;
struct PxVehicleClutchCommandResponseParams;
struct PxVehicleClutchCommandResponseState;
struct PxVehicleEngineDriveThrottleCommandResponseState;
struct PxVehicleDifferentialState;
struct PxVehicleMultiWheelDriveDifferentialParams;
struct PxVehicleFourWheelDriveDifferentialParams;
struct PxVehicleTankDriveDifferentialParams;
struct PxVehicleFourWheelDriveDifferentialLegacyParams;
struct PxVehicleClutchParams;
struct PxVehicleWheelConstraintGroupState;
struct PxVehicleClutchSlipState;

/**
\brief Compute the drive torque response to a throttle command.
\param[in] throttle is the throttle command.
\param[in] transmissionCommands is the gearing command to apply to the direct drive tranmission.
\param[in] longitudinalSpeed is the longitudinal speed of the vehicle's rigid body.
\param[in] wheelId specifies the wheel that is to have its throttle response computed.
\param[in] throttleResponseParams specifies the per wheel drive torque response to the throttle command as a nonlinear function of throttle command and longitudinal speed.
\param[out] throttleResponseState is the drive torque response to the input throttle command. 
*/
void PxVehicleDirectDriveThrottleCommandResponseUpdate
(const PxReal throttle, const PxVehicleDirectDriveTransmissionCommandState& transmissionCommands, const PxReal longitudinalSpeed,
 const PxU32 wheelId, const PxVehicleDirectDriveThrottleCommandResponseParams& throttleResponseParams,
 PxReal& throttleResponseState);

/**
\brief Determine the actuation state of a wheel given the brake torque, handbrake torque and drive torque applied to it.
\param[in] brakeTorque is the brake torque to be applied to the wheel.
\param[in] driveTorque is the drive torque to be applied to the wheel.
\param[out] actuationState contains a binary record of whether brake or drive torque is applied to the wheel.
*/
void PxVehicleDirectDriveActuationStateUpdate
(const PxReal brakeTorque, const PxReal driveTorque,
 PxVehicleWheelActuationState& actuationState);

/**
\brief Forward integrate the angular speed of a wheel given the brake and drive torque applied to it
\param[in] wheelParams specifies the moment of inertia of the wheel.
\param[in] actuationState is a binary record of whether brake and drive torque are to be applied to the wheel.
\param[in] brakeTorque is the brake torque to be applied to the wheel.
\param[in] driveTorque is the drive torque to be applied to the wheel.
\param[in] tireForce specifies the torque to apply to the wheel as a response to the longitudinal tire force.
\param[in] dt is the timestep of the forward integration.
\param[out] wheelRigidBody1dState describes the angular speed of the wheel.
*/
void PxVehicleDirectDriveUpdate
(const PxVehicleWheelParams& wheelParams,
 const PxVehicleWheelActuationState& actuationState, 
 const PxReal brakeTorque, const PxReal driveTorque, const PxVehicleTireForce& tireForce,
 const PxF32 dt, 
 PxVehicleWheelRigidBody1dState& wheelRigidBody1dState);

/**
\param[in] engineParams specifies the engine configuration.
\param[in] gearboxParams specifies the gear ratios and the time required to complete a gear change.
\param[in] autoboxParams specifies the conditions for switching gear.
\param[in] engineState contains the current angular speed of the engine.
\param[in] gearboxState describes the current and target gear.
\param[in] dt is the time that has lapsed since the last call to PxVehicleAutoBoxUpdate.
\param[in,out] targetGearCommand specifies the desired target gear for the gearbox. If set to
               PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR, the value will get overwritten
			   with a target gear chosen by the autobox.
\param[out] autoboxState specifies the time that has lapsed since the last automated gear change and contains a record of 
any ongoing automated gear change.
\param[out] throttle A throttle command value in [0, 1] that will be set to 0 if a gear change is initiated or is ongoing.

\note The autobox will not begin a gear change if a gear change is already ongoing.
\note The autobox will not begin a gear change until a threshold time has lapsed since the last automated gear change.
\note A gear change is considered as ongoing for as long as PxVehicleGearboxState::currentGear is different from 
 PxVehicleGearboxState::targetGear.
\note The autobox will not shift down from 1st gear or up from reverse gear.
\note The autobox shifts in single gear increments or decrements.
\note The autobox instantiates a gear change by setting PxVehicleCommandState::targetGear to be different from
from PxVehicleGearboxState::currentGear
*/
void PxVehicleAutoBoxUpdate
(const PxVehicleEngineParams& engineParams, const PxVehicleGearboxParams& gearboxParams, const PxVehicleAutoboxParams& autoboxParams,
 const PxVehicleEngineState& engineState, const PxVehicleGearboxState& gearboxState,
 const PxReal dt,
 PxU32& targetGearCommand, PxVehicleAutoboxState& autoboxState,
 PxReal& throttle);

/**
\brief Propagate input gear commands to the gearbox state.
\param[in] targetGearCommand specifies the target gear for the gearbox.
\param[in] gearboxParams specifies the number of gears and the index of neutral gear.
\param[out] gearboxState contains a record of the current and target gear.
\note Any ongoing gear change must complete before starting another.
\note A gear change is considered as ongoing for as long as PxVehicleGearboxState::currentGear is different from
 PxVehicleGearboxState::targetGear. 
 \note The gearbox remains in neutral for the duration of the gear change.
 \note A gear change begins if PxVehicleCommandState::targetGear is different from PxVehicleGearboxState::currentGear.
*/
void PxVehicleGearCommandResponseUpdate
(const PxU32 targetGearCommand,
	const PxVehicleGearboxParams& gearboxParams,
	PxVehicleGearboxState& gearboxState);

/**
\brief Propagate the input clutch command to the clutch response state.
\param[in] clutchCommand specifies the state of the clutch pedal.
\param[in] clutchResponseParams specifies how the clutch responds to the input clutch command.
\param[out] clutchResponse specifies the response of the clutch to the input clutch command.
*/
void PxVehicleClutchCommandResponseLinearUpdate
(const PxReal clutchCommand,
	const PxVehicleClutchCommandResponseParams& clutchResponseParams,
	PxVehicleClutchCommandResponseState& clutchResponse);

/**
\brief Propagate the input throttle command to the throttle response state.
\param[in] commands specifies the state of the throttle pedal.
\param[out] throttleResponse specifies how the clutch responds to the input throttle command.
*/
void PxVehicleEngineDriveThrottleCommandResponseLinearUpdate
(const PxVehicleCommandState& commands,
	PxVehicleEngineDriveThrottleCommandResponseState& throttleResponse);

/**
\brief Determine the actuation state of all wheels on a vehicle.
\param[in] axleDescription is a decription of the axles of the vehicle and the wheels on each axle.
\param[in] gearboxParams specifies the index of the neutral gear of the gearbox.
\param[in] brakeResponseStates specifies the response of each wheel to the input brake command.
\param[in] throttleResponseState specifies the response of the engine to the input throttle command.
\param[in] gearboxState specifies the current gear.
\param[in] diffState specifies the fraction of available drive torque to be delivered to each wheel.
\param[in] clutchResponseState specifies the response of the clutch to the input throttle command.
\param[out] actuationStates is an array of binary records determining whether brake and drive torque are to be applied to each wheel.
\note Drive torque is not applied to a wheel if 
   a) the gearbox is in neutral 
   b) the differential delivers no torque to the wheel
   c) no throttle is applied to the engine
   c) the clutch is fully disengaged.
*/
void PxVehicleEngineDriveActuationStateUpdate
(const PxVehicleAxleDescription& axleDescription,
 const PxVehicleGearboxParams& gearboxParams,
 const PxVehicleArrayData<const PxReal>& brakeResponseStates,
 const PxVehicleEngineDriveThrottleCommandResponseState& throttleResponseState,
 const PxVehicleGearboxState& gearboxState, const PxVehicleDifferentialState& diffState, const PxVehicleClutchCommandResponseState& clutchResponseState,
 PxVehicleArrayData<PxVehicleWheelActuationState>& actuationStates);

/**
@deprecated

\brief Compute the fraction of available torque to be delivered to each wheel and gather a list of all 
wheels connected to the differential.
\param[in] diffParams specifies the operation of a differential that can be connected to up to four wheels.
\param[in] wheelStates describes the angular speed of each wheel
\param[out] diffState contains the fraction of available drive torque to be delivered to each wheel.
\note If the handbrake is on then torque is only delivered to the wheels specified as the front wheels of the differential. 
*/
void PX_DEPRECATED PxVehicleDifferentialStateUpdate
(const PxVehicleFourWheelDriveDifferentialLegacyParams& diffParams,
 const PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelStates,
 PxVehicleDifferentialState& diffState);

/**
\brief Compute  the fraction of available torque to be delivered to each wheel and gather a list of all 
wheels connected to the differential. Additionally, add wheel constraints for wheel pairs whose
rotational speed ratio exceeds the corresponding differential bias.
\param[in] axleDescription is a decription of the axles of the vehicle and the wheels on each axle.
\param[in] diffParams describe the division of available drive torque and the biases of the limited 
slip differential.
\param[in] wheelStates describes the rotational speeds of each wheel.
\param[in] dt is the simulation time that has passes since the last call to PxVehicleDifferentialStateUpdate()
\param[out] diffState contains the fraction of available drive torque to be delivered to each wheel.
\param[out] wheelConstraintGroupState describes the groups of wheels that have exceeded their corresponding
differential biases.
*/
void PxVehicleDifferentialStateUpdate
(const PxVehicleAxleDescription& axleDescription, const PxVehicleFourWheelDriveDifferentialParams& diffParams,
 const PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelStates,
 const PxReal dt,
 PxVehicleDifferentialState& diffState, PxVehicleWheelConstraintGroupState& wheelConstraintGroupState);

/**
\brief Compute the fraction of available torque to be delivered to each wheel and gather a list of all
wheels connected to the differential.
\param[in] axleDescription is a decription of the axles of the vehicle and the wheels on each axle.
\param[in] diffParams specifies the operation of a differential that can be connected to any combination of wheels.
\param[out] diffState contains the fraction of available drive torque to be delivered to each wheel connected to the differential.
*/
void PxVehicleDifferentialStateUpdate
(const PxVehicleAxleDescription& axleDescription, const PxVehicleMultiWheelDriveDifferentialParams& diffParams,
 PxVehicleDifferentialState& diffState);

/**
\brief Compute the fraction of available torque to be delivered to each wheel and gather a list of all
wheels connected to the differential.
\param[in] axleDescription is a decription of the axles of the vehicle and the wheels on each axle.
\param[in] wheelParams is an array that describes the wheel radius of each wheel.
\param[in] diffParams specifies the operation of a tank differential.
\param[in] thrustCommand0 is the state of one of the two thrust controllers.
\param[in] thrustCommand1 is the state of one of the two thrust controllers.
\param[out] diffState contains the fraction of available drive torque to be delivered to each wheel connected to the differential.
\param[out] wheelConstraintGroupState describes the groups of wheels connected by sharing a tank track.
*/
void PxVehicleDifferentialStateUpdate
(const PxVehicleAxleDescription& axleDescription, 
 const PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams, const PxVehicleTankDriveDifferentialParams& diffParams,
 const PxReal thrustCommand0, PxReal thrustCommand1,
 PxVehicleDifferentialState& diffState, PxVehicleWheelConstraintGroupState& wheelConstraintGroupState);


/**
\brief Update the current gear of the gearbox. If a gear change is ongoing then complete the gear change if a threshold 
time has passed since the beginning of the gear change.
\param[in] gearboxParams describes the time required to complete a gear change.
\param[in] dt is the time that has lapsed since the last call to PxVehicleGearboxUpdate.
\param[out] gearboxState is the gearbox state to be updated.
\note A gear change is considered as ongoing for as long as PxVehicleGearboxState::currentGear is different from
 PxVehicleGearboxState::targetGear.
*/
void PxVehicleGearboxUpdate
(const PxVehicleGearboxParams& gearboxParams, 
 const PxF32 dt, 
 PxVehicleGearboxState& gearboxState);

/**
\brief Forward  integrate the angular speed of the vehicle's wheels and engine, given the state of clutch, differential and gearbox.
\param[in] axleDescription is a decription of the axles of the vehicle and the wheels on each axle.
\param[in] wheelParams specifies the moment of inertia of each wheel.
\param[in] engineParams specifies the torque curve of the engine and its moment of inertia.
\param[in] clutchParams specifies the maximum clutch strength that happens when the clutch is fully engaged.
\param[in] gearboxParams specifies the gearing ratios of the gearbox.
\param[in] brakeResponseStates describes the per wheel response to the input brake command.
\param[in] actuationStates is a binary record of whether brake or drive torque is applied to each wheel.
\param[in] tireForces describes the torque to apply to each wheel as a response to the longitudinal tire force. 
\param[in] gearboxState describes the current gear.
\param[in] throttleResponse describes the engine response to the input throttle pedal.
\param[in] clutchResponse describes the clutch response to the input clutch pedal.
\param[in] diffState describes the fraction of available drive torque to be delivered to each wheel.
\param[in] constraintGroupState describes groups of wheels with rotational speed constrained to the same value.
\param[in] dt is the time that has lapsed since the last call to PxVehicleEngineDrivetrainUpdate
\param[out] wheelRigidbody1dStates describes the angular speed of each wheel.
\param[out] engineState describes the angular speed of the engine.
\param[out] clutchState describes the clutch slip.
\note If constraintGroupState is NULL then it is assumed that there are no wheels subject to rotational speed constraints.
*/
void PxVehicleEngineDrivetrainUpdate
(const PxVehicleAxleDescription& axleDescription,
 const PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
 const PxVehicleEngineParams& engineParams, const PxVehicleClutchParams& clutchParams, const PxVehicleGearboxParams& gearboxParams, 
 const PxVehicleArrayData<const PxReal>& brakeResponseStates, 
 const PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
 const PxVehicleArrayData<const PxVehicleTireForce>& tireForces,
 const PxVehicleGearboxState& gearboxState, const PxVehicleEngineDriveThrottleCommandResponseState& throttleResponse, const PxVehicleClutchCommandResponseState& clutchResponse, 
 const PxVehicleDifferentialState& diffState, const PxVehicleWheelConstraintGroupState* constraintGroupState,
 const PxReal dt,
 PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidbody1dStates,
 PxVehicleEngineState& engineState, PxVehicleClutchSlipState& clutchState);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
