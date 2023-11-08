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

#ifndef PSFOUNDATION_PSUSERALLOCATED_H
#define PSFOUNDATION_PSUSERALLOCATED_H

#include "PsAllocator.h"

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
/**
Provides new and delete using a UserAllocator.
Guarantees that 'delete x;' uses the UserAllocator too.
*/
class UserAllocated
{
  public:
	// PX_SERIALIZATION
	PX_INLINE void* operator new(size_t, void* address)
	{
		return address;
	}
	//~PX_SERIALIZATION
	// Matching operator delete to the above operator new.  Don't ask me
	// how this makes any sense - Nuernberger.
	PX_INLINE void operator delete(void*, void*)
	{
	}

	template <typename Alloc>
	PX_INLINE void* operator new(size_t size, Alloc alloc, const char* fileName, int line)
	{
		return alloc.allocate(size, fileName, line);
	}
	template <typename Alloc>
	PX_INLINE void* operator new [](size_t size, Alloc alloc, const char* fileName, int line)
	{ return alloc.allocate(size, fileName, line); }

	// placement delete
	template <typename Alloc>
	PX_INLINE void operator delete(void* ptr, Alloc alloc, const char* fileName, int line)
	{
		PX_UNUSED(fileName);
		PX_UNUSED(line);
		alloc.deallocate(ptr);
	}
	template <typename Alloc>
	PX_INLINE void operator delete [](void* ptr, Alloc alloc, const char* fileName, int line)
	{
		PX_UNUSED(fileName);
		PX_UNUSED(line);
		alloc.deallocate(ptr);
	} PX_INLINE void
	operator delete(void* ptr)
	{
		NonTrackingAllocator().deallocate(ptr);
	}
	PX_INLINE void operator delete [](void* ptr)
	{ NonTrackingAllocator().deallocate(ptr); }
};
} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSUSERALLOCATED_H
