// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PSFOUNDATION_PSALIGNEDMALLOC_H
#define PSFOUNDATION_PSALIGNEDMALLOC_H

#include "PsUserAllocated.h"

/*!
Allocate aligned memory.
Alignment must be a power of 2!
-- should be templated by a base allocator
*/

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
/**
Allocator, which is used to access the global PxAllocatorCallback instance
(used for dynamic data types template instantiation), which can align memory
*/

// SCS: AlignedMalloc with 3 params not found, seems not used on PC either
// disabled for now to avoid GCC error

template <uint32_t N, typename BaseAllocator = NonTrackingAllocator>
class AlignedAllocator : public BaseAllocator
{
  public:
	AlignedAllocator(const BaseAllocator& base = BaseAllocator()) : BaseAllocator(base)
	{
	}

	void* allocate(size_t size, const char* file, int line)
	{
		size_t pad = N - 1 + sizeof(size_t); // store offset for delete.
		uint8_t* base = reinterpret_cast<uint8_t*>(BaseAllocator::allocate(size + pad, file, line));
		if(!base)
			return NULL;

		uint8_t* ptr = reinterpret_cast<uint8_t*>(size_t(base + pad) & ~(size_t(N) - 1)); // aligned pointer, ensuring N
		// is a size_t
		// wide mask
		reinterpret_cast<size_t*>(ptr)[-1] = size_t(ptr - base); // store offset

		return ptr;
	}
	void deallocate(void* ptr)
	{
		if(ptr == NULL)
			return;

		uint8_t* base = reinterpret_cast<uint8_t*>(ptr) - reinterpret_cast<size_t*>(ptr)[-1];
		BaseAllocator::deallocate(base);
	}
};

} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSALIGNEDMALLOC_H
