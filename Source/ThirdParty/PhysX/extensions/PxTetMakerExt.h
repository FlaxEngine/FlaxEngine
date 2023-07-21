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

#ifndef PX_TETMAKER_EXT_H
#define PX_TETMAKER_EXT_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxVec3.h"
#include "common/PxCoreUtilityTypes.h"
#include "foundation/PxArray.h"
#include "PxTriangleMeshAnalysisResult.h"
#include "PxTetrahedronMeshAnalysisResult.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxTriangleMesh;
class PxTetrahedronMeshDesc;
class PxSoftBodySimulationDataDesc;
struct PxTetMakerData;
class PxSimpleTriangleMesh;

/**
\brief Provides functionality to create a tetrahedral mesh from a triangle mesh.
*/
class PxTetMaker
{
public:

	/**
	\brief Create conforming tetrahedron mesh using TetMaker

	\param[in] triangleMesh The description of the triangle mesh including vertices and indices
	\param[out] outVertices The vertices to store the conforming tetrahedral mesh
	\param[out] outTetIndices The indices to store the conforming tetrahedral mesh
	\param[in] validate If set to true the input triangle mesh will get analyzed to find possible deficiencies
	\param[in] volumeThreshold Tetrahedra with a volume smaller than the specified threshold will be removed from the mesh
	\return True if success
	*/
	static bool createConformingTetrahedronMesh(const PxSimpleTriangleMesh& triangleMesh, physx::PxArray<physx::PxVec3>& outVertices, physx::PxArray<physx::PxU32>& outTetIndices, 
		const bool validate = true, PxReal volumeThreshold = 0.0f);

	/**
	\brief Create voxel-based tetrahedron mesh using TetMaker

	\param[in] tetMesh The description of the tetrahedral mesh including vertices and indices
	\param[in] numVoxelsAlongLongestBoundingBoxAxis The number of voxels along the longest bounding box axis
	\param[out] outVertices The vertices to store the voxel-based tetrahedral mesh
	\param[out] outTetIndices The indices to store the voxel-based tetrahedral mesh
	\param[out] inputPointToOutputTetIndex Buffer with the size of nbTetVerts that contains the tetrahedron index containing the input point with the same index
	\param[in] anchorNodeIndices Some input vertices may not be referenced by any tetrahedron. They can be mapped to another input vertex that is used by a tetrahedron to support embedding of additional points.
	\return True if success
	*/
	static bool createVoxelTetrahedronMesh(const PxTetrahedronMeshDesc& tetMesh, const PxU32 numVoxelsAlongLongestBoundingBoxAxis,
		physx::PxArray<physx::PxVec3>& outVertices, physx::PxArray<physx::PxU32>& outTetIndices, PxI32* inputPointToOutputTetIndex = NULL, const PxU32* anchorNodeIndices = NULL);

	/**
	\brief Create voxel-based tetrahedron mesh using TetMaker

	\param[in] tetMesh The description of the tetrahedral mesh including vertices and indices
	\param[in] voxelEdgeLength The edge length of a voxel.Can be adjusted slightly such that a multiple of it matches the input points' bounding box size
	\param[out] outVertices The vertices to store the voxel-based tetrahedral mesh
	\param[out] outTetIndices The indices to store the voxel-based tetrahedral mesh	
	\param[out] inputPointToOutputTetIndex Buffer with the size of nbTetVerts that contains the tetrahedron index containing the input point with the same index
	\param[in] anchorNodeIndices Some input vertices may not be referenced by any tetrahedron. They can be mapped to another input vertex that is used by a tetrahedron to support embedding of additional points.
	\return True if success
	*/
	static bool createVoxelTetrahedronMeshFromEdgeLength(const PxTetrahedronMeshDesc& tetMesh, const PxReal voxelEdgeLength,
		 physx::PxArray<physx::PxVec3>& outVertices, physx::PxArray<physx::PxU32>& outTetIndices, PxI32* inputPointToOutputTetIndex = NULL, const PxU32* anchorNodeIndices = NULL);

	/**
	\brief Analyzes the triangle mesh to get a report about deficiencies. Some deficiencies can be handled by the tetmesher, others cannot.

	\param[in] triangleMesh The description of the triangle mesh including vertices and indices
	\param[in] minVolumeThreshold Minimum volume the mesh must have such that no volume warning is generated
	\param[in] minTriangleAngleRadians Minimum angle allowed for triangles such that no angle warning is generated
	\return Flags that describe the triangle mesh's deficiencies
	*/
	static PxTriangleMeshAnalysisResults validateTriangleMesh(const PxSimpleTriangleMesh& triangleMesh, const PxReal minVolumeThreshold = 1e-6f, const PxReal minTriangleAngleRadians = 10.0f*3.1415926535898f / 180.0f);

	/**
	\brief Analyzes the tetrahedron mesh to get a report about deficiencies. Some deficiencies can be handled by the softbody cooker, others cannot.

	\param[in] points The mesh's points
	\param[in] tetrahedra The mesh's tetrahedra (index buffer)
	\param[in] minTetVolumeThreshold Minimum volume every tetrahedron in the mesh must have such that no volume warning is generated
	\return Flags that describe the tetrahedron mesh's deficiencies
	*/
	static PxTetrahedronMeshAnalysisResults validateTetrahedronMesh(const PxBoundedData& points, const PxBoundedData& tetrahedra, const PxReal minTetVolumeThreshold = 1e-8f);

	/**
	\brief Simplifies (decimates) a triangle mesh using quadric simplification.

	\param[in] inputVertices The vertices of the input triangle mesh
	\param[in] inputIndices The indices of the input triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[in] targetTriangleCount Desired number of triangles in the output mesh
	\param[in] maximalEdgeLength Edges below this length will not be collapsed. A value of zero means there is no limit.
	\param[out] outputVertices The vertices of the output (decimated) triangle mesh
	\param[out] outputIndices The indices of the output (decimated) triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[out] vertexMap Optional parameter which returns the mapping from input to output vertices. Note that multiple input vertices are typically collapsed into the same output vertex.
	\param[in] edgeLengthCostWeight Factor to scale influence of edge length when prioritizing edge collapses. Has no effect if set to zero.
	\param[in] flatnessDetectionThreshold Threshold used to detect edges in flat regions and to improve the placement of the collapsed point. If set to a large value it will have no effect.
	*/
	static void simplifyTriangleMesh(const PxArray<PxVec3>& inputVertices, const PxArray<PxU32>&inputIndices, int targetTriangleCount, PxF32 maximalEdgeLength,
		PxArray<PxVec3>& outputVertices, PxArray<PxU32>& outputIndices,
		PxArray<PxU32> *vertexMap = NULL, PxReal edgeLengthCostWeight = 0.1f, PxReal flatnessDetectionThreshold = 0.01f);

	/**
	\brief Creates a new mesh from a given mesh. The input mesh is first voxelized. The new surface is created from the voxel surface and subsequent projection to the original mesh.

	\param[in] inputVertices The vertices of the input triangle mesh
	\param[in] inputIndices The indices of the input triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[in] gridResolution Size of the voxel grid (number of voxels along the longest dimension)
	\param[out] outputVertices The vertices of the output (decimated) triangle mesh
	\param[out] outputIndices The indices of the output (decimated) triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[out] vertexMap Optional parameter which returns a mapping from input to output vertices. Since the meshes are independent, the mapping returns an output vertex that is topologically close to the input vertex.
	*/
	static void remeshTriangleMesh(const PxArray<PxVec3>& inputVertices, const PxArray<PxU32>&inputIndices, int gridResolution,
		PxArray<PxVec3>& outputVertices, PxArray<PxU32>& outputIndices,
		PxArray<PxU32> *vertexMap = NULL);

	/**
	\brief Creates a tetrahedral mesh using an octree. 

	\param[in] inputVertices The vertices of the input triangle mesh
	\param[in] inputIndices The indices of the input triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[in] useTreeNodes Using the nodes of the octree as tetrahedral vertices
	\param[out] outputVertices The vertices of the output tetrahedral mesh
	\param[out] outputIndices The indices of the output tetrahedral mesh of the form  (id0, id1, id2, id3),  (id0, id1, id2, id3), ..
	\param[in] volumeThreshold Tetrahedra with a volume smaller than the specified threshold will be removed from the mesh
	*/
	static void createTreeBasedTetrahedralMesh(const PxArray<PxVec3>& inputVertices, const PxArray<PxU32>&inputIndices,
		bool useTreeNodes, PxArray<PxVec3>& outputVertices, PxArray<PxU32>& outputIndices, PxReal volumeThreshold = 0.0f);

	/**
	\brief Creates a tetrahedral mesh by relaxing a voxel mesh around the input mesh

	\param[in] inputVertices The vertices of the input triangle mesh
	\param[in] inputIndices The indices of the input triangle mesh of the form  (id0, id1, id2),  (id0, id1, id2), ..
	\param[out] outputVertices The vertices of the output tetrahedral mesh
	\param[out] outputIndices The indices of the output tetrahedral mesh of the form  (id0, id1, id2, id3),  (id0, id1, id2, id3), ..
	\param[in] resolution The grid spacing is computed as the diagonal of the bounding box of the input mesh divided by the resolution.
	\param[in] numRelaxationIterations Number of iterations to pull the tetrahedral mesh towards the input mesh
	\param[in] relMinTetVolume Constrains the volumes of the tetrahedra to stay abobe relMinTetvolume times the tetrahedron's rest volume.
	*/
	static void createRelaxedVoxelTetrahedralMesh(const PxArray<PxVec3>& inputVertices, const PxArray<PxU32>&inputIndices,
		PxArray<PxVec3>& outputVertices, PxArray<PxU32>& outputIndices, 
		PxI32 resolution, PxI32 numRelaxationIterations = 5, PxF32 relMinTetVolume = 0.05f);

};

#if !PX_DOXYGEN
}
#endif

/** @} */
#endif

