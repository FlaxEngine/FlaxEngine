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

#ifndef PX_INLINE_ALLOCATOR_H
#define PX_INLINE_ALLOCATOR_H

#include "foundation/PxUserAllocated.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
// this is used by the array class to allocate some space for a small number
// of objects along with the metadata
template <PxU32 N, typename BaseAllocator>
class PxInlineAllocator : private BaseAllocator
{
  public:
	PxInlineAllocator(const PxEMPTY v) : BaseAllocator(v)
	{
	}

	PxInlineAllocator(const BaseAllocator& alloc = BaseAllocator()) : BaseAllocator(alloc), mBufferUsed(false)
	{
	}

	PxInlineAllocator(const PxInlineAllocator& aloc) : BaseAllocator(aloc), mBufferUsed(false)
	{
	}

	void* allocate(PxU32 size, const char* filename, PxI32 line)
	{
		if(!mBufferUsed && size <= N)
		{
			mBufferUsed = true;
			return mBuffer;
		}
		return BaseAllocator::allocate(size, filename, line);
	}

	void deallocate(void* ptr)
	{
		if(ptr == mBuffer)
			mBufferUsed = false;
		else
			BaseAllocator::deallocate(ptr);
	}

	PX_FORCE_INLINE PxU8* getInlineBuffer()
	{
		return mBuffer;
	}
	PX_FORCE_INLINE bool isBufferUsed() const
	{
		return mBufferUsed;
	}

  protected:
	PxU8 mBuffer[N];
	bool mBufferUsed;
};
#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

