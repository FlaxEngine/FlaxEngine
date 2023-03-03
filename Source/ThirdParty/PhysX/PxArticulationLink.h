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

#ifndef PX_ARTICULATION_LINK_H
#define PX_ARTICULATION_LINK_H
/** \addtogroup physics 
@{ */

#include "PxPhysXConfig.h"
#include "PxRigidBody.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief A component of an articulation that represents a rigid body.

Articulation links have a restricted subset of the functionality of a PxRigidDynamic:
- They may not be kinematic, and do not support contact-force thresholds.
- Their velocity or global pose cannot be set directly, but must be set via the articulation-root and joint positions/velocities.
- Sleep state and solver iteration counts are properties of the entire articulation rather than the individual links.

@see PxArticulationReducedCoordinate, PxArticulationReducedCoordinate::createLink, PxArticulationJointReducedCoordinate, PxRigidBody
*/

class PxArticulationLink : public PxRigidBody
{
public:
	/**
	\brief Releases the link from the articulation.
	
	\note Only a leaf articulation link can be released.
	\note Releasing a link is not allowed while the articulation link is in a scene. In order to release a link,
	remove and then re-add the corresponding articulation to the scene.

	@see PxArticulationReducedCoordinate::createLink()
	*/
	virtual		void						release() = 0;

	/**
	\brief Gets the articulation that the link is a part of.

	\return The articulation.

	@see PxArticulationReducedCoordinate
	*/
	virtual		PxArticulationReducedCoordinate&			getArticulation() const = 0;

	/**
	\brief Gets the joint which connects this link to its parent.
	
	\return The joint connecting the link to the parent. NULL for the root link.

	@see PxArticulationJointReducedCoordinate
	*/
	virtual		PxArticulationJointReducedCoordinate*	getInboundJoint() const = 0;

	/**
	\brief Gets the number of degrees of freedom of the joint which connects this link to its parent.

	- The root link DOF-count is defined to be 0 regardless of PxArticulationFlag::eFIX_BASE.
	- The return value is only valid for articulations that are in a scene.

	\return The number of degrees of freedom, or 0xFFFFFFFF if the articulation is not in a scene.

	@see PxArticulationJointReducedCoordinate
	*/
	virtual		PxU32						getInboundJointDof() const = 0;

	/**
	\brief Gets the number of child links.

	\return The number of child links.

	@see getChildren
	*/
	virtual		PxU32						getNbChildren() const = 0;

	/**
	\brief Gets the low-level link index that may be used to index into members of PxArticulationCache.

	The return value is only valid for articulations that are in a scene.

	\return The low-level index, or 0xFFFFFFFF if the articulation is not in a scene.

	@see PxArticulationCache
	*/
	virtual		PxU32						getLinkIndex() const = 0;

	/**
	\brief Retrieves the child links.

	\param[out] userBuffer The buffer to receive articulation link pointers.
	\param[in] bufferSize The size of the provided user buffer, use getNbChildren() for sizing.
	\param[in] startIndex The index of the first child pointer to be retrieved.

	\return The number of articulation links written to the buffer.

	@see getNbChildren
	*/
	virtual		PxU32						getChildren(PxArticulationLink** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;


	/**
	\brief Set the constraint-force-mixing scale term.

	The cfm scale term is a stabilization term that helps avoid instabilities with over-constrained
	configurations. It should be a small value that is multiplied by 1/mass internally to produce
	an additional bias added to the unit response term in the solver.

	\param[in] cfm The constraint-force-mixing scale term.

	<b>Default:</b> 0.025
	<b>Range:</b> [0, 1]

	\note This call is not allowed while the simulation is running.

	@see getCfmScale
	*/
	virtual		void						setCfmScale(const PxReal cfm) = 0;

	/**
	\brief Get the constraint-force-mixing scale term.
	\return The constraint-force-mixing scale term.

	@see setCfmScale
	*/
	virtual		PxReal						getCfmScale() const = 0;

	/**
	\brief Get the linear velocity of the link.

	- The linear velocity is with respect to the link's center of mass and not the actor frame origin.
	- For performance, prefer PxArticulationCache::linkVelocity to get link spatial velocities in a batch query.
	- When the articulation state is updated via non-cache API, use PxArticulationReducedCoordinate::updateKinematic before querying velocity.

	\return The linear velocity of the link.

	\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
	and in PxContactModifyCallback or in contact report callbacks.

	@see PxRigidBody::getCMassLocalPose
	*/
	virtual		PxVec3						getLinearVelocity() const = 0;

	/**
	\brief Get the angular velocity of the link.

	- For performance, prefer PxArticulationCache::linkVelocity to get link spatial velocities in a batch query.
	- When the articulation state is updated via non-cache API, use PxArticulationReducedCoordinate::updateKinematic before querying velocity.

	\return The angular velocity of the link.

	\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
	and in PxContactModifyCallback or in contact report callbacks.
	*/
	virtual		PxVec3						getAngularVelocity() const = 0;

	/**
	\brief Returns the string name of the dynamic type.

	\return The string name.
	*/
	virtual		const char*					getConcreteTypeName()		const		{ return "PxArticulationLink";	}

protected:
	PX_INLINE								PxArticulationLink(PxType concreteType, PxBaseFlags baseFlags) : PxRigidBody(concreteType, baseFlags) {}
	PX_INLINE								PxArticulationLink(PxBaseFlags baseFlags) : PxRigidBody(baseFlags)	{}
	virtual									~PxArticulationLink()	{}
	virtual		bool						isKindOf(const char* name)	const		{ return !::strcmp("PxArticulationLink", name) || PxRigidBody::isKindOf(name);	}
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
