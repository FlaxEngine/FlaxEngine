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

#ifndef PX_BVH_DESC_H
#define PX_BVH_DESC_H
/** \addtogroup cooking
@{
*/

#include "common/PxCoreUtilityTypes.h"
#include "foundation/PxTransform.h"
#include "foundation/PxBounds3.h"
#include "geometry/PxBVHBuildStrategy.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Descriptor class for #PxBVH.

@see PxBVH
*/
class PxBVHDesc
{
public:
	PX_INLINE PxBVHDesc();

	/**
	\brief Pointer to first bounding box.
	*/
	PxBoundedData bounds;

	/**
	\brief Bounds enlargement

	Passed bounds are slightly enlarged before creating the BVH. This is done to avoid numerical issues when
	e.g. raycasts just graze the bounds. The performed operation is:

	extents = (bounds.maximum - bounds.minimum)/2
	enlagedBounds.minimum = passedBounds.minium - extents * enlargement
	enlagedBounds.maximum = passedBounds.maxium + extents * enlargement

	Users can pass pre-enlarged bounds to the BVH builder, in which case just set the enlargement value to zero.

	<b>Default value:</b> 0.01
	*/
	float	enlargement;

	/**
	\brief Max primitives per leaf limit.

	<b>Range:</b> [0, 16)<br>
	<b>Default value:</b> 4
	*/
	PxU32	numPrimsPerLeaf;

	/**
	\brief Desired build strategy for the BVH

	<b>Default value:</b> eDEFAULT
	*/
	PxBVHBuildStrategy::Enum	buildStrategy;

	/**
	\brief	Initialize the BVH descriptor
	*/
	PX_INLINE void setToDefault();

	/**
	\brief Returns true if the descriptor is valid.
	\return true if the current settings are valid.
	*/
	PX_INLINE bool isValid() const;

protected:	
};

PX_INLINE PxBVHDesc::PxBVHDesc() : enlargement(0.01f), numPrimsPerLeaf(4), buildStrategy(PxBVHBuildStrategy::eDEFAULT)
{
}

PX_INLINE void PxBVHDesc::setToDefault()
{
	*this = PxBVHDesc();
}

PX_INLINE bool PxBVHDesc::isValid() const
{
	// Check BVH desc data
	if(!bounds.data)
		return false;
	if(bounds.stride < sizeof(PxBounds3))	//should be at least one bounds' worth of data
		return false;

	if(bounds.count == 0)
		return false;

	if(enlargement<0.0f)
		return false;

	if(numPrimsPerLeaf>=16)
		return false;

	return true;
}

	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxBVHDesc PxBVHStructureDesc;

#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
