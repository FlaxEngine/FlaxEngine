#ifndef PX_FOUNDATION_PS_WEB_INTRINSICS_H
#define PX_FOUNDATION_PS_WEB_INTRINSICS_H

#include "NvCloth/ps/Ps.h"

// this file is for internal intrinsics - that is, intrinsics that are used in
// cross platform code but do not appear in the API

#if !PX_WEB
	#error "This file should only be included by Web builds!!"
#endif

#include <math.h>

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{

/*
	* Implements a memory barrier
	*/
PX_FORCE_INLINE void PxMemoryBarrier()
{
	__sync_synchronize();
}

/*!
Returns the index of the highest set bit. Not valid for zero arg.
*/
PX_FORCE_INLINE uint32_t PxHighestSetBitUnsafe(uint32_t v)
{
	// http://graphics.stanford.edu/~seander/bithacks.html
	static const uint32_t MultiplyDeBruijnBitPosition[32] = 
	{
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	v |= v >> 1; // first round up to one less than a power of 2 
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}

/*!
Returns the index of the highest set bit. Undefined for zero arg.
*/
PX_FORCE_INLINE uint32_t PxLowestSetBitUnsafe(uint32_t v)
{
	// http://graphics.stanford.edu/~seander/bithacks.html
	static const uint32_t MultiplyDeBruijnBitPosition[32] = 
	{
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	int32_t w = v;
	return MultiplyDeBruijnBitPosition[(uint32_t)((w & -w) * 0x077CB531U) >> 27];
}

/*!
Returns the number of leading zeros in v. Returns 32 for v=0.
*/
PX_FORCE_INLINE uint32_t PxCountLeadingZeros(uint32_t v)
{
	int32_t result = 0;
	uint32_t testBit = (1<<31);
	while ((v & testBit) == 0 && testBit != 0)
	{
		result++;
		testBit >>= 1;
	}
	return result;
}

/*!
Prefetch aligned cache size around \c ptr+offset.
*/
PX_FORCE_INLINE void PxPrefetchLine(const void* ptr, uint32_t offset = 0)
{
	__builtin_prefetch((char* PX_RESTRICT)(const_cast<void*>(ptr)) + offset, 0, 3);
}

/*!
Prefetch \c count bytes starting at \c ptr.
*/
PX_FORCE_INLINE void PxPrefetch(const void* ptr, uint32_t count = 1)
{
	const char* cp = (char*)const_cast<void*>(ptr);
	uint64_t p = size_t(ptr);
	uint64_t startLine = p >> 6, endLine = (p + count - 1) >> 6;
	uint64_t lines = endLine - startLine + 1;
	do
	{
		PxPrefetchLine(cp);
		cp += 64;
	} while(--lines);
}

} // namespace ps
} // namespace cloth
} // namespace nv

#endif
