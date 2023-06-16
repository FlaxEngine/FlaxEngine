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

#ifndef PX_ALLOCA_H
#define PX_ALLOCA_H

#include "foundation/PxTempAllocator.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
template <typename T, typename Alloc = PxTempAllocator>
class PxScopedPointer : private Alloc
{
  public:
	~PxScopedPointer()
	{
		if(mOwned)
			Alloc::deallocate(mPointer);
	}

	operator T*() const
	{
		return mPointer;
	}

	T* mPointer;
	bool mOwned;
};

#if !PX_DOXYGEN
} // namespace physx
#endif

  // Don't use inline for alloca !!!
#if PX_WINDOWS_FAMILY
#include <malloc.h>
#define PxAlloca(x) _alloca(x)
#elif PX_LINUX || PX_ANDROID
#include <malloc.h>
#define PxAlloca(x) alloca(x)
#elif PX_APPLE_FAMILY
#include <alloca.h>
#define PxAlloca(x) alloca(x)
#elif PX_PS4 || PX_PS5
#include <memory.h>
#define PxAlloca(x) alloca(x)
#elif PX_SWITCH
#include <malloc.h>
#define PxAlloca(x) alloca(x)
#endif

#define PxAllocaAligned(x, alignment) ((size_t(PxAlloca(x + alignment)) + (alignment - 1)) & ~size_t(alignment - 1))

/*! Stack allocation for \c count instances of \c type. Falling back to temp allocator if using more than 1kB. */
#define PX_ALLOCA(var, type, count)                                                                                    \
	physx::PxScopedPointer<type> var;                                                                            \
	{                                                                                                                  \
		uint32_t size = sizeof(type) * (count);                                                                        \
		var.mOwned = size > 1024;                                                                                      \
		if(var.mOwned)                                                                                                 \
			var.mPointer = reinterpret_cast<type*>(physx::PxTempAllocator().allocate(size, __FILE__, __LINE__)); \
		else                                                                                                           \
			var.mPointer = reinterpret_cast<type*>(PxAlloca(size));                                                    \
	}
#endif

