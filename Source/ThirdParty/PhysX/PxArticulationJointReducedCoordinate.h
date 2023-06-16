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

#ifndef PX_ARTICULATION_JOINT_RC_H
#define PX_ARTICULATION_JOINT_RC_H
/** \addtogroup physics
@{ */

#include "PxPhysXConfig.h"
#include "common/PxBase.h"
#include "solver/PxSolverDefs.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief A joint between two links in an articulation.

	@see PxArticulationReducedCoordinate, PxArticulationLink
	*/
	class PxArticulationJointReducedCoordinate : public PxBase
	{
	public:

		/**
		\brief Gets the parent articulation link of this joint.

		\return The parent link.
		*/
		virtual		PxArticulationLink&	getParentArticulationLink() const = 0;

		/**
		\brief Sets the joint pose in the parent link actor frame.

		\param[in] pose The joint pose.
		<b>Default:</b> The identity transform.

		\note This call is not allowed while the simulation is running.

		@see getParentPose
		*/
		virtual		void				setParentPose(const PxTransform& pose) = 0;

		/**
		\brief Gets the joint pose in the parent link actor frame.

		\return The joint pose.

		@see setParentPose
		*/
		virtual		PxTransform			getParentPose() const = 0;

		/**
		\brief Gets the child articulation link of this joint.

		\return The child link.
		*/
		virtual		PxArticulationLink&	getChildArticulationLink() const = 0;

		/**
		\brief Sets the joint pose in the child link actor frame.

		\param[in] pose The joint pose.
		<b>Default:</b> The identity transform.

		\note This call is not allowed while the simulation is running.

		@see getChildPose
		*/
		virtual		void				setChildPose(const PxTransform& pose) = 0;

		/**
		\brief Gets the joint pose in the child link actor frame.

		\return The joint pose.

		@see setChildPose
		*/
		virtual		PxTransform			getChildPose() const = 0;

		/**
		\brief Sets the joint type (e.g. revolute).

		\param[in] jointType The joint type to set.

		\note Setting the joint type is not allowed while the articulation is in a scene.
		In order to set the joint type, remove and then re-add the articulation to the scene.

		@see PxArticulationJointType, getJointType
		*/
		virtual		void				setJointType(PxArticulationJointType::Enum jointType) = 0;
		
		/**
		\brief Gets the joint type.

		\return The joint type.

		@see PxArticulationJointType, setJointType
		*/
		virtual		PxArticulationJointType::Enum	getJointType() const = 0;

		/**
		\brief Sets the joint motion for a given axis.

		\param[in] axis The target axis.
		\param[in] motion The motion type to set.

		\note Setting the motion of joint axes is not allowed while the articulation is in a scene.
		In order to set the motion, remove and then re-add the articulation to the scene.

		@see PxArticulationAxis, PxArticulationMotion, getMotion
		*/
		virtual		void				setMotion(PxArticulationAxis::Enum axis, PxArticulationMotion::Enum motion) = 0;

		/**
		\brief Returns the joint motion for the given axis.

		\param[in] axis The target axis.

		\return The joint motion of the given axis.

		@see PxArticulationAxis, PxArticulationMotion, setMotion
		*/
		virtual		PxArticulationMotion::Enum	getMotion(PxArticulationAxis::Enum axis) const = 0;

		/**
		\brief Sets the joint limits for a given axis.

		- The motion of the corresponding axis should be set to PxArticulationMotion::eLIMITED in order for the limits to be enforced.
		- The lower limit should be strictly smaller than the higher limit. If the limits should be equal, use PxArticulationMotion::eLOCKED
		and an appropriate offset in the parent/child joint frames.

		\param[in] axis The target axis.
		\param[in] lowLimit The lower joint limit.<br>
		<b>Range:</b> [-PX_MAX_F32, highLimit)<br>
		<b>Default:</b> 0.0
		\param[in] highLimit The higher joint limit.<br>
		<b>Range:</b> (lowLimit, PX_MAX_F32]<br>
		<b>Default:</b> 0.0

		\note This call is not allowed while the simulation is running.

		\deprecated Use #setLimitParams instead. Deprecated since PhysX version 5.1.

		@see setLimitParams, PxArticulationAxis
		*/
		PX_DEPRECATED PX_FORCE_INLINE void	setLimit(PxArticulationAxis::Enum axis, const PxReal lowLimit, const PxReal highLimit)
		{
			setLimitParams(axis, PxArticulationLimit(lowLimit, highLimit));
		}

		/**
		\brief Returns the joint limits for a given axis.

		\param[in] axis The target axis.
		\param[out] lowLimit The lower joint limit.
		\param[out] highLimit The higher joint limit.

		\deprecated Use #getLimitParams instead. Deprecated since PhysX version 5.1.
		
		@see getLimitParams, PxArticulationAxis
		*/
		PX_DEPRECATED PX_FORCE_INLINE void	getLimit(PxArticulationAxis::Enum axis, PxReal& lowLimit, PxReal& highLimit) const
		{
			PxArticulationLimit pair = getLimitParams(axis);
			lowLimit = pair.low;
			highLimit = pair.high;
		}

		/**
		\brief Sets the joint limits for a given axis.

		- The motion of the corresponding axis should be set to PxArticulationMotion::eLIMITED in order for the limits to be enforced.
		- The lower limit should be strictly smaller than the higher limit. If the limits should be equal, use PxArticulationMotion::eLOCKED
			and an appropriate offset in the parent/child joint frames.

		\param[in] axis The target axis.
		\param[in] limit The joint limits.
		
		\note This call is not allowed while the simulation is running.

		\note For spherical joints, limit.min and limit.max must both be in range [-Pi, Pi].

		@see getLimitParams, PxArticulationAxis, PxArticulationLimit
		*/
		virtual		void				setLimitParams(PxArticulationAxis::Enum axis, const PxArticulationLimit& limit) = 0;

		/**
		\brief Returns the joint limits for a given axis.

		\param[in] axis The target axis.

		\return The joint limits.

		@see setLimitParams, PxArticulationAxis, PxArticulationLimit
		*/
		virtual		PxArticulationLimit	getLimitParams(PxArticulationAxis::Enum axis) const = 0;

		/**
		\brief Configures a joint drive for the given axis.

		See PxArticulationDrive for parameter details; and the manual for further information, and the drives' implicit spring-damper (i.e. PD control) implementation in particular.

		\param[in] axis The target axis.
		\param[in] stiffness The drive stiffness, i.e. the proportional gain of the implicit PD controller.<br>
		<b>Range:</b> [0, PX_MAX_F32]<br>
		<b>Default:</b> 0.0
		\param[in] damping The drive damping, i.e. the derivative gain of the implicit PD controller.<br>
		<b>Range:</b> [0, PX_MAX_F32]<br>
		<b>Default:</b> 0.0
		\param[in] maxForce The force limit of the drive (this parameter also limits the force for an acceleration-type drive).<br>
		<b>Range:</b> [0, PX_MAX_F32]<br>
		<b>Default:</b> 0.0
		\param[in] driveType The drive type, @see PxArticulationDriveType.

		\note This call is not allowed while the simulation is running.

		\deprecated Use #setDriveParams instead. Deprecated since PhysX version 5.1.

		@see setDriveParams, PxArticulationAxis, PxArticulationDriveType, PxArticulationDrive, PxArticulationFlag::eDRIVE_LIMITS_ARE_FORCES
		*/
		PX_DEPRECATED PX_FORCE_INLINE void				setDrive(PxArticulationAxis::Enum axis, const PxReal stiffness, const PxReal damping, const PxReal maxForce, PxArticulationDriveType::Enum driveType = PxArticulationDriveType::eFORCE)
		{
			setDriveParams(axis, PxArticulationDrive(stiffness, damping, maxForce, driveType));
		}

		/**
		\brief Gets the joint drive configuration for the given axis.

		\param[in] axis The motion axis.
		\param[out] stiffness The drive stiffness.
		\param[out] damping The drive damping.
		\param[out] maxForce The force limit.
		\param[out] driveType The drive type.

		\deprecated Use #getDriveParams instead. Deprecated since PhysX version 5.1.

		@see getDriveParams, PxArticulationAxis, PxArticulationDriveType, PxArticulationDrive
		*/
		PX_DEPRECATED PX_FORCE_INLINE void			getDrive(PxArticulationAxis::Enum axis, PxReal& stiffness, PxReal& damping, PxReal& maxForce, PxArticulationDriveType::Enum& driveType) const
		{
			PxArticulationDrive drive = getDriveParams(axis);
			stiffness = drive.stiffness;
			damping = drive.damping;
			maxForce = drive.maxForce;
			driveType = drive.driveType;
		}

		/**
		\brief Configures a joint drive for the given axis.

		See PxArticulationDrive for parameter details; and the manual for further information, and the drives' implicit spring-damper (i.e. PD control) implementation in particular.

		\param[in] axis The target axis.
		\param[in] drive The drive parameters

		\note This call is not allowed while the simulation is running.
		
		@see getDriveParams, PxArticulationAxis, PxArticulationDrive
		*/
		virtual		void				setDriveParams(PxArticulationAxis::Enum axis, const PxArticulationDrive& drive) = 0;

		/**
		\brief Gets the joint drive configuration for the given axis.

		\param[in] axis The target axis.
		\return The drive parameters.

		@see setDriveParams, PxArticulationAxis, PxArticulationDrive
		*/
		virtual		PxArticulationDrive	getDriveParams(PxArticulationAxis::Enum axis) const = 0;
		
		/**
		\brief Sets the joint drive position target for the given axis.

		The target units are linear units (equivalent to scene units) for a translational axis, or rad for a rotational axis.

		\param[in] axis The target axis.
		\param[in] target The target position.
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		\note This call is not allowed while the simulation is running.

		\note For spherical joints, target must be in range [-Pi, Pi].

		\note The target is specified in the parent frame of the joint. If Gp, Gc are the parent and child actor poses in the world frame and Lp, Lc are the parent and child joint frames expressed in the parent and child actor frames then the joint will drive the parent and child links to poses that obey Gp * Lp * J = Gc * Lc. For joints restricted to angular motion, J has the form PxTranfsorm(PxVec3(PxZero), PxExp(PxVec3(twistTarget, swing1Target, swing2Target))).  For joints restricted to linear motion, J has the form PxTransform(PxVec3(XTarget, YTarget, ZTarget), PxQuat(PxIdentity)).

		\note For spherical joints with more than 1 degree of freedom, the joint target angles taken together can collectively represent a rotation of greater than Pi around a vector. When this happens the rotation that matches the joint drive target is not the shortest path rotation.  The joint pose J that is the outcome after driving to the target pose will always be the equivalent of the shortest path rotation.

		@see PxArticulationAxis, getDriveTarget
		*/
		virtual		void				setDriveTarget(PxArticulationAxis::Enum axis, const PxReal target, bool autowake = true) = 0;

		/**
		\brief Returns the joint drive position target for the given axis.

		\param[in] axis The target axis.
		
		\return The target position.

		@see PxArticulationAxis, setDriveTarget
		*/
		virtual		PxReal				getDriveTarget(PxArticulationAxis::Enum axis) const = 0;
		
		/**
		\brief Sets the joint drive velocity target for the given axis.

		The target units are linear units (equivalent to scene units) per second for a translational axis, or radians per second for a rotational axis.

		\param[in] axis The target axis.
		\param[in] targetVel The target velocity.
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		\note This call is not allowed while the simulation is running.

		@see PxArticulationAxis, getDriveVelocity
		*/
		virtual		void				setDriveVelocity(PxArticulationAxis::Enum axis, const PxReal targetVel, bool autowake = true) = 0;

		/**
		\brief Returns the joint drive velocity target for the given axis.

		\param[in] axis The target axis.

		\return The target velocity.

		@see PxArticulationAxis, setDriveVelocity
		*/
		virtual		PxReal				getDriveVelocity(PxArticulationAxis::Enum axis) const = 0;
		
		/**
		\brief Sets the joint armature for the given axis.

		- The armature is directly added to the joint-space spatial inertia of the corresponding axis.
		- The armature is in mass units for a prismatic (i.e. linear) joint, and in mass units * (scene linear units)^2 for a rotational joint.

		\param[in] axis The target axis.
		\param[in] armature The joint axis armature.

		\note This call is not allowed while the simulation is running.

		@see PxArticulationAxis, getArmature
		*/
		virtual		void				setArmature(PxArticulationAxis::Enum axis, const PxReal armature) = 0;

		/**
		\brief Gets the joint armature for the given axis.

		\param[in] axis The target axis.
		\return The armature set on the given axis.

		@see PxArticulationAxis, setArmature
		*/
		virtual		PxReal				getArmature(PxArticulationAxis::Enum axis) const = 0;

		/**
		\brief Sets the joint friction coefficient, which applies to all joint axes.

		- The joint friction is unitless and relates the magnitude of the spatial force [F_trans, T_trans] transmitted from parent to child link to
		the maximal friction force F_resist that may be applied by the solver to resist joint motion, per axis; i.e. |F_resist| <= coefficient * (|F_trans| + |T_trans|),
		where F_resist may refer to a linear force or torque depending on the joint axis.
		- The simulated friction effect is therefore similar to static and Coulomb friction. In order to simulate dynamic joint friction, use a joint drive with
		zero stiffness and zero velocity target, and an appropriately dimensioned damping parameter.

		\param[in] coefficient The joint friction coefficient.

		\note This call is not allowed while the simulation is running.

		@see getFrictionCoefficient
		*/
		virtual		void				setFrictionCoefficient(const PxReal coefficient) = 0;

		/**
		\brief Gets the joint friction coefficient.

		\return The joint friction coefficient.

		@see setFrictionCoefficient
		*/
		virtual		PxReal				getFrictionCoefficient() const = 0;

		/**
		\brief Sets the maximal joint velocity enforced for all axes.

		- The solver will apply appropriate joint-space impulses in order to enforce the per-axis joint-velocity limit.
		- The velocity units are linear units (equivalent to scene units) per second for a translational axis, or radians per second for a rotational axis.

		\param[in] maxJointV The maximal per-axis joint velocity.

		\note This call is not allowed while the simulation is running.

		@see getMaxJointVelocity
		*/
		virtual		void				setMaxJointVelocity(const PxReal maxJointV) = 0;

		/**
		\brief Gets the maximal joint velocity enforced for all axes.

		\return The maximal per-axis joint velocity.

		@see setMaxJointVelocity
		*/
		virtual		PxReal				getMaxJointVelocity() const = 0;

		/**
		\brief Sets the joint position for the given axis.

		- For performance, prefer PxArticulationCache::jointPosition to set joint positions in a batch articulation state update.
		- Use PxArticulationReducedCoordinate::updateKinematic after all state updates to the articulation via non-cache API such as this method,
		in order to update link states for the next simulation frame or querying.

		\param[in] axis The target axis.
		\param[in] jointPos The joint position in linear units (equivalent to scene units) for a translational axis, or radians for a rotational axis.

		\note This call is not allowed while the simulation is running.

		\note For spherical joints, jointPos must be in range [-Pi, Pi].

		\note Joint position is specified in the parent frame of the joint. If Gp, Gc are the parent and child actor poses in the world frame and Lp, Lc are the parent and child joint frames expressed in the parent and child actor frames then the parent and child links will be given poses that obey Gp * Lp * J = Gc * Lc with J denoting the joint pose. For joints restricted to angular motion, J has the form PxTranfsorm(PxVec3(PxZero), PxExp(PxVec3(twistPos, swing1Pos, swing2Pos))).  For joints restricted to linear motion, J has the form PxTransform(PxVec3(xPos, yPos, zPos), PxQuat(PxIdentity)).

		\note For spherical joints with more than 1 degree of freedom, the input joint positions taken together can collectively represent a rotation of greater than Pi around a vector. When this happens the rotation that matches the joint positions is not the shortest path rotation.  The joint pose J that is the outcome of setting and applying the joint positions will always be the equivalent of the shortest path rotation.

		@see PxArticulationAxis, getJointPosition, PxArticulationCache::jointPosition, PxArticulationReducedCoordinate::updateKinematic
		*/
		virtual		void				setJointPosition(PxArticulationAxis::Enum axis, const PxReal jointPos) = 0;

		/**
		\brief Gets the joint position for the given axis, i.e. joint degree of freedom (DOF).

		For performance, prefer PxArticulationCache::jointPosition to get joint positions in a batch query.

		\param[in] axis The target axis.

		\return The joint position in linear units (equivalent to scene units) for a translational axis, or radians for a rotational axis.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see PxArticulationAxis, setJointPosition, PxArticulationCache::jointPosition
		*/
		virtual		PxReal				getJointPosition(PxArticulationAxis::Enum axis) const = 0;

		/**
		\brief Sets the joint velocity for the given axis.

		- For performance, prefer PxArticulationCache::jointVelocity to set joint velocities in a batch articulation state update.
		- Use PxArticulationReducedCoordinate::updateKinematic after all state updates to the articulation via non-cache API such as this method,
		in order to update link states for the next simulation frame or querying.

		\param[in] axis The target axis.
		\param[in] jointVel The joint velocity in linear units (equivalent to scene units) per second for a translational axis, or radians per second for a rotational axis.

		\note This call is not allowed while the simulation is running.

		@see PxArticulationAxis, getJointVelocity, PxArticulationCache::jointVelocity, PxArticulationReducedCoordinate::updateKinematic
		*/
		virtual		void				setJointVelocity(PxArticulationAxis::Enum axis, const PxReal jointVel) = 0;

		/**
		\brief Gets the joint velocity for the given axis.

		For performance, prefer PxArticulationCache::jointVelocity to get joint velocities in a batch query.

		\param[in] axis The target axis.

		\return The joint velocity in linear units (equivalent to scene units) per second for a translational axis, or radians per second for a rotational axis.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see PxArticulationAxis, setJointVelocity, PxArticulationCache::jointVelocity
		*/
		virtual		PxReal				getJointVelocity(PxArticulationAxis::Enum axis) const = 0;

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*						getConcreteTypeName() const { return "PxArticulationJointReducedCoordinate"; }

		virtual									~PxArticulationJointReducedCoordinate() {}

		//public variables:
		void*									userData;	//!< The user can assign this to whatever, usually to create a 1:1 relationship with a user object.

	protected:
		PX_INLINE								PxArticulationJointReducedCoordinate(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE								PxArticulationJointReducedCoordinate(PxBaseFlags baseFlags) : PxBase(baseFlags) {}
		
		virtual	bool							isKindOf(const char* name)	const { return !::strcmp("PxArticulationJointReducedCoordinate", name) || PxBase::isKindOf(name); }
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
