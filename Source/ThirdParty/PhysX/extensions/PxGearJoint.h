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

#ifndef PX_GEAR_JOINT_H
#define PX_GEAR_JOINT_H
/** \addtogroup extensions
  @{
*/

#include "extensions/PxJoint.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxGearJoint;

	/**
	\brief Create a gear Joint.

	\param[in] physics		The physics SDK
	\param[in] actor0		An actor to which the joint is attached. NULL may be used to attach the joint to a specific point in the world frame
	\param[in] localFrame0	The position and orientation of the joint relative to actor0
	\param[in] actor1		An actor to which the joint is attached. NULL may be used to attach the joint to a specific point in the world frame
	\param[in] localFrame1	The position and orientation of the joint relative to actor1

	@see PxGearJoint
	*/
	PxGearJoint*	PxGearJointCreate(PxPhysics& physics, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1);

	/**
	\brief A joint that connects two existing revolute joints and constrains their relative angular velocity and position with respect to each other.

	@see PxGearJointCreate PxJoint
	*/
	class PxGearJoint : public PxJoint
	{
	public:

		/**
		\brief Set the hinge/revolute joints connected by the gear joint.

		The passed joints can be either PxRevoluteJoint, PxD6Joint or PxArticulationJointReducedCoordinate. 
		The joints must define degrees of freedom around the twist axis. They cannot be null.

		Note that these joints are only used to compute the positional error correction term,
		used to adjust potential drift between jointed actors. The gear joint can run without
		calling this function, but in that case some visible overlap may develop over time between
		the teeth of the gear meshes.

		\note	Calling this function resets the internal positional error correction term.

		\param[in]	hinge0		The first hinge joint
		\param[in]	hinge1		The second hinge joint
		\return		true if success
		*/
		virtual	bool		setHinges(const PxBase* hinge0, const PxBase* hinge1)	= 0;

		/**
		\brief Set the desired gear ratio.

		For two gears with n0 and n1 teeth respectively, the gear ratio is n0/n1.

		\note	You may need to use a negative gear ratio if the joint frames of involved actors are not oriented in the same direction.

		\note	Calling this function resets the internal positional error correction term.

		\param[in]	ratio	Desired ratio between the two hinges.
		*/
		virtual	void		setGearRatio(float ratio)	= 0;

		/**
		\brief Get the gear ratio.

		\return		Current ratio
		*/
		virtual	float		getGearRatio()	const		= 0;

		virtual	const char*	getConcreteTypeName() const { return "PxGearJoint"; }

	protected:

		PX_INLINE			PxGearJoint(PxType concreteType, PxBaseFlags baseFlags) : PxJoint(concreteType, baseFlags) {}

		PX_INLINE			PxGearJoint(PxBaseFlags baseFlags) : PxJoint(baseFlags)	{}

		virtual	bool		isKindOf(const char* name) const { return !::strcmp("PxGearJoint", name) || PxJoint::isKindOf(name);	}
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
