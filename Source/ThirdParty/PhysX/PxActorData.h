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


#ifndef PX_ACTOR_DATA_H
#define PX_ACTOR_DATA_H

/** \addtogroup physics
  @{
*/

#include "foundation/PxVec4.h"
#include "foundation/PxQuat.h"
#include "foundation/PxFlags.h"
#include "PxNodeIndex.h"


#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Identifies each type of information for retrieving from actor.
	@see PxScene::applyActorData
	*/
	struct PxActorCacheFlag
	{
		enum Enum
		{
			eACTOR_DATA = (1 << 0), //include transform and velocity
			eFORCE = (1 << 2),
			eTORQUE = (1 << 3)
		};
	};

	/**
	\brief Collection of set bits defined in PxActorCacheFlag.

	@see PxActorCacheFlag
	*/
	typedef PxFlags<PxActorCacheFlag::Enum, PxU16> PxActorCacheFlags;
	PX_FLAGS_OPERATORS(PxActorCacheFlag::Enum, PxU16)

	/**
	\brief State of a body used when interfacing with the GPU rigid body pipeline
	@see PxScene.copyBodyData()
	*/
	PX_ALIGN_PREFIX(16)
	struct PxGpuBodyData
	{
		PxQuat			quat;		/*!< actor global pose quaternion in world frame */
		PxVec4			pos;		/*!< (x,y,z members): actor global pose position in world frame */
		PxVec4			linVel;		/*!< (x,y,z members): linear velocity at center of gravity in world frame */
		PxVec4			angVel;		/*!< (x,y,z members): angular velocity in world frame */
	}
	PX_ALIGN_SUFFIX(16);

	/**
	\brief Pair correspondence used for matching array indices with body node indices
	*/
	PX_ALIGN_PREFIX(8)
	struct PxGpuActorPair
	{
		PxU32 srcIndex;					//Defines which index in src array we read
		PxNodeIndex nodeIndex;			//Defines which actor this entry in src array is updating
	}
	PX_ALIGN_SUFFIX(8);

	/**
	\brief Maps numeric index to a data pointer.

	@see PxScene::computeDenseJacobians(), PxScene::computeGeneralizedMassMatrices(), PxScene::computeGeneralizedGravityForces(), PxScene::computeCoriolisAndCentrifugalForces()
	*/	
	struct PxIndexDataPair
	{
		PxU32 index;
		void* data;
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
