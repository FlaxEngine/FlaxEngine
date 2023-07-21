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

#include "vehicle2/commands/PxVehicleCommandStates.h"
#include "vehicle2/commands/PxVehicleCommandHelpers.h"
#include "vehicle2/rigidBody/PxVehicleRigidBodyParams.h"
#include "vehicle2/roadGeometry/PxVehicleRoadGeometryState.h"
#include "vehicle2/wheel/PxVehicleWheelParams.h"

#include "PxVehicleSuspensionParams.h"
#include "PxVehicleSuspensionStates.h"
#include "PxVehicleSuspensionFunctions.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif


class PxVehicleSuspensionComponent : public PxVehicleComponent
{
public:

	PxVehicleSuspensionComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleSuspensionComponent() {}

	/**
	\brief Retrieve pointers to the parameter and state data required to compute the suspension state and the forces/torques that arise from the suspension state.
	\param[out] axleDescription must be returned as a non-null pointer to a single PxVehicleAxleDescription instance that describes the wheels and axles
	of the vehicle.
	\param[out] rigidBodyParams must be returned as a a non-null pointer to a single PxVehicleRigidBodyParams instance that describes the mass and moment
	of inertia of the vehicle's rigid body.
	\param[out] suspensionStateCalculationParams must be returned as a non-null pointer to a single PxVehicleSuspensionStateCalculationParams instance
	that describes the jounce computation type etc.
	\param[out] steerResponseStates must be returned as a non-null pointer to a single PxVehicleSteerCommandResponseStates instance that describes the steer 
	state of the wheels.
	\param[out] rigidBodyState must be returned as a non-null pointer to a single PxVehicleRigidBodyState instance that describes the pose and momentum of 
	the vehicle's rigid body.
	\param[out] wheelParams must be set to a non-null pointer to an array of PxVehicleWheelParams containing per-wheel parameters for each wheel 
	referenced by axleDescription.
	\param[out] suspensionParams must be set to a non-null pointer to an array of PxVehicleSuspensionParams containing per-wheel parameters for each 
	wheel referenced by axleDescription.
	\param[out] suspensionComplianceParams must be set to a non-null pointer to an array of PxVehicleSuspensionComplianceParams containing per-wheel 
	parameters for each wheel referenced by axleDescription.
	\param[out] suspensionForceParams must be set to a non-null pointer to an array of PxVehicleSuspensionForceParams containing per-wheel parameters 
	for each wheel referenced by axleDescription.
	\param[out] antiRollForceParams is optionally returned as a non-null pointer to an array of PxVehicleAntiRollForceParams with each element in the array 
	describing a unique anti-roll bar connecting a pair of wheels.  
	\param[out] wheelRoadGeomStates must be set to non-null pointer to an array of PxVehicleRoadGeometryState containing per-wheel road geometry for 
	each wheel referenced by axleDescription.
	\param[out] suspensionStates must be set to a non-null pointer to an array of PxVehicleSuspensionState containing per-wheel suspension state for each 
	wheel referenced by axleDescription.
	\param[out] suspensionComplianceStates must be set to a non-null pointer to an array of PxVehicleSuspensionComplianceState containing per-wheel suspension 
	compliance state for each wheel referenced by axleDescription.
	\param[out] suspensionForces must be set to a non-null pointer to an array of PxVehicleSuspensionForce containing per-wheel suspension forces for each 
	wheel referenced by axleDescription.
	\param[out] antiRollTorque is optionally returned as a non-null pointer to a single PxVehicleAntiRollTorque instance that will store the accumulated anti-roll 
	torque to be applied to the vheicle's rigid body.
	\note antiRollForceParams and antiRollTorque should be returned either both non-NULL or both NULL. 
	*/
	virtual void getDataForSuspensionComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleRigidBodyParams*& rigidBodyParams,
		const PxVehicleSuspensionStateCalculationParams*& suspensionStateCalculationParams,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspensionComplianceParams,
		PxVehicleArrayData<const PxVehicleSuspensionForceParams>& suspensionForceParams,
		PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams,
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& wheelRoadGeomStates,
		PxVehicleArrayData<PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<PxVehicleSuspensionForce>& suspensionForces,
		PxVehicleAntiRollTorque*& antiRollTorque) = 0;

	/**
	\brief Update the suspension state and suspension compliance state and use those updated states to compute suspension and anti-roll forces/torques
	to apply to the vehicle's rigid body.
	\param[in] dt is the simulation time that has passed since the last call to PxVehicleSuspensionComponent::update()
	\param[in] context describes a variety of global simulation constants such as frame and scale of the simulation and the gravitational  acceleration
	of the simulated environment.
	\note The suspension and anti-roll forces/torques are computed in the world frame.
	*/
	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_PROFILE_ZONE("PxVehicleSuspensionComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleRigidBodyParams* rigidBodyParams;
		const PxVehicleSuspensionStateCalculationParams* suspensionStateCalculationParams;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceParams> suspensionComplianceParams;
		PxVehicleArrayData<const PxVehicleSuspensionForceParams> suspensionForceParams;
		PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams> antiRollForceParams;
		PxVehicleArrayData<const PxVehicleRoadGeometryState> wheelRoadGeomStates;
		PxVehicleArrayData<PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<PxVehicleSuspensionForce> suspensionForces;
		PxVehicleAntiRollTorque* antiRollTorque;

		getDataForSuspensionComponent(axleDescription, rigidBodyParams, suspensionStateCalculationParams,
			steerResponseStates, rigidBodyState, 
			wheelParams, suspensionParams,
			suspensionComplianceParams, suspensionForceParams, antiRollForceParams, 
			wheelRoadGeomStates, suspensionStates, suspensionComplianceStates, 
			suspensionForces, antiRollTorque);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			//Update the suspension state (jounce, jounce speed)
			PxVehicleSuspensionStateUpdate(
				wheelParams[wheelId], suspensionParams[wheelId], *suspensionStateCalculationParams,
				suspensionForceParams[wheelId].stiffness, suspensionForceParams[wheelId].damping,
				steerResponseStates[wheelId], wheelRoadGeomStates[wheelId], 
				*rigidBodyState,
				dt, context.frame, context.gravity,
				suspensionStates[wheelId]);

			//Update the compliance from the suspension state.
			PxVehicleSuspensionComplianceUpdate(
				suspensionParams[wheelId], suspensionComplianceParams[wheelId],
				suspensionStates[wheelId],
				suspensionComplianceStates[wheelId]);

			//Compute the suspension force from the suspension and compliance states.
			PxVehicleSuspensionForceUpdate(
				suspensionParams[wheelId], suspensionForceParams[wheelId],
				wheelRoadGeomStates[wheelId], suspensionStates[wheelId], 
				suspensionComplianceStates[wheelId], *rigidBodyState,
				context.gravity, rigidBodyParams->mass,
				suspensionForces[wheelId]);
		}

		if (antiRollForceParams.size>0 && antiRollTorque)
		{
			PxVehicleAntiRollForceUpdate(
				suspensionParams, antiRollForceParams, 
				suspensionStates.getConst(), suspensionComplianceStates.getConst(), *rigidBodyState, 
				*antiRollTorque);
		}

		return true;
	}
};

/**
 * @deprecated
 */
class PX_DEPRECATED PxVehicleLegacySuspensionComponent : public PxVehicleComponent
{
public:

	PxVehicleLegacySuspensionComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleLegacySuspensionComponent() {}

	/**
	\brief Retrieve pointers to the parameter and state data required to compute the suspension state and the forces/torques that arise from the suspension state.
	\param[out] axleDescription must be returned as a non-null pointer to a single PxVehicleAxleDescription instance that describes the wheels and axles
	of the vehicle.
	\param[out] suspensionStateCalculationParams must be returned as a non-null pointer to a single PxVehicleSuspensionStateCalculationParams instance
	that describes the jounce computation type etc.
	\param[out] steerResponseStates must be returned as a non-null pointer to a single PxVehicleSteerCommandResponseStates instance that describes the steer 
	state of the wheels.
	\param[out] rigidBodyState must be returned as a non-null pointer to a single PxVehicleRigidBodyState instance that describes the pose and momentum of
	the vehicle's rigid body.
	\param[out] wheelParams must be set to a non-null pointer to an array of PxVehicleWheelParams containing per-wheel parameters for each wheel 
	referenced by axleDescription.
	\param[out] suspensionParams must be set to a non-null pointer to an array of PxVehicleSuspensionParams containing per-wheel parameters for each 
	wheel referenced by axleDescription.
	\param[out] suspensionComplianceParams must be set to a non-null pointer to an array of PxVehicleSuspensionComplianceParams containing per-wheel 
	parameters for each wheel referenced by axleDescription.
	\param[out] suspensionForceParams must be set to a non-null pointer to an array of PxVehicleSuspensionForceLegacyParams containing per-wheel parameters
	for each wheel referenced by axleDescription.
	\param[out] antiRollForceParams is optionally returned as a non-null pointer to an array of PxVehicleAntiRollForceParams with each element in the array
	describing a unique anti-roll bar connecting a pair of wheels.
	\param[out] wheelRoadGeomStates must be set to non-null pointer to an array of PxVehicleRoadGeometryState containing per-wheel road geometry for 
	each wheel referenced by axleDescription.
	\param[out] suspensionStates must be set to a non-null pointer to an array of PxVehicleSuspensionState containing per-wheel suspension state for each 
	wheel referenced by axleDescription.
	\param[out] suspensionComplianceStates must be set to a non-null pointer to an array of PxVehicleSuspensionComplianceState containing per-wheel suspension 
	compliance state for each wheel referenced by axleDescription.
	\param[out] suspensionForces must be set to a non-null pointer to an array of PxVehicleSuspensionForce containing per-wheel suspension forces for each 
	wheel referenced by axleDescription.
	\param[out] antiRollTorque is optionally returned as a non-null pointer to a single PxVehicleAntiRollTorque instance that will store the accumulated anti-roll
	torque to be applied to the vheicle's rigid body.
	\note antiRollForceParams and antiRollTorque should be returned either both non-NULL or both NULL.
	*/
	virtual void getDataForLegacySuspensionComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleSuspensionStateCalculationParams*& suspensionStateCalculationParams,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspensionComplianceParams,
		PxVehicleArrayData<const PxVehicleSuspensionForceLegacyParams>& suspensionForceParams,
		PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams, 
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& wheelRoadGeomStates,
		PxVehicleArrayData<PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<PxVehicleSuspensionForce>& suspensionForces,
		PxVehicleAntiRollTorque*& antiRollTorque) = 0;

	/**
	\brief Update the suspension state and suspension compliance state and use those updated states to compute suspension and anti-roll forces/torques
	to apply to the vehicle's rigid body.
	\param[in] dt is the simulation time that has passed since the last call to PxVehicleSuspensionComponent::update()
	\param[in] context describes a variety of global simulation constants such as frame and scale of the simulation and the gravitational  acceleration
	of the simulated environment.
	\note The suspension and anti-roll forces are computed in the world frame.
	\note PxVehicleLegacySuspensionComponent::update() implements legacy suspension behaviour.
	*/
	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_PROFILE_ZONE("PxVehicleLegacySuspensionComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleSuspensionStateCalculationParams* suspensionStateCalculationParams;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceParams> suspensionComplianceParams;
		PxVehicleArrayData<const PxVehicleSuspensionForceLegacyParams> suspensionForceParams;
		PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams> antiRollForceParams;
		PxVehicleArrayData<const PxVehicleRoadGeometryState> wheelRoadGeomStates;
		PxVehicleArrayData<PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<PxVehicleSuspensionForce> suspensionForces;
		PxVehicleAntiRollTorque* antiRollTorque;

		getDataForLegacySuspensionComponent(axleDescription, suspensionStateCalculationParams,
			steerResponseStates, rigidBodyState, wheelParams, 
			suspensionParams, suspensionComplianceParams, 
			suspensionForceParams, antiRollForceParams,
			wheelRoadGeomStates, suspensionStates, suspensionComplianceStates, 
			suspensionForces, antiRollTorque);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			//Update the suspension state (jounce, jounce speed)
			PxVehicleSuspensionStateUpdate(
				wheelParams[wheelId], suspensionParams[wheelId], *suspensionStateCalculationParams,
				suspensionForceParams[wheelId].stiffness, suspensionForceParams[wheelId].damping,
				steerResponseStates[wheelId], wheelRoadGeomStates[wheelId], *rigidBodyState,
				dt, context.frame, context.gravity,
				suspensionStates[wheelId]);

			//Update the compliance from the suspension state.
			PxVehicleSuspensionComplianceUpdate(
				suspensionParams[wheelId], suspensionComplianceParams[wheelId],
				suspensionStates[wheelId],
				suspensionComplianceStates[wheelId]);

			//Compute the suspension force from the suspension and compliance states.
			PxVehicleSuspensionLegacyForceUpdate(
				suspensionParams[wheelId], suspensionForceParams[wheelId],
				wheelRoadGeomStates[wheelId], suspensionStates[wheelId], 
				suspensionComplianceStates[wheelId], *rigidBodyState,
				context.gravity,
				suspensionForces[wheelId]);
		}

		if (antiRollForceParams.size>0 && antiRollTorque)
		{
			PxVehicleAntiRollForceUpdate(
				suspensionParams, antiRollForceParams,
				suspensionStates.getConst(), suspensionComplianceStates.getConst(), *rigidBodyState,
				*antiRollTorque);
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
