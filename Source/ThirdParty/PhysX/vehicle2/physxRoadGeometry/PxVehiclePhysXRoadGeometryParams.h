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

#include "foundation/PxFoundation.h"

#include "PxQueryFiltering.h"

#if !PX_DOXYGEN
namespace physx
{
class PxMaterial;

namespace vehicle2
{
#endif

struct PxVehicleFrame;
struct PxVehicleScale;

/**
\brief PhysX scene queries may be raycasts or sweeps.
\note eNONE will result in no PhysX scene query. This option will not overwrite the associated PxVehicleRoadGeometryState.
*/
struct PxVehiclePhysXRoadGeometryQueryType
{
	enum Enum
	{
		eNONE = 0,  //!< Info about the road geometry below the wheel is provided by the user
		eRAYCAST,   //!< The road geometry below the wheel is analyzed using a raycast query
		eSWEEP,     //!< The road geometry below the wheel is analyzed using a sweep query
		eMAX_NB
	};
};

/**
\brief A description of type of PhysX scene query and the filter data to apply to the query.
*/
struct PxVehiclePhysXRoadGeometryQueryParams
{
	/**
	\brief A description of the type of physx scene query to employ.
	@see PxSceneQuerySystemBase::raycast
	@see PxSceneQuerySystemBase::sweep
	*/
	PxVehiclePhysXRoadGeometryQueryType::Enum roadGeometryQueryType;

	/**
	\brief The filter data to use for the physx scene query.
	@see PxSceneQuerySystemBase::raycast
	@see PxSceneQuerySystemBase::sweep
	*/
	PxQueryFilterData filterData;

	/**
	\brief A filter callback to be used by the physx scene query
	\note A null pointer is allowed.
	@see PxSceneQuerySystemBase::raycast
	@see PxSceneQuerySystemBase::sweep
	*/
	PxQueryFilterCallback* filterCallback;

	PX_FORCE_INLINE PxVehiclePhysXRoadGeometryQueryParams transformAndScale(
		const PxVehicleFrame& srcFrame, const PxVehicleFrame& trgFrame, const PxVehicleScale& srcScale, const PxVehicleScale& trgScale) const
	{
		PX_UNUSED(srcFrame);
		PX_UNUSED(trgFrame);
		PX_UNUSED(srcScale);
		PX_UNUSED(trgScale);
		return *this;
	}

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(roadGeometryQueryType < PxVehiclePhysXRoadGeometryQueryType::eMAX_NB, "PxVehiclePhysXRoadGeometryQueryParams.roadGeometryQueryType has illegal value", false);
		return true;
	}
};

/**
A mapping between PxMaterial and a friction value to be used by the tire model.
@see PxVehiclePhysXMaterialFrictionParams
*/
struct PxVehiclePhysXMaterialFriction
{
	/**
	\brief A PxMaterial instance that is to be mapped to a friction value. 
	*/
	const PxMaterial* material;

	/**
	\brief A friction value that is to be mapped to a PxMaterial instance.
	\note friction must have value greater than or equal to zero.

	<b>Range:</b> [0, inf)<br>

	@see PxVehicleTireGripState::friction
	*/
	PxReal friction;

	PX_FORCE_INLINE bool isValid() const
	{
		PX_CHECK_AND_RETURN_VAL(friction >= 0.0f, "PxVehiclePhysXMaterialFriction.friction must be greater than or equal to zero", false);
		return true;
	}
};

/**
\brief A mappping between PxMaterial instance and friction for multiple PxMaterial intances.
*/
struct PxVehiclePhysXMaterialFrictionParams
{
	PxVehiclePhysXMaterialFriction* materialFrictions; //!< An array of mappings between PxMaterial and friction.
	PxU32 nbMaterialFrictions; //!< The number of mappings between PxMaterial and friction.
	PxReal defaultFriction; //!< A default friction value to be used in the event that the PxMaterial under the tire is not found in the array #materialFrictions.

	PX_FORCE_INLINE bool isValid() const
	{
		for (PxU32 i = 0; i < nbMaterialFrictions; i++)
		{
			if (!materialFrictions[i].isValid())
				return false;
		}
		PX_CHECK_AND_RETURN_VAL(defaultFriction >= 0.0f, "PxVehiclePhysXMaterialFrictionParams.defaultFriction must be greater than or equal to zero", false);
		return true;
	}
};

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
