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

#include "vehicle2/PxVehicleComponent.h"
#include "vehicle2/PxVehicleParams.h"
#include "PxVehiclePvdFunctions.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

#define VEHICLE_FLOAT_ARRAY_DATA(b)  PxVehicleArrayData<PxReal> b; b.setEmpty();
#define VEHICLE_ARRAY_DATA(a, b)  PxVehicleArrayData<const a> b; b.setEmpty();
#define VEHICLE_SIZED_ARRAY_DATA(a, b)  PxVehicleSizedArrayData<const a> b; b.setEmpty();

class PxVehiclePVDComponent : public PxVehicleComponent
{
public:
	
	PxVehiclePVDComponent() : PxVehicleComponent() , firstTime(true) {}
	virtual ~PxVehiclePVDComponent() {}

	virtual void getDataForPVDComponent(
	    const PxVehicleAxleDescription*& axleDescription, 
		const PxVehicleRigidBodyParams*& rbodyParams, const PxVehicleRigidBodyState*& rbodyState,
		const PxVehicleSuspensionStateCalculationParams*& suspStateCalcParams,
		PxVehicleSizedArrayData<const PxVehicleBrakeCommandResponseParams>& brakeResponseParams,
		const PxVehicleSteerCommandResponseParams*& steerResponseParams, 
		PxVehicleArrayData<PxReal>& brakeResponseStates,
		PxVehicleArrayData<PxReal>& steerResponseStates, 
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& wheelActuationStates,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
		PxVehicleArrayData<const PxVehicleWheelLocalPose>& wheelLocalPoses,
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& roadGeomStates,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspParams,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspCompParams,
		PxVehicleArrayData<const PxVehicleSuspensionForceParams>& suspForceParams,
		PxVehicleArrayData<const PxVehicleSuspensionState>& suspStates,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspCompStates,
		PxVehicleArrayData<const PxVehicleSuspensionForce>& suspForces,
		PxVehicleArrayData<const PxVehicleTireForceParams>& tireForceParams,
		PxVehicleArrayData<const PxVehicleTireDirectionState>& tireDirectionStates,
		PxVehicleArrayData<const PxVehicleTireSpeedState>& tireSpeedStates,
		PxVehicleArrayData<const PxVehicleTireSlipState>& tireSlipStates,
		PxVehicleArrayData<const PxVehicleTireStickyState>& tireStickyStates,
		PxVehicleArrayData<const PxVehicleTireGripState>& tireGripStates,
		PxVehicleArrayData<const PxVehicleTireCamberAngleState>& tireCamberStates,
		PxVehicleArrayData<const PxVehicleTireForce>& tireForces, 
		PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams,
		const PxVehicleAntiRollTorque*& antiRollTorque,
		const PxVehicleCommandState*& commandState, 
		const PxVehicleDirectDriveThrottleCommandResponseParams*& directDriveThrottleResponseParams,
		const PxVehicleDirectDriveTransmissionCommandState*& directDriveTransmissionState,
		PxVehicleArrayData<PxReal>& directDrivethrottleResponseState,
		const PxVehicleClutchCommandResponseParams*& clutchResponseParams,
		const PxVehicleClutchParams*& clutchParams,
		const PxVehicleEngineParams*& engineParams,
		const PxVehicleGearboxParams*& gearboxParams,
		const PxVehicleAutoboxParams*& autoboxParams,
		const PxVehicleMultiWheelDriveDifferentialParams*& multiWheelDiffParams,
		const PxVehicleFourWheelDriveDifferentialParams*& fourWheelDiffParams,
		const PxVehicleEngineDriveTransmissionCommandState*& engineDriveTransmissionState,
		const PxVehicleClutchCommandResponseState*& clutchResponseState,
		const PxVehicleEngineDriveThrottleCommandResponseState*& engineDriveThrottleResponseState,
		const PxVehicleEngineState*& engineState,
		const PxVehicleGearboxState*& gearboxState,
		const PxVehicleAutoboxState*& autoboxState,
		const PxVehicleDifferentialState*& diffState,
		const PxVehicleClutchSlipState*& clutchSlipState,
		PxVehicleArrayData<const PxVehiclePhysXSuspensionLimitConstraintParams>& physxConstraintParams,
		PxVehicleArrayData<const PxVehiclePhysXMaterialFrictionParams>& physxMaterialFrictionParams,
		const PxVehiclePhysXActor*& physxActor,
		const PxVehiclePhysXRoadGeometryQueryParams*& physxRoadGeomQryParams,
		PxVehicleArrayData<const PxVehiclePhysXRoadGeometryQueryState>& physxRoadGeomStates,
		PxVehicleArrayData<const PxVehiclePhysXConstraintState>& physxConstraintStates,
		PxVehiclePvdObjectHandles*& objectHandles) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{ 
		PX_UNUSED(dt);
		PX_UNUSED(context);

		if(!context.pvdContext.attributeHandles || !context.pvdContext.writer)
			return true;

		const PxVehicleAxleDescription* axleDesc = NULL;
		const PxVehicleRigidBodyParams* rbodyParams = NULL;
		const PxVehicleRigidBodyState* rbodyState = NULL;
		const PxVehicleSuspensionStateCalculationParams* suspStateCalcParams = NULL;
		VEHICLE_SIZED_ARRAY_DATA(PxVehicleBrakeCommandResponseParams, brakeResponseParams);
		const PxVehicleSteerCommandResponseParams* steerResponseParams = NULL;
		VEHICLE_FLOAT_ARRAY_DATA(brakeResponseStates);
		VEHICLE_FLOAT_ARRAY_DATA(steerResponseStates);
		VEHICLE_ARRAY_DATA(PxVehicleWheelParams, wheelParams);
		VEHICLE_ARRAY_DATA(PxVehicleWheelActuationState, wheelActuationStates);
		VEHICLE_ARRAY_DATA(PxVehicleWheelRigidBody1dState, wheelRigidBody1dStates);
		VEHICLE_ARRAY_DATA(PxVehicleWheelLocalPose, wheelLocalPoses);
		VEHICLE_ARRAY_DATA(PxVehicleRoadGeometryState, roadGeomStates);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionParams, suspParams);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionComplianceParams, suspComplianceParams);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionForceParams, suspForceParams);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionState, suspStates);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionComplianceState, suspComplianceStates);
		VEHICLE_ARRAY_DATA(PxVehicleSuspensionForce, suspForces);
		VEHICLE_ARRAY_DATA(PxVehicleTireForceParams, tireForceParams);
		VEHICLE_ARRAY_DATA(PxVehicleTireDirectionState, tireDirectionStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireSpeedState, tireSpeedStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireSlipState, tireSlipStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireStickyState, tireStickyStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireGripState, tireGripStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireCamberAngleState, tireCamberStates);
		VEHICLE_ARRAY_DATA(PxVehicleTireForce, tireForces);
		VEHICLE_SIZED_ARRAY_DATA(PxVehicleAntiRollForceParams, antiRollParams);
		const PxVehicleAntiRollTorque* antiRollTorque = NULL;
		const PxVehicleCommandState* commandState = NULL;
		const PxVehicleDirectDriveThrottleCommandResponseParams* directDriveThrottleResponseParams = NULL;
		const PxVehicleDirectDriveTransmissionCommandState* directDriveTransmissionState = NULL;
		VEHICLE_FLOAT_ARRAY_DATA(directDrivethrottleResponseState);
		const PxVehicleClutchCommandResponseParams* clutchResponseParams = NULL;
		const PxVehicleClutchParams* clutchParams = NULL;
		const PxVehicleEngineParams* engineParams = NULL;
		const PxVehicleGearboxParams* gearboxParams = NULL;
		const PxVehicleAutoboxParams* autoboxParams = NULL;
		const PxVehicleMultiWheelDriveDifferentialParams* multiWheelDiffParams = NULL;
		const PxVehicleFourWheelDriveDifferentialParams* fourWheelDiffParams = NULL;
		const PxVehicleEngineDriveTransmissionCommandState* engineDriveTransmissionCommandState = NULL;
		const PxVehicleClutchCommandResponseState* clutchResponseState = NULL;
		const PxVehicleEngineDriveThrottleCommandResponseState* throttleResponseState = NULL;
		const PxVehicleEngineState* engineState = NULL;
		const PxVehicleGearboxState* gearboxState = NULL;
		const PxVehicleAutoboxState* autoboxState = NULL;
		const PxVehicleDifferentialState* diffState = NULL;
		const PxVehicleClutchSlipState* clutchSlipState = NULL;
		VEHICLE_ARRAY_DATA(PxVehiclePhysXSuspensionLimitConstraintParams, physxConstraintParams);
		VEHICLE_ARRAY_DATA(PxVehiclePhysXMaterialFrictionParams, physxMaterialFrictionParams);
		const PxVehiclePhysXActor* physxActor = NULL;
		const PxVehiclePhysXRoadGeometryQueryParams* physxRoadGeomQryParams = NULL;
		VEHICLE_ARRAY_DATA(PxVehiclePhysXRoadGeometryQueryState, physxRoadGeomStates);
		VEHICLE_ARRAY_DATA(PxVehiclePhysXConstraintState, physxConstraintStates);
		PxVehiclePvdObjectHandles* omniPvdObjectHandles = NULL;

		getDataForPVDComponent(
			axleDesc, 
			rbodyParams, rbodyState,
			suspStateCalcParams,
			brakeResponseParams, steerResponseParams,
			brakeResponseStates, steerResponseStates,
			wheelParams, 
			wheelActuationStates, wheelRigidBody1dStates, wheelLocalPoses,
			roadGeomStates,
			suspParams,	suspComplianceParams, suspForceParams,
			suspStates, suspComplianceStates,
			suspForces,
			tireForceParams,
			tireDirectionStates, tireSpeedStates, tireSlipStates, tireStickyStates, tireGripStates, tireCamberStates,
			tireForces,
			antiRollParams, antiRollTorque,
			commandState,
			directDriveThrottleResponseParams, directDriveTransmissionState, directDrivethrottleResponseState,
			clutchResponseParams, clutchParams, engineParams, gearboxParams, autoboxParams, multiWheelDiffParams, fourWheelDiffParams,
			engineDriveTransmissionCommandState,
			clutchResponseState, throttleResponseState, engineState, gearboxState, autoboxState, diffState, clutchSlipState,
			physxConstraintParams, physxMaterialFrictionParams,
			physxActor, physxRoadGeomQryParams, physxRoadGeomStates, physxConstraintStates,
			omniPvdObjectHandles);

		if(!omniPvdObjectHandles)
			return true;

		if(firstTime)
		{
			PxVehiclePvdRigidBodyRegister(
				rbodyParams, rbodyState,  
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdSuspensionStateCalculationParamsRegister(
				suspStateCalcParams,
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);
	
			PxVehiclePvdCommandResponseRegister(
				brakeResponseParams, steerResponseParams, 
				brakeResponseStates, steerResponseStates,
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);
			
			PxVehiclePvdWheelAttachmentsRegister(
				*axleDesc,
				wheelParams, wheelActuationStates, wheelRigidBody1dStates, wheelLocalPoses, 
				roadGeomStates, 
				suspParams, suspComplianceParams, suspForceParams, suspStates, suspComplianceStates, 
				suspForces,
				tireForceParams,
				tireDirectionStates, tireSpeedStates, tireSlipStates, tireStickyStates, tireGripStates, tireCamberStates,
				tireForces,
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdAntiRollsRegister(
				antiRollParams, antiRollTorque, 
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdDirectDrivetrainRegister(
				commandState, directDriveTransmissionState, 
				directDriveThrottleResponseParams, 
				directDrivethrottleResponseState, 
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdEngineDrivetrainRegister(
				commandState, engineDriveTransmissionCommandState, 
				clutchResponseParams, clutchParams, engineParams, gearboxParams, autoboxParams, multiWheelDiffParams, fourWheelDiffParams,
				clutchResponseState, throttleResponseState, engineState, gearboxState, autoboxState, diffState, clutchSlipState,
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdPhysXWheelAttachmentRegister(
				*axleDesc, 
				physxConstraintParams, physxMaterialFrictionParams,
				physxActor, physxRoadGeomQryParams,  physxRoadGeomStates, physxConstraintStates,
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);

			PxVehiclePvdPhysXRigidActorRegister(
				physxActor, 
				*context.pvdContext.attributeHandles, omniPvdObjectHandles, context.pvdContext.writer);
						
			firstTime = false;
		}

		PxVehiclePvdRigidBodyWrite(
			rbodyParams, rbodyState,
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		PxVehiclePvdSuspensionStateCalculationParamsWrite(
			suspStateCalcParams,
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		PxVehiclePvdCommandResponseWrite(
			*axleDesc, brakeResponseParams, steerResponseParams, brakeResponseStates, steerResponseStates,
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);
		
		PxVehiclePvdWheelAttachmentsWrite(
			*axleDesc,
			wheelParams, wheelActuationStates, wheelRigidBody1dStates, wheelLocalPoses, 
			roadGeomStates, 
			suspParams, suspComplianceParams, suspForceParams, suspStates, suspComplianceStates, 
			suspForces,
			tireForceParams,
			tireDirectionStates, tireSpeedStates, tireSlipStates, tireStickyStates, tireGripStates, tireCamberStates,
			tireForces,
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		PxVehiclePvdAntiRollsWrite(
			antiRollParams, antiRollTorque, 
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);
		
		PxVehiclePvdDirectDrivetrainWrite(
			*axleDesc, 
			commandState, directDriveTransmissionState, 
			directDriveThrottleResponseParams, 
			directDrivethrottleResponseState, 
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		PxVehiclePvdEngineDrivetrainWrite(
			commandState, engineDriveTransmissionCommandState, 
			clutchResponseParams, clutchParams, engineParams, gearboxParams, autoboxParams, multiWheelDiffParams, fourWheelDiffParams,
			clutchResponseState, throttleResponseState, engineState, gearboxState, autoboxState, diffState, clutchSlipState,
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		PxVehiclePvdPhysXWheelAttachmentWrite(
			*axleDesc, 
			physxConstraintParams, physxMaterialFrictionParams,
			physxActor, physxRoadGeomQryParams, physxRoadGeomStates, physxConstraintStates, 
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);
		
		PxVehiclePvdPhysXRigidActorWrite(
			physxActor, 
			*context.pvdContext.attributeHandles, *omniPvdObjectHandles, context.pvdContext.writer);

		return true;
	}
	
private:

	bool firstTime;
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
