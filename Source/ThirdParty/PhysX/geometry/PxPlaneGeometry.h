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

#ifndef PX_PLANE_GEOMETRY_H
#define PX_PLANE_GEOMETRY_H
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
\brief Class describing a plane geometry.

The plane geometry specifies the half-space volume x<=0. As with other geometry types, 
when used in a PxShape the collision volume is obtained by transforming the halfspace 
by the shape local pose and the actor global pose.

To generate a PxPlane from a PxTransform, transform PxPlane(1,0,0,0).

To generate a PxTransform from a PxPlane, use PxTransformFromPlaneEquation.

@see PxShape.setGeometry() PxShape.getPlaneGeometry() PxTransformFromPlaneEquation 
*/
class PxPlaneGeometry : public PxGeometry 
{
public:
	/**
	\brief Constructor.
	*/
	PX_INLINE PxPlaneGeometry() : PxGeometry(PxGeometryType::ePLANE) {}

	/**
	\brief Copy constructor.

	\param[in] that		Other object
	*/
	PX_INLINE PxPlaneGeometry(const PxPlaneGeometry& that) : PxGeometry(that) {}

	/**
	\brief Assignment operator
	*/
	PX_INLINE void operator=(const PxPlaneGeometry& that)
	{
		mType = that.mType;
	}

	/**
	\brief Returns true if the geometry is valid.

	\return True if the current settings are valid
	*/
	PX_INLINE bool isValid() const;
};

PX_INLINE bool PxPlaneGeometry::isValid() const
{
	if(mType != PxGeometryType::ePLANE)
		return false;

	return true;
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
