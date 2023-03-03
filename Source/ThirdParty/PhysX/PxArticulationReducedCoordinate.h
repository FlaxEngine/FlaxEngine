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

#ifndef PX_ARTICULATION_RC_H
#define PX_ARTICULATION_RC_H
/** \addtogroup physics
@{ */

#include "PxPhysXConfig.h"
#include "common/PxBase.h"
#include "foundation/PxVec3.h"
#include "foundation/PxTransform.h"
#include "solver/PxSolverDefs.h"
#include "PxArticulationFlag.h"
#include "PxArticulationTendon.h"
#include "PxArticulationFlag.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	PX_ALIGN_PREFIX(16)
	/**
	\brief Data structure to represent spatial forces.
	*/
	struct PxSpatialForce
	{
		PxVec3 force;
		PxReal pad0;
		PxVec3 torque;
		PxReal pad1;
	}
	PX_ALIGN_SUFFIX(16);

	PX_ALIGN_PREFIX(16)
	/**
	\brief Data structure to represent spatial velocities.
	*/
	struct PxSpatialVelocity
	{
		PxVec3 linear;
		PxReal pad0;
		PxVec3 angular;
		PxReal pad1;
	}
	PX_ALIGN_SUFFIX(16);

	class PxConstraint;
	class PxScene;

	/**
	\brief Data structure used to access the root link state and acceleration.

	@see PxArticulationCache
	*/
	struct PxArticulationRootLinkData
	{
		PxTransform	transform;		//!< Actor transform
		// The velocities and accelerations below are with respect to the center of mass (COM) of the root link. The COM and actor frame origin may not coincide.
		PxVec3		worldLinVel;	//!< Link linear velocity
		PxVec3		worldAngVel;	//!< Link angular velocity
		PxVec3		worldLinAccel;	//!< Link classical linear acceleration
		PxVec3		worldAngAccel;	//!< Link angular acceleration
	};

	/**
	\brief Data structure used to read and write internal articulation data.

	@see PxArticulationCacheFlag, PxArticulationReducedCoordinate::createCache, PxArticulationReducedCoordinate::applyCache,
	PxArticulationReducedCoordinate::copyInternalStateToCache
	*/
	class PxArticulationCache
	{
	public:
		PxArticulationCache() :
			externalForces		(NULL),
			denseJacobian		(NULL),
			massMatrix			(NULL),
			jointVelocity		(NULL),
			jointAcceleration	(NULL),
			jointPosition		(NULL),
			jointForce			(NULL),
			jointSolverForces	(NULL),
			linkVelocity		(NULL),
			linkAcceleration	(NULL),
			rootLinkData		(NULL),
			sensorForces		(NULL),
			coefficientMatrix	(NULL),
			lambda				(NULL),
			scratchMemory		(NULL),
			scratchAllocator	(NULL),
			version				(0)
		{}

		/**
		\brief Releases an articulation cache.

		@see PxArticulationReducedCoordinate::createCache, PxArticulationReducedCoordinate::applyCache,
			 PxArticulationReducedCoordinate::copyInternalStateToCache
		*/
		PX_PHYSX_CORE_API void		release();

		/**
		\brief External forces acting on the articulation links for inverse dynamics computation.

		- N = getNbLinks().
		- Indexing follows the low-level link indices, see PxArticulationLink::getLinkIndex.
		- The forces are with respect to the center of mass of the link.

		@see PxArticulationReducedCoordinate::computeGeneralizedExternalForce
		*/
		PxSpatialForce*				externalForces;

		/**
		\brief Dense Jacobian data.

		- N = nbRows * nbCols = (6 * getNbLinks()) * (6 + getDofs()) -> size includes possible floating-base DOFs regardless of PxArticulationFlag::eFIX_BASE flag.
		- The links, i.e. rows are in order of the low-level link indices (minus one if PxArticulationFlag::eFIX_BASE is true), see PxArticulationLink::getLinkIndex.
		The corresponding spatial velocities are stacked [vx; vy; vz; wx; wy; wz], where vx and wx refer to the linear and rotational velocity in world X.
		- The DOFs, i.e. column indices correspond to the low-level DOF indices, see PxArticulationCache::jointVelocity.

		@see PxArticulationReducedCoordinate::computeDenseJacobian
		*/
		PxReal*						denseJacobian;

		/**
		\brief The generalized mass matrix that maps joint accelerations to joint forces.

		- N = getDofs() * getDofs().
		- The indexing follows the internal DOF index order, see PxArticulationCache::jointVelocity.

		@see PxArticulationReducedCoordinate::computeGeneralizedMassMatrix
		*/
		PxReal*						massMatrix;

		/**
		\brief The articulation joint DOF velocities.

		- N = getDofs().
		- Read/write using PxArticulationCacheFlag::eVELOCITY.
		- The indexing follows the internal DOF index order. Therefore, the application should calculate the DOF data indices by summing the joint DOFs in the order of
		the links' low-level indices (see the manual Section "Cache Indexing" for a snippet for this calculation):
		\verbatim Low-level link index:   | link 0 | link 1 | link 2 | link 3 | ... | <- PxArticulationLink::getLinkIndex()       \endverbatim
		\verbatim Link inbound joint DOF: | 0      | 1      | 2      | 1      | ... | <- PxArticulationLink::getInboundJointDof() \endverbatim
		\verbatim Low-level DOF index:    | -      | 0      | 1, 2   | 3      | ... |                                             \endverbatim
		The root link always has low-level index 0 and always has zero inbound joint DOFs. The link DOF indexing follows the order in PxArticulationAxis::Enum.
		For example, assume that low-level link 2 has an inbound spherical joint with two DOFs: eSWING1 and eSWING2. The corresponding low-level joint DOF indices
		are therefore 1 for eSWING1 and 2 for eSWING2.
		*/
		PxReal*						jointVelocity;

		/**
		\brief The articulation joint DOF accelerations.

		- N = getDofs().
		- Read using PxArticulationCacheFlag::eACCELERATION.
		- The indexing follows the internal DOF index order, see PxArticulationCache::jointVelocity.
		- Delta joint DOF velocities can be computed from acceleration * dt.
		*/
		PxReal*						jointAcceleration;

		/**
		\brief The articulation joint DOF positions.

		- N = getDofs().
		- Read/write using PxArticulationCacheFlag::ePOSITION.
		- The indexing follows the internal DOF index order, see PxArticulationCache::jointVelocity.
        - For spherical joints, the joint position for each axis on the joint must be in range [-Pi, Pi].
		*/
		PxReal*						jointPosition;

		/**
		\brief The articulation joint DOF forces.

		- N = getDofs().
		- Read/Write using PxArticulationCacheFlag::eFORCE.
		- The indexing follows the internal DOF index order, see PxArticulationCache::jointVelocity.
		- Applied joint forces persist and are applied each frame until changed.
		*/
		PxReal*						jointForce;

		/**
		\brief Solver constraint joint DOF forces.

		- N = getDofs().
		- Read using PxArticulationCacheFlag::eJOINT_SOLVER_FORCES.
		- The indexing follows the internal DOF index order, see PxArticulationCache::jointVelocity.
		- Raise PxArticulationFlag::eCOMPUTE_JOINT_FORCES to enable reading the solver forces.
		*/
		PxReal*						jointSolverForces;

		/**
		\brief Link spatial velocity.

		- N = getNbLinks().
		- Read using PxArticulationCacheFlag::eLINK_VELOCITY.
		- The indexing follows the internal link indexing, see PxArticulationLink::getLinkIndex.
		- The velocity is with respect to the link's center of mass.

		@see PxRigidBody::getCMassLocalPose
		*/
		PxSpatialVelocity*			linkVelocity;

		/**
		\brief Link classical acceleration.

		- N = getNbLinks().
		- Read using PxArticulationCacheFlag::eLINK_ACCELERATION.
		- The indexing follows the internal link indexing, see PxArticulationLink::getLinkIndex.
		- The acceleration is with respect to the link's center of mass.

		@see PxArticulationReducedCoordinate::getLinkAcceleration, PxRigidBody::getCMassLocalPose
		*/
		PxSpatialVelocity*			linkAcceleration;

		/**
		\brief Root link transform, velocities, and accelerations.

		- N = 1.
		- Read/write using PxArticulationCacheFlag::eROOT_TRANSFORM and PxArticulationCacheFlag::eROOT_VELOCITIES (accelerations are read-only).

		@see PxArticulationRootLinkData
		*/
		PxArticulationRootLinkData*	rootLinkData;

		/**
		\brief Link sensor spatial forces.

		- N = getNbSensors().
		- Read using PxArticulationCacheFlag::eSENSOR_FORCES.
		- For indexing, see PxArticulationSensor::getIndex.

		@see PxArticulationSensor
		*/
		PxSpatialForce*				sensorForces;

		// Members and memory below here are not zeroed when zeroCache is called, and are not included in the size returned by PxArticulationReducedCoordinate::getCacheDataSize.

		/**
		\brief Constraint coefficient matrix.

		- N = getCoefficentMatrixSize().
		- The user needs to allocate memory and set this member to the allocated memory.

		@see PxArticulationReducedCoordinate::computeCoefficientMatrix
		*/
		PxReal*						coefficientMatrix;

		/**
		\brief Constraint lambda values (impulses applied by the respective constraints).

		- N = getNbLoopJoints().
		- The user needs to allocate memory and set this member to the allocated memory.

		@see PxArticulationReducedCoordinate::computeLambda
		*/
		PxReal*						lambda;

		void*						scratchMemory;		//!< The scratch memory is used for internal calculations.
		void*						scratchAllocator;	//!< The scratch allocator is used for internal calculations.
		PxU32						version;			//!< The cache version used internally to check compatibility with the articulation, i.e. detect if the articulation configuration changed after the cache was created.
	};

	/**
	\brief Flags to configure the forces reported by articulation link sensors.

	@see PxArticulationSensor::setFlag
	*/
	struct PxArticulationSensorFlag
	{
		enum Enum
		{
			eFORWARD_DYNAMICS_FORCES = 1 << 0,		//!< Raise to receive forces from forward dynamics.
			eCONSTRAINT_SOLVER_FORCES = 1 << 1,		//!< Raise to receive forces from constraint solver.
			eWORLD_FRAME = 1 << 2					//!< Raise to receive forces in the world rotation frame, otherwise they will be reported in the sensor's local frame.
		};
	};

	typedef physx::PxFlags<PxArticulationSensorFlag::Enum, PxU8> PxArticulationSensorFlags;

	/**
	\brief A force sensor that can be attached to articulation links to measure spatial force.

	@see PxArticulationReducedCoordinate::createSensor
	*/
	class PxArticulationSensor : public PxBase
	{
	public:
		/**
		\brief Releases the sensor.

		\note Releasing a sensor is not allowed while the articulation is in a scene. In order to
		release a sensor, remove and then re-add the articulation to the scene.
		*/
		virtual void release() = 0;

		/**
		\brief Returns the spatial force in the local frame of the sensor.

		\return The spatial force.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see setRelativePose, getRelativePose
		*/
		virtual PxSpatialForce getForces() const = 0;

		/**
		\brief Returns the relative pose between this sensor and the body frame of the link that the sensor is attached to.

		The link body frame is at the center of mass and aligned with the principal axes of inertia, see PxRigidBody::getCMassLocalPose.

		\return The transform link body frame -> sensor frame.

		@see setRelativePose
		*/
		virtual PxTransform getRelativePose() const = 0;

		/**
		\brief Sets the relative pose between this sensor and the body frame of the link that the sensor is attached to.

		The link body frame is at the center of mass and aligned with the principal axes of inertia, see PxRigidBody::getCMassLocalPose.

		\param[in] pose The transform link body frame -> sensor frame.

		\note Setting the sensor relative pose is not allowed while the articulation is in a scene. In order to
		set the pose, remove and then re-add the articulation to the scene.

		@see getRelativePose
		*/
		virtual void setRelativePose(const PxTransform& pose) = 0;

		/**
		\brief Returns the link that this sensor is attached to.

		\return A pointer to the link.
		*/
		virtual PxArticulationLink* getLink() const = 0;

		/**
		\brief Returns the index of this sensor inside the articulation.

		The return value is only valid for sensors attached to articulations that are in a scene.

		\return The low-level index, or 0xFFFFFFFF if the articulation is not in a scene.
		*/
		virtual PxU32 getIndex() const = 0;

		/**
		\brief Returns the articulation that this sensor is part of.

		\return A pointer to the articulation.
		*/
		virtual PxArticulationReducedCoordinate* getArticulation() const = 0;

		/**
		\brief Returns the sensor's flags.

		\return The current set of flags of the sensor.

		@see PxArticulationSensorFlag
		*/
		virtual PxArticulationSensorFlags getFlags() const = 0;

		/**
		\brief Sets a flag of the sensor.

		\param[in] flag The flag to set.
		\param[in] enabled The value to set the flag to.

		\note Setting the sensor flags is not allowed while the articulation is in a scene. In order to
		set the flags, remove and then re-add the articulation to the scene.

		@see PxArticulationSensorFlag
		*/
		virtual void setFlag(PxArticulationSensorFlag::Enum flag, bool enabled) = 0;

		/**
		\brief Returns the string name of the dynamic type.

		\return The string name.
		*/
		virtual	const char*						getConcreteTypeName() const { return "PxArticulationSensor"; }

		virtual									~PxArticulationSensor() {}

				void*							userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.

	protected:
		PX_INLINE	PxArticulationSensor(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE	PxArticulationSensor(PxBaseFlags baseFlags) : PxBase(baseFlags) {}
	};


	/**
	\brief Flag that configures articulation-state updates by PxArticulationReducedCoordinate::updateKinematic.
	*/
	struct PxArticulationKinematicFlag
	{
		enum Enum
		{
			ePOSITION = 1 << 0,		//!< Raise after any changes to the articulation root or joint positions using non-cache API calls. Updates links' positions and velocities.
			eVELOCITY = 1 << 1		//!< Raise after velocity-only changes to the articulation root or joints using non-cache API calls. Updates links' velocities.
		};
	};

	typedef physx::PxFlags<PxArticulationKinematicFlag::Enum, PxU8> PxArticulationKinematicFlags;
	PX_FLAGS_OPERATORS(PxArticulationKinematicFlag::Enum, PxU8)

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4435)
#endif

	/**
	\brief A tree structure of bodies connected by joints that is treated as a unit by the dynamics solver. Parametrized in reduced (joint) coordinates.

	@see PxArticulationJointReducedCoordinate, PxArticulationLink, PxPhysics::createArticulationReducedCoordinate
	*/
	class PxArticulationReducedCoordinate : public PxBase
	{
	public:

		/**
		\brief Returns the scene which this articulation belongs to.

		\return Owner Scene. NULL if not part of a scene.

		@see PxScene
		*/
		virtual		PxScene*		getScene()	const = 0;

		/**
		\brief Sets the solver iteration counts for the articulation.

		The solver iteration count determines how accurately contacts, drives, and limits are resolved.
		Setting a higher position iteration count may therefore help in scenarios where the articulation
		is subject to many constraints; for example, a manipulator articulation with drives and joint limits
		that is grasping objects, or several such articulations interacting through contacts. Other situations
		where higher position iterations may improve simulation fidelity are: large mass ratios within the
		articulation or between the articulation and an object in contact with it; or strong drives in the
		articulation being used to manipulate a light object.

		If intersecting bodies are being depenetrated too violently, increase the number of velocity
		iterations. More velocity iterations will drive the relative exit velocity of the intersecting
		objects closer to the correct value given the restitution.

		\param[in] minPositionIters Number of position iterations the solver should perform for this articulation. <b>Range:</b> [1,255]
		\param[in] minVelocityIters Number of velocity iterations the solver should perform for this articulation. <b>Range:</b> [1,255]

		\note This call may not be made during simulation.

		@see getSolverIterationCounts()
		*/
		virtual		void				setSolverIterationCounts(PxU32 minPositionIters, PxU32 minVelocityIters = 1) = 0;

		/**
		\brief Returns the solver iteration counts.

		@see setSolverIterationCounts()
		*/
		virtual		void				getSolverIterationCounts(PxU32& minPositionIters, PxU32& minVelocityIters) const = 0;

		/**
		\brief Returns true if this articulation is sleeping.

		When an actor does not move for a period of time, it is no longer simulated in order to save time. This state
		is called sleeping. However, because the object automatically wakes up when it is either touched by an awake object,
		or a sleep-affecting property is changed by the user, the entire sleep mechanism should be transparent to the user.

		An articulation can only go to sleep if all links are ready for sleeping. An articulation is guaranteed to be awake
		if at least one of the following holds:

		\li The wake counter is positive (see #setWakeCounter()).
		\li The linear or angular velocity of any link is non-zero.
		\li A non-zero force or torque has been applied to the articulation or any of its links.

		If an articulation is sleeping, the following state is guaranteed:

		\li The wake counter is zero.
		\li The linear and angular velocity of all links is zero.
		\li There is no force update pending.

		When an articulation gets inserted into a scene, it will be considered asleep if all the points above hold, else it will
		be treated as awake.

		If an articulation is asleep after the call to #PxScene::fetchResults() returns, it is guaranteed that the poses of the
		links were not changed. You can use this information to avoid updating the transforms of associated objects.

		\return True if the articulation is sleeping.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation,
		except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see wakeUp() putToSleep()  getSleepThreshold()
		*/
		virtual		bool				isSleeping() const = 0;

		/**
		\brief Sets the mass-normalized energy threshold below which the articulation may go to sleep.

		The articulation will sleep if the energy of each link is below this threshold.

		\param[in] threshold Energy below which the articulation may go to sleep. <b>Range:</b> [0, PX_MAX_F32)

		\note This call may not be made during simulation.

		@see isSleeping() getSleepThreshold() wakeUp() putToSleep()
		*/
		virtual		void				setSleepThreshold(PxReal threshold) = 0;

		/**
		\brief Returns the mass-normalized energy below which the articulation may go to sleep.

		\return The energy threshold for sleeping.

		@see isSleeping() wakeUp() putToSleep() setSleepThreshold()
		*/
		virtual		PxReal				getSleepThreshold() const = 0;

		/**
		\brief Sets the mass-normalized kinetic energy threshold below which the articulation may participate in stabilization.

		Articulations whose kinetic energy divided by their mass is above this threshold will not participate in stabilization.

		This value has no effect if PxSceneFlag::eENABLE_STABILIZATION was not enabled on the PxSceneDesc.

		<b>Default:</b> 0.01 * PxTolerancesScale::speed * PxTolerancesScale::speed

		\param[in] threshold Energy below which the articulation may participate in stabilization. <b>Range:</b> [0,inf)

		\note This call may not be made during simulation.

		@see  getStabilizationThreshold() PxSceneFlag::eENABLE_STABILIZATION
		*/
		virtual		void				setStabilizationThreshold(PxReal threshold) = 0;

		/**
		\brief Returns the mass-normalized kinetic energy below which the articulation may participate in stabilization.

		Articulations whose kinetic energy divided by their mass is above this threshold will not participate in stabilization.

		\return The energy threshold for participating in stabilization.

		@see setStabilizationThreshold() PxSceneFlag::eENABLE_STABILIZATION
		*/
		virtual		PxReal				getStabilizationThreshold() const = 0;

		/**
		\brief Sets the wake counter for the articulation in seconds.

		- The wake counter value determines the minimum amount of time until the articulation can be put to sleep.
		- An articulation will not be put to sleep if the energy is above the specified threshold (see #setSleepThreshold())
		or if other awake objects are touching it.
		- Passing in a positive value will wake up the articulation automatically.

		<b>Default:</b> 0.4s (which corresponds to 20 frames for a time step of 0.02s)

		\param[in] wakeCounterValue Wake counter value in seconds. <b>Range:</b> [0, PX_MAX_F32)

		\note This call may not be made during simulation, except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see isSleeping() getWakeCounter()
		*/
		virtual		void				setWakeCounter(PxReal wakeCounterValue) = 0;

		/**
		\brief Returns the wake counter of the articulation in seconds.

		\return The wake counter of the articulation in seconds.

		\note This call may not be made during simulation, except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see isSleeping() setWakeCounter()
		*/
		virtual		PxReal				getWakeCounter() const = 0;

		/**
		\brief Wakes up the articulation if it is sleeping.

		- The articulation will get woken up and might cause other touching objects to wake up as well during the next simulation step.
		- This will set the wake counter of the articulation to the value specified in #PxSceneDesc::wakeCounterResetValue.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation,
		except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see isSleeping() putToSleep()
		*/
		virtual		void				wakeUp() = 0;

		/**
		\brief Forces the articulation to sleep.

		- The articulation will stay asleep during the next simulation step if not touched by another non-sleeping actor.
		- This will set any applied force, the velocity, and the wake counter of all bodies in the articulation to zero.

		\note This call may not be made during simulation, and may only be made on articulations that are in a scene.

		@see isSleeping() wakeUp()
		*/
		virtual		void				putToSleep() = 0;

		/**
		\brief Sets the limit on the magnitude of the linear velocity of the articulation's center of mass.

		- The limit acts on the linear velocity of the entire articulation. The velocity is calculated from the total momentum
		and the spatial inertia of the articulation.
		- The limit only applies to floating-base articulations.
		- A benefit of the COM velocity limit is that it is evenly applied to the whole articulation, which results in fewer visual
		artifacts compared to link rigid-body damping or joint-velocity limits. However, these per-link or per-degree-of-freedom
		limits may still help avoid numerical issues.

		\note This call may not be made during simulation.

		\param[in]  maxLinearVelocity The maximal linear velocity magnitude. <b>Range:</b> [0, PX_MAX_F32); <b>Default:</b> PX_MAX_F32.

		@see setMaxCOMAngularVelocity, PxRigidBody::setLinearDamping, PxRigidBody::setAngularDamping, PxArticulationJointReducedCoordinate::setMaxJointVelocity
		*/
		virtual		void				setMaxCOMLinearVelocity(const PxReal maxLinearVelocity) = 0;

		/**
		\brief Gets the limit on the magnitude of the linear velocity of the articulation's center of mass.

		\return The maximal linear velocity magnitude.

		@see setMaxCOMLinearVelocity
		*/
		virtual		PxReal				getMaxCOMLinearVelocity() const = 0;

		/**
		\brief Sets the limit on the magnitude of the angular velocity at the articulation's center of mass.

		- The limit acts on the angular velocity of the entire articulation. The velocity is calculated from the total momentum
		and the spatial inertia of the articulation.
		- The limit only applies to floating-base articulations.
		- A benefit of the COM velocity limit is that it is evenly applied to the whole articulation, which results in fewer visual
		artifacts compared to link rigid-body damping or joint-velocity limits. However, these per-link or per-degree-of-freedom
		limits may still help avoid numerical issues.

		\note This call may not be made during simulation.

		\param[in]  maxAngularVelocity The maximal angular velocity magnitude. <b>Range:</b> [0, PX_MAX_F32); <b>Default:</b> PX_MAX_F32.

		@see setMaxCOMLinearVelocity, PxRigidBody::setLinearDamping, PxRigidBody::setAngularDamping, PxArticulationJointReducedCoordinate::setMaxJointVelocity
		*/
		virtual		void				setMaxCOMAngularVelocity(const PxReal maxAngularVelocity) = 0;

		/**
		\brief Gets the limit on the magnitude of the angular velocity at the articulation's center of mass.

		\return The maximal angular velocity magnitude.

		@see setMaxCOMAngularVelocity
		*/
		virtual		PxReal				getMaxCOMAngularVelocity() const = 0;

		/**
		\brief Adds a link to the articulation with default attribute values.

		\param[in] parent The parent link in the articulation. Must be NULL if (and only if) this is the root link.
		\param[in] pose The initial pose of the new link. Must be a valid transform.

		\return The new link, or NULL if the link cannot be created.

		\note Creating a link is not allowed while the articulation is in a scene. In order to add a link,
		remove and then re-add the articulation to the scene.

		@see PxArticulationLink
		*/
		virtual		PxArticulationLink*	createLink(PxArticulationLink* parent, const PxTransform& pose) = 0;

		/**
		\brief Releases the articulation, and all its links and corresponding joints.

		Attached sensors and tendons are released automatically when the articulation is released.

		\note This call may not be made during simulation.
		*/
		virtual		void				release() = 0;

		/**
		\brief Returns the number of links in the articulation.

		\return The number of links.
		*/
		virtual		PxU32				getNbLinks() const = 0;

		/**
		\brief Returns the set of links in the articulation in the order that they were added to the articulation using createLink.

		\param[in] userBuffer Buffer into which to write the array of articulation link pointers.
		\param[in] bufferSize The size of the buffer. If the buffer is not large enough to contain all the pointers to links,
		only as many as will fit are written.
		\param[in] startIndex Index of first link pointer to be retrieved.

		\return The number of links written into the buffer.

		@see PxArticulationLink
		*/
		virtual		PxU32				getLinks(PxArticulationLink** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0)	const = 0;

		/**
		\brief Returns the number of shapes in the articulation.

		\return The number of shapes.
		*/
		virtual		PxU32				getNbShapes() const = 0;

		/**
		\brief Sets a name string for the articulation that can be retrieved with getName().

		This is for debugging and is not used by the SDK. The string is not copied by the SDK,
		only the pointer is stored.

		\param[in] name String to set the articulation's name to.

		@see getName()
		*/
		virtual		void				setName(const char* name) = 0;

		/**
		\brief Returns the name string set with setName().

		\return Name string associated with the articulation.

		@see setName()
		*/
		virtual		const char*			getName()			const = 0;

		/**
		\brief Returns the axis-aligned bounding box enclosing the articulation.

		\param[in] inflation Scale factor for computed world bounds. Box extents are multiplied by this value.

		\return The articulation's bounding box.

		\note It is not allowed to use this method while the simulation is running, except in a split simulation
		during #PxScene::collide() and up to #PxScene::advance(), and in PxContactModifyCallback or in contact report callbacks.

		@see PxBounds3
		*/
		virtual		PxBounds3			getWorldBounds(float inflation = 1.01f) const = 0;

		/**
		\brief Returns the aggregate the articulation might be a part of.

		\return The aggregate the articulation is a part of, or NULL if the articulation does not belong to an aggregate.

		@see PxAggregate
		*/
		virtual		PxAggregate*		getAggregate() const = 0;

		/**
		\brief Sets flags on the articulation.

		\param[in] flags The articulation flags.

		\note This call may not be made during simulation.

		@see PxArticulationFlag
		*/
		virtual		void					setArticulationFlags(PxArticulationFlags flags) = 0;

		/**
		\brief Raises or clears a flag on the articulation.

		\param[in] flag The articulation flag.
		\param[in] value The value to set the flag to.

		\note This call may not be made during simulation.

		@see PxArticulationFlag
		*/
		virtual		void					setArticulationFlag(PxArticulationFlag::Enum flag, bool value) = 0;

		/**
		\brief Returns the articulation's flags.

		\return The flags.

		@see PxArticulationFlag
		*/
		virtual		PxArticulationFlags		getArticulationFlags() const = 0;

		/**
		\brief Returns the total number of joint degrees-of-freedom (DOFs) of the articulation.

		- The six DOFs of the base of a floating-base articulation are not included in this count.
		- Example: Both a fixed-base and a floating-base double-pendulum with two revolute joints will have getDofs() == 2.
		- The return value is only valid for articulations that are in a scene.

		\return The number of joint DOFs, or 0xFFFFFFFF if the articulation is not in a scene.

		*/
		virtual			PxU32				getDofs() const = 0;

		/**
		\brief Creates an articulation cache that can be used to read and write internal articulation data.

		- When the structure of the articulation changes (e.g. adding a link or sensor) after the cache was created,
		the cache needs to be released and recreated.
		- Free the memory allocated for the cache by calling the release() method on the cache.
		- Caches can only be created by articulations that are in a scene.

		\return The cache, or NULL if the articulation is not in a scene.

		@see applyCache, copyInternalStateToCache
		*/
		virtual		PxArticulationCache*	createCache() const = 0;

		/**
		\brief Returns the size of the articulation cache in bytes.

		- The size does not include: the user-allocated memory for the coefficient matrix or lambda values;
		the scratch-related memory/members; and the cache version. See comment in #PxArticulationCache.
		- The return value is only valid for articulations that are in a scene.

		\return The byte size of the cache, or 0xFFFFFFFF if the articulation is not in a scene.

		@see PxArticulationCache
		*/
		virtual		PxU32					getCacheDataSize() const = 0;

		/**
		\brief Zeroes all data in the articulation cache, except user-provided and scratch memory, and cache version.

		\note This call may only be made on articulations that are in a scene.

		@see PxArticulationCache
		*/
		virtual		void					zeroCache(PxArticulationCache& cache) const = 0;

		/**
		\brief Applies the data in the cache to the articulation.

		This call wakes the articulation if it is sleeping, and the autowake parameter is true (default) or:
		- a nonzero joint velocity is applied or
		- a nonzero joint force is applied or
		- a nonzero root velocity is applied

		\param[in] cache The articulation data.
		\param[in] flags Indicate which data in the cache to apply to the articulation.
		\param[in] autowake If true, the call wakes up the articulation and increases the wake counter to #PxSceneDesc::wakeCounterResetValue
		if the counter value is below the reset value.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see PxArticulationCache, PxArticulationCacheFlags, createCache, copyInternalStateToCache, PxScene::applyArticulationData
		*/
		virtual		void					applyCache(PxArticulationCache& cache, const PxArticulationCacheFlags flags, bool autowake = true) = 0;

		/**
		\brief Copies internal data of the articulation to the cache.

		\param[in] cache The articulation data.
		\param[in] flags Indicate which data to copy from the articulation to the cache.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see PxArticulationCache, PxArticulationCacheFlags, createCache, applyCache
		*/
		virtual		void					copyInternalStateToCache(PxArticulationCache& cache, const PxArticulationCacheFlags flags) const = 0;


		/**
		\brief Converts maximal-coordinate joint DOF data to reduced coordinates.

		- Indexing into the maximal joint DOF data is via the link's low-level index minus 1 (the root link is not included).
		- The reduced-coordinate data follows the cache indexing convention, see PxArticulationCache::jointVelocity.

		\param[in]	maximum The maximal-coordinate joint DOF data,	N = (getNbLinks() - 1) * 6
		\param[out]	reduced	The reduced-coordinate joint DOF data,	N = getDofs()

		\note The articulation must be in a scene.

		@see unpackJointData
		*/
		virtual		void					packJointData(const PxReal* maximum, PxReal* reduced) const = 0;

		/**
		\brief Converts reduced-coordinate joint DOF data to maximal coordinates.

		- Indexing into the maximal joint DOF data is via the link's low-level index minus 1 (the root link is not included).
		- The reduced-coordinate data follows the cache indexing convention, see PxArticulationCache::jointVelocity.

		\param[in]	reduced	The reduced-coordinate joint DOF data,	N = getDofs().
		\param[out]	maximum The maximal-coordinate joint DOF data,	N = (getNbLinks() - 1) * 6.

		\note The articulation must be in a scene.

		@see packJointData
		*/
		virtual		void					unpackJointData(const PxReal* reduced, PxReal* maximum) const = 0;

		/**
		\brief Prepares common articulation data based on articulation pose for inverse dynamics calculations.

		Usage:
		-# Set articulation pose (joint positions and base transform) via articulation cache and applyCache().
		-# Call commonInit.
		-# Call inverse dynamics computation method.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see computeGeneralizedGravityForce, computeCoriolisAndCentrifugalForce
		*/
		virtual		void					commonInit() const = 0;

		/**
		\brief Computes the joint DOF forces required to counteract gravitational forces for the given articulation pose.

		- Inputs:	Articulation pose (joint positions + base transform).
		- Outputs:	Joint forces to counteract gravity (in cache).

		- The joint forces returned are determined purely by gravity for the articulation in the current joint and base pose, and joints at rest;
		i.e. external forces, joint velocities, and joint accelerations are set to zero. Joint drives are also not considered in the computation.
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[out] cache Out: PxArticulationCache::jointForce.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeGeneralizedGravityForce(PxArticulationCache& cache) const = 0;

		/**
		\brief Computes the joint DOF forces required to counteract Coriolis and centrifugal forces for the given articulation state.

		- Inputs:	Articulation state (joint positions and velocities (in cache), and base transform and spatial velocity).
		- Outputs:	Joint forces to counteract Coriolis and centrifugal forces (in cache).

		- The joint forces returned are determined purely by the articulation's state; i.e. external forces, gravity, and joint accelerations are set to zero.
		Joint drives and potential damping terms, such as link angular or linear damping, or joint friction, are also not considered in the computation.
		- Prior to the computation, update/set the base spatial velocity with PxArticulationCache::rootLinkData and applyCache().
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[in,out] cache In: PxArticulationCache::jointVelocity; Out: PxArticulationCache::jointForce.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeCoriolisAndCentrifugalForce(PxArticulationCache& cache) const = 0;

		/**
		\brief Computes the joint DOF forces required to counteract external spatial forces applied to articulation links.

		- Inputs:	External forces on links (in cache), articulation pose (joint positions + base transform).
		- Outputs:	Joint forces to counteract the external forces (in cache).

		- Only the external spatial forces provided in the cache and the articulation pose are considered in the computation.
		- The external spatial forces are with respect to the links' centers of mass, and not the actor's origin.
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[in,out] cache In: PxArticulationCache::externalForces; Out: PxArticulationCache::jointForce.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeGeneralizedExternalForce(PxArticulationCache& cache) const = 0;

		/**
		\brief Computes the joint accelerations for the given articulation state and joint forces.

		- Inputs:	Joint forces (in cache) and articulation state (joint positions and velocities (in cache), and base transform and spatial velocity).
		- Outputs:	Joint accelerations (in cache).

		- The computation includes Coriolis terms and gravity. However, joint drives and potential damping terms are not considered in the computation
		(for example, linear link damping or joint friction).
		- Prior to the computation, update/set the base spatial velocity with PxArticulationCache::rootLinkData and applyCache().
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[in,out] cache In: PxArticulationCache::jointForce and PxArticulationCache::jointVelocity; Out: PxArticulationCache::jointAcceleration.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeJointAcceleration(PxArticulationCache& cache) const = 0;

		/**
		\brief Computes the joint forces for the given articulation state and joint accelerations, not considering gravity.

		- Inputs:	Joint accelerations (in cache) and articulation state (joint positions and velocities (in cache), and base transform and spatial velocity).
		- Outputs:	Joint forces (in cache).

		- The computation includes Coriolis terms. However, joint drives and potential damping terms are not considered in the computation
		(for example, linear link damping or joint friction).
		- Prior to the computation, update/set the base spatial velocity with PxArticulationCache::rootLinkData and applyCache().
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[in,out] cache In: PxArticulationCache::jointAcceleration and PxArticulationCache::jointVelocity; Out: PxArticulationCache::jointForce.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeJointForce(PxArticulationCache& cache) const = 0;

		/**
		\brief Compute the dense Jacobian for the articulation in world space, including the DOFs of a potentially floating base.

		This computes the dense representation of an inherently sparse matrix. Multiplication with this matrix maps
		joint space velocities to world-space linear and angular (i.e. spatial) velocities of the centers of mass of the links.

		\param[out] cache Sets cache.denseJacobian matrix. The matrix is indexed [nCols * row + column].
		\param[out] nRows Set to number of rows in matrix, which corresponds to nbLinks() * 6, minus 6 if PxArticulationFlag::eFIX_BASE is true.
		\param[out] nCols Set to number of columns in matrix, which corresponds to the number of joint DOFs, plus 6 in the case PxArticulationFlag::eFIX_BASE is false.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.
		*/
		virtual		void					computeDenseJacobian(PxArticulationCache& cache, PxU32& nRows, PxU32& nCols) const = 0;

		/**
		\brief Computes the coefficient matrix for contact forces.

		- The matrix dimension is getCoefficientMatrixSize() = getDofs() * getNbLoopJoints(), and the DOF (column) indexing follows the internal DOF order, see PxArticulationCache::jointVelocity.
		- Each column in the matrix is the joint forces effected by a contact based on impulse strength 1.
		- The user must allocate memory for PxArticulationCache::coefficientMatrix where the required size of the PxReal array is equal to getCoefficientMatrixSize().
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[out] cache Out: PxArticulationCache::coefficientMatrix.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit, getCoefficientMatrixSize
		*/
		virtual		void					computeCoefficientMatrix(PxArticulationCache& cache) const = 0;

		/**
		\brief Computes the lambda values when the test impulse is 1.

		- The user must allocate memory for PxArticulationCache::lambda where the required size of the PxReal array is equal to getNbLoopJoints().
		- commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[out] cache Out: PxArticulationCache::lambda.
		\param[in] initialState The initial state of the articulation system.
		\param[in] jointTorque M(q)*qddot + C(q,qdot) + g(q) <- calculate by summing joint forces obtained with computeJointForce and computeGeneralizedGravityForce.
		\param[in] maxIter Maximum number of solver iterations to run. If the system converges, fewer iterations may be used.

		\return True if convergence was achieved within maxIter; False if convergence was not achieved or the operation failed otherwise.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit, getNbLoopJoints
		*/
		virtual		bool					computeLambda(PxArticulationCache& cache, PxArticulationCache& initialState, const PxReal* const jointTorque, const PxU32 maxIter) const = 0;

		/**
		\brief Compute the joint-space inertia matrix that maps joint accelerations to joint forces: forces = M * accelerations.

		- Inputs:	Articulation pose (joint positions and base transform).
		- Outputs:	Mass matrix (in cache).

		commonInit() must be called before the computation, and after setting the articulation pose via applyCache().

		\param[out] cache Out: PxArticulationCache::massMatrix.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see commonInit
		*/
		virtual		void					computeGeneralizedMassMatrix(PxArticulationCache& cache) const = 0;

		/**
		\brief Adds a loop joint to the articulation system for inverse dynamics.

		\param[in] joint The joint to add.

		\note This call may not be made during simulation.

		@see PxContactJoint, PxFixedJoint, PxSphericalJoint, PxRevoluteJoint, PxPrismaticJoint, PxDistanceJoint, PxD6Joint
		*/
		virtual		void					addLoopJoint(PxConstraint* joint) = 0;

		/**
		\brief Removes a loop joint from the articulation for inverse dynamics.

		\note This call may not be made during simulation.

		\param[in] joint The joint to remove.
		*/
		virtual		void					removeLoopJoint(PxConstraint* joint) = 0;

		/**
		\brief Returns the number of loop joints in the articulation for inverse dynamics.

		\return The number of loop joints.
		*/
		virtual		PxU32					getNbLoopJoints() const = 0;

		/**
		\brief Returns the set of loop constraints (i.e. joints) in the articulation.

		\param[in] userBuffer Target buffer for the constraint pointers.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to the constraints,
		only as many as will fit are written. Use getNbLoopJoints() to size the buffer for retrieving all constraints.
		\param[in] startIndex Index of first constraint pointer to be retrieved.

		\return The number of constraints written into the buffer.
		*/
		virtual		PxU32					getLoopJoints(PxConstraint** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

		/**
		\brief Returns the required size of the coefficient matrix in the articulation.

		\return Size of the coefficient matrix (equal to getDofs() * getNbLoopJoints()).

		\note This call may only be made on articulations that are in a scene.

		@see computeCoefficientMatrix
		*/
		virtual		PxU32					getCoefficientMatrixSize() const = 0;

		/**
		\brief Sets the root link transform (world to actor frame).

		- For performance, prefer PxArticulationCache::rootLinkData to set the root link transform in a batch articulation state update.
		- Use updateKinematic() after all state updates to the articulation via non-cache API such as this method,
		in order to update link states for the next simulation frame or querying.

		\param[in] pose The new root link transform.
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		\note This call may not be made during simulation.

		@see getRootGlobalPose, updateKinematic, PxArticulationCache, applyCache
		*/
		virtual		void					setRootGlobalPose(const PxTransform& pose, bool autowake = true) = 0;

		/**
		\brief Returns the root link transform (world to actor frame).

		For performance, prefer PxArticulationCache::rootLinkData to get the root link transform in a batch query.

		\return The root link transform.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see setRootGlobalPose, PxArticulationCache, copyInternalStateToCache
		*/
		virtual		PxTransform				getRootGlobalPose() const = 0;

		/**
		\brief Sets the root link linear center-of-mass velocity.

		- The linear velocity is with respect to the link's center of mass and not the actor frame origin.
		- For performance, prefer PxArticulationCache::rootLinkData to set the root link velocity in a batch articulation state update.
		- The articulation is woken up if the input velocity is nonzero (ignoring autowake) and the articulation is in a scene.
		- Use updateKinematic() after all state updates to the articulation via non-cache API such as this method,
		in order to update link states for the next simulation frame or querying.

		\param[in] linearVelocity The new root link center-of-mass linear velocity.
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		\note This call may not be made during simulation, except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see updateKinematic, getRootLinearVelocity, setRootAngularVelocity, getRootAngularVelocity, PxRigidBody::getCMassLocalPose, PxArticulationCache, applyCache
		*/
		virtual		void					setRootLinearVelocity(const PxVec3& linearVelocity, bool autowake = true) = 0;

		/**
		\brief Gets the root link center-of-mass linear velocity.

		- The linear velocity is with respect to the link's center of mass and not the actor frame origin.
		- For performance, prefer PxArticulationCache::rootLinkData to get the root link velocity in a batch query.

		\return The root link center-of-mass linear velocity.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see setRootLinearVelocity, setRootAngularVelocity, getRootAngularVelocity, PxRigidBody::getCMassLocalPose, PxArticulationCache, applyCache
		*/
		virtual		PxVec3					getRootLinearVelocity(void) const = 0;

		/**
		\brief Sets the root link angular velocity.

		- For performance, prefer PxArticulationCache::rootLinkData to set the root link velocity in a batch articulation state update.
		- The articulation is woken up if the input velocity is nonzero (ignoring autowake) and the articulation is in a scene.
		- Use updateKinematic() after all state updates to the articulation via non-cache API such as this method,
		in order to update link states for the next simulation frame or querying.

		\param[in] angularVelocity The new root link angular velocity.
		\param[in] autowake If true and the articulation is in a scene, the call wakes up the articulation and increases the wake counter
		to #PxSceneDesc::wakeCounterResetValue if the counter value is below the reset value.

		\note This call may not be made during simulation, except in a split simulation in-between #PxScene::fetchCollision and #PxScene::advance.

		@see updateKinematic, getRootAngularVelocity, setRootLinearVelocity, getRootLinearVelocity, PxArticulationCache, applyCache
		*/
		virtual		void					setRootAngularVelocity(const PxVec3& angularVelocity, bool autowake = true) = 0;

		/**
		\brief Gets the root link angular velocity.

		For performance, prefer PxArticulationCache::rootLinkData to get the root link velocity in a batch query.

		\return The root link angular velocity.

		\note This call is not allowed while the simulation is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(),
		and in PxContactModifyCallback or in contact report callbacks.

		@see setRootAngularVelocity, setRootLinearVelocity, getRootLinearVelocity, PxArticulationCache, applyCache
		*/
		virtual		PxVec3					getRootAngularVelocity(void) const = 0;

		/**
		\brief Returns the (classical) link acceleration in world space for the given low-level link index.

		- The returned acceleration is not a spatial, but a classical, i.e. body-fixed acceleration (https://en.wikipedia.org/wiki/Spatial_acceleration).
		- The (linear) acceleration is with respect to the link's center of mass and not the actor frame origin.

		\param[in] linkId The low-level link index, see PxArticulationLink::getLinkIndex.

		\return The link's center-of-mass classical acceleration, or 0 if the call is made before the articulation participated in a first simulation step.

		\note This call may only be made on articulations that are in a scene, and it is not allowed to use this method while the simulation
		is running except in a split simulation during #PxScene::collide() and up to #PxScene::advance(), and in PxContactModifyCallback or in contact report callbacks.

		@see PxArticulationLink::getLinkIndex, PxRigidBody::getCMassLocalPose
		*/
		virtual		PxSpatialVelocity		getLinkAcceleration(const PxU32 linkId) = 0;

		/**
		\brief Returns the GPU articulation index.

		\return The GPU index, or 0xFFFFFFFF if the articulation is not in a scene or PxSceneFlag::eSUPPRESS_READBACK is not set.
		*/
		virtual		PxU32					getGpuArticulationIndex() = 0;

		/**
		\brief Creates a spatial tendon to attach to the articulation with default attribute values.

		\return The new spatial tendon.

		\note Creating a spatial tendon is not allowed while the articulation is in a scene. In order to
		add the tendon, remove and then re-add the articulation to the scene.

		@see PxArticulationSpatialTendon
		*/
		virtual		PxArticulationSpatialTendon*	createSpatialTendon() = 0;

		/**
		\brief Creates a fixed tendon to attach to the articulation with default attribute values.

		\return The new fixed tendon.

		\note Creating a fixed tendon is not allowed while the articulation is in a scene. In order to
		add the tendon, remove and then re-add the articulation to the scene.

		@see PxArticulationFixedTendon
		*/
		virtual		PxArticulationFixedTendon*		createFixedTendon() = 0;

		/**
		\brief Creates a force sensor attached to a link of the articulation.

		\param[in] link The link to attach the sensor to.
		\param[in] relativePose The sensor frame's relative pose to the link's body frame, i.e. the transform body frame -> sensor frame.
		The link body frame is at the center of mass and aligned with the principal axes of inertia, see PxRigidBody::getCMassLocalPose.

		\return The new sensor.

		\note Creating a sensor is not allowed while the articulation is in a scene. In order to
		add the sensor, remove and then re-add the articulation to the scene.

		@see PxArticulationSensor
		*/
		virtual		PxArticulationSensor*	createSensor(PxArticulationLink* link, const PxTransform& relativePose) = 0;


		/**
		\brief Returns the spatial tendons attached to the articulation.

		The order of the tendons in the buffer is not necessarily identical to the order in which the tendons were added to the articulation.

		\param[in] userBuffer The buffer into which to write the array of pointers to the tendons.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to tendons,
		only as many as will fit are written. Use getNbSpatialTendons to size for all attached tendons.
		\param[in] startIndex Index of first tendon pointer to be retrieved.

		\return The number of tendons written into the buffer.

		@see PxArticulationSpatialTendon, getNbSpatialTendons
		*/
		virtual		PxU32					getSpatialTendons(PxArticulationSpatialTendon** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0)	const = 0;

		/**
		\brief Returns the number of spatial tendons in the articulation.

		\return The number of tendons.
		*/
		virtual		PxU32					getNbSpatialTendons() = 0;

		/**
		\brief Returns the fixed tendons attached to the articulation.

		The order of the tendons in the buffer is not necessarily identical to the order in which the tendons were added to the articulation.

		\param[in] userBuffer The buffer into which to write the array of pointers to the tendons.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to tendons,
		only as many as will fit are written. Use getNbFixedTendons to size for all attached tendons.
		\param[in] startIndex Index of first tendon pointer to be retrieved.

		\return The number of tendons written into the buffer.

		@see PxArticulationFixedTendon, getNbFixedTendons
		*/
		virtual		PxU32					getFixedTendons(PxArticulationFixedTendon** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0)	const = 0;

		/**
		\brief Returns the number of fixed tendons in the articulation.

		\return The number of tendons.
		*/
		virtual		PxU32					getNbFixedTendons() = 0;

		/**
		\brief Returns the sensors attached to the articulation.

		The order of the sensors in the buffer is not necessarily identical to the order in which the sensors were added to the articulation.

		\param[in] userBuffer The buffer into which to write the array of pointers to the sensors.
		\param[in] bufferSize The size of the buffer. If this is not large enough to contain all the pointers to sensors,
		only as many as will fit are written. Use getNbSensors to size for all attached sensors.
		\param[in] startIndex Index of first sensor pointer to be retrieved.

		\return The number of sensors written into the buffer.

		@see PxArticulationSensor, getNbSensors
		*/

		virtual		PxU32					getSensors(PxArticulationSensor** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0)	const = 0;

		/**
		\brief Returns the number of sensors in the articulation.

		\return The number of sensors.
		*/
		virtual		PxU32					getNbSensors() = 0;

		/**
		\brief Update link velocities and/or positions in the articulation.

	    For performance, prefer the PxArticulationCache API that performs batch articulation state updates.

		If the application updates the root state (position and velocity) or joint state via any combination of
		the non-cache API calls

		- setRootGlobalPose(), setRootLinearVelocity(), setRootAngularVelocity()
		- PxArticulationJointReducedCoordinate::setJointPosition(), PxArticulationJointReducedCoordinate::setJointVelocity()

		the application needs to call this method after the state setting in order to update the link states for
		the next simulation frame or querying.

		Use
		- PxArticulationKinematicFlag::ePOSITION after any changes to the articulation root or joint positions using non-cache API calls. Updates links' positions and velocities.
		- PxArticulationKinematicFlag::eVELOCITY after velocity-only changes to the articulation root or joints using non-cache API calls. Updates links' velocities only.

		\note This call may only be made on articulations that are in a scene, and may not be made during simulation.

		@see PxArticulationKinematicFlags, PxArticulationCache, applyCache
		*/
		virtual		void					updateKinematic(PxArticulationKinematicFlags flags) = 0;

		virtual								~PxArticulationReducedCoordinate() {}

		void*								userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.

	protected:
		PX_INLINE							PxArticulationReducedCoordinate(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags) {}
		PX_INLINE							PxArticulationReducedCoordinate(PxBaseFlags baseFlags) : PxBase(baseFlags) {}

	};

#if PX_VC
#pragma warning(pop)
#endif

#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
