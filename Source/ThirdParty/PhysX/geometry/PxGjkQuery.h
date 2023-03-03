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

#ifndef PX_GJK_QUERY_H
#define PX_GJK_QUERY_H

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"
#include "foundation/PxQuat.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Collection of GJK query functions (sweeps, raycasts, overlaps, ...).
*/
class PxGjkQuery
{
public:

	/**
	\brief Abstract interface for a user defined shape GJK mapping support.
	A user defined shape consists of a core shape and a margin. If the distance
	between two shapes' cores is equal to the sum of their margins, these shapes are
	considered touching.
	*/
	struct Support
	{
		/* Virtual destructor */
		virtual ~Support() {}
		/**
		\brief Return the user defined shape margin. Margin should be greater than or equal to 0

		\return		Margin.
		*/
		virtual PxReal getMargin() const = 0;
		/**
		\brief Return the farthest point on the user defined shape's core in given direction.

		\param[in] dir		Direction

		\return				Farthest point in given direction.
		*/
		virtual PxVec3 supportLocal(const PxVec3& dir) const = 0;
	};

	/**
	\brief Computes proximity information for two shapes using GJK-EPA algorithm

	\param[in] a				Shape A support mapping
	\param[in] b				Shape B support mapping
	\param[in] poseA			Shape A transformation
	\param[in] poseB			Shape B transformation
	\param[in] contactDistance	The distance at which proximity info begins to be computed between the shapes
	\param[in] toleranceLength	The toleranceLength. Used for scaling distance-based thresholds internally to produce appropriate results given simulations in different units
	\param[out] pointA			The closest/deepest point on shape A surface
	\param[out] pointB			The closest/deepest point on shape B surface
	\param[out] separatingAxis	Translating shape B along 'separatingAxis' by 'separation' makes the shapes touching
	\param[out] separation		Translating shape B along 'separatingAxis' by 'separation' makes the shapes touching

	\return						False if the distance greater than contactDistance.
	*/
	PX_PHYSX_COMMON_API static bool proximityInfo(const Support& a, const Support& b, const PxTransform& poseA, const PxTransform& poseB,
		PxReal contactDistance, PxReal toleranceLength, PxVec3& pointA, PxVec3& pointB, PxVec3& separatingAxis, PxReal& separation);

	/**
	\brief Raycast test against the given shape.

	\param[in] shape		Shape support mapping
	\param[in] pose			Shape transformation
	\param[in] rayStart		The start point of the ray to test the shape against
	\param[in] unitDir		Normalized direction of the ray to test the shape against
	\param[in] maxDist		Maximum ray length, has to be in the [0, inf) range
	\param[out] t			Hit distance
	\param[out] n			Hit normal
	\param[out] p			Hit point

	\return					True if there is a hit.
	*/
	PX_PHYSX_COMMON_API static bool raycast(const Support& shape, const PxTransform& pose, const PxVec3& rayStart,
		const PxVec3& unitDir, PxReal maxDist, PxReal& t, PxVec3& n, PxVec3& p);

	/**
	\brief Overlap test for two shapes.

	\param[in] a				Shape A support mapping
	\param[in] b				Shape B support mapping
	\param[in] poseA			Shape A transformation
	\param[in] poseB			Shape B transformation

	\return					True if the shapes overlap.
	*/
	PX_PHYSX_COMMON_API static bool overlap(const Support& a, const Support& b, const PxTransform& poseA, const PxTransform& poseB);

	/**
	\brief Sweep the shape B in space and test for collision with the shape A.

	\param[in] a				Shape A support mapping
	\param[in] b				Shape B support mapping
	\param[in] poseA			Shape A transformation
	\param[in] poseB			Shape B transformation
	\param[in] unitDir		Normalized direction of the ray to test the shape against
	\param[in] maxDist		Maximum ray length, has to be in the [0, inf) range
	\param[out] t			Hit distance
	\param[out] n			Hit normal
	\param[out] p			Hit point

	\return					True if there is a hit.
	*/
	PX_PHYSX_COMMON_API static bool sweep(const Support& a, const Support& b, const PxTransform& poseA, const PxTransform& poseB,
		const PxVec3& unitDir, PxReal maxDist, PxReal& t, PxVec3& n, PxVec3& p);
};

#if !PX_DOXYGEN
}
#endif

#endif
