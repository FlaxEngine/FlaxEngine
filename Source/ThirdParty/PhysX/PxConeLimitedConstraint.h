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

#ifndef PX_CONE_LIMITED_CONSTRAINT_H
#define PX_CONE_LIMITED_CONSTRAINT_H

/** \addtogroup physics
@{
*/

#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief A constraint descriptor for limiting movement to a conical region.
*/
struct PxConeLimitedConstraint
{
	PxConeLimitedConstraint()
	{
		mAxis = PxVec3(0.f, 0.f, 0.f);
		mAngle = 0.f;
		mLowLimit = 0.f;
		mHighLimit = 0.f;
	}

	PxVec3 mAxis;  //!< Axis of the cone in the actor space of the rigid body
	PxReal mAngle; //!< Opening angle in radians
	PxReal mLowLimit; //!< Minimum distance
	PxReal mHighLimit; //!< Maximum distance
};

/**
\brief Compressed form of cone limit parameters
@see PxConeLimitedConstraint
*/
PX_ALIGN_PREFIX(16)
struct PxConeLimitParams
{
	PxVec4 lowHighLimits; // [lowLimit, highLimit, unused, unused]
	PxVec4 axisAngle; // [axis.x, axis.y, axis.z, angle]
}PX_ALIGN_SUFFIX(16);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
