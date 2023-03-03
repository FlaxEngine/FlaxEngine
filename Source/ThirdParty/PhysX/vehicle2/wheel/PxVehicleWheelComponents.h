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
#include "vehicle2/tire/PxVehicleTireStates.h"

#include "PxVehicleWheelFunctions.h"
#include "PxVehicleWheelHelpers.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

class PxVehicleWheelComponent : public PxVehicleComponent
{
public:

	PxVehicleWheelComponent() : PxVehicleComponent() {}
	virtual ~PxVehicleWheelComponent() {}

	virtual void getDataForWheelComponent(
		const PxVehicleAxleDescription*& axleDescription,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehicleWheelActuationState>& actuationStates,
		PxVehicleArrayData<const PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<const PxVehicleTireSpeedState>& tireSpeedStates,
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState>& wheelRigidBody1dStates,
  	    PxVehicleArrayData<PxVehicleWheelLocalPose>& wheelLocalPoses) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_PROFILE_ZONE("PxVehicleWheelComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehicleWheelActuationState> actuationStates;
		PxVehicleArrayData<const PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<const PxVehicleTireSpeedState> tireSpeedStates;
		PxVehicleArrayData<PxVehicleWheelRigidBody1dState> wheelRigidBody1dStates;
		PxVehicleArrayData<PxVehicleWheelLocalPose> wheelLocalPoses;

		getDataForWheelComponent(axleDescription, steerResponseStates,
			wheelParams, suspensionParams, actuationStates, suspensionStates,
			suspensionComplianceStates, tireSpeedStates, wheelRigidBody1dStates,
			wheelLocalPoses);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			PxVehicleWheelRotationAngleUpdate(
				wheelParams[wheelId], 
				actuationStates[wheelId], suspensionStates[wheelId], 
				tireSpeedStates[wheelId], 
				context.thresholdForwardSpeedForWheelAngleIntegration, dt, 
				wheelRigidBody1dStates[wheelId]);

			wheelLocalPoses[wheelId].localPose = PxVehicleComputeWheelLocalPose(context.frame, 
				suspensionParams[wheelId],
				suspensionStates[wheelId], 
				suspensionComplianceStates[wheelId],
				steerResponseStates[wheelId],
				wheelRigidBody1dStates[wheelId]);
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
