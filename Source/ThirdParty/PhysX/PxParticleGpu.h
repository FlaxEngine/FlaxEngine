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

#ifndef PX_GPU_PARTICLE_SYSTEM_H
#define PX_GPU_PARTICLE_SYSTEM_H
/** \addtogroup physics
@{ */

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxVec3.h"

#include "PxParticleSystem.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
@brief Common material properties for particles. See #PxParticleMaterial.

Accessed by either integration or particle-rigid collisions
*/
struct PxsParticleMaterialData
{
	PxReal			friction;			 // 4
	PxReal			damping;			 // 8
	PxReal			adhesion;			 // 12 
	PxReal			gravityScale;		 // 16
	PxReal			adhesionRadiusScale; // 20
};

#if !PX_DOXYGEN
} // namespace physx
#endif

#if PX_SUPPORT_GPU_PHYSX

struct float4;

PX_CUDA_CALLABLE inline physx::PxU32 PxGetGroup(physx::PxU32 phase) { return phase & physx::PxParticlePhaseFlag::eParticlePhaseGroupMask; }
PX_CUDA_CALLABLE inline bool PxGetFluid(physx::PxU32 phase) { return (phase & physx::PxParticlePhaseFlag::eParticlePhaseFluid) != 0; }
PX_CUDA_CALLABLE inline bool PxGetSelfCollide(physx::PxU32 phase) { return (phase & physx::PxParticlePhaseFlag::eParticlePhaseSelfCollide) != 0; }
PX_CUDA_CALLABLE inline bool PxGetSelfCollideFilter(physx::PxU32 phase) { return (phase & physx::PxParticlePhaseFlag::eParticlePhaseSelfCollideFilter) != 0; }

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
@brief An iterator class to iterate over the neighbors of a particle during particle system simulation. 
*/
class PxNeighborhoodIterator
{

	const PxU32* PX_RESTRICT    mCollisionIndex;    //!< Pointer to the current state of the iterator. 
	PxU32                       mMaxParticles;		//!< The maximum number of particles of the particle system this iterator is used on.

public:
	PX_CUDA_CALLABLE PxNeighborhoodIterator(const PxU32* PX_RESTRICT collisionIndex, PxU32 maxParticles) :
		mCollisionIndex(collisionIndex), mMaxParticles(maxParticles)
	{
	}

	PX_CUDA_CALLABLE PxU32 getNextIndex()
	{
		PxU32 result = *mCollisionIndex;
		mCollisionIndex += mMaxParticles;
		return result;
	}

	PX_INLINE PxNeighborhoodIterator(const PxNeighborhoodIterator& params)
	{
		mCollisionIndex = params.mCollisionIndex;
		mMaxParticles = params.mMaxParticles;
	}

	PX_INLINE void operator = (const PxNeighborhoodIterator& params)
	{
		mCollisionIndex = params.mCollisionIndex;
		mMaxParticles = params.mMaxParticles;
	}
};

/**
@brief Structure that holds simulation parameters of a #PxGpuParticleSystem.
*/
struct PxGpuParticleData
{
	PxVec3	mPeriod;                        //!< Size of the unit cell for periodic boundary conditions. If 0, the size of the simulation domain is specified by the mGridSize * mParticleContactDistance.

	PxU32	mGridSizeX;                     //!< Size of the x-dimension of the background simulation grid. Translates to an absolute size of mGridSizeX * mParticleContactDistance. 
	PxU32	mGridSizeY;                     //!< Size of the y-dimension of the background simulation grid. Translates to an absolute size of mGridSizeY * mParticleContactDistance.
	PxU32	mGridSizeZ;                     //!< Size of the z-dimension of the background simulation grid. Translates to an absolute size of mGridSizeZ * mParticleContactDistance.

	PxReal	mParticleContactDistance;       //!< Two particles start interacting if their distance is lower than mParticleContactDistance.
	PxReal	mParticleContactDistanceInv;    //!< 1.f / mParticleContactDistance.
	PxReal	mParticleContactDistanceSq;     //!< mParticleContactDistance * mParticleContactDistance.

	PxU32	mNumParticles;                  //!< The number of particles in this particle system.
	PxU32	mMaxParticles;                  //!< The maximum number of particles that can be simulated in this particle system.
	PxU32	mMaxNeighborhood;               //!< The maximum number of particles considered when computing neighborhood based particle interactions.
	PxU32	mMaxDiffuseParticles;           //!< The maximum number of diffuse particles that can be simulated using this particle system.
	PxU32	mNumParticleBuffers;            //!< The number of particle buffers that are simulated in this particle system.
};

/**
@brief Container class for a GPU particle system. Used to communicate particle system parameters and simulation state 
between the internal SDK simulation and the particle system callbacks.

See #PxParticleSystem, #PxParticleSystemCallback.
*/
class PxGpuParticleSystem
{
public:

	/**
	@brief Returns the number of cells of the background simulation grid.
	 
	@return PxU32 the number of cells. 
	*/
	PX_FORCE_INLINE PxU32 getNumCells() { return mCommonData.mGridSizeX * mCommonData.mGridSizeY * mCommonData.mGridSizeZ; }
	
	/* Unsorted particle state buffers */
	float4*                      mUnsortedPositions_InvMass;     //!< GPU pointer to unsorted particle positions and inverse masses.
	float4*	                     mUnsortedVelocities;            //!< GPU pointer to unsorted particle velocities.
	PxU32*	                     mUnsortedPhaseArray;            //!< GPU pointer to unsorted particle phase array. See #PxParticlePhase.

	/* Sorted particle state buffers. Sorted by increasing hash value in background grid. */
	float4*                      mSortedPositions_InvMass;       //!< GPU pointer to sorted particle positions
	float4*                      mSortedVelocities;              //!< GPU pointer to sorted particle velocities
	PxU32*	                     mSortedPhaseArray;              //!< GPU pointer to sorted particle phase array

	/* Mappings to/from sorted particle states */
	PxU32*                       mUnsortedToSortedMapping;       //!< GPU pointer to the mapping from unsortedParticle ID to sorted particle ID
	PxU32*                       mSortedToUnsortedMapping;       //!< GPU pointer to the mapping from sorted particle ID to unsorted particle ID

	/* Neighborhood information */
	PxU32*	                     mParticleSelfCollisionCount;    //!< Per-particle neighborhood count
	PxU32*	                     mCollisionIndex;                //!< Set of sorted particle indices per neighbor

	PxsParticleMaterialData*     mParticleMaterials;             //!< GPU pointer to the particle materials used in this particle system.
	PxGpuParticleData            mCommonData;                    //!< Structure holding simulation parameters and state for this particle system. See #PxGpuParticleData.

	/**
	@brief Get a PxNeighborhoodIterator initialized for usage with this particle system.

	@param particleId An initial particle index for the initialization of the iterator. 

	@return An initialized PxNeighborhoodIterator.
	*/
	PX_CUDA_CALLABLE PxNeighborhoodIterator getIterator(PxU32 particleId) const
	{
		return PxNeighborhoodIterator(mCollisionIndex + particleId, mCommonData.mMaxParticles);
	}
};

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

/** @} */
#endif

