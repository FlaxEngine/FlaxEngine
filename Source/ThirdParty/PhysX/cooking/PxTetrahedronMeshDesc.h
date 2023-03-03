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

#ifndef PX_TETRAHEDRON_MESH_DESC_H
#define PX_TETRAHEDRON_MESH_DESC_H
/** \addtogroup cooking
@{
*/

#include "PxPhysXConfig.h"
#include "foundation/PxVec3.h"
#include "foundation/PxFlags.h"
#include "common/PxCoreUtilityTypes.h"
#include "geometry/PxSimpleTriangleMesh.h"
#include "foundation/PxArray.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	/**
	\brief Descriptor class for #PxTetrahedronMesh (contains only pure geometric data).

	@see PxTetrahedronMesh PxShape
	*/
	class PxTetrahedronMeshDesc
	{
	public:

		/**
		\brief Defines the tetrahedron structure of a mesh. 
		*/
		enum PxMeshFormat
		{
			eTET_MESH,	//!< Normal tetmesh with arbitrary tetrahedra
			eHEX_MESH 	//!< 6 tetrahedra in a row will form a hexahedron
		};


		/**
		Optional pointer to first material index, or NULL. There are PxTetrahedronMesh::numTriangles indices in total.
		Caller may add materialIndexStride bytes to the pointer to access the next triangle.

		When a tetrahedron mesh collides with another object, a material is required at the collision point.
		If materialIndices is NULL, then the material of the PxShape instance is used.
		Otherwise, if the point of contact is on a tetrahedron with index i, then the material index is determined as:
		PxFEMMaterialTableIndex	index = *(PxFEMMaterialTableIndex *)(((PxU8*)materialIndices) + materialIndexStride * i);

		If the contact point falls on a vertex or an edge, a tetrahedron adjacent to the vertex or edge is selected, and its index
		used to look up a material. The selection is arbitrary but consistent over time.

		<b>Default:</b> NULL

		@see materialIndexStride
		*/
		PxTypedStridedData<PxFEMMaterialTableIndex> materialIndices;

		/**
		\brief Pointer to first vertex point.
		*/
		PxBoundedData points;

		/**
		\brief Pointer to first tetrahedron.

		Caller may add tetrhedronStrideBytes bytes to the pointer to access the next tetrahedron.

		These are quadruplets of 0 based indices:
		vert0 vert1 vert2 vert3
		vert0 vert1 vert2 vert3
		vert0 vert1 vert2 vert3
		...

		where vertex is either a 32 or 16 bit unsigned integer. There are numTetrahedrons*4 indices.

		This is declared as a void pointer because it is actually either an PxU16 or a PxU32 pointer.
		*/
		PxBoundedData tetrahedrons;


		/**
		\brief Flags bits, combined from values of the enum ::PxMeshFlag
		*/
		PxMeshFlags flags;

		/**
		\brief Used for simulation meshes only. Defines if this tet mesh should be simulated as a tet mesh,
		or if a set of tetrahedra should be used to represent another shape, e.g. a hexahedral mesh constructed 
		from 6 elements.
		*/
		PxU16 tetsPerElement;

		/**
		\brief Constructor to build an empty tetmesh description
		*/
		PxTetrahedronMeshDesc()
		{
			points.count = 0;
			points.stride = 0;
			points.data = NULL;

			tetrahedrons.count = 0;
			tetrahedrons.stride = 0;
			tetrahedrons.data = NULL;

			tetsPerElement = 1;
		}

		/**
		\brief Constructor to build a tetmeshdescription that links to the vertices and indices provided
		*/
		PxTetrahedronMeshDesc(physx::PxArray<physx::PxVec3>& meshVertices, physx::PxArray<physx::PxU32>& meshTetIndices, const PxTetrahedronMeshDesc::PxMeshFormat meshFormat = eTET_MESH)
		{
			points.count = meshVertices.size();
			points.stride = sizeof(float) * 3;
			points.data = meshVertices.begin();

			tetrahedrons.count = meshTetIndices.size() / 4;
			tetrahedrons.stride = sizeof(int) * 4;
			tetrahedrons.data = meshTetIndices.begin();

			if (meshFormat == eTET_MESH)
				tetsPerElement = 1;
			else
				tetsPerElement = 6;
		}

		PX_INLINE bool isValid() const
		{
			// Check geometry of the collision mesh
			if (points.count < 4) 	//at least 1 tetrahedron's worth of points
				return false;
			if ((!tetrahedrons.data) && (points.count % 4))		// Non-indexed mesh => we must ensure the geometry defines an implicit number of tetrahedrons // i.e. numVertices can't be divided by 4
				return false;
			if (points.count > 0xffff && flags & PxMeshFlag::e16_BIT_INDICES)
				return false;
			if (!points.data)
				return false;
			if (points.stride < sizeof(PxVec3))	//should be at least one point's worth of data
				return false;		

			//add more validity checks here
			if (materialIndices.data && materialIndices.stride < sizeof(PxFEMMaterialTableIndex))
				return false;

			// The tetrahedrons pointer is not mandatory
			if (tetrahedrons.data)
			{
				// Indexed collision mesh
				PxU32 limit = (flags & PxMeshFlag::e16_BIT_INDICES) ? sizeof(PxU16) * 4 : sizeof(PxU32) * 4;
				if (tetrahedrons.stride < limit)
					return false;
			}

			//The model can only be either a tetmesh (1 tet per element), or have 5 or 6 tets per hex element, otherwise invalid.
			if (tetsPerElement != 1 && tetsPerElement != 6)
				return false;

			return true;
		}
	};

	///**
	//\brief Descriptor class for #PxSoftBodyMesh (contains only additional data used for softbody simulation).

	//@see PxSoftBodyMesh PxShape
	//*/
	class PxSoftBodySimulationDataDesc
	{
	public:
		/**
		\brief Pointer to first index of tetrahedron that contains the vertex at the same location in the vertex buffer.		
		if left unassigned it will be computed automatically. If a point is inside multiple tetrahedra (ambiguous case), the frist one found will be taken.
		*/
		PxBoundedData vertexToTet;

		/**
		\brief Constructor to build an empty simulation description
		*/
		PxSoftBodySimulationDataDesc()
		{
			vertexToTet.count = 0;
			vertexToTet.stride = 0;
			vertexToTet.data = NULL;
		}

		/**
		\brief Constructor to build a simulation description with a defined vertex to tetrahedron mapping
		*/
		PxSoftBodySimulationDataDesc(physx::PxArray<physx::PxI32>& vertToTet)
		{
			vertexToTet.count = vertToTet.size();
			vertexToTet.stride = sizeof(PxI32);
			vertexToTet.data = vertToTet.begin();
		}

		PX_INLINE bool isValid() const
		{			
			return true;
		}
	};
	
#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
