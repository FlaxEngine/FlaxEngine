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

#ifndef PSFOUNDATION_PSBITUTILS_H
#define PSFOUNDATION_PSBITUTILS_H

#include "foundation/PxIntrinsics.h"
#include "../Callbacks.h"
#include "PsIntrinsics.h"
#include "Ps.h"

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
PX_INLINE uint32_t bitCount(uint32_t v)
{
	// from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	uint32_t const w = v - ((v >> 1) & 0x55555555);
	uint32_t const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
	return (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

PX_INLINE bool isPowerOfTwo(uint32_t x)
{
	return x != 0 && (x & (x - 1)) == 0;
}

// "Next Largest Power of 2
// Given a binary integer value x, the next largest power of 2 can be computed by a SWAR algorithm
// that recursively "folds" the upper bits into the lower bits. This process yields a bit vector with
// the same most significant 1 as x, but all 1's below it. Adding 1 to that value yields the next
// largest power of 2. For a 32-bit value:"
PX_INLINE uint32_t nextPowerOfTwo(uint32_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

/*!
Return the index of the highest set bit. Not valid for zero arg.
*/

PX_INLINE uint32_t lowestSetBit(uint32_t x)
{
	NV_CLOTH_ASSERT(x);
	return PxLowestSetBitUnsafe(x);
}

/*!
Return the index of the highest set bit. Not valid for zero arg.
*/

PX_INLINE uint32_t highestSetBit(uint32_t x)
{
	NV_CLOTH_ASSERT(x);
	return PxHighestSetBitUnsafe(x);
}

// Helper function to approximate log2 of an integer value
// assumes that the input is actually power of two.
// todo: replace 2 usages with 'highestSetBit'
PX_INLINE uint32_t ilog2(uint32_t num)
{
	for(uint32_t i = 0; i < 32; i++)
	{
		num >>= 1;
		if(num == 0)
			return i;
	}

	NV_CLOTH_ASSERT(0);
	return uint32_t(-1);
}

} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSBITUTILS_H
