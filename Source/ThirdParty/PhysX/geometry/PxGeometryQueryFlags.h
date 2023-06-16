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

#ifndef PX_GEOMETRY_QUERY_FLAGS_H
#define PX_GEOMETRY_QUERY_FLAGS_H

#include "foundation/PxFlags.h"
#include "common/PxPhysXCommonConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Geometry-level query flags.

	@see PxScene::raycast PxScene::overlap PxScene::sweep PxBVH::raycast PxBVH::overlap PxBVH::sweep PxGeometryQuery::raycast PxGeometryQuery::overlap PxGeometryQuery::sweep
	@see PxGeometryQuery::computePenetration PxGeometryQuery::pointDistance PxGeometryQuery::computeGeomBounds
	@see PxMeshQuery::findOverlapTriangleMesh PxMeshQuery::findOverlapHeightField PxMeshQuery::sweep
	*/
	struct PxGeometryQueryFlag
	{
		enum Enum
		{
			eSIMD_GUARD	= (1<<0),	//!< Saves/restores SIMD control word for each query (safer but slower). Omit this if you took care of it yourself in your app.

			eDEFAULT	= eSIMD_GUARD
		};
	};

	/**
	\brief collection of set bits defined in PxGeometryQueryFlag.

	@see PxGeometryQueryFlag
	*/
	PX_FLAGS_TYPEDEF(PxGeometryQueryFlag, PxU32)

#if !PX_DOXYGEN
}
#endif

#endif
