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

#include "NvCloth/Callbacks.h"
#include "NvCloth/Allocator.h"
#include "NvCloth/ps/PsAtomic.h"

namespace nv
{
namespace cloth
{

class Factory;

// abstract cloth constraints and triangle indices
class Fabric : public UserAllocated
{
protected:
	Fabric(const Fabric&);
	Fabric& operator = (const Fabric&);

protected:
	Fabric() : mRefCount(1)
	{
	}

	virtual ~Fabric()
	{
		NV_CLOTH_ASSERT(0 == mRefCount);
	}

public:
	/** \brief Returns the Factory used to create this Fabric.*/
	virtual Factory& getFactory() const = 0;

	/** \brief Returns the number of constraint solve phases stored.
		Phases are groups of constraints that make up the general structure of the fabric.
		Cloth instances can have different configuration settings per phase (see Cloth::setPhaseConfig()).
		Phases are usually split by type (horizontal, vertical, bending, shearing), depending on the cooker used.
		*/
	virtual uint32_t getNumPhases() const = 0;

	/** \brief Returns the number of rest lengths stored.
		Each constraint uses the rest value to determine if the two connected particles need to be pulled together or pushed apart.
	*/
	virtual uint32_t getNumRestvalues() const = 0;

	/** \brief Returns the number of constraint stiffness values stored.
		It is optional for a Fabric to have per constraint stiffness values provided.
		This function will return 0 if no values are stored.
		Stiffness per constraint values stored here can be used if more fine grain control is required (as opposed to the values stored in the cloth's phase configuration).
		The Cloth 's phase configuration stiffness values will be ignored if stiffness per constraint values are used.
	*/
	virtual uint32_t getNumStiffnessValues() const = 0;


	/** \brief Returns the number of sets stored.
		Sets connect a phase to a range of indices.
	*/
	virtual uint32_t getNumSets() const = 0;

	/** \brief Returns the number of indices stored.
		Each constraint has a pair of indices that indicate which particles it connects.
	*/
	virtual uint32_t getNumIndices() const = 0;
	/// Returns the number of particles.
	virtual uint32_t getNumParticles() const = 0;
	/// Returns the number of Tethers stored.
	virtual uint32_t getNumTethers() const = 0;
	/// Returns the number of triangles that make up the cloth mesh.
	virtual uint32_t getNumTriangles() const = 0;

	/** Scales all constraint rest lengths.*/
	virtual void scaleRestvalues(float) = 0;
	/** Scales all tether lengths.*/
	virtual void scaleTetherLengths(float) = 0;

	void incRefCount()
	{
		ps::atomicIncrement(&mRefCount);
		NV_CLOTH_ASSERT(mRefCount > 0);
	}

	/// Returns true if the object is destroyed
	bool decRefCount()
	{
		NV_CLOTH_ASSERT(mRefCount > 0);
		int result = ps::atomicDecrement(&mRefCount);
		if (result == 0)
		{
			delete this;
			return true;
		}
		return false;
	}

  protected:
	int32_t mRefCount;
};

} // namespace cloth
} // namespace nv
