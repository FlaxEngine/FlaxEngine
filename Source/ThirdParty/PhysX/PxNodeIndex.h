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

#ifndef PX_NODEINDEX_H
#define PX_NODEINDEX_H

#include "foundation/PxSimpleTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#define PX_INVALID_NODE 0xFFFFFFFFu


	/**
	\brief PxNodeIndex

	Node index is the unique index for each actor referenced by the island gen. It contains details like 
	if the actor is an articulation or rigid body. If it is an articulation, the node index also contains
	the link index of the rigid body within the articulation. Also, it contains information to detect whether
	the rigid body is static body or not
	*/
	class PxNodeIndex
	{
	protected:
		PxU64 ind;

	public:

		explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxNodeIndex(PxU32 id, PxU32 articLinkId) : ind((PxU64(id) << 32) | (articLinkId << 1) | 1)
		{
		}

		explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxNodeIndex(PxU32 id = PX_INVALID_NODE) : ind((PxU64(id) << 32))
		{
		}

		PX_CUDA_CALLABLE PX_FORCE_INLINE PxU32 index() const { return PxU32(ind >> 32); }
		PX_CUDA_CALLABLE PX_FORCE_INLINE PxU32 articulationLinkId() const { return PxU32((ind >> 1) & 0x7FFFFFFF); }
		PX_CUDA_CALLABLE PX_FORCE_INLINE PxU32 isArticulation() const { return PxU32(ind & 1); }

		PX_CUDA_CALLABLE PX_FORCE_INLINE bool isStaticBody() const { return PxU32(ind >> 32) == PX_INVALID_NODE; }

		PX_CUDA_CALLABLE bool isValid() const { return PxU32(ind >> 32) != PX_INVALID_NODE; }

		PX_CUDA_CALLABLE void setIndices(PxU32 index, PxU32 articLinkId) { ind = ((PxU64(index) << 32) | (articLinkId << 1) | 1); }

		PX_CUDA_CALLABLE void setIndices(PxU32 index) { ind = ((PxU64(index) << 32)); }

		PX_CUDA_CALLABLE bool operator < (const PxNodeIndex& other) const { return ind < other.ind; }

		PX_CUDA_CALLABLE bool operator <= (const PxNodeIndex& other) const { return ind <= other.ind; }

		PX_CUDA_CALLABLE bool operator == (const PxNodeIndex& other) const { return ind == other.ind; }

		PX_CUDA_CALLABLE PxU64 getInd() const { return ind; }
	};
#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
