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

#ifndef PX_PARTICLE_EXT_H
#define PX_PARTICLE_EXT_H
/** \addtogroup extensions
  @{
*/

#include "PxParticleSystem.h"
#include "PxParticleBuffer.h"
#include "foundation/PxArray.h"
#include "foundation/PxHashMap.h"
#include "foundation/PxUserAllocated.h"
#include "PxAttachment.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

namespace ExtGpu
{

/**
\brief Structure to define user-defined particle state when constructing a new particle system.
*/
struct PxParticleBufferDesc
{
	PxVec4* positions;
	PxVec4* velocities;
	PxU32*  phases;
	PxParticleVolume* volumes;
	PxU32	numActiveParticles;
	PxU32	maxParticles;
	PxU32	numVolumes;
	PxU32	maxVolumes;

	PxParticleBufferDesc() : positions(NULL), velocities(NULL), phases(NULL), volumes(NULL), numActiveParticles(0), maxParticles(0), numVolumes(0), maxVolumes(0) { }
};

/**
\brief Structure to define user-defined particle state when constructing a new particle system that includes diffuse particles.
*/
struct PxParticleAndDiffuseBufferDesc : public PxParticleBufferDesc
{
	PxDiffuseParticleParams diffuseParams;
	PxU32 maxDiffuseParticles;
	PxU32 maxActiveDiffuseParticles;

	PxParticleAndDiffuseBufferDesc() : PxParticleBufferDesc() { }
};

/**
\brief Structure to define user-defined particle state when constructing a new particle system that includes shape-matched rigid bodies.
*/
struct PxParticleRigidDesc
{
	PxParticleRigidDesc() : rigidOffsets(NULL), rigidCoefficients(NULL), rigidTranslations(NULL), rigidRotations(NULL), 
		rigidLocalPositions(NULL), rigidLocalNormals(NULL), maxRigids(0), numActiveRigids(0) { }

	PxU32*		rigidOffsets;
	PxReal*		rigidCoefficients;
	PxVec4*		rigidTranslations;
	PxQuat*		rigidRotations;
	PxVec4*		rigidLocalPositions;
	PxVec4*		rigidLocalNormals;
	PxU32		maxRigids;
	PxU32		numActiveRigids;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
\brief Helper class to manage PxParticleClothDesc buffers used for communicating particle based cloths to PxParticleClothBuffer.
*/
class PxParticleClothBufferHelper
{
public:
	virtual void release() = 0;

	virtual PxU32 getMaxCloths() const = 0;		//!< \return The maximum number of cloths this PxParticleClothBufferHelper can hold.
	virtual PxU32 getNumCloths() const = 0;		//!< \return The current number of cloths in this PxParticleClothBufferHelper. 
	virtual PxU32 getMaxSprings() const = 0;	//!< \return The maximum number of springs this PxParticleClothBufferHelper can hold.
	virtual PxU32 getNumSprings() const = 0;	//!< \return The current number of springs in this PxParticleClothBufferHelper.
	virtual PxU32 getMaxTriangles() const = 0;  //!< \return The maximum number of triangles this PxParticleClothBufferHelper can hold.
	virtual PxU32 getNumTriangles() const = 0;	//!< \return The current number of triangles in this PxParticleClothBufferHelper.
	virtual PxU32 getMaxParticles() const = 0;	//!< \return The maximum number of particles this PxParticleClothBufferHelper can hold.
	virtual PxU32 getNumParticles() const = 0;	//!< \return The current number of particles in this PxParticleClothBufferHelper.

	/**
	\brief Adds a PxParticleCloth to this PxParticleClothBufferHelper instance.
	
	\param[in] particleCloth The PxParticleCloth to be added.
	\param[in] triangles A pointer to the triangles
	\param[in] numTriangles The number of triangles
	\param[in] springs A pointer to the springs
	\param[in] numSprings The number of springs
	\param[in] restPositions A pointer to the particle rest positions
	\param[in] numParticles The number of particles in this cloth
	
	@see PxParticleCloth PxParticleSpring
	*/
	virtual void addCloth(const PxParticleCloth& particleCloth, 
		const PxU32* triangles, const PxU32 numTriangles,
		const PxParticleSpring* springs, const PxU32 numSprings, const PxVec4* restPositions, const PxU32 numParticles) = 0;

	/**
	\brief Adds a cloth to this PxParticleClothBufferHelper instance.

	Adds a cloth to this PxParticleClothBufferHelper instance. With this method the relevant parameters for inflatable simulation
	(restVolume, pressure) can be set directly.

	\param[in] blendScale This should be 1.f / (numPartitions + 1) if the springs are partitioned by the user. Otherwise this will be set during spring partitioning.
	\param[in] restVolume The rest volume of the inflatable
	\param[in] pressure The pressure of the inflatable. The target inflatable volume is defined as restVolume * pressure. Setting this to > 0.0 will enable inflatable simulation.
	\param[in] triangles A pointer to the triangles
	\param[in] numTriangles The number of triangles
	\param[in] springs A pointer to the springs
	\param[in] numSprings The number of springs
	\param[in] restPositions A pointer to the particle rest positions
	\param[in] numParticles The number of particles in this cloth

	@see PxParticleSpring
	*/
	virtual void addCloth(const PxReal blendScale, const PxReal restVolume, const PxReal pressure, 
		const PxU32* triangles, const PxU32 numTriangles,
		const PxParticleSpring* springs, const PxU32 numSprings, 
		const PxVec4* restPositions, const PxU32 numParticles) = 0;

	/**
	\brief Returns a PxParticleClothDesc for this PxParticleClothBufferHelper instance to be used for spring partitioning.

	\return the PxParticleClothDesc.

	@see PxCreateAndPopulateParticleClothBuffer, PxParticleClothPreProcessor::partitionSprings
	*/
	virtual PxParticleClothDesc& getParticleClothDesc() = 0;

protected:
	virtual ~PxParticleClothBufferHelper() {}
};


/**
\brief Helper struct that holds information about a specific mesh in a PxParticleVolumeBufferHelper.
*/
struct PxParticleVolumeMesh
{
	PxU32 startIndex;	//!< The index of the first triangle of this mesh in the triangle array of the PxParticleVolumeBufferHelper instance.
	PxU32 count;		//!< The number of triangles of this mesh.
};

/**
\brief Helper class to manage communicating PxParticleVolumes data to PxParticleBuffer.
*/
class PxParticleVolumeBufferHelper
{
public:
	virtual void release() = 0;

	virtual PxU32 getMaxVolumes() const = 0;	//!< \return The maximum number of PxParticleVolume this PxParticleVolumeBufferHelper instance can hold.
	virtual PxU32 getNumVolumes() const = 0;	//!< \return The current number of PxParticleVolume in this PxParticleVolumeBufferHelper instance.
	virtual PxU32 getMaxTriangles() const = 0;	//!< \return The maximum number of triangles this PxParticleVolumeBufferHelper instance can hold.
	virtual PxU32 getNumTriangles() const = 0;  //!< \return The current number of triangles in this PxParticleVolumeBufferHelper instance.

	virtual PxParticleVolume* getParticleVolumes() = 0;				//!< \return A pointer to the PxParticleVolume s of this PxParticleVolumeBufferHelper instance.
	virtual PxParticleVolumeMesh* getParticleVolumeMeshes() = 0;	//!< \return A pointer to the PxParticleVolumeMesh structs describing the PxParticleVolumes of this PxParticleVolumeBufferHelper instance.
	virtual PxU32* getTriangles() = 0;								//!< \return A pointer to the triangle indices in this PxParticleVolumeBufferHelper instance.

	/**
	\brief Adds a PxParticleVolume with a PxParticleVolumeMesh

	\param[in] volume The PxParticleVolume to be added.
	\param[in] volumeMesh A PxParticleVolumeMesh that describes the volumes to be added. startIndex is the index into the triangle list of the PxParticleVolumeBufferHelper instance. 
	\param[in] triangles A pointer to the triangle indices of the PxParticleVolume to be added.
	\param[in] numTriangles The number of triangles of the PxParticleVolume to be added.
	*/
	virtual void addVolume(const PxParticleVolume& volume, const PxParticleVolumeMesh& volumeMesh, const PxU32* triangles, const PxU32 numTriangles) = 0;

	/**
	\brief Adds a volume

	\param[in] particleOffset The index of the first particle of the cloth that maps to this volume in the PxParticleClothBufferHelper instance.
	\param[in] numParticles The number of particles of the cloth that maps to this volume in the PxParticleClothBufferHelper instance.
	\param[in] triangles A pointer to the triangle indices of this volume.
	\param[in] numTriangles The number of triangles in this volume.
	*/
	virtual void addVolume(const PxU32 particleOffset, const PxU32 numParticles, const PxU32* triangles, const PxU32 numTriangles) = 0;

protected:
	virtual ~PxParticleVolumeBufferHelper() {}
};


/**
\brief Helper class to manage PxParticleRigidDesc buffers used for communicating particle based rigids to PxPaticleSystem.
*/
class PxParticleRigidBufferHelper
{
public:
	virtual void release() = 0;

	virtual PxU32 getMaxRigids() const = 0;		//!< \return The maximum number of rigids this PxParticleRigidBufferHelper instance can hold.
	virtual PxU32 getNumRigids() const = 0;		//!< \return The current number of rigids in this PxParticleRigidBufferHelper instance.
	virtual PxU32 getMaxParticles() const = 0;  //!< \return The maximum number of particles this PxParticleRigidBufferHelper instance can hold.
	virtual PxU32 getNumParticles() const = 0;  //!< \return The current number of particles in this PxParticleRigidBufferHelper instance.

	/**
	\brief Adds a rigid.
	
	\param[in] translation The world-space location of the rigid.
	\param[in] rotation The world-space rotation of the rigid.		
	\param[in] coefficient The stiffness of the rigid.
	\param[in] localPositions The particle positions in local space.
	\param[in] localNormals The surface normal for all the particles in local space. Each PxVec4 has the normal in the first 3 components and the SDF in the last component.
	\param[in] numParticles The number of particles in this rigid.
	*/
	virtual void addRigid(const PxVec3& translation, const PxQuat& rotation, const PxReal coefficient,
		const PxVec4* localPositions, const PxVec4* localNormals, PxU32 numParticles) = 0;

	/**
	\brief Get the PxParticleRigidDesc for this buffer.

	\returns A PxParticleRigidDesc.
	*/
	virtual PxParticleRigidDesc& getParticleRigidDesc() = 0;

protected:
	virtual ~PxParticleRigidBufferHelper() {}
};

///////////////////////////////////////////////////////////////////////////////

/**
\brief Holds user-defined attachment data to attach particles to other bodies
*/
class PxParticleAttachmentBuffer : public PxUserAllocated
{		
	PxArray<PxParticleRigidAttachment> mAttachments;
	PxArray<PxParticleRigidFilterPair> mFilters;
	PxHashMap<PxRigidActor*, PxU32> mReferencedBodies;
	PxArray<PxRigidActor*> mNewReferencedBodies;
	PxArray<PxRigidActor*> mDestroyedRefrencedBodies;

	PxParticleBuffer& mParticleBuffer;

	PxParticleRigidAttachment* mDeviceAttachments;
	PxParticleRigidFilterPair* mDeviceFilters;
	PxU32 mNumDeviceAttachments;
	PxU32 mNumDeviceFilters;

	PxCudaContextManager* mCudaContextManager;

	PxParticleSystem& mParticleSystem;

	bool mDirty;

	PX_NOCOPY(PxParticleAttachmentBuffer)

public:

	PxParticleAttachmentBuffer(PxParticleBuffer& particleBuffer, PxParticleSystem& particleSystem);

	~PxParticleAttachmentBuffer();

	// adds attachment to attachment buffer - localPose is in actor space for attachments to all types of rigids.
	void addRigidAttachment(PxRigidActor* rigidBody, const PxU32 particleID, const PxVec3& localPose, PxConeLimitedConstraint* coneLimit = NULL);
	bool removeRigidAttachment(PxRigidActor* rigidBody, const PxU32 particleID);
	void addRigidFilter(PxRigidActor* rigidBody, const PxU32 particleID);
	bool removeRigidFilter(PxRigidActor* rigidBody, const PxU32 particleID);

	void copyToDevice(CUstream stream = 0);
};

/**
\brief Creates a PxParticleRigidBufferHelper.

\param[in] maxRigids The maximum number of rigids this PxParticleRigidsBuffers instance should hold.
\param[in] maxParticles The maximum number of particles this PxParticleRigidBufferHelper instance should hold.
\param[in] cudaContextManager A pointer to a PxCudaContextManager.

\return A pointer to the new PxParticleRigidBufferHelper.
*/
PxParticleRigidBufferHelper* 			PxCreateParticleRigidBufferHelper(PxU32 maxRigids, PxU32 maxParticles, PxCudaContextManager* cudaContextManager);

/**
\brief Creates a PxParticleClothBufferHelper helper.

\param[in] maxCloths The maximum number of cloths this PxParticleClothBufferHelper should hold.
\param[in] maxTriangles The maximum number of triangles this PxParticleClothBufferHelper should hold. 
\param[in] maxSprings The maximum number of springs this PxParticleClothBufferHelper should hold.
\param[in] maxParticles The maximum number of particles this PxParticleClothBufferHelper should hold.
\param[in] cudaContextManager A pointer to a PxCudaContextManager.

\return A pointer to the PxParticleClothBufferHelper that was created.
*/
PxParticleClothBufferHelper* 			PxCreateParticleClothBufferHelper(const PxU32 maxCloths, const PxU32 maxTriangles, const PxU32 maxSprings, const PxU32 maxParticles, PxCudaContextManager* cudaContextManager);

/**
\brief Creates a PxParticleVolumeBufferHelper.

\param[in] maxVolumes The maximum number of PxParticleVolume s this PxParticleVolumeBufferHelper instance should hold.
\param[in] maxTriangles The maximum number of triangles this PxParticleVolumeBufferHelper instance should hold.
\param[in] cudaContextManager A pointer to a PxCudaContextManager.

\return A pointer to the new PxParticleVolumeBufferHelper.
*/
PxParticleVolumeBufferHelper* 			PxCreateParticleVolumeBufferHelper(PxU32 maxVolumes, PxU32 maxTriangles, PxCudaContextManager* cudaContextManager);

/**
\brief Creates a particle attachment buffer

\param[in] particleBuffer The particle buffer that contains particles that should get attached to something
\param[in] particleSystem The particle system that is used to simulate the userBuffer
\return An attachment buffer ready to use
*/
PxParticleAttachmentBuffer*				PxCreateParticleAttachmentBuffer(PxParticleBuffer& particleBuffer, PxParticleSystem& particleSystem);

/**
\brief Creates and populates a particle buffer

\param[in] desc The particle buffer descriptor
\param[in] cudaContextManager A cuda context manager
\return A fully populated particle buffer ready to use
*/
PxParticleBuffer*						PxCreateAndPopulateParticleBuffer(const ExtGpu::PxParticleBufferDesc& desc, PxCudaContextManager* cudaContextManager);
		
/**
\brief Creates and populates a particle buffer that includes support for diffuse particles

\param[in] desc The particle buffer descriptor
\param[in] cudaContextManager A cuda context manager
\return A fully populated particle buffer ready to use
*/
PxParticleAndDiffuseBuffer*				PxCreateAndPopulateParticleAndDiffuseBuffer(const ExtGpu::PxParticleAndDiffuseBufferDesc& desc, PxCudaContextManager* cudaContextManager);

/**
\brief Creates and populates a particle cloth buffer

\param[in] desc The particle buffer descriptor
\param[in] clothDesc The cloth descriptor
\param[out] output A cloth output object to further configure the behavior of the cloth
\param[in] cudaContextManager A cuda context manager
\return A fully populated particle cloth buffer ready to use
*/
PxParticleClothBuffer*					PxCreateAndPopulateParticleClothBuffer(const ExtGpu::PxParticleBufferDesc& desc, const PxParticleClothDesc& clothDesc,
											PxPartitionedParticleCloth& output, PxCudaContextManager* cudaContextManager);

/**
\brief Creates and populates a particle rigid buffer. Particle rigids are particles that try to keep their relative positions. They are a bit commpressible similar to softbodies.

\param[in] desc The particle buffer descriptor
\param[in] rigidDesc The rigid descriptor
\param[in] cudaContextManager A cuda context manager
\return A fully populated particle rigid buffer ready to use
*/
PxParticleRigidBuffer*					PxCreateAndPopulateParticleRigidBuffer(const ExtGpu::PxParticleBufferDesc& desc, const ExtGpu::PxParticleRigidDesc& rigidDesc,
											PxCudaContextManager* cudaContextManager);

} // namespace ExtGpu

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

