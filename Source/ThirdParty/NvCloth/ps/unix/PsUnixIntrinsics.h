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

#ifndef PSFOUNDATION_PSUNIXINTRINSICS_H
#define PSFOUNDATION_PSUNIXINTRINSICS_H

#include "NvCloth/ps/Ps.h"
#include "NvCloth/Callbacks.h"
#include <math.h>

#if PX_ANDROID || (PX_LINUX && !(PX_X64 || PX_X64)) // x86[_64] Linux uses inline assembly for debug break
#include <signal.h> // for Ns::debugBreak() { raise(SIGTRAP); }
#endif

#if 0
#include <libkern/OSAtomic.h>
#endif

// this file is for internal intrinsics - that is, intrinsics that are used in
// cross platform code but do not appear in the API

#if !(PX_LINUX || PX_ANDROID || PX_PS4 || PX_PS5 || PX_APPLE_FAMILY)
#error "This file should only be included by unix builds!!"
#endif

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{


PX_FORCE_INLINE void PxMemoryBarrier()
{
	__sync_synchronize();
}

/*!
Return the index of the highest set bit. Undefined for zero arg.
*/
PX_INLINE uint32_t PxHighestSetBitUnsafe(uint32_t v)
{

	return 31 - __builtin_clz(v);
}

/*!
Return the index of the highest set bit. Undefined for zero arg.
*/
PX_INLINE int32_t PxLowestSetBitUnsafe(uint32_t v)
{
	return __builtin_ctz(v);
}

/*!
Returns the index of the highest set bit. Returns 32 for v=0.
*/
PX_INLINE uint32_t PxCountLeadingZeros(uint32_t v)
{
	if(v)
		return __builtin_clz(v);
	else
		return 32;
}

/*!
Prefetch aligned 64B x86, 32b ARM around \c ptr+offset.
*/
PX_FORCE_INLINE void PxPrefetchLine(const void* ptr, uint32_t offset = 0)
{
	__builtin_prefetch(reinterpret_cast<const char* PX_RESTRICT>(ptr) + offset, 0, 3);
}

/*!
Prefetch \c count bytes starting at \c ptr.
*/
#if PX_ANDROID || PX_IOS
PX_FORCE_INLINE void prefetch(const void* ptr, uint32_t count = 1)
{
	const char* cp = static_cast<const char*>(ptr);
	size_t p = reinterpret_cast<size_t>(ptr);
	uint32_t startLine = uint32_t(p >> 5), endLine = uint32_t((p + count - 1) >> 5);
	uint32_t lines = endLine - startLine + 1;
	do
	{
		PxPrefetchLine(cp);
		cp += 32;
	} while(--lines);
}
#else
PX_FORCE_INLINE void prefetch(const void* ptr, uint32_t count = 1)
{
	const char* cp = reinterpret_cast<const char*>(ptr);
	uint64_t p = size_t(ptr);
	uint64_t startLine = p >> 6, endLine = (p + count - 1) >> 6;
	uint64_t lines = endLine - startLine + 1;
	do
	{
		PxPrefetchLine(cp);
		cp += 64;
	} while(--lines);
}
#endif

} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSUNIXINTRINSICS_H
