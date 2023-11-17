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


/** \file Allocator.h
	This file together with Callbacks.h define most memory management interfaces for internal use.
*/

/** \cond HIDDEN_SYMBOLS */
#pragma once

#include "NvCloth/ps/PsArray.h"
#include "NvCloth/ps/PsHashMap.h"
#include "NvCloth/Callbacks.h"
#include "NvCloth/ps/PsAlignedMalloc.h"

namespace nv
{
namespace cloth
{

void* allocate(size_t);
void deallocate(void*);


/* templated typedefs for convenience */

template <typename T>
struct Vector
{
	typedef ps::Array<T, ps::NonTrackingAllocator> Type;
};

template <typename T, size_t alignment>
struct AlignedVector
{
	typedef ps::Array<T, ps::AlignedAllocator<alignment, ps::NonTrackingAllocator> > Type;
};

template <class Key, class Value, class HashFn = ps::Hash<Key> >
struct HashMap
{
	typedef ps::HashMap<Key, Value, HashFn, ps::NonTrackingAllocator> Type;
};

struct NvClothOverload{};
#define NV_CLOTH_NEW(T) new (__FILE__, __LINE__, nv::cloth::NvClothOverload()) T
#define NV_CLOTH_ALLOC(n, name) GetNvClothAllocator()->allocate(n, name, __FILE__, __LINE__)
#define NV_CLOTH_FREE(x) GetNvClothAllocator()->deallocate(x)
#define NV_CLOTH_DELETE(x) delete x

} // namespace cloth
} // namespace nv

//new/delete operators need to be declared in global scope
template <typename T>
PX_INLINE void* operator new(size_t size, const char* fileName,
                             typename nv::cloth::ps::EnableIfPod<T, int>::Type line, nv::cloth::NvClothOverload overload)
{
	PX_UNUSED(overload);
	return GetNvClothAllocator()->allocate(size, "<TypeName Unknown>", fileName, line);
}

template <typename T>
PX_INLINE void* operator new [](size_t size, const char* fileName,
                                typename nv::cloth::ps::EnableIfPod<T, int>::Type line, nv::cloth::NvClothOverload overload)
{
	PX_UNUSED(overload);
	return GetNvClothAllocator()->allocate(size, "<TypeName Unknown>", fileName, line);
}

// If construction after placement new throws, this placement delete is being called.
template <typename T>
PX_INLINE void operator delete(void* ptr, const char* fileName,
                               typename nv::cloth::ps::EnableIfPod<T, int>::Type line, nv::cloth::NvClothOverload overload)
{
	PX_UNUSED(fileName);
	PX_UNUSED(line);
	PX_UNUSED(overload);

	return GetNvClothAllocator()->deallocate(ptr);
}

// If construction after placement new throws, this placement delete is being called.
template <typename T>
PX_INLINE void operator delete [](void* ptr, const char* fileName,
                                  typename nv::cloth::ps::EnableIfPod<T, int>::Type line, nv::cloth::NvClothOverload overload)
{
	PX_UNUSED(fileName);
	PX_UNUSED(line);
	PX_UNUSED(overload);

	return GetNvClothAllocator()->deallocate(ptr);
}

namespace nv
{
namespace cloth
{

class UserAllocated
{
  public:
	PX_INLINE void* operator new(size_t size, const char* fileName, int line, NvClothOverload overload)
	{
		PX_UNUSED(overload);
		return GetNvClothAllocator()->allocate(size, "<TypeName Unknown>", fileName, line);
	}
	PX_INLINE void* operator new [](size_t size, const char* fileName, int line, NvClothOverload overload)
	{
		PX_UNUSED(overload);
		return GetNvClothAllocator()->allocate(size, "<TypeName Unknown>", fileName, line);
	}

	// placement delete
	PX_INLINE void operator delete(void* ptr, const char* fileName, int line, NvClothOverload overload)
	{
		PX_UNUSED(fileName);
		PX_UNUSED(line);
		PX_UNUSED(overload);
		GetNvClothAllocator()->deallocate(ptr);
	}
	PX_INLINE void operator delete [](void* ptr, const char* fileName, int line, NvClothOverload overload)
	{
		PX_UNUSED(fileName);
		PX_UNUSED(line);
		PX_UNUSED(overload);
		GetNvClothAllocator()->deallocate(ptr);
	} 
	PX_INLINE void operator delete(void* ptr)
	{
		return GetNvClothAllocator()->deallocate(ptr);
	}
	PX_INLINE void operator delete [](void* ptr)
	{
		return GetNvClothAllocator()->deallocate(ptr);
	}
};

} // namespace cloth
} // namespace nv
/** \endcond */
