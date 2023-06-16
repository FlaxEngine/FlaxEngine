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

#include "vehicle2/commands/PxVehicleCommandHelpers.h"
#include "vehicle2/roadGeometry/PxVehicleRoadGeometryState.h"
#include "vehicle2/suspension/PxVehicleSuspensionParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionStates.h"
#include "vehicle2/wheel/PxVehicleWheelStates.h"
#include "vehicle2/wheel/PxVehicleWheelParams.h"

#include "PxVehicleTireFunctions.h"
#include "PxVehicleTireParams.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

class PxVehicleTireComponent : public PxVehicleComponent
{
public:

	PxVehicleTireComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleTireComponent() {}

	virtual void getDataForTireComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehicleTireForceParams>& tireForceParams,
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& roadGeomStates,
		PxVehicleArrayData<const PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<const PxVehicleSuspensionForce>& suspensionForces,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1DStates,
		PxVehicleArrayData<PxVehicleTireGripState>& tireGripStates,
		PxVehicleArrayData<PxVehicleTireDirectionState>& tireDirectionStates,
		PxVehicleArrayData<PxVehicleTireSpeedState>& tireSpeedStates,
		PxVehicleArrayData<PxVehicleTireSlipState>& tireSlipStates,
		PxVehicleArrayData<PxVehicleTireCamberAngleState>& tireCamberAngleStates, 
		PxVehicleArrayData<PxVehicleTireStickyState>& tireStickyStates,
		PxVehicleArrayData<PxVehicleTireForce>& tireForces) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_PROFILE_ZONE("PxVehicleTireComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelActuationState> actuationStates;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehicleTireForceParams> tireForceParams;
		PxVehicleArrayData<const PxVehicleRoadGeometryState> roadGeomStates;
		PxVehicleArrayData<const PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<const PxVehicleSuspensionForce> suspensionForces;
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState> wheelRigidBody1DStates;
		PxVehicleArrayData<PxVehicleTireGripState> tireGripStates;
		PxVehicleArrayData<PxVehicleTireDirectionState> tireDirectionStates;
		PxVehicleArrayData<PxVehicleTireSpeedState> tireSpeedStates;
		PxVehicleArrayData<PxVehicleTireSlipState> tireSlipStates;
		PxVehicleArrayData<PxVehicleTireCamberAngleState> tireCamberAngleStates;
		PxVehicleArrayData<PxVehicleTireStickyState> tireStickyStates;
		PxVehicleArrayData<PxVehicleTireForce> tireForces;

		getDataForTireComponent(axleDescription, steerResponseStates, 
			rigidBodyState, actuationStates, wheelParams, suspensionParams, tireForceParams,
			roadGeomStates, suspensionStates, suspensionComplianceStates, suspensionForces,
			wheelRigidBody1DStates, tireGripStates, tireDirectionStates, tireSpeedStates,
			tireSlipStates, tireCamberAngleStates, tireStickyStates, tireForces);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			//Compute the tire slip directions
			PxVehicleTireDirsUpdate(
				suspensionParams[wheelId],
				steerResponseStates[wheelId],
				roadGeomStates[wheelId], suspensionComplianceStates[wheelId],
				*rigidBodyState,
				context.frame,
				tireDirectionStates[wheelId]);

			//Compute the rigid body speeds along the tire slip directions.
			PxVehicleTireSlipSpeedsUpdate(
				wheelParams[wheelId], suspensionParams[wheelId],
				steerResponseStates[wheelId], suspensionStates[wheelId], tireDirectionStates[wheelId],
				*rigidBodyState, roadGeomStates[i],
				context.frame,
				tireSpeedStates[wheelId]);

			//Compute the tire slip angles.
			PxVehicleTireSlipsUpdate(
				wheelParams[wheelId], context.tireSlipParams,
				actuationStates[wheelId], tireSpeedStates[wheelId],
				wheelRigidBody1DStates[wheelId],
				tireSlipStates[wheelId]);

			//Update the camber angle
			PxVehicleTireCamberAnglesUpdate(
				suspensionParams[wheelId], steerResponseStates[wheelId], roadGeomStates[wheelId],
				suspensionComplianceStates[wheelId], *rigidBodyState,
				context.frame,
				tireCamberAngleStates[wheelId]);

			//Compute the friction
			PxVehicleTireGripUpdate(
				tireForceParams[wheelId], roadGeomStates[wheelId], 
				suspensionStates[wheelId], suspensionForces[wheelId],
				tireSlipStates[wheelId], tireGripStates[wheelId]);

			//Update the tire sticky state
			//
			//Note: this should be skipped if tires do not use the sticky feature
			PxVehicleTireStickyStateUpdate(
				*axleDescription,
				wheelParams[wheelId],
				context.tireStickyParams,
				actuationStates, tireGripStates[wheelId],
				tireSpeedStates[wheelId], wheelRigidBody1DStates[wheelId],
				dt,
				tireStickyStates[wheelId]);

			//If sticky tire is active set the slip angle to zero.
			//
			//Note: this should be skipped if tires do not use the sticky feature
			PxVehicleTireSlipsAccountingForStickyStatesUpdate(
				tireStickyStates[wheelId],
				tireSlipStates[wheelId]);

			//Compute the tire forces
			PxVehicleTireForcesUpdate(
				wheelParams[wheelId], suspensionParams[wheelId],
				tireForceParams[wheelId],
				suspensionComplianceStates[wheelId],
				tireGripStates[wheelId], tireDirectionStates[wheelId],
				tireSlipStates[wheelId], tireCamberAngleStates[wheelId],
				*rigidBodyState,
				tireForces[wheelId]);
		}

		return true;
	}
};

/**
 * @deprecated
 */
class PX_DEPRECATED PxVehicleLegacyTireComponent : public PxVehicleComponent
{
public:

	PxVehicleLegacyTireComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleLegacyTireComponent() {}

	virtual void getDataForLegacyTireComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehicleTireForceParams>& tireForceParams,
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& roadGeomStates,
		PxVehicleArrayData<const PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<const PxVehicleSuspensionForce>& suspensionForces,
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState>& wheelRigidBody1DStates,
		PxVehicleArrayData<PxVehicleTireGripState>& tireGripStates,
		PxVehicleArrayData<PxVehicleTireDirectionState>& tireDirectionStates,
		PxVehicleArrayData<PxVehicleTireSpeedState>& tireSpeedStates,
		PxVehicleArrayData<PxVehicleTireSlipState>& tireSlipStates,
		PxVehicleArrayData<PxVehicleTireCamberAngleState>& tireCamberAngleStates, 
		PxVehicleArrayData<PxVehicleTireStickyState>& tireStickyStates,
		PxVehicleArrayData<PxVehicleTireForce>& tireForces) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_PROFILE_ZONE("PxVehicleLegacyTireComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelActuationState> actuationStates;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehicleTireForceParams> tireForceParams;
		PxVehicleArrayData<const PxVehicleRoadGeometryState> roadGeomStates;
		PxVehicleArrayData<const PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<const PxVehicleSuspensionForce> suspensionForces;
		PxVehicleArrayData<const PxVehicleWheelRigidBody1dState> wheelRigidBody1DStates;
		PxVehicleArrayData<PxVehicleTireGripState> tireGripStates;
		PxVehicleArrayData<PxVehicleTireDirectionState> tireDirectionStates;
		PxVehicleArrayData<PxVehicleTireSpeedState> tireSpeedStates;
		PxVehicleArrayData<PxVehicleTireSlipState> tireSlipStates;
		PxVehicleArrayData<PxVehicleTireCamberAngleState> tireCamberAngleStates;
		PxVehicleArrayData<PxVehicleTireStickyState> tireStickyStates;
		PxVehicleArrayData<PxVehicleTireForce> tireForces;

		getDataForLegacyTireComponent(axleDescription, steerResponseStates, 
			rigidBodyState, actuationStates,
			wheelParams, suspensionParams, tireForceParams,
			roadGeomStates, suspensionStates, suspensionComplianceStates,
			suspensionForces, wheelRigidBody1DStates,
			tireGripStates, tireDirectionStates, tireSpeedStates, tireSlipStates,
			tireCamberAngleStates, tireStickyStates, tireForces);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			//Compute the tire slip directions
			PxVehicleTireDirsLegacyUpdate(
				suspensionParams[wheelId],
				steerResponseStates[wheelId],
				roadGeomStates[wheelId], *rigidBodyState,
				context.frame,
				tireDirectionStates[wheelId]);

			//Compute the rigid body speeds along the tire slip directions.
			PxVehicleTireSlipSpeedsUpdate(
				wheelParams[wheelId], suspensionParams[wheelId],
				steerResponseStates[wheelId], suspensionStates[wheelId], tireDirectionStates[wheelId],
				*rigidBodyState, roadGeomStates[wheelId],
				context.frame,
				tireSpeedStates[wheelId]);

			//Compute the tire slip angles.
			PxVehicleTireSlipsLegacyUpdate(
				wheelParams[wheelId], context.tireSlipParams,
				actuationStates[wheelId], tireSpeedStates[wheelId],
				wheelRigidBody1DStates[wheelId],
				tireSlipStates[wheelId]);

			//Update the camber angle
			PxVehicleTireCamberAnglesUpdate(
				suspensionParams[wheelId], steerResponseStates[wheelId], roadGeomStates[wheelId],
				suspensionComplianceStates[wheelId], *rigidBodyState,
				context.frame,
				tireCamberAngleStates[wheelId]);

			//Compute the friction
			PxVehicleTireGripUpdate(
				tireForceParams[wheelId], roadGeomStates[wheelId], 
				suspensionStates[wheelId], suspensionForces[wheelId],
				tireSlipStates[wheelId], tireGripStates[wheelId]);

			//Update the tire sticky state
			//
			//Note: this should be skipped if tires do not use the sticky feature
			PxVehicleTireStickyStateUpdate(
				*axleDescription,
				wheelParams[wheelId],
				context.tireStickyParams,
				actuationStates, tireGripStates[wheelId],
				tireSpeedStates[wheelId], wheelRigidBody1DStates[wheelId],
				dt,
				tireStickyStates[wheelId]);

			//If sticky tire is active set the slip angle to zero.
			//
			//Note: this should be skipped if tires do not use the sticky feature
			PxVehicleTireSlipsAccountingForStickyStatesUpdate(
				tireStickyStates[wheelId],
				tireSlipStates[wheelId]);

			//Compute the tire forces
			PxVehicleTireForcesUpdate(
				wheelParams[wheelId], suspensionParams[wheelId],
				tireForceParams[wheelId],
				suspensionComplianceStates[wheelId],
				tireGripStates[wheelId], tireDirectionStates[wheelId],
				tireSlipStates[wheelId], tireCamberAngleStates[wheelId],
				*rigidBodyState,
				tireForces[wheelId]);
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */


