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

#ifndef PX_CAPSULE_GEOMETRY_H
#define PX_CAPSULE_GEOMETRY_H
/** \addtogroup geomutils
@{
*/
#include "geometry/PxGeometry.h"
#include "foundation/PxFoundationConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Class representing the geometry of a capsule.

Capsules are shaped as the union of a cylinder of length 2 * halfHeight and with the 
given radius centered at the origin and extending along the x axis, and two hemispherical ends.
\note The scaling of the capsule is expected to be baked into these values, there is no additional scaling parameter.

The function PxTransformFromSegment is a helper for generating an appropriate transform for the capsule from the capsule's interior line segment.

@see PxTransformFromSegment
*/
class PxCapsuleGeometry : public PxGeometry      
{
public:
	/**
	\brief Constructor, initializes to a capsule with passed radius and half height.
	*/
	PX_INLINE PxCapsuleGeometry(PxReal radius_=0.0f, PxReal halfHeight_=0.0f) :	PxGeometry(PxGeometryType::eCAPSULE), radius(radius_), halfHeight(halfHeight_)	{}

	/**
	\brief Copy constructor.

	\param[in] that		Other object
	*/
	PX_INLINE PxCapsuleGeometry(const PxCapsuleGeometry& that) : PxGeometry(that), radius(that.radius), halfHeight(that.halfHeight)								{}

	/**
	\brief Assignment operator
	*/
	PX_INLINE void operator=(const PxCapsuleGeometry& that)
	{
		mType = that.mType;
		radius = that.radius;
		halfHeight = that.halfHeight;
	}

	/**
	\brief Returns true if the geometry is valid.

	\return True if the current settings are valid.

	\note A valid capsule has radius > 0, halfHeight >= 0.
	It is illegal to call PxRigidActor::createShape and PxPhysics::createShape with a capsule that has zero radius or height.

	@see PxRigidActor::createShape, PxPhysics::createShape
	*/
	PX_INLINE bool isValid() const;

public:
	/**
	\brief The radius of the capsule.
	*/
	PxReal radius;

	/**
	\brief half of the capsule's height, measured between the centers of the hemispherical ends.
	*/
	PxReal halfHeight;
};

PX_INLINE bool PxCapsuleGeometry::isValid() const
{
	if(mType != PxGeometryType::eCAPSULE)
		return false;
	if(!PxIsFinite(radius) || !PxIsFinite(halfHeight))
		return false;
	if(radius <= 0.0f || halfHeight < 0.0f)
		return false;

	return true;
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
