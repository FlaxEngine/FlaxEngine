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

#ifndef PX_TETRAHEDRON_MESH_EXT_H
#define PX_TETRAHEDRON_MESH_EXT_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxVec3.h"
#include "foundation/PxArray.h"


#if !PX_DOXYGEN
namespace physx
{
#endif
	class PxTetrahedronMesh;

	/**
	\brief utility functions for use with PxTetrahedronMesh and subclasses
	*/
	class PxTetrahedronMeshExt
	{
	public:
		/** Returns the index of the tetrahedron that contains a point

		\param[in] mesh The tetmesh
		\param[in] point The point to find the enclosing tetrahedron for
		\param[in] bary The barycentric coordinates of the point inside the enclosing tetrahedron
		\param[in] tolerance Tolerance value used classify points as inside if they lie exactly a tetrahedron's surface
		\return The index of the tetrahedon containing the point, -1 if not tetrahedron contains the opoint
		*/
		static PxI32 findTetrahedronContainingPoint(const PxTetrahedronMesh* mesh, const PxVec3& point, PxVec4& bary, PxReal tolerance = 1e-6f);

		/** Returns the index of the tetrahedron closest to a point

		\param[in] mesh The tetmesh
		\param[in] point The point to find the closest tetrahedron for
		\param[in] bary The barycentric coordinates of the point in the tetrahedron
		\return The index of the tetrahedon closest to the point
		*/
		static PxI32 findTetrahedronClosestToPoint(const PxTetrahedronMesh* mesh, const PxVec3& point, PxVec4& bary);

		/** Extracts the surface triangles of a tetmesh

		The extracted triangle's vertex indices point to the vertex buffer of the tetmesh.

		\param[in] tetrahedra The tetrahedra indices
		\param[in] numTetrahedra The number of tetrahedra
		\param[in] sixteenBitIndices If set to true, the tetrahedra indices are read as 16bit integers, otherwise 32bit integers are used
		\param[in] surfaceTriangles The resulting surface triangles
		\param[in] surfaceTriangleToTet Optional array to get the index of a tetrahedron that is adjacent to the surface triangle with the corresponding index
		\param[in] flipTriangleOrientation Reverses the orientation of the ouput triangles
		*/
		static void extractTetMeshSurface(const void* tetrahedra, PxU32 numTetrahedra, bool sixteenBitIndices, PxArray<PxU32>& surfaceTriangles, PxArray<PxU32>* surfaceTriangleToTet = NULL, bool flipTriangleOrientation = false);

		/** Extracts the surface triangles of a tetmesh

		The extracted triangle's vertex indices point to the vertex buffer of the tetmesh.

		\param[in] mesh The mesh from which the surface shall be computed
		\param[in] surfaceTriangles The resulting surface triangles
		\param[in] surfaceTriangleToTet Optional array to get the index of a tetrahedron that is adjacent to the surface triangle with the corresponding index
		\param[in] flipTriangleOrientation Reverses the orientation of the ouput triangles
		*/
		static void extractTetMeshSurface(const PxTetrahedronMesh* mesh, PxArray<PxU32>& surfaceTriangles, PxArray<PxU32>* surfaceTriangleToTet = NULL, bool flipTriangleOrientation = false);
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
