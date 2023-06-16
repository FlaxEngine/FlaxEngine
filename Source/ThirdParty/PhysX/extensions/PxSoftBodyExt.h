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

#ifndef PX_SOFT_BODY_EXT_H
#define PX_SOFT_BODY_EXT_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxTransform.h"
#include "PxSoftBody.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

struct PxCookingParams;
class PxSimpleTriangleMesh;
class PxInsertionCallback;
class PxSoftBodyMesh;

/**
\brief utility functions for use with PxSoftBody and subclasses
*/
class PxSoftBodyExt
{
public:
	/** 
	\brief Computes the SoftBody's vertex masses from the provided density and the volume of the tetrahedra

	The buffers affected by this operation can be obtained from the SoftBody using the methods getSimPositionInvMassCPU() and getSimVelocityInvMassCPU()

	The inverse mass is stored in the 4th component (the first three components are x, y, z coordinates) of the simulation mesh's position and velocity buffer. 
	Performance optimizations are the reason why the mass inverse is stored in two locations.

	\param[in] softBody The soft body which will get its mass updated
	\param[in] density The density to used to calculate the mass from the body's volume
	\param[in] maxInvMassRatio Maximum allowed ratio defined as max(vertexMasses) / min(vertexMasses) where vertexMasses is a list of float values with a mass for every vertex in the simulation mesh

	@see PxSoftBody PxSoftBody::getSimPositionInvMassCPU() PxSoftBody::getSimVelocityInvMassCPU()
	*/
	static void updateMass(PxSoftBody& softBody, const PxReal density, const PxReal maxInvMassRatio);

	/**
	\brief Computes the SoftBody's vertex masses such that the sum of all masses is equal to the provided mass

	The buffers affected by this operation can be obtained from the SoftBody using the methods getSimPositionInvMassCPU() and getSimVelocityInvMassCPU()

	The inverse mass is stored in the 4th component (the first three components are x, y, z coordinates) of the simulation mesh's position and velocity buffer.
	Performance optimizations are the reason why the mass inverse is stored in two locations.

	\param[in] softBody The soft body which will get its mass updated
	\param[in] mass The SoftBody's mass
	\param[in] maxInvMassRatio Maximum allowed ratio defined as max(vertexMasses) / min(vertexMasses) where vertexMasses is a list of float values with a mass for every vertex in the simulation mesh
	
	@see PxSoftBody PxSoftBody::getSimPositionInvMassCPU() PxSoftBody::getSimVelocityInvMassCPU()
	*/
	static void setMass(PxSoftBody& softBody, const PxReal mass, const PxReal maxInvMassRatio);

	/**
	\brief Transforms a SoftBody

	The buffers affected by this operation can be obtained from the SoftBody using the methods getSimPositionInvMassCPU() and getSimVelocityInvMassCPU()

	Applies a transformation to the simulation mesh's positions an velocities. Velocities only get rotated and scaled (translation is not applicable to direction vectors).
	It does not modify the body's mass. 
	If the method is called multiple times, the transformation will compound with the ones previously applied.

	\param[in] softBody The soft body which is transformed
	\param[in] transform The transform to apply
	\param[in] scale A scaling factor
	
	@see PxSoftBody PxSoftBody::getSimPositionInvMassCPU() PxSoftBody::getSimVelocityInvMassCPU()
	*/
	static void transform(PxSoftBody& softBody, const PxTransform& transform, const PxReal scale);

	/**
	\brief Updates the collision mesh's vertex positions to match the simulation mesh's transformation and scale.
	
	The buffer affected by this operation can be obtained from the SoftBody using the method getPositionInvMassCPU()

	\param[in] softBody The soft body which will get its collision mesh vertices updated
	
	@see PxSoftBody PxSoftBody::getPositionInvMassCPU()
	*/
	static void updateEmbeddedCollisionMesh(PxSoftBody& softBody);

	/**
	\brief Uploads prepared SoftBody data to the GPU. It ensures that the embedded collision mesh matches the simulation mesh's transformation and scale.

	\param[in] softBody The soft body which will perform the data upload
	\param[in] flags Specifies which buffers the data transfer should include
	\param[in] flush If set to true, the upload will get processed immediately, otherwise it will take place before the data is needed for calculations on the GPU
	
	@see PxSoftBody
	*/
	static void commit(PxSoftBody& softBody, PxSoftBodyDataFlags flags, bool flush = false);

	/**
	\brief Creates a full SoftBody mesh matching the shape given as input. Uses a voxel mesh for FEM simulation and a surface-matching mesh for collision detection. 

	\param[in] params Cooking params instance required for mesh processing
	\param[in] surfaceMesh Input triangle mesh that represents the surface of the SoftBody
	\param[in] numVoxelsAlongLongestAABBAxis The number of voxels along the longest bounding box axis
	\param[in] insertionCallback The insertion interface from PxPhysics
	\param[in] validate If set to true the input triangle mesh will get analyzed to find possible deficiencies
	\return SoftBody mesh if cooking was successful, NULL otherwise
	@see PxSoftBodyMesh
	*/
	static PxSoftBodyMesh* createSoftBodyMesh(const PxCookingParams& params, const PxSimpleTriangleMesh& surfaceMesh, PxU32 numVoxelsAlongLongestAABBAxis, PxInsertionCallback& insertionCallback, const bool validate = true);

	/**
	\brief Creates a full SoftBody mesh matching the shape given as input. Uses the same surface-matching mesh for collision detection and FEM simulation.

	\param[in] params Cooking params instance required for mesh processing
	\param[in] surfaceMesh Input triangle mesh that represents the surface of the SoftBody
	\param[in] insertionCallback The insertion interface from PxPhysics
	\param[in] maxWeightRatioInTet Upper limit for the ratio of node weights that are adjacent to the same tetrahedron. The closer to one (while remaining larger than one), the more stable the simulation. 
	\param[in] validate If set to true the input triangle mesh will get analyzed to find possible deficiencies
	\return SoftBody mesh if cooking was successful, NULL otherwise
	@see PxSoftBodyMesh
	*/
	static PxSoftBodyMesh* createSoftBodyMeshNoVoxels(const PxCookingParams& params, const PxSimpleTriangleMesh& surfaceMesh, PxInsertionCallback& insertionCallback, PxReal maxWeightRatioInTet = 1.5f, const bool validate = true);


	/**
	\brief Creates a SoftBody instance from a SoftBody mesh

	\param[in] softBodyMesh The SoftBody mesh
	\param[in] transform The transform that defines initial position and orientation of the SoftBody
	\param[in] material The material
	\param[in] cudaContextManager A cuda context manager
	\param[in] density The density used to compute the mass properties
	\param[in] solverIterationCount The number of iterations the solver should apply during simulation
	\param[in] femParams Additional parameters to specify e. g. damping
	\param[in] scale The scaling of the SoftBody
	\return SoftBody instance
	@see PxSoftBodyMesh, PxSoftBody
	*/
	static PxSoftBody* createSoftBodyFromMesh(PxSoftBodyMesh* softBodyMesh, const PxTransform& transform, 
		const PxFEMSoftBodyMaterial& material, PxCudaContextManager& cudaContextManager, PxReal density = 100.0f, PxU32 solverIterationCount = 30,
		const PxFEMParameters& femParams = PxFEMParameters(), PxReal scale = 1.0f);


	/**
	\brief Creates a SoftBody instance with a box shape

	\param[in] transform The transform that defines initial position and orientation of the SoftBody
	\param[in] boxDimensions The dimensions (side lengths) of the box shape
	\param[in] material The material
	\param[in] cudaContextManager A cuda context manager
	\param[in] maxEdgeLength The maximal length of a triangle edge. Subdivision will get applied until the edge length criteria is matched. -1 means no subdivision is applied.
	\param[in] density The density used to compute the mass properties
	\param[in] solverIterationCount The number of iterations the solver should apply during simulation
	\param[in] femParams Additional parameters to specify e. g. damping
	\param[in] numVoxelsAlongLongestAABBAxis The number of voxels to use for the simulation mesh along the longest bounding box dimension
	\param[in] scale The scaling of the SoftBody
	\return SoftBody instance
	@see PxSoftBodyMesh, PxSoftBody
	*/
	static PxSoftBody* createSoftBodyBox(const PxTransform& transform, const PxVec3& boxDimensions, const PxFEMSoftBodyMaterial& material,
		PxCudaContextManager& cudaContextManager, PxReal maxEdgeLength = -1.0f, PxReal density = 100.0f, PxU32 solverIterationCount = 30, 
		const PxFEMParameters& femParams = PxFEMParameters(), PxU32 numVoxelsAlongLongestAABBAxis = 10, PxReal scale = 1.0f);
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
