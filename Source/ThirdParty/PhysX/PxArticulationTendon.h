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


#ifndef PX_ARTICULATION_TENDON_H
#define PX_ARTICULATION_TENDON_H
/** \addtogroup physics
@{ */

#include "PxPhysXConfig.h"
#include "common/PxBase.h"
#include "solver/PxSolverDefs.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	class PxArticulationSpatialTendon;
	class PxArticulationFixedTendon;
	class PxArticulationLink;
	/**
	\brief Defines the low/high limits of the length of a tendon.
	*/
	class PxArticulationTendonLimit
	{
	public:
		PxReal lowLimit;
		PxReal highLimit;
	};

	/**
	\brief Defines a spatial tendon attachment point on a link.
	*/
	class PxArticulationAttachment : public PxBase
	{

	public:

		virtual ~PxArticulationAttachment() {}
		/**
		\brief Sets the spring rest length for the sub-tendon from the root to this leaf attachment.

		Setting this on non-leaf attachments has no effect.

		\param[in] restLength The rest length of the spring.
		<b>Default:</b> 0

		@see getRestLength(), isLeaf()
		*/
		virtual		void							setRestLength(const PxReal restLength) = 0;

		/**
		\brief Gets the spring rest length for the sub-tendon from the root to this leaf attachment.

		\return The rest length.

		@see setRestLength()
		*/
		virtual		PxReal							getRestLength() const = 0;

		/**
		\brief Sets the low and high limit on the length of the sub-tendon from the root to this leaf attachment.

		Setting this on non-leaf attachments has no effect.

		\param[in] parameters Struct with the low and high limit.
		<b>Default:</b> (PX_MAX_F32, -PX_MAX_F32) (i.e. an invalid configuration that can only work if stiffness is zero)

		@see PxArticulationTendonLimit, getLimitParameters(), isLeaf()
		*/
		virtual		void							setLimitParameters(const PxArticulationTendonLimit& parameters) = 0;

		/**
		\brief Gets the low and high limit on the length of the sub-tendon from the root to this leaf attachment.

		\return Struct with the low and high limit.

		@see PxArticulationTendonLimit, setLimitParameters()
		*/
		virtual		PxArticulationTendonLimit		getLimitParameters() const = 0;

		/**
		\brief Sets the attachment's relative offset in the link actor frame.

		\param[in] offset The relative offset in the link actor frame.

		@see getRelativeOffset()
		*/
		virtual		void							setRelativeOffset(const PxVec3& offset) = 0;

		/**
		\brief Gets the attachment's relative offset in the link actor frame.

		\return The relative offset in the link actor frame.

		@see setRelativeOffset()
		*/
		virtual		PxVec3							getRelativeOffset() const = 0;

		/**
		\brief Sets the attachment coefficient.

		\param[in] coefficient The scale that the distance between this attachment and its parent is multiplied by when summing up the spatial tendon's length.

		@see getCoefficient()
		*/
		virtual		void							setCoefficient(const PxReal coefficient) = 0;

		/**
		\brief Gets the attachment coefficient.

		\return The scale that the distance between this attachment and its parent is multiplied by when summing up the spatial tendon's length.

		@see setCoefficient()
		*/
		virtual		PxReal							getCoefficient() const = 0;

		/**
		\brief Gets the articulation link.

		\return The articulation link that this attachment is attached to.
		*/
		virtual		PxArticulationLink*				getLink() const = 0;

		/**
		\brief Gets the parent attachment.

		\return The parent attachment.
		*/
		virtual		PxArticulationAttachment*		getParent() const = 0;

		/**
		\brief Indicates that this attachment is a leaf, and thus defines a sub-tendon from the root to this attachment.

		\return True: This attachment is a leaf and has zero children; False: Not a leaf.
		*/
		virtual		bool							isLeaf() const = 0;

		/**
		\brief Gets the spatial tendon that the attachment is a part of.

		\return The tendon.

		@see PxArticulationSpatialTendon
		*/
		virtual		PxArticulationSpatialTendon*	getTendon() const = 0;

		/**
		\brief Releases the attachment.

		\note Releasing the attachment is not allowed while the articulation is in a scene. In order to
		release the attachment, remove and then re-add the articulation to the scene.

		@see PxArticulationSpatialTendon::createAttachment()
		*/
		virtual		void							release() = 0;

					void*							userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.	

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*						getConcreteTypeName() const { return "PxArticulationAttachment"; }

	protected:

		PX_INLINE	PxArticulationAttachment(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationAttachment(PxBaseFlags baseFlags) : PxBase(baseFlags) {}
	};


	/**
	\brief Defines a fixed-tendon joint on an articulation joint degree of freedom.
	*/
	class PxArticulationTendonJoint : public PxBase
	{

	public:

		virtual ~PxArticulationTendonJoint() {}

		/**
		\brief Sets the tendon joint coefficient.

		\param[in] axis The degree of freedom that the tendon joint operates on (must correspond to a degree of freedom of the associated link's incoming joint).
		\param[in] coefficient The scale that the axis' joint position is multiplied by when summing up the fixed tendon's length.
		\param[in] recipCoefficient The scale that the tendon's response is multiplied by when applying to this tendon joint.

		\note RecipCoefficient is commonly expected to be 1/coefficient, but it can be set to different values to tune behavior; for example, zero can be used to
		have a joint axis only participate in the length computation of the tendon, but not have any tendon force applied to it.

		@see getCoefficient()
		*/
		virtual		void							setCoefficient(const PxArticulationAxis::Enum axis, const PxReal coefficient, const PxReal recipCoefficient) = 0;

		/**
		\brief Gets the tendon joint coefficient.

		\param[out] axis The degree of freedom that the tendon joint operates on.
		\param[out] coefficient The scale that the axis' joint position is multiplied by when summing up the fixed tendon's length.
		\param[in] recipCoefficient The scale that the tendon's response is multiplied by when applying to this tendon joint.

		@see setCoefficient()
		*/
		virtual		void							getCoefficient(PxArticulationAxis::Enum& axis, PxReal& coefficient, PxReal& recipCoefficient) const = 0;

		/**
		\brief Gets the articulation link.

		\return The articulation link (and its incoming joint in particular) that this tendon joint is associated with.
		*/
		virtual		PxArticulationLink*				getLink() const = 0;

		/**
		\brief Gets the parent tendon joint.

		\return The parent tendon joint.
		*/
		virtual		PxArticulationTendonJoint*		getParent() const = 0;

		/**
		\brief Gets the tendon that the joint is a part of.

		\return The tendon.

		@see PxArticulationFixedTendon
		*/
		virtual		PxArticulationFixedTendon*		getTendon() const = 0;

		/**
		\brief Releases a tendon joint.

		\note Releasing a tendon joint is not allowed while the articulation is in a scene. In order to
		release the joint, remove and then re-add the articulation to the scene.

		@see PxArticulationFixedTendon::createTendonJoint()
		*/
		virtual		void							release() = 0;

					void*							userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*							getConcreteTypeName() const { return "PxArticulationTendonJoint"; }

	protected:

		PX_INLINE	PxArticulationTendonJoint(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationTendonJoint(PxBaseFlags baseFlags) : PxBase(baseFlags) {}
	};


	/**
	\brief Common API base class shared by PxArticulationSpatialTendon and PxArticulationFixedTendon.
	*/
	class PxArticulationTendon : public PxBase
	{
	public:
		/**
		\brief Sets the spring stiffness term acting on the tendon length.

		\param[in] stiffness The spring stiffness.
		<b>Default:</b> 0

		@see getStiffness()
		*/
		virtual		void							setStiffness(const PxReal stiffness) = 0;

		/**
		\brief Gets the spring stiffness of the tendon.

		\return The spring stiffness.

		@see setStiffness()
		*/
		virtual		PxReal							getStiffness() const = 0;

		/**
		\brief Sets the damping term acting both on the tendon length and tendon-length limits.

		\param[in] damping The damping term.
		<b>Default:</b> 0

		@see getDamping()
		*/
		virtual		void							setDamping(const PxReal damping) = 0;

		/**
		\brief Gets the damping term acting both on the tendon length and tendon-length limits.

		\return The damping term.

		@see setDamping()
		*/
		virtual		PxReal							getDamping() const = 0;

		/**
		\brief Sets the limit stiffness term acting on the tendon's length limits.

		For spatial tendons, this parameter applies to all its leaf attachments / sub-tendons.

		\param[in] stiffness The limit stiffness term.
		<b>Default:</b> 0

		@see getLimitStiffness()
		*/
		virtual		void							setLimitStiffness(const PxReal stiffness) = 0;

		/**
		\brief Gets the limit stiffness term acting on the tendon's length limits.

		For spatial tendons, this parameter applies to all its leaf attachments / sub-tendons.

		\return The limit stiffness term.

		@see setLimitStiffness()
		*/
		virtual		PxReal							getLimitStiffness() const = 0;

		/**
		\brief Sets the length offset term for the tendon.

		An offset defines an amount to be added to the accumulated length computed for the tendon. It allows the
		application to actuate the tendon by shortening or lengthening it.

		\param[in] offset The offset term. <b>Default:</b> 0
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		@see getOffset()
		*/
		virtual		void							setOffset(const PxReal offset, bool autowake = true) = 0;

		/**
		\brief Gets the length offset term for the tendon.

		\return The offset term.

		@see setOffset()
		*/
		virtual		PxReal							getOffset() const = 0;

		/**
		\brief Gets the articulation that the tendon is a part of.

		\return The articulation.

		@see PxArticulationReducedCoordinate
		*/
		virtual		PxArticulationReducedCoordinate* getArticulation() const = 0;

		/**
		\brief Releases a tendon to remove it from the articulation and free its associated memory.

		When an articulation is released, its attached tendons are automatically released.

		\note Releasing a tendon is not allowed while the articulation is in a scene. In order to
		release the tendon, remove and then re-add the articulation to the scene.
		*/

		virtual		void							release() = 0;

		virtual										~PxArticulationTendon() {}

					void*							userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.

	protected:
		PX_INLINE	PxArticulationTendon(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationTendon(PxBaseFlags baseFlags) : PxBase(baseFlags) {}
	};

	/**
	\brief A spatial tendon that attaches to an articulation.

	A spatial tendon attaches to multiple links in an articulation using a set of PxArticulationAttachments.
	The tendon is defined as a tree of attachment points, where each attachment can have an arbitrary number of children.
	Each leaf of the attachment tree defines a subtendon between itself and the root attachment. The subtendon then
	applies forces at the leaf, and an equal but opposing force at the root, in order to satisfy the spring-damper and limit
	constraints that the user sets up. Attachments in between the root and leaf do not exert any force on the articulation,
	but define the geometry of the tendon from which the length is computed together with the attachment coefficients.
	*/
	class PxArticulationSpatialTendon : public PxArticulationTendon
	{
	public:
		/**
		\brief Creates an articulation attachment and adds it to the list of children in the parent attachment.

		Creating an attachment is not allowed while the articulation is in a scene. In order to
		add the attachment, remove and then re-add the articulation to the scene.

		\param[in] parent The parent attachment. Can be NULL for the root attachment of a tendon.
		\param[in] coefficient A user-defined scale that the accumulated length is scaled by.
		\param[in] relativeOffset An offset vector in the link's actor frame to the point where the tendon attachment is attached to the link.
		\param[in] link The link that this attachment is associated with.

		\return The newly-created attachment if creation was successful, otherwise a null pointer.

		@see releaseAttachment()
		*/
		virtual		PxArticulationAttachment*		createAttachment(PxArticulationAttachment* parent, const PxReal coefficient, const PxVec3 relativeOffset, PxArticulationLink* link) = 0;

		/**
		\brief Fills a user-provided buffer of attachment pointers with the set of attachments.

		\param[in] userBuffer The user-provided buffer.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to attachments,
		only as many as can fit are written. Use getNbAttachments to size for all attachments.
		\param[in] startIndex Index of first attachment pointer to be retrieved.

		\return The number of attachments that were filled into the user buffer.

		@see getNbAttachments
		*/
		virtual		PxU32							getAttachments(PxArticulationAttachment** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

		/**
		\brief Returns the number of attachments in the tendon.

		\return The number of attachments.
		*/
		virtual		PxU32							getNbAttachments() const = 0;

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*						getConcreteTypeName() const { return "PxArticulationSpatialTendon"; }

		virtual									~PxArticulationSpatialTendon() {}

	protected:
		PX_INLINE	PxArticulationSpatialTendon(PxType concreteType, PxBaseFlags baseFlags) : PxArticulationTendon(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationSpatialTendon(PxBaseFlags baseFlags) : PxArticulationTendon(baseFlags) {}

	};

	/**
	\brief A fixed tendon that can be used to link multiple degrees of freedom of multiple articulation joints via length and limit constraints.

	Fixed tendons allow the simulation of coupled relationships between joint degrees of freedom in an articulation. Fixed tendons do not allow
	linking arbitrary joint axes of the articulation: The respective joints must all be directly connected to each other in the articulation structure,
	i.e. each of the joints in the tendon must be connected by a single articulation link to another joint in the same tendon. This implies both that 
	1) fixed tendons can branch along a branching articulation; and 2) they cannot be used to create relationships between axes in a spherical joint with
	more than one degree of freedom. Locked joint axes or fixed joints are currently not supported.
	*/
	class PxArticulationFixedTendon : public PxArticulationTendon
	{
	public:
		/**
		\brief Creates an articulation tendon joint and adds it to the list of children in the parent tendon joint.

		Creating a tendon joint is not allowed while the articulation is in a scene. In order to
		add the joint, remove and then re-add the articulation to the scene.

		\param[in] parent The parent tendon joint. Can be NULL for the root tendon joint of a tendon.
		\param[in] axis The degree of freedom that this tendon joint is associated with.
		\param[in] coefficient A user-defined scale that the accumulated tendon length is scaled by.
		\param[in] recipCoefficient The scale that the tendon's response is multiplied by when applying to this tendon joint.
		\param[in] link The link (and the link's incoming joint in particular) that this tendon joint is associated with.

		\return The newly-created tendon joint if creation was successful, otherwise a null pointer.

		\note
		- The axis motion must not be configured as PxArticulationMotion::eLOCKED.
		- The axis cannot be part of a fixed joint, i.e. joint configured as PxArticulationJointType::eFIX.

		@see PxArticulationTendonJoint PxArticulationAxis
		*/
		virtual		PxArticulationTendonJoint*		createTendonJoint(PxArticulationTendonJoint* parent, PxArticulationAxis::Enum axis, const PxReal coefficient, const PxReal recipCoefficient, PxArticulationLink* link) = 0;

		/**
		\brief Fills a user-provided buffer of tendon-joint pointers with the set of tendon joints.

		\param[in] userBuffer The user-provided buffer.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to tendon joints,
		only as many as can fit are written. Use getNbTendonJoints to size for all tendon joints.
		\param[in] startIndex Index of first tendon joint pointer to be retrieved.

		\return The number of tendon joints filled into the user buffer.

		@see getNbTendonJoints
		*/
		virtual		PxU32							getTendonJoints(PxArticulationTendonJoint** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

		/**
		\brief Returns the number of tendon joints in the tendon.

		\return The number of tendon joints.
		*/
		virtual		PxU32							getNbTendonJoints() const = 0;

		/**
		\brief Sets the spring rest length of the tendon.

		The accumulated "length" of a fixed tendon is a linear combination of the joint axis positions that the tendon is
		associated with, scaled by the respective tendon joints' coefficients. As such, when the joint positions of all
		joints are zero, the accumulated length of a fixed tendon is zero.

		The spring of the tendon is not exerting any force on the articulation when the rest length is equal to the
		tendon's accumulated length plus the tendon offset.

		\param[in] restLength The spring rest length of the tendon.

		@see getRestLength()
		*/
		virtual		void							setRestLength(const PxReal restLength) = 0;

		/**
		\brief Gets the spring rest length of the tendon.

		\return The spring rest length of the tendon.

		@see setRestLength()
		*/
		virtual		PxReal							getRestLength() const = 0;

		/**
		\brief Sets the low and high limit on the length of the tendon.

		\param[in] parameter Struct with the low and high limit.

		The limits, together with the damping and limit stiffness parameters, act on the accumulated length of the tendon.

		@see PxArticulationTendonLimit getLimitParameters() setRestLength()
		*/
		virtual		void							setLimitParameters(const PxArticulationTendonLimit& parameter) = 0;


		/**
		\brief Gets the low and high limit on the length of the tendon.

		\return Struct with the low and high limit.

		@see PxArticulationTendonLimit setLimitParameters()
		*/
		virtual		PxArticulationTendonLimit		getLimitParameters() const = 0;

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*						getConcreteTypeName() const { return "PxArticulationFixedTendon"; }

		virtual									~PxArticulationFixedTendon() {}

	protected:
		PX_INLINE	PxArticulationFixedTendon(PxType concreteType, PxBaseFlags baseFlags) : PxArticulationTendon(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationFixedTendon(PxBaseFlags baseFlags) : PxArticulationTendon(baseFlags) {}
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

