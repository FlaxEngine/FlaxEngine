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

#ifndef PX_TETRAHEDRON_GEOMETRY_H
#define PX_TETRAHEDRON_GEOMETRY_H
/** \addtogroup geomutils
@{
*/
#include "geometry/PxGeometry.h"
#include "geometry/PxMeshScale.h"
#include "common/PxCoreUtilityTypes.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	class PxTetrahedronMesh;

	/**
	\brief Tetrahedron mesh geometry class.

	This class wraps a tetrahedron mesh such that it can be used in contexts where a PxGeometry type is needed.
	*/
	class PxTetrahedronMeshGeometry : public PxGeometry
	{
	public:
		/**
		\brief Constructor. By default creates an empty object with a NULL mesh and identity scale.
		*/
		PX_INLINE PxTetrahedronMeshGeometry(PxTetrahedronMesh* mesh = NULL) :
			PxGeometry(PxGeometryType::eTETRAHEDRONMESH),
			tetrahedronMesh(mesh)
		{}

		/**
		\brief Copy constructor.

		\param[in] that		Other object
		*/
		PX_INLINE PxTetrahedronMeshGeometry(const PxTetrahedronMeshGeometry& that) :
			PxGeometry(that),
			tetrahedronMesh(that.tetrahedronMesh)
		{}

		/**
		\brief Assignment operator
		*/
		PX_INLINE void operator=(const PxTetrahedronMeshGeometry& that)
		{
			mType = that.mType;
			tetrahedronMesh = that.tetrahedronMesh;
		}

		/**
		\brief Returns true if the geometry is valid.

		\return  True if the current settings are valid for shape creation.

		\note A valid tetrahedron mesh has a positive scale value in each direction (scale.scale.x > 0, scale.scale.y > 0, scale.scale.z > 0).
		It is illegal to call PxRigidActor::createShape and PxPhysics::createShape with a tetrahedron mesh that has zero extents in any direction.

		@see PxRigidActor::createShape, PxPhysics::createShape
		*/
		PX_INLINE bool isValid() const;

	public:
		PxTetrahedronMesh*		tetrahedronMesh;		//!< A reference to the mesh object.
	};

	PX_INLINE bool PxTetrahedronMeshGeometry::isValid() const
	{
		if(mType != PxGeometryType::eTETRAHEDRONMESH)
			return false;

		if(!tetrahedronMesh)
			return false;

		return true;
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
