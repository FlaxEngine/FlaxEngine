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

#ifndef PX_MEMORY_H
#define PX_MEMORY_H

/** \addtogroup foundation
@{
*/

#include "foundation/Px.h"
#include "foundation/PxMathIntrinsics.h"
#include "foundation/PxSimpleTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief Sets the bytes of the provided buffer to zero.

	\param	dest	[out]	Pointer to block of memory to set zero.
	\param	count	[in]	Number of bytes to set to zero.

	\return Pointer to memory block (same as input)
	*/
	PX_FORCE_INLINE void* PxMemZero(void* dest, PxU32 count)
	{
		return physx::intrinsics::memZero(dest, count);
	}

	/**
	\brief Sets the bytes of the provided buffer to the specified value.

	\param	dest	[out]	Pointer to block of memory to set to the specified value.
	\param	c		[in]	Value to set the bytes of the block of memory to.
	\param	count	[in]	Number of bytes to set to the specified value.

	\return Pointer to memory block (same as input)
	*/
	PX_FORCE_INLINE void* PxMemSet(void* dest, PxI32 c, PxU32 count)
	{
		return physx::intrinsics::memSet(dest, c, count);
	}

	/**
	\brief Copies the bytes of one memory block to another. The memory blocks must not overlap.

	\note Use #PxMemMove if memory blocks overlap.

	\param dest		[out]	Pointer to block of memory to copy to.
	\param src		[in]	Pointer to block of memory to copy from.
	\param count	[in]	Number of bytes to copy.

	\return Pointer to destination memory block
	*/
	PX_FORCE_INLINE void* PxMemCopy(void* dest, const void* src, PxU32 count)
	{
		return physx::intrinsics::memCopy(dest, src, count);
	}

	/**
	\brief Copies the bytes of one memory block to another. The memory blocks can overlap.

	\note Use #PxMemCopy if memory blocks do not overlap.

	\param dest		[out]	Pointer to block of memory to copy to.
	\param src		[in]	Pointer to block of memory to copy from.
	\param count	[in]	Number of bytes to copy.

	\return Pointer to destination memory block
	*/
	PX_FORCE_INLINE void* PxMemMove(void* dest, const void* src, PxU32 count)
	{
		return physx::intrinsics::memMove(dest, src, count);
	}

	/**
	Mark a specified amount of memory with 0xcd pattern. This is used to check that the meta data 
	definition for serialized classes is complete in checked builds.

	\param ptr		[out]	Pointer to block of memory to initialize.
	\param byteSize	[in]	Number of bytes to initialize.
	*/
	PX_INLINE void PxMarkSerializedMemory(void* ptr, PxU32 byteSize)
	{
#if PX_CHECKED
		PxMemSet(ptr, 0xcd, byteSize);
#else
		PX_UNUSED(ptr);
		PX_UNUSED(byteSize);
#endif
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

