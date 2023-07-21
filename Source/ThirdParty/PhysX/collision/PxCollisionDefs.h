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

#ifndef PX_COLLISION_DEFS_H
#define PX_COLLISION_DEFS_H

#include "PxPhysXConfig.h"
#include "foundation/PxSimpleTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief A callback class to allocate memory to cache information used in contact generation.
*/
class PxCacheAllocator
{
public:
	/**
	\brief Allocates cache data for contact generation. This data is stored inside PxCache objects.
	The application can retain and provide this information for future contact generation passes
	for a given pair to improve contact generation performance. It is the application's responsibility
	to release this memory appropriately. If the memory is released, the application must ensure that
	this memory is no longer referenced by any PxCache objects passed to PxGenerateContacts.

	\param	byteSize	[in] size of the allocation in bytes

	\return the newly-allocated memory. The returned address must be 16-byte aligned.

	@see PxCache, PxGenerateContacts
	*/
	virtual PxU8* allocateCacheData(const PxU32 byteSize) = 0;

	virtual ~PxCacheAllocator() {}
};

/**
\brief A structure to cache contact information produced by low-level contact generation functions.
*/
struct PxCache
{
	PxU8*		mCachedData;	//!< Cached data pointer. Allocated via PxCacheAllocator
	PxU16		mCachedSize;	//!< The total size of the cached data 
	PxU8		mPairData;		//!< Pair data information used and cached internally by some contact generation functions to accelerate performance.
	PxU8		mManifoldFlags;	//!< Manifold flags used to identify the format the cached data is stored in. @see Gu::ManifoldFlags

	PX_FORCE_INLINE	PxCache() : mCachedData(NULL), mCachedSize(0), mPairData(0), mManifoldFlags(0)
	{
	}

	PX_FORCE_INLINE void reset()
	{
		mCachedData = NULL;
		mCachedSize = 0;
		mPairData = 0;
		mManifoldFlags = 0;
	}
};

#if !PX_DOXYGEN
}
#endif

#endif

