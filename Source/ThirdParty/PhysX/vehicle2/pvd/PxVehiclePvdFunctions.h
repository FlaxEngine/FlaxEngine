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
#include "vehicle2/braking/PxVehicleBrakingParams.h"
#include "vehicle2/commands/PxVehicleCommandStates.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainParams.h"
#include "vehicle2/drivetrain/PxVehicleDrivetrainStates.h"
#include "vehicle2/physxActor/PxVehiclePhysXActorStates.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintParams.h"
#include "vehicle2/physxConstraints/PxVehiclePhysXConstraintStates.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryParams.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryState.h"
#include "vehicle2/roadGeometry/PxVehicleRoadGeometryState.h"
#include "vehicle2/steering/PxVehicleSteeringParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionStates.h"
#include "vehicle2/tire/PxVehicleTireParams.h"
#include "vehicle2/tire/PxVehicleTireStates.h"
#include "vehicle2/wheel/PxVehicleWheelParams.h"
#include "vehicle2/wheel/PxVehicleWheelStates.h"

class OmniPvdWriter;
namespace physx
{
class PxAllocatorCallback;
namespace vehicle2
{
struct PxVehiclePvdAttributeHandles;
struct PxVehiclePvdObjectHandles;
} // namespace vehicle2
} // namespace physx

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

/**
\brief Create object instances in omnipvd that will be used to reflect the parameters and state of the rigid body of a vehicle instance.
Handles to the created object instances will be stored in a PxVehiclePvdObjectHandles instance.
\param[in] rbodyParams describes the parameters of the vehicle's rigid body.
\param[in] rbodyState describes the state of the vehicle's rigid body.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If rbodyParams is NULL, omnipvd will not reflect rigid body parameters.
\note If rbodyState is NULL, omnipvd will not reflect rigid body state.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
\note PxVehiclePvdRigidBodyRegister should be called once for each vehicle instance.
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdRigidBodyWrite
*/
void PxVehiclePvdRigidBodyRegister
(const PxVehicleRigidBodyParams* rbodyParams, const PxVehicleRigidBodyState* rbodyState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write the parameters and state of the rigid body of a vehicle instance to omnipvd.
\param[in] rbodyParams describes the parameters of the vehicle's rigid body.
\param[in] rbodyState describes the state of the vehicle's rigid body.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If rbodyParams is NULL but a non-NULL value was used in PxVehiclePvdRigidBodyRegister(), the rigid body parameters will not be updated in omnipvd.
\note If rbodyState is NULL but a non-NULL value was used in PxVehiclePvdRigidBodyRegister(), the rigid body state will not be updated in omnipvd.
\note omniWriter must be non-NULL and must be the same instance used in PxVehiclePvdRigidBodyRegister().
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdRigidBodyRegister
*/
void PxVehiclePvdRigidBodyWrite
(const PxVehicleRigidBodyParams* rbodyParams, const PxVehicleRigidBodyState* rbodyState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect the suspension state calculation parameters of a vehicle instance.
\param[in] suspStateCalcParams describes parameters used to calculate suspension state.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If suspStateCalcParams is NULL, omnipvd will not reflect the suspension state calculation parameters.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdSuspensionStateCalculationParamsWrite
*/
void PxVehiclePvdSuspensionStateCalculationParamsRegister
(const PxVehicleSuspensionStateCalculationParams* suspStateCalcParams, 
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write the parameters and state of the rigid body of a vehicle instance to omnipvd.
\param[in] suspStateCalcParams describes parameters used to calculate suspension state.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If suspStateCalcParams is NULL but a non-NULL value was used in void PxVehiclePvdSuspensionStateCalculationParamsRegister(), 
the suspension state calculation parameters will not be updated in omnipvd.
\note omniWriter must be non-NULL and must be the same instance used in PxVehiclePvdSuspensionStateCalculationParamsRegister().
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdSuspensionStateCalculationParamsRegister
*/
void PxVehiclePvdSuspensionStateCalculationParamsWrite
(const PxVehicleSuspensionStateCalculationParams* suspStateCalcParams, 
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect the brake and steer command response parameters of a vehicle instance.
\param[in] brakeResponseParams is an array of brake command response parameters.
\param[in] steerResponseParams describes the steer command response parameters.
\param[in] brakeResponseStates is an array of brake responses torqyes,
\param[in] steerResponseStates is an array of steer response angles.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If brakeResponseParams is empty, omnipvd will not reflect any brake command response parameters. 
\note If steerResponseParams is NULL, omnipvd will not reflect the steer command response parameters.
\note If brakeResponseStates is empty, omnipvd will not reflect any brake command response state.
\note If steerResponseStates is empty, omnipvd will not reflect any steer command response state.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdCommandResponseWrite
*/
void PxVehiclePvdCommandResponseRegister
(const PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams,
 const PxVehicleSteerCommandResponseParams* steerResponseParams, 
 const PxVehicleArrayData<PxReal>& brakeResponseStates,
 const PxVehicleArrayData<PxReal>& steerResponseStates,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write brake and steer command response parameters to omnipvd.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] brakeResponseParams is an array of brake command response parameters.
\param[in] steerResponseParams describes the steer command response parameters.
\param[in] brakeResponseStates is an array of brake response torques.
\param[in] steerResponseStates is an array of steer response angles.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If brakeResponseParams is empty but a non-empty array was used in PxVehiclePvdCommandResponseRegister(), 
the brake command response parameters will not be updated in omnipvd.
\note If steerResponseParams is non-NULL but a NULL value was used in PxVehiclePvdCommandResponseRegister(), 
the steer command parameters will not be updated in omnipvd. 
\note If brakeResponseStates is empty but a non-empty array was used in PxVehiclePvdCommandResponseRegister(), 
the brake response states will not be updated in omnipvd.
\note If steerResponseStates is empty but a non-empty array was used in PxVehiclePvdCommandResponseRegister(), 
the steer response states will not be updated in omnipvd. 
\note omniWriter must be non-NULL and must be the same instance used in PxVehiclePvdCommandResponseRegister().
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdCommandResponseRegister
*/
void PxVehiclePvdCommandResponseWrite
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams,
 const PxVehicleSteerCommandResponseParams* steerResponseParams, 
 const PxVehicleArrayData<PxReal>& brakeResponseStates,
 const PxVehicleArrayData<PxReal>& steerResponseStates,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect wheel attachment data such as tires, suspensions and wheels.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] wheelParams is an array of wheel parameters.
\param[in] wheelActuationStates is an array of wheel actuation states.
\param[in] wheelRigidBody1dStates is an array of wheel rigid body states.
\param[in] wheelLocalPoses is an array of wheel local poses.
\param[in] roadGeometryStates is an array of road geometry states.
\param[in] suspParams is an array of suspension parameters.
\param[in] suspCompParams is an array of suspension compliance parameters.
\param[in] suspForceParams is an array of suspension force parameters.
\param[in] suspStates is an array of suspension states.
\param[in] suspCompStates is an array of suspension compliance states.
\param[in] suspForces is an array of suspension forces.
\param[in] tireForceParams is an array of tire force parameters.
\param[in] tireDirectionStates is an array of tire direction states.
\param[in] tireSpeedStates is an array of tire speed states.
\param[in] tireSlipStates is an array of tire slip states.
\param[in] tireStickyStates is an array of tire sticky states.
\param[in] tireGripStates is an array of tire grip states.
\param[in] tireCamberStates is an array of tire camber states.
\param[in] tireForces is an array of tire forces.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any array is empty, the corresponding data will not be reflected in omnipvd.
\note Each array must either be empty or contain an entry for each wheel present in axleDesc.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdWheelAttachmentsWrite
*/
void PxVehiclePvdWheelAttachmentsRegister
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
 const PxVehicleArrayData<const PxVehicleWheelActuationState>& wheelActuationStates,
 const PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
 const PxVehicleArrayData<const PxVehicleWheelLocalPose>& wheelLocalPoses,
 const PxVehicleArrayData<const PxVehicleRoadGeometryState>& roadGeometryStates,
 const PxVehicleArrayData<const PxVehicleSuspensionParams>& suspParams,
 const PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspCompParams,
 const PxVehicleArrayData<const PxVehicleSuspensionForceParams>& suspForceParams,
 const PxVehicleArrayData<const PxVehicleSuspensionState>& suspStates,
 const PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspCompStates,
 const PxVehicleArrayData<const PxVehicleSuspensionForce>& suspForces,
 const PxVehicleArrayData<const PxVehicleTireForceParams>& tireForceParams,
 const PxVehicleArrayData<const PxVehicleTireDirectionState>& tireDirectionStates,
 const PxVehicleArrayData<const PxVehicleTireSpeedState>& tireSpeedStates,
 const PxVehicleArrayData<const PxVehicleTireSlipState>& tireSlipStates,
 const PxVehicleArrayData<const PxVehicleTireStickyState>& tireStickyStates,
 const PxVehicleArrayData<const PxVehicleTireGripState>& tireGripStates,
 const PxVehicleArrayData<const PxVehicleTireCamberAngleState>& tireCamberStates,
 const PxVehicleArrayData<const PxVehicleTireForce>& tireForces, 
 const PxVehiclePvdAttributeHandles& attributeHandles, 
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write wheel attachment data such as tire, suspension and wheel data to omnipvd.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] wheelParams is an array of wheel parameters.
\param[in] wheelActuationStates is an array of wheel actuation states.
\param[in] wheelRigidBody1dStates is an array of wheel rigid body states.
\param[in] wheelLocalPoses is an array of wheel local poses.
\param[in] roadGeometryStates is an array of road geometry states.
\param[in] suspParams is an array of suspension parameters.
\param[in] suspCompParams is an array of suspension compliance parameters.
\param[in] suspForceParams is an array of suspension force parameters.
\param[in] suspStates is an array of suspension states.
\param[in] suspCompStates is an array of suspension compliance states.
\param[in] suspForces is an array of suspension forces.
\param[in] tireForceParams is an array of tire force parameters.
\param[in] tireDirectionStates is an array of tire direction states.
\param[in] tireSpeedStates is an array of tire speed states.
\param[in] tireSlipStates is an array of tire slip states.
\param[in] tireStickyStates is an array of tire sticky states.
\param[in] tireGripStates is an array of tire grip states.
\param[in] tireCamberStates is an array of tire camber states.
\param[in] tireForces is an array of tire forces.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any array is empty but the corresponding argument in PxVehiclePvdWheelAttachmentsRegister was not empty, the corresponding data will not be updated in omnipvd.
\note Each array must either be empty or contain an entry for each wheel present in axleDesc.
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdWheelAttachmentsRegister
*/
void PxVehiclePvdWheelAttachmentsWrite
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
 const PxVehicleArrayData<const PxVehicleWheelActuationState>& wheelActuationStates,
 const PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
 const PxVehicleArrayData<const PxVehicleWheelLocalPose>& wheelLocalPoses,
 const PxVehicleArrayData<const PxVehicleRoadGeometryState>& roadGeometryStates,
 const PxVehicleArrayData<const PxVehicleSuspensionParams>& suspParams,
 const PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspCompParams,
 const PxVehicleArrayData<const PxVehicleSuspensionForceParams>& suspForceParams,
 const PxVehicleArrayData<const PxVehicleSuspensionState>& suspStates,
 const PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspCompStates,
 const PxVehicleArrayData<const PxVehicleSuspensionForce>& suspForces,
 const PxVehicleArrayData<const PxVehicleTireForceParams>& tireForceParams,
 const PxVehicleArrayData<const PxVehicleTireDirectionState>& tireDirectionStates,
 const PxVehicleArrayData<const PxVehicleTireSpeedState>& tireSpeedStates,
 const PxVehicleArrayData<const PxVehicleTireSlipState>& tireSlipStates,
 const PxVehicleArrayData<const PxVehicleTireStickyState>& tireStickyStates,
 const PxVehicleArrayData<const PxVehicleTireGripState>& tireGripStates,
 const PxVehicleArrayData<const PxVehicleTireCamberAngleState>& tireCamberStates,
 const PxVehicleArrayData<const PxVehicleTireForce>& tireForces, 
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect the antiroll bars of a vehicle instance.
\param[in] antiRollForceParams is an array of antiroll parameters.
\param[in] antiRollTorque describes the instantaneous accumulated torque from all antiroll bas.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If antiRollForceParams is empty, the corresponding data will not be reflected in omnipvd.
\note If antiRollTorque is NULL, the corresponding data will not be reflected in omnipvd.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdAntiRollsWrite
*/
void PxVehiclePvdAntiRollsRegister
(const PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams, const PxVehicleAntiRollTorque* antiRollTorque,
 const PxVehiclePvdAttributeHandles& attributeHandles, 
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write antiroll data to omnipvd.
\param[in] antiRollForceParams is an array of antiroll parameters.
\param[in] antiRollTorque describes the instantaneous accumulated torque from all antiroll bas.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If antiRollForceParams is empty but the corresponding argument was not empty, the corresponding data will not be updated in omnipvd.
\note If antiRollTorque is NULL but the corresponding argument was not NULL, the corresponding data will not be updated in omnipvd.
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdAntiRollsRegister
*/
void PxVehiclePvdAntiRollsWrite
(const PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams, const PxVehicleAntiRollTorque* antiRollTorque,
 const PxVehiclePvdAttributeHandles& attributeHandles, 
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect the direct drivetrain of a vehicle instance.
\param[in] commandState describes the control values applied to the vehicle.
\param[in] transmissionCommandState describes the state of the direct drive transmission.
\param[in] directDriveThrottleResponseParams describes the vehicle's torque response to a throttle command.
\param[in] directDriveThrottleResponseState is the instantaneous torque response of each wheel to a throttle command.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If commandState is NULL, the corresponding data will not be reflected in omnipvd.
\note If transmissionCommandState is NULL, the corresponding data will not be reflected in omnipvd.
\note If directDriveThrottleResponseParams is NULL, the corresponding data will not be reflected in omnipvd.
\note If directDriveThrottleResponseState is empty, the corresponding data will not be reflected in omnipvd. 
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdDirectDrivetrainWrite
*/
void PxVehiclePvdDirectDrivetrainRegister
(const PxVehicleCommandState* commandState, const PxVehicleDirectDriveTransmissionCommandState* transmissionCommandState,
 const PxVehicleDirectDriveThrottleCommandResponseParams* directDriveThrottleResponseParams,
 const PxVehicleArrayData<PxReal>& directDriveThrottleResponseState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write direct drivetrain data to omnipvd.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] commandState describes the control values applied to the vehicle.
\param[in] transmissionCommandState describes the state of the direct drive transmission.
\param[in] directDriveThrottleResponseParams describes the vehicle's torque response to a throttle command.
\param[in] directDriveThrottleResponseState is the instantaneous torque response of each wheel to a throttle command.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If commandState is NULL but the corresponding entry in PxVehiclePvdDirectDrivetrainRegister() was not NULL, the corresponding data will not be updated in omnipvd.
\note If transmissionCommandState is NULL but the corresponding entry in PxVehiclePvdDirectDrivetrainRegister() was not NULL, the corresponding data will not be updated in omnipvd.
\note If directDriveThrottleResponseParams is NULL but the corresponding entry in PxVehiclePvdDirectDrivetrainRegister() was not NULL, the corresponding data will not be updated in omnipvd.
\note If directDriveThrottleResponseState is empty but the corresponding entry in PxVehiclePvdDirectDrivetrainRegister() was not empty, the corresponding data will not be updated in omnipvd.
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdDirectDrivetrainRegister
*/
void PxVehiclePvdDirectDrivetrainWrite
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleCommandState* commandState, const PxVehicleDirectDriveTransmissionCommandState* transmissionCommandState,
 const PxVehicleDirectDriveThrottleCommandResponseParams* directDriveThrottleResponseParams,
 const PxVehicleArrayData<PxReal>& directDriveThrottleResponseState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register object instances in omnipvd that will be used to reflect the engine drivetrain of a vehicle instance.
\param[in] commandState describes the control values applied to the vehicle.
\param[in] transmissionCommandState describes the state of the engine drive transmission.
\param[in] clutchResponseParams describes the vehicle's response to a clutch command.
\param[in] clutchParams describes the vehicle's clutch.
\param[in] engineParams describes the engine.
\param[in] gearboxParams describes the gearbox.
\param[in] autoboxParams describes the automatic gearbox.
\param[in] multiWheelDiffParams describes a multiwheel differential without limited slip compensation.
\param[in] fourWheelDiffPrams describes a differential that delivers torque to four drive wheels with limited slip compensation.
\param[in] clutchResponseState is the instantaneous reponse of the clutch to a clutch command.
\param[in] throttleResponseState is the instantaneous response to a throttle command.
\param[in] engineState is the instantaneous state of the engine.
\param[in] gearboxState is the instantaneous state of the gearbox.
\param[in] autoboxState is the instantaneous state of the automatic gearbox.
\param[in] diffState is the instantaneous state of the differential.
\param[in] clutchSlipState is the instantaneous slip recorded at the clutch.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any pointer is NULL, the corresponding data will not be reflected in omnipvd.
\note objectHandles must be non-NULL
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdEngineDrivetrainWrite
*/
void PxVehiclePvdEngineDrivetrainRegister
(const PxVehicleCommandState* commandState, const PxVehicleEngineDriveTransmissionCommandState* transmissionCommandState,
 const PxVehicleClutchCommandResponseParams* clutchResponseParams,
 const PxVehicleClutchParams* clutchParams,
 const PxVehicleEngineParams* engineParams,
 const PxVehicleGearboxParams* gearboxParams,
 const PxVehicleAutoboxParams* autoboxParams,
 const PxVehicleMultiWheelDriveDifferentialParams* multiWheelDiffParams,
 const PxVehicleFourWheelDriveDifferentialParams* fourWheelDiffPrams,
 const PxVehicleClutchCommandResponseState* clutchResponseState,
 const PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState,
 const PxVehicleEngineState* engineState,
 const PxVehicleGearboxState* gearboxState,
 const PxVehicleAutoboxState* autoboxState,
 const PxVehicleDifferentialState* diffState,
 const PxVehicleClutchSlipState* clutchSlipState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write engine drivetrain data of a vehicle instance to omnipvd.
\param[in] commandState describes the control values applied to the vehicle.
\param[in] transmissionCommandState describes the state of the engine drive transmission.
\param[in] clutchResponseParams describes the vehicle's response to a clutch command.
\param[in] clutchParams describes the vehicle's clutch.
\param[in] engineParams describes the engine.
\param[in] gearboxParams describes the gearbox.
\param[in] autoboxParams describes the automatic gearbox.
\param[in] multiWheelDiffParams describes a multiwheel differential without limited slip compensation.
\param[in] fourWheelDiffPrams describes a differential that delivers torque to four drive wheels with limited slip compensation.
\param[in] clutchResponseState is the instantaneous reponse of the clutch to a clutch command.
\param[in] throttleResponseState is the instantaneous response to a throttle command.
\param[in] engineState is the instantaneous state of the engine.
\param[in] gearboxState is the instantaneous state of the gearbox.
\param[in] autoboxState is the instantaneous state of the automatic gearbox.
\param[in] diffState is the instantaneous state of the differential.
\param[in] clutchSlipState is the instantaneous slip recorded at the clutch.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any pointer is NULL and the corresponding argument in PxVehiclePvdEngineDrivetrainRegister() was not NULL, the corresponding data will not be reflected in omnipvd.
\note omniWriter must be non-NULL
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdEngineDrivetrainRegister
*/
void PxVehiclePvdEngineDrivetrainWrite
(const PxVehicleCommandState* commandState, const PxVehicleEngineDriveTransmissionCommandState* transmissionCommandState,
 const PxVehicleClutchCommandResponseParams* clutchResponseParams,
 const PxVehicleClutchParams* clutchParams,
 const PxVehicleEngineParams* engineParams,
 const PxVehicleGearboxParams* gearboxParams,
 const PxVehicleAutoboxParams* autoboxParams,
 const PxVehicleMultiWheelDriveDifferentialParams* multiWheelDiffParams,
 const PxVehicleFourWheelDriveDifferentialParams* fourWheelDiffPrams,
 const PxVehicleClutchCommandResponseState* clutchResponseState,
 const PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState,
 const PxVehicleEngineState* engineState,
 const PxVehicleGearboxState* gearboxState,
 const PxVehicleAutoboxState* autoboxState,
 const PxVehicleDifferentialState* diffState,
 const PxVehicleClutchSlipState* clutchSlipState,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register per wheel attachment data that involves the vehicle's integration with a PhysX scene.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] physxSuspLimitConstraintParams describes the method used by PhysX to enforce suspension travel limits.
\param[in] physxMaterialFrictionParams describes the friction response of each wheel to a set of PxMaterial instances.
\param[in] physxActor describes the PxRigidActor and PxShape instances that are used to represent the vehicle's rigid body and wheel shapes in PhysX.
\param[in] physxRoadGeometryQueryParams describes the physx scene query method used to place each wheel on the ground.
\param[in] physxRoadGeomState is an array of per wheel physx scene query results.
\param[in] physxConstraintStates is an array of constraints states used by PhysX to enforce sticky tire and suspension travel limit constraints.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any array is empty, the corresponding data will not be reflected in omnipvd.
\note If physxActor is NULL,  the corresponding data will not be reflected in omnipvd. 
\note If physxRoadGeometryQueryParams is NULL,  the corresponding data will not be reflected in omnipvd. 
\note Each array must either be empty or contain an entry for each wheel present in axleDesc.
\note objectHandles must be non-NULL.
\note omniWriter must be non-NULL.
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdPhysXWheelAttachmentWrite
*/
void PxVehiclePvdPhysXWheelAttachmentRegister
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleArrayData<const PxVehiclePhysXSuspensionLimitConstraintParams>& physxSuspLimitConstraintParams,
 const PxVehicleArrayData<const PxVehiclePhysXMaterialFrictionParams>& physxMaterialFrictionParams,
 const PxVehiclePhysXActor* physxActor, const PxVehiclePhysXRoadGeometryQueryParams* physxRoadGeometryQueryParams,
 const PxVehicleArrayData<const PxVehiclePhysXRoadGeometryQueryState>&  physxRoadGeomState,
 const PxVehicleArrayData<const PxVehiclePhysXConstraintState>& physxConstraintStates,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write to omnipvd the per wheel attachment data that involves the vehicle's integration with a PhysX scene.
\param[in] axleDesc is a description of the wheels and axles of a vehicle.
\param[in] physxSuspLimitConstraintParams describes the method used by PhysX to enforce suspension travel limits.
\param[in] physxMaterialFrictionParams describes the friction response of each wheel to a set of PxMaterial instances.
\param[in] physxActor describes the PxShape instances that are used to represent the vehicle's wheel shapes in PhysX.
\param[in] physxRoadGeometryQueryParams describes the physx scene query method used to place each wheel on the ground.
\param[in] physxRoadGeomState is an array of per wheel physx scene query results.
\param[in] physxConstraintStates is an array of constraints states used by PhysX to enforce sticky tire and suspension travel limit constraints.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If any array is empty but the corresponding array in PxVehiclePvdPhysXWheelAttachmentRegister() was not empty, the corresponding data will not be updated in omnipvd.
\note If physxActor is NULL but the corresponding argument in PxVehiclePvdPhysXWheelAttachmentRegister was not NULL,  the corresponding data will not be reflected in omnipvd.
\note If physxRoadGeometryQueryParams is NULL but the corresponding argument in PxVehiclePvdPhysXWheelAttachmentRegister was not NULL,  the corresponding data will not be reflected in omnipvd.
\note Each array must either be empty or contain an entry for each wheel present in axleDesc.
\note omniWriter must be non-NULL.
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdPhysXWheelAttachmentWrite
*/
void PxVehiclePvdPhysXWheelAttachmentWrite
(const PxVehicleAxleDescription& axleDesc,
 const PxVehicleArrayData<const PxVehiclePhysXSuspensionLimitConstraintParams>& physxSuspLimitConstraintParams,
 const PxVehicleArrayData<const PxVehiclePhysXMaterialFrictionParams>& physxMaterialFrictionParams,
 const PxVehiclePhysXActor* physxActor, const PxVehiclePhysXRoadGeometryQueryParams* physxRoadGeometryQueryParams,
 const PxVehicleArrayData<const PxVehiclePhysXRoadGeometryQueryState>& physxRoadGeomState,
 const PxVehicleArrayData<const PxVehiclePhysXConstraintState>& physxConstraintStates, 
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Register the PxRigidActor instance that represents the vehicle's rigid body in a PhysX scene.
\param[in] physxActor describes the PxRigidActor instance that is used to represent the vehicle's rigid body in PhysX.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If physxActor is NULL, the corresponding data will not be reflected in omnipvd.
\note objectHandles must be non-NULL.
\note omniWriter must be non-NULL.
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdPhysXRigidActorWrite
*/
void PxVehiclePvdPhysXRigidActorRegister
(const PxVehiclePhysXActor* physxActor,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 PxVehiclePvdObjectHandles* objectHandles, OmniPvdWriter* omniWriter);

/**
\brief Write the PxRigidActor instance to omnipvd.
\param[in] physxActor describes the PxRigidActor instance that is used to represent the vehicle's rigid body in PhysX.
\param[in] attributeHandles contains a general description of vehicle parameters and states that will be reflected in omnipvd.
\param[in] objectHandles contains unique handles for the parameters and states of each vehicle instance.
\param[in] omniWriter is an OmniPvdWriter instance used to communicate state and parameter data to omnipvd.
\note If physxActor is NULL and the corresponding argument in PxVehiclePvdPhysXRigidActorRegister was not NULL, the corresponding data will not be updated in omnipvd.
\note objectHandles must be non-NULL.
\note omniWriter must be non-NULL.
@see PxVehiclePvdAttributesCreate
@see PxVehiclePvdObjectCreate
@see PxVehiclePvdPhysXRigidActorRegister
*/
void PxVehiclePvdPhysXRigidActorWrite
(const PxVehiclePhysXActor* physxActor,
 const PxVehiclePvdAttributeHandles& attributeHandles,
 const PxVehiclePvdObjectHandles& objectHandles, OmniPvdWriter* omniWriter);

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
