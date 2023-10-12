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

#ifndef PSFOUNDATION_PSATOMIC_H
#define PSFOUNDATION_PSATOMIC_H

#include "NvCloth/ps/Ps.h"
#include "NvCloth/Callbacks.h"

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
/* set *dest equal to val. Return the old value of *dest */
NV_CLOTH_IMPORT int32_t atomicExchange(volatile int32_t* dest, int32_t val);

/* if *dest == comp, replace with exch. Return original value of *dest */
NV_CLOTH_IMPORT int32_t atomicCompareExchange(volatile int32_t* dest, int32_t exch, int32_t comp);

/* if *dest == comp, replace with exch. Return original value of *dest */
NV_CLOTH_IMPORT void* atomicCompareExchangePointer(volatile void** dest, void* exch, void* comp);

/* increment the specified location. Return the incremented value */
NV_CLOTH_IMPORT int32_t atomicIncrement(volatile int32_t* val);

/* decrement the specified location. Return the decremented value */
NV_CLOTH_IMPORT int32_t atomicDecrement(volatile int32_t* val);

/* add delta to *val. Return the new value */
NV_CLOTH_IMPORT int32_t atomicAdd(volatile int32_t* val, int32_t delta);

/* compute the maximum of dest and val. Return the new value */
NV_CLOTH_IMPORT int32_t atomicMax(volatile int32_t* val, int32_t val2);

} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSATOMIC_H
