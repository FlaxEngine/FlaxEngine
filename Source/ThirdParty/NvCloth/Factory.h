// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

#include "NvCloth/Range.h"
#include <foundation/PxVec4.h>
#include <foundation/PxVec3.h>
#include "NvCloth/Allocator.h"

typedef struct CUctx_st* CUcontext;

namespace nv
{
namespace cloth
{
class DxContextManagerCallback;
class Factory;
}
}
NV_CLOTH_API(nv::cloth::Factory*) NvClothCreateFactoryCPU();
NV_CLOTH_API(nv::cloth::Factory*) NvClothCreateFactoryCUDA(CUcontext);
NV_CLOTH_API(nv::cloth::Factory*) NvClothCreateFactoryDX11(nv::cloth::DxContextManagerCallback*);
NV_CLOTH_API(void) NvClothDestroyFactory(nv::cloth::Factory*);

///Returns true if this dll was compiled with cuda support
NV_CLOTH_API(bool) NvClothCompiledWithCudaSupport();
///Returns true if this dll was compiled with DX support
NV_CLOTH_API(bool) NvClothCompiledWithDxSupport();

namespace nv
{
namespace cloth
{

class Fabric;
class Cloth;
class Solver;

enum struct Platform
{
	CPU,
	CUDA,
	DX11
};

/// abstract factory to create context-specific simulation components
/// such as cloth, solver, collision, etc.
class Factory : public UserAllocated
{
  protected:
	Factory() {}
	Factory(const Factory&);
	Factory& operator = (const Factory&);
	virtual ~Factory() {}

	friend NV_CLOTH_IMPORT void NV_CLOTH_CALL_CONV ::NvClothDestroyFactory(nv::cloth::Factory*);

  public:
	virtual Platform getPlatform() const = 0;

	/**
	   \brief Create fabric data used to setup cloth object.
	   Look at the cooking extension for helper functions to create fabrics from meshes.
		The returned fabric will have a refcount of 1.
	    @param numParticles number of particles, must be larger than any particle index
	    @param phaseIndices map from phase to set index
	    @param sets inclusive prefix sum of restvalue count per set
	    @param restvalues array of constraint rest values
	    @param indices array of particle index pair per constraint
	 */
	virtual Fabric* createFabric(uint32_t numParticles, Range<const uint32_t> phaseIndices, Range<const uint32_t> sets,
								 Range<const float> restvalues, Range<const float> stiffnessValues, Range<const uint32_t> indices,
								 Range<const uint32_t> anchors, Range<const float> tetherLengths,
								 Range<const uint32_t> triangles) = 0;

	/**
	    \brief Create cloth object.
	    @param particles initial particle positions.
	    @param fabric edge distance constraint structure
	 */
	virtual Cloth* createCloth(Range<const physx::PxVec4> particles, Fabric& fabric) = 0;

	/**
	   \brief Create cloth solver object.
	 */
	virtual Solver* createSolver() = 0;

	/**
	    \brief Create a copy of a cloth instance
	    @param cloth the instance to be cloned, need not match the factory type
	 */
	virtual Cloth* clone(const Cloth& cloth) = 0;

	/**
	    \brief Extract original data from a fabric object.
		Use the getNum* methods on Cloth to get the memory requirements before calling this function.
	    @param fabric to extract from, must match factory type
	    @param phaseIndices pre-allocated memory range to write phase => set indices
	    @param sets pre-allocated memory range to write sets
	    @param restvalues pre-allocated memory range to write restvalues
	    @param indices pre-allocated memory range to write indices
	 */
	virtual void extractFabricData(const Fabric& fabric, Range<uint32_t> phaseIndices, Range<uint32_t> sets,
	                               Range<float> restvalues, Range<float> stiffnessValues, Range<uint32_t> indices, Range<uint32_t> anchors,
	                               Range<float> tetherLengths, Range<uint32_t> triangles) const = 0;

	/**
	    \brief Extract current collision spheres and capsules from a cloth object.
		Use the getNum* methods on Cloth to get the memory requirements before calling this function.
	    @param cloth the instance to extract from, must match factory type
	    @param spheres pre-allocated memory range to write spheres
	    @param capsules pre-allocated memory range to write capsules
	    @param planes pre-allocated memory range to write planes
	    @param convexes pre-allocated memory range to write convexes
	    @param triangles pre-allocated memory range to write triangles
	 */
	virtual void extractCollisionData(const Cloth& cloth, Range<physx::PxVec4> spheres, Range<uint32_t> capsules,
	                                  Range<physx::PxVec4> planes, Range<uint32_t> convexes, Range<physx::PxVec3> triangles) const = 0;

	/**
	    Extract current motion constraints from a cloth object
		Use the getNum* methods on Cloth to get the memory requirements before calling this function.
	    @param cloth the instance to extract from, must match factory type
	    @param destConstraints pre-allocated memory range to write constraints
	 */
	virtual void extractMotionConstraints(const Cloth& cloth, Range<physx::PxVec4> destConstraints) const = 0;

	/**
	    Extract current separation constraints from a cloth object
	    @param cloth the instance to extract from, must match factory type
	    @param destConstraints pre-allocated memory range to write constraints
	 */
	virtual void extractSeparationConstraints(const Cloth& cloth, Range<physx::PxVec4> destConstraints) const = 0;

	/**
	    Extract current particle accelerations from a cloth object
	    @param cloth the instance to extract from, must match factory type
	    @param destAccelerations pre-allocated memory range to write accelerations
	 */
	virtual void extractParticleAccelerations(const Cloth& cloth, Range<physx::PxVec4> destAccelerations) const = 0;

	/**
	    Extract virtual particles from a cloth object
	    @param cloth the instance to extract from, must match factory type
	    @param destIndices pre-allocated memory range to write indices
	    @param destWeights pre-allocated memory range to write weights
	 */
	virtual void extractVirtualParticles(const Cloth& cloth, Range<uint32_t[4]> destIndices,
	                                     Range<physx::PxVec3> destWeights) const = 0;

	/**
	    Extract self collision indices from cloth object.
	    @param cloth the instance to extract from, must match factory type
	    @param destIndices pre-allocated memory range to write indices
	*/
	virtual void extractSelfCollisionIndices(const Cloth& cloth, Range<uint32_t> destIndices) const = 0;

	/**
	    Extract particle rest positions from cloth object.
	    @param cloth the instance to extract from, must match factory type
	    @param destRestPositions pre-allocated memory range to write rest positions
	*/
	virtual void extractRestPositions(const Cloth& cloth, Range<physx::PxVec4> destRestPositions) const = 0;
};

} // namespace cloth
} // namespace nv
