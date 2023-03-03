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

#ifndef PX_SAMPLING_EXT_H
#define PX_SAMPLING_EXT_H
/** \addtogroup extensions
  @{
*/

#include "foundation/PxArray.h"
#include "geometry/PxGeometry.h"
#include "foundation/PxUserAllocated.h"
#include "geometry/PxSimpleTriangleMesh.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief utility functions to sample vertices on or inside a triangle mesh or other geometries
*/
class PxSamplingExt
{
public:
	/** Computes samples on a triangle mesh's surface that are not closer to each other than a given distance. Optionally the mesh's interior can be filled with samples as well.

	\param[in] mesh The triangle mesh	
	\param[in] r The closest distance two surface samples are allowed to have
	\param[out] result Equally distributed samples on and if specified inside the triangle mesh	
	\param[in] rVolume The average distance of samples inside the mesh. If set to zero, samples will only be placed on the mesh's surface	
	\param[out] triangleIds Optional output containing the index of the triangle for all samples on the mesh's surface. The array will contain less entries than output vertices if volume samples are active since volume samples are not on the surface.
	\param[out] barycentricCoordinates Optional output containing the barycentric coordinates for all samples on the mesh's surface. The array will contain less entries than output vertices if volume samples are active since volume samples are not on the surface.
	\param[in] axisAlignedBox A box that limits the space where samples can get created
	\param[in] boxOrientation The orientation of the box that limits the space where samples can get created	
	\param[in] maxNumSamples If larger than zero, the sampler will stop when the sample count reaches maxNumSamples
	\param[in] numSampleAttemptsAroundPoint Number of repetitions the underlying algorithm performs to find a new valid sample that matches all criteria like minimal distance to existing samples etc.
	\return Returns true if the sampling was successful and false if there was a problem. Usually an internal overflow is the problem for very big meshes or very small sampling radii.
	*/
	static bool poissonSample(const PxSimpleTriangleMesh& mesh, PxReal r, PxArray<PxVec3>& result, PxReal rVolume = 0.0f, PxArray<PxI32>* triangleIds = NULL, PxArray<PxVec3>* barycentricCoordinates = NULL,
		const PxBounds3* axisAlignedBox = NULL, const PxQuat* boxOrientation = NULL, PxU32 maxNumSamples = 0, PxU32 numSampleAttemptsAroundPoint = 30);
	
	/** Computes samples on a geometry's surface that are not closer to each other than a given distance.

	\param[in] geometry The geometry that defines the surface on which the samples get created
	\param[in] transform The geometry's global pose
	\param[in] worldBounds The geometry's bounding box
	\param[in] r The closest distance two surface samples are allowed to have
	\param[out] result Equally distributed samples on and if specified inside the triangle mesh
	\param[in] rVolume The average distance of samples inside the mesh. If set to zero, samples will only be placed on the mesh's surface
	\param[in] axisAlignedBox A box that limits the space where samples can get created
	\param[in] boxOrientation The orientation of the box that limits the space where samples can get created	
	\param[in] maxNumSamples If larger than zero, the sampler will stop when the sample count reaches maxNumSamples
	\param[in] numSampleAttemptsAroundPoint Number of repetitions the underlying algorithm performs to find a new valid sample that matches all criteria like minimal distance to existing samples etc.
	\return Returns true if the sampling was successful and false if there was a problem. Usually an internal overflow is the problem for very big meshes or very small sampling radii.
	*/
	static bool poissonSample(const PxGeometry& geometry, const PxTransform& transform, const PxBounds3& worldBounds, PxReal r, PxArray<PxVec3>& result, PxReal rVolume = 0.0f,
		const PxBounds3* axisAlignedBox = NULL, const PxQuat* boxOrientation = NULL, PxU32 maxNumSamples = 0, PxU32 numSampleAttemptsAroundPoint = 30);
};

/**
\brief Sampler to generate Poisson Samples locally on a triangle mesh or a shape. For every local addition of new samples, an individual sampling density can be used.
*/
class PxPoissonSampler : public PxUserAllocated
{
public:
	/** Sets the sampling radius
	\param[in] samplingRadius The closest distance two surface samples are allowed to have. Changing the sampling radius is a bit an expensive operation.
	\return Returns true if the sampling was successful and false if there was a problem. Usually an internal overflow is the problem for very big meshes or very small sampling radii.
	*/
	virtual bool setSamplingRadius(PxReal samplingRadius) = 0;
	
	/** Adds samples
	\param[in] samples The samples to add. Adding samples is a bit an expensive operation.
	*/
	virtual void addSamples(const PxArray<PxVec3>& samples) = 0;

	/** Adds samples
	\param[in] samples The samples to remove. Removing samples is a bit an expensive operation.
	\return Returns the number of removed samples. If some samples were not found, then the number of actually removed samples will be smaller than the number of samples requested to remove
	*/
	virtual PxU32 removeSamples(const PxArray<PxVec3>& samples) = 0;

	/** Adds new Poisson Samples inside the sphere specified
	\param[in] sphereCenter The sphere's center. Used to define the region where new samples get added.
	\param[in] sphereRadius The sphere's radius. Used to define the region where new samples get added. 
	\param[in] createVolumeSamples If set to true, samples will also get generated inside of the mesh, not just on its surface.
	*/
	virtual void addSamplesInSphere(const PxVec3& sphereCenter, PxReal sphereRadius, bool createVolumeSamples = false) = 0;

	/** Adds new Poisson Samples inside the box specified
	\param[in] axisAlignedBox The axis aligned bounding box. Used to define the region where new samples get added.
	\param[in] boxOrientation The orientation making an oriented bounding box out of the axis aligned one. Used to define the region where new samples get added.
	\param[in] createVolumeSamples If set to true, samples will also get generated inside of the mesh, not just on its surface.
	*/
	virtual void addSamplesInBox(const PxBounds3& axisAlignedBox, const PxQuat& boxOrientation, bool createVolumeSamples = false) = 0;

	/** Gets the Poisson Samples
	\return Returns the generated Poisson Samples
	*/
	virtual const PxArray<PxVec3>& getSamples() const = 0;
	
	virtual ~PxPoissonSampler() { }
};


/** Creates a shape sampler
\param[in] geometry The shape that defines the surface on which the samples get created
\param[in] transform The shape's global pose
\param[in] worldBounds The shapes bounding box
\param[in] initialSamplingRadius The closest distance two surface samples are allowed to have
\param[in] numSampleAttemptsAroundPoint Number of repetitions the underlying algorithm performs to find a new valid sample that matches all criteria like minimal distance to existing samples etc.
\return Returns the sampler
*/
PxPoissonSampler* PxCreateShapeSampler(const PxGeometry& geometry, const PxTransform& transform, const PxBounds3& worldBounds, PxReal initialSamplingRadius, PxI32 numSampleAttemptsAroundPoint = 30);


/**
\brief Sampler to generate Poisson Samples on a triangle mesh.
*/
class PxTriangleMeshPoissonSampler : public virtual PxPoissonSampler
{
public:
	/** Gets the Poisson Samples' triangle indices
	\return Returns the generated Poisson Samples' triangle indices
	*/
	virtual const PxArray<PxI32>& getSampleTriangleIds() const = 0;

	/** Gets the Poisson Samples' barycentric coordinates
	\return Returns the generated Poisson Samples' barycentric coordinates
	*/
	virtual const PxArray<PxVec3>& getSampleBarycentrics() const = 0;

	/** Checks whether a point is inside the triangle mesh
	\return Returns true if the point is inside the triangle mesh
	*/
	virtual bool isPointInTriangleMesh(const PxVec3& p) = 0;

	virtual ~PxTriangleMeshPoissonSampler() { }
};


/** Creates a triangle mesh sampler
\param[in] triangles The triangle indices of the mesh
\param[in] numTriangles The total number of triangles
\param[in] vertices The vertices of the mesh
\param[in] numVertices The total number of vertices
\param[in] initialSamplingRadius The closest distance two surface samples are allowed to have
\param[in] numSampleAttemptsAroundPoint Number of repetitions the underlying algorithm performs to find a new valid sample that matches all criteria like minimal distance to existing samples etc.
\return Returns the sampler
*/
PxTriangleMeshPoissonSampler* PxCreateTriangleMeshSampler(const PxU32* triangles, PxU32 numTriangles, const PxVec3* vertices, PxU32 numVertices, PxReal initialSamplingRadius, PxI32 numSampleAttemptsAroundPoint = 30);


#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
