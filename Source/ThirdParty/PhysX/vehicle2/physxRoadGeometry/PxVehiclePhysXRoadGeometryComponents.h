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
#include "vehicle2/wheel/PxVehicleWheelParams.h"

#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryFunctions.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryParams.h"
#include "vehicle2/physxRoadGeometry/PxVehiclePhysXRoadGeometryState.h"

#include "common/PxProfileZone.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

class PxVehiclePhysXRoadGeometrySceneQueryComponent : public PxVehicleComponent
{
public:
	PxVehiclePhysXRoadGeometrySceneQueryComponent() : PxVehicleComponent() {}
	virtual ~PxVehiclePhysXRoadGeometrySceneQueryComponent() {}

	/**
	\brief Provide vehicle data items for this component.
	
	\param[out] axleDescription identifies the wheels on each axle.
	\param[out] roadGeomParams The road geometry parameters of the vehicle.
	\param[out] steerResponseStates The steer response state of the wheels.
	\param[out] rigidBodyState The pose, velocity etc. of the vehicle rigid body.
	\param[out] wheelParams The wheel parameters for the wheels.
	\param[out] suspensionParams The suspension parameters for the wheels.
	\param[out] materialFrictionParams The tire friction tables for the wheels.
	\param[out] roadGeometryStates The detected ground surface plane, friction value etc. for the wheels.
	\param[out] physxRoadGeometryStates Optional buffer to store additional information about the query (like actor/shape that got hit etc.).
	            Set to empty if not desired.
	*/
	virtual void getDataForPhysXRoadGeometrySceneQueryComponent(
		const PxVehicleAxleDescription*& axleDescription,
		const PxVehiclePhysXRoadGeometryQueryParams*& roadGeomParams,
		PxVehicleArrayData<const PxReal>& steerResponseStates,
		const PxVehicleRigidBodyState*& rigidBodyState,
		PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
		PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
		PxVehicleArrayData<const PxVehiclePhysXMaterialFrictionParams>& materialFrictionParams,
		PxVehicleArrayData<PxVehicleRoadGeometryState>& roadGeometryStates,
		PxVehicleArrayData<PxVehiclePhysXRoadGeometryQueryState>& physxRoadGeometryStates) = 0;

	virtual bool update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_UNUSED(dt);

		PX_PROFILE_ZONE("PxVehiclePhysXRoadGeometrySceneQueryComponent::update", 0);

		const PxVehicleAxleDescription* axleDescription;
		const PxVehiclePhysXRoadGeometryQueryParams* roadGeomParams;
		PxVehicleArrayData<const PxReal> steerResponseStates;
		const PxVehicleRigidBodyState* rigidBodyState;
		PxVehicleArrayData<const PxVehicleWheelParams> wheelParams;
		PxVehicleArrayData<const PxVehicleSuspensionParams> suspensionParams;
		PxVehicleArrayData<const PxVehiclePhysXMaterialFrictionParams> materialFrictionParams;
		PxVehicleArrayData<PxVehicleRoadGeometryState> roadGeometryStates;
		PxVehicleArrayData<PxVehiclePhysXRoadGeometryQueryState> physxRoadGeometryStates;

		getDataForPhysXRoadGeometrySceneQueryComponent(axleDescription, 
			roadGeomParams, steerResponseStates, rigidBodyState,
			wheelParams, suspensionParams, materialFrictionParams,
			roadGeometryStates, physxRoadGeometryStates);

		if (context.getType() == PxVehicleSimulationContextType::ePHYSX)
		{
			const PxVehiclePhysXSimulationContext& physxContext = static_cast<const PxVehiclePhysXSimulationContext&>(context);

			for(PxU32 i = 0; i < axleDescription->nbWheels; i++)
			{
				const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

				PxVehiclePhysXRoadGeometryQueryUpdate(
					wheelParams[wheelId], suspensionParams[wheelId], 
					*roadGeomParams, materialFrictionParams[wheelId],
					steerResponseStates[wheelId], *rigidBodyState,
					*physxContext.physxScene, physxContext.physxUnitCylinderSweepMesh, context.frame,
					roadGeometryStates[wheelId], 
					!physxRoadGeometryStates.isEmpty() ? &physxRoadGeometryStates[wheelId] : NULL);
			}
		}
		else
		{
			PX_ALWAYS_ASSERT();

			for(PxU32 i = 0; i < axleDescription->nbWheels; i++)
			{
				const PxU32 wheelId = axleDescription->wheelIdsInAxleOrder[i];

				roadGeometryStates[wheelId].setToDefault();
			}
		}

		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
