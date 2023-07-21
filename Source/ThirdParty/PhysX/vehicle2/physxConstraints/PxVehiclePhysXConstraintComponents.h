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

#include "vehicle2/roadGeometry/PxVehicleRoadGeometryState.h"
#include "vehicle2/suspension/PxVehicleSuspensionParams.h"
#include "vehicle2/suspension/PxVehicleSuspensionStates.h"

#include "PxVehiclePhysXConstraintFunctions.h"
#include "PxVehiclePhysXConstraintHelpers.h"
#include "PxVehiclePhysXConstraintStates.h"
#include "PxVehiclePhysXConstraintParams.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

class PxVehiclePhysXConstraintComponent : public PxVehicleComponent
{
public:
	PxVehiclePhysXConstraintComponent() : PxVehicleComponent() {}
	virtual ~PxVehiclePhysXConstraintComponent() {}

	virtual void getDataForPhysXConstraintComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehiclePhysXSuspensionLimitConstraintParams>& suspensionLimitParams,
		PxVehicleArrayData<const PxVehicleSuspensionState>& suspensionStates,
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
		PxVehicleArrayData<const PxVehicleRoadGeometryState>& wheelRoadGeomStates,
		PxVehicleArrayData<const PxVehicleTireDirectionState>& tireDirectionStates,
		PxVehicleArrayData<const PxVehicleTireStickyState>& tireStickyStates,
		PxVehiclePhysXConstraints*& constraints) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);
		PX_UNUSED(context);

		PX_PROFILE_ZONE("PxVehiclePhysXConstraintComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehiclePhysXSuspensionLimitConstraintParams> suspensionLimitParams;
		PxVehicleArrayData<const PxVehicleSuspensionState> suspensionStates;
		PxVehicleArrayData<const PxVehicleSuspensionComplianceState> suspensionComplianceStates;
		PxVehicleArrayData<const PxVehicleRoadGeometryState> wheelRoadGeomStates;
		PxVehicleArrayData<const PxVehicleTireDirectionState> tireDirectionStates;
		PxVehicleArrayData<const PxVehicleTireStickyState> tireStickyStates;
		PxVehiclePhysXConstraints* constraints;

		getDataForPhysXConstraintComponent(axleDescription, rigidBodyState, 
			suspensionParams, suspensionLimitParams, suspensionStates, suspensionComplianceStates,
			wheelRoadGeomStates, tireDirectionStates, tireStickyStates,
			constraints);

		PxVehicleConstraintsDirtyStateUpdate(*constraints);

		for (PxU32 i = 0; i < axleDescription->nbWheels; i++)
		{
			const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

			PxVehiclePhysXConstraintStatesUpdate(
				suspensionParams[wheelId], suspensionLimitParams[wheelId],
				suspensionStates[wheelId], suspensionComplianceStates[wheelId], 
				wheelRoadGeomStates[wheelId].plane.n,
				context.tireStickyParams.stickyParams[PxVehicleTireDirectionModes::eLONGITUDINAL].damping,
				context.tireStickyParams.stickyParams[PxVehicleTireDirectionModes::eLATERAL].damping,
				tireDirectionStates[wheelId], tireStickyStates[wheelId], 
				*rigidBodyState,
				constraints->constraintStates[wheelId]);
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
