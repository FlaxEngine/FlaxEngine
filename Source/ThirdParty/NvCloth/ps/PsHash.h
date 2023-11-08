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

#ifndef PSFOUNDATION_PSHASH_H
#define PSFOUNDATION_PSHASH_H

#include "Ps.h"
#include "PsBasicTemplates.h"

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4302)
#endif

#if PX_LINUX
#include "foundation/PxSimpleTypes.h"
#endif

/*!
Central definition of hash functions
*/

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
// Hash functions

// Thomas Wang's 32 bit mix
// http://www.cris.com/~Ttwang/tech/inthash.htm
PX_FORCE_INLINE uint32_t hash(const uint32_t key)
{
	uint32_t k = key;
	k += ~(k << 15);
	k ^= (k >> 10);
	k += (k << 3);
	k ^= (k >> 6);
	k += ~(k << 11);
	k ^= (k >> 16);
	return uint32_t(k);
}

PX_FORCE_INLINE uint32_t hash(const int32_t key)
{
	return hash(uint32_t(key));
}

// Thomas Wang's 64 bit mix
// http://www.cris.com/~Ttwang/tech/inthash.htm
PX_FORCE_INLINE uint32_t hash(const uint64_t key)
{
	uint64_t k = key;
	k += ~(k << 32);
	k ^= (k >> 22);
	k += ~(k << 13);
	k ^= (k >> 8);
	k += (k << 3);
	k ^= (k >> 15);
	k += ~(k << 27);
	k ^= (k >> 31);
	return uint32_t(UINT32_MAX & k);
}

#if PX_APPLE_FAMILY
// hash for size_t, to make gcc happy
PX_INLINE uint32_t hash(const size_t key)
{
#if PX_P64_FAMILY
	return hash(uint64_t(key));
#else
	return hash(uint32_t(key));
#endif
}
#endif

// Hash function for pointers
PX_INLINE uint32_t hash(const void* ptr)
{
#if PX_P64_FAMILY
	return hash(uint64_t(ptr));
#else
	return hash(uint32_t(UINT32_MAX & size_t(ptr)));
#endif
}

// Hash function for pairs
template <typename F, typename S>
PX_INLINE uint32_t hash(const Pair<F, S>& p)
{
	uint32_t seed = 0x876543;
	uint32_t m = 1000007;
	return hash(p.second) ^ (m * (hash(p.first) ^ (m * seed)));
}

// hash object for hash map template parameter
template <class Key>
struct Hash
{
	uint32_t operator()(const Key& k) const
	{
		return hash(k);
	}
	bool equal(const Key& k0, const Key& k1) const
	{
		return k0 == k1;
	}
};

// specialization for strings
template <>
struct Hash<const char*>
{
  public:
	uint32_t operator()(const char* _string) const
	{
		// "DJB" string hash
		const uint8_t* string = reinterpret_cast<const uint8_t*>(_string);
		uint32_t h = 5381;
		for(const uint8_t* ptr = string; *ptr; ptr++)
			h = ((h << 5) + h) ^ uint32_t(*ptr);
		return h;
	}
	bool equal(const char* string0, const char* string1) const
	{
		return !strcmp(string0, string1);
	}
};

} // namespace ps
} // namespace cloth
} // namespace nv

#if PX_VC
#pragma warning(pop)
#endif

#endif // #ifndef PSFOUNDATION_PSHASH_H
