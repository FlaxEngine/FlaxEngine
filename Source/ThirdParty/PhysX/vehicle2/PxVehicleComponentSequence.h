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

#pragma once
/** \addtogroup vehicle2
  @{
*/
#include "foundation/PxAssert.h"
#include "foundation/PxErrors.h"
#include "foundation/PxFoundation.h"
#include "PxVehicleComponent.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif


struct PxVehicleComponentSequenceLimits
{
	enum Enum
	{
		eMAX_NB_SUBGROUPS = 16,
		eMAX_NB_COMPONENTS = 64,
		eMAX_NB_SUBGROUPELEMENTS = eMAX_NB_SUBGROUPS + eMAX_NB_COMPONENTS
	};
};

struct PxVehicleComponentSequence
{
	enum
	{
		eINVALID_SUBSTEP_GROUP = 0xff
	};

	PxVehicleComponentSequence()
		: mNbComponents(0), mNbSubgroups(1), mNbSubGroupElements(0), mActiveSubgroup(0)
	{
	}

	/**
	\brief Add a component to the sequence.

	\param[in] component The component to add to the sequence.
	\return True on success, else false (for example due to component count limit being reached).
	*/
	PX_FORCE_INLINE bool add(PxVehicleComponent* component);

	/**
	\brief Start a substepping group.
	\note All components added using #add() will be added to the new substepping group until either the group
	is marked as complete with a call to #endSubstepGroup() or a subsequent substepping group is started with 
	a call to #beginSubstepGroup().
	\note Groups can be nested with stacked calls to #beginSubstepGroup().
	\note Each group opened by #beginSubstepGroup() must be closed with a complementary #endSubstepGroup() prior to calling #update().
	\param[in] nbSubSteps is the number of substeps for the group's sequence. This can be changed with a call to #setSubsteps().
	\return Handle for the substepping group on success, else eINVALID_SUBSTEP_GROUP
	@see setSubsteps()
	@see endSubstepGroup()
	*/
	PX_FORCE_INLINE PxU8 beginSubstepGroup(const PxU8 nbSubSteps = 1);

	/**
	\brief End a substepping group
	\note The group most recently opened with #beginSubstepGroup() will be closed by this call.
	@see setSubsteps()
	@see beginSubstepGroup()
	*/
	PX_FORCE_INLINE void endSubstepGroup()
	{
		mActiveSubgroup = mSubGroups[mActiveSubgroup].parentGroup;
	}

	/**
	\brief Set the number of substeps to perform  for a specific substepping group.
	\param[in] subGroupHandle specifies the substepping group
	\param[in] nbSteps is the  number of times to invoke the sequence of components and groups in the specified substepping group.
	@see beginSubstepGroup()
	@see endSubstepGroup()
	*/
	void setSubsteps(const PxU8 subGroupHandle, const PxU8 nbSteps)
	{
		PX_ASSERT(subGroupHandle < mNbSubgroups);
		mSubGroups[subGroupHandle].nbSteps = nbSteps;
	}

	/**
	\brief Update each component in the sequence.

	\note If the update method of a component in the sequence returns false, the update process gets aborted.

	\param[in] dt is the timestep of the update. The provided value has to be positive.
	\param[in] context specifies global quantities of the simulation such as gravitational acceleration.
	*/
	void update(const PxReal dt, const PxVehicleSimulationContext& context)
	{
		PX_ASSERT(0 == mActiveSubgroup);

		if (dt > 0.0f)
		{
			updateSubGroup(dt, context, 0, 1);
		}
		else
		{
			PxGetFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__,
				"PxVehicleComponentSequence::update: The timestep must be positive!");
		}
	}

private:
	enum
	{
		eINVALID_COMPONENT = 0xff,
		eINVALID_SUB_GROUP_ELEMENT = 0xff
	};

	//Elements have the form of a linked list to allow traversal over a list of elements.
	//Each element is either a single component or a subgroup.
	struct SubGroupElement
	{
		SubGroupElement()
			: childGroup(eINVALID_SUBSTEP_GROUP),
			component(eINVALID_COMPONENT),
			nextElement(eINVALID_SUB_GROUP_ELEMENT)
		{
		}

		PxU8 childGroup;
		PxU8 component;
		PxU8 nextElement;
	};

	//A group is a linked list of  elements to be processed in sequence.
	//Each group stores the first element in the sequence.
	//Each element in the sequence stores the next element in the sequence
	//to allow traversal over the  list of elements in the group.
	struct Group
	{
		Group()
			: parentGroup(eINVALID_SUBSTEP_GROUP),
			firstElement(eINVALID_SUB_GROUP_ELEMENT),
			nbSteps(1)
		{
		}
		PxU8 parentGroup;
		PxU8 firstElement;
		PxU8 nbSteps;
	};

	PxVehicleComponent* mComponents[PxVehicleComponentSequenceLimits::eMAX_NB_COMPONENTS];
	PxU8 mNbComponents;

	Group mSubGroups[PxVehicleComponentSequenceLimits::eMAX_NB_SUBGROUPS];
	PxU8 mNbSubgroups;

	SubGroupElement mSubGroupElements[PxVehicleComponentSequenceLimits::eMAX_NB_SUBGROUPELEMENTS];
	PxU8 mNbSubGroupElements;

	PxU8 mActiveSubgroup;

	bool updateSubGroup(const PxReal dt, const PxVehicleSimulationContext& context, const PxU8 groupId, const PxU8 parentSepMultiplier)
	{
		const PxU8 nbSteps = mSubGroups[groupId].nbSteps;
		const PxU8 stepMultiplier = parentSepMultiplier * nbSteps;
		const PxReal timestepForGroup = dt / PxReal(stepMultiplier);
		for (PxU8 k = 0; k < nbSteps; k++)
		{
			PxU8 nextElement = mSubGroups[groupId].firstElement;
			while (eINVALID_SUB_GROUP_ELEMENT != nextElement)
			{
				const SubGroupElement& e = mSubGroupElements[nextElement];
				PX_ASSERT(e.component != eINVALID_COMPONENT || e.childGroup != eINVALID_SUBSTEP_GROUP);
				if (eINVALID_COMPONENT != e.component)
				{
					PxVehicleComponent* c = mComponents[e.component];
					if (!c->update(timestepForGroup, context))
						return false;
				}
				else
				{
					PX_ASSERT(eINVALID_SUBSTEP_GROUP != e.childGroup);
					if (!updateSubGroup(dt, context, e.childGroup, stepMultiplier))
						return false;
				}
				nextElement = e.nextElement;
			}
		}

		return true;
	}

	PxU8 getLastKnownElementInGroup(const PxU8 groupId) const
	{
		PxU8 currElement = mSubGroups[groupId].firstElement;
		PxU8 nextElement = mSubGroups[groupId].firstElement;
		while (nextElement != eINVALID_SUB_GROUP_ELEMENT)
		{
			currElement = nextElement;
			nextElement = mSubGroupElements[nextElement].nextElement;
		}
		return currElement;
	}
};


bool PxVehicleComponentSequence::add(PxVehicleComponent* c)
{
	if (PxVehicleComponentSequenceLimits::eMAX_NB_COMPONENTS == mNbComponents)
		return false;
	if (PxVehicleComponentSequenceLimits::eMAX_NB_SUBGROUPELEMENTS == mNbSubGroupElements)
		return false;

	//Create a new element and point it at the component.
	SubGroupElement& nextElementInGroup = mSubGroupElements[mNbSubGroupElements];
	nextElementInGroup.childGroup = eINVALID_SUBSTEP_GROUP;
	nextElementInGroup.component = mNbComponents;
	nextElementInGroup.nextElement = eINVALID_SUB_GROUP_ELEMENT;

	if (eINVALID_SUB_GROUP_ELEMENT == mSubGroups[mActiveSubgroup].firstElement)
	{
		//The group is empty so add the first element to it.
		//Point the group at the new element because this will 
		//be the first element in the group.
		mSubGroups[mActiveSubgroup].firstElement = mNbSubGroupElements;
	}
	else
	{
		//We are extending the sequence of element of the group.			
		//Add the new element to the end of the group's sequence.
		mSubGroupElements[getLastKnownElementInGroup(mActiveSubgroup)].nextElement = mNbSubGroupElements;
	}

	//Increment the number of elements.
	mNbSubGroupElements++;

	//Record the component and increment the number of components.
	mComponents[mNbComponents] = c;
	mNbComponents++;

	return true;
}

PxU8 PxVehicleComponentSequence::beginSubstepGroup(const PxU8 nbSubSteps)
{
	if (mNbSubgroups == PxVehicleComponentSequenceLimits::eMAX_NB_SUBGROUPS)
		return eINVALID_SUBSTEP_GROUP;
	if (mNbSubGroupElements == PxVehicleComponentSequenceLimits::eMAX_NB_SUBGROUPELEMENTS)
		return eINVALID_SUBSTEP_GROUP;

	//We have a parent and child group relationship.
	const PxU8 parentGroup = mActiveSubgroup;
	const PxU8 childGroup = mNbSubgroups;

	//Set up the child group.
	mSubGroups[childGroup].parentGroup = parentGroup;
	mSubGroups[childGroup].firstElement = eINVALID_SUB_GROUP_ELEMENT;
	mSubGroups[childGroup].nbSteps = nbSubSteps;

	//Create a new element to add to the parent group and point it at the child group.
	SubGroupElement& nextElementIInGroup = mSubGroupElements[mNbSubGroupElements];
	nextElementIInGroup.childGroup = childGroup;
	nextElementIInGroup.nextElement = eINVALID_SUB_GROUP_ELEMENT;
	nextElementIInGroup.component = eINVALID_COMPONENT;

	//Add the new element to the parent group.
	if (eINVALID_SUB_GROUP_ELEMENT == mSubGroups[parentGroup].firstElement)
	{
		//The parent group is empty so add the first element to it.
		//Point the parent group at the new element because this will 
		//be the first element in the group.
		mSubGroups[parentGroup].firstElement = mNbSubGroupElements;
	}
	else
	{
		//We are extending the sequence of elements of the parent group.			
		//Add the new element to the end of the group's sequence.
		mSubGroupElements[getLastKnownElementInGroup(parentGroup)].nextElement = mNbSubGroupElements;
	}

	//Push the active group.
	//All subsequent operations will now address the child group and we push or pop the group.
	mActiveSubgroup = childGroup;

	//Increment the number of elements.
	mNbSubGroupElements++;

	//Increment the number of groups.
	mNbSubgroups++;

	//Return the group id.
	return mActiveSubgroup;
}

#if !PX_DOXYGEN
} // namespace vehicle2
} // namespace physx
#endif

/** @} */
