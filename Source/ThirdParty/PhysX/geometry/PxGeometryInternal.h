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

#ifndef PX_GEOMETRY_INTERNAL_H
#define PX_GEOMETRY_INTERNAL_H
/** \addtogroup geomutils 
@{ */

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"
#include "geometry/PxTriangleMesh.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxTriangleMesh;

	struct PxTriangleMeshInternalData
	{
		PxU32	mNbVertices;
		PxU32	mNbTriangles;
		PxVec3*	mVertices;
		void*	mTriangles;
		PxU32*	mFaceRemap;
		PxVec3	mAABB_Center;
		PxVec3	mAABB_Extents;
		PxReal	mGeomEpsilon;
		PxU8	mFlags;
		//
		PxU32	mNbNodes;
		PxU32	mNodeSize;
		void*	mNodes;
		PxU32	mInitData;
		PxVec3	mCenterOrMinCoeff;
		PxVec3	mExtentsOrMaxCoeff;
		bool	mQuantized;

		PX_FORCE_INLINE	PxU32	getSizeofVerticesInBytes()	const
		{
			return mNbVertices * sizeof(PxVec3);
		}

		PX_FORCE_INLINE	PxU32	getSizeofTrianglesInBytes()	const
		{
			const PxU32 triangleSize = mFlags & PxTriangleMeshFlag::e16_BIT_INDICES ? sizeof(PxU16) : sizeof(PxU32);
			return mNbTriangles * 3 * triangleSize;
		}

		PX_FORCE_INLINE	PxU32	getSizeofFaceRemapInBytes()	const
		{
			return mNbTriangles * sizeof(PxU32);
		}

		PX_FORCE_INLINE	PxU32	getSizeofNodesInBytes()	const
		{
			return mNbNodes * mNodeSize;
		}
	};

	PX_C_EXPORT PX_PHYSX_COMMON_API bool PX_CALL_CONV PxGetTriangleMeshInternalData(PxTriangleMeshInternalData& data, const PxTriangleMesh& mesh, bool takeOwnership);

	class PxBVH;

	struct PxBVHInternalData
	{
		PxU32	mNbIndices;
		PxU32	mNbNodes;
		PxU32	mNodeSize;
		void*	mNodes;
		PxU32*	mIndices;	// Can be null
		void*	mBounds;

		PX_FORCE_INLINE	PxU32	getSizeofNodesInBytes()	const
		{
			return mNbNodes * mNodeSize;
		}

		PX_FORCE_INLINE	PxU32	getSizeofIndicesInBytes()	const
		{
			return mNbIndices * sizeof(PxU32);
		}

		PX_FORCE_INLINE	PxU32	getSizeofBoundsInBytes()	const
		{
			return (mNbIndices+1)*6;
		}
	};

	PX_C_EXPORT PX_PHYSX_COMMON_API bool PX_CALL_CONV PxGetBVHInternalData(PxBVHInternalData& data, const PxBVH& bvh, bool takeOwnership);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
