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

#ifndef PX_SOLVER_DEFS_H
#define PX_SOLVER_DEFS_H

#include "PxPhysXConfig.h"
#include "foundation/PxVec3.h"
#include "foundation/PxMat33.h"
#include "foundation/PxTransform.h"
#include "PxConstraintDesc.h"
#include "geomutils/PxContactPoint.h"

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment
#endif

#if !PX_DOXYGEN
namespace physx
{
#endif

struct PxTGSSolverBodyVel;

/**
\brief Struct that the solver uses to store velocity updates for a body
*/
struct PxSolverBody
{
	PX_ALIGN(16, PxVec3) linearVelocity;			//!< Delta linear velocity computed by the solver
	PxU16				maxSolverNormalProgress;	//!< Progress counter used by constraint batching and parallel island solver. 
	PxU16				maxSolverFrictionProgress;	//!< Progress counter used by constraint batching and parallel island solver.

	PxVec3				angularState;				//!< Delta angular velocity state computed by the solver.

	PxU32				solverProgress;				//!< Progress counter used by constraint batching and parallel island solver

	PxSolverBody() : linearVelocity(0.f), maxSolverNormalProgress(0), maxSolverFrictionProgress(0), angularState(0), solverProgress(0)
	{
	}
};
PX_COMPILE_TIME_ASSERT(sizeof(PxSolverBody) == 32);

/**
\brief Struct that the solver uses to store the state and other properties of a body
*/
struct PxSolverBodyData
{
	PX_ALIGN(16, PxVec3 linearVelocity);	//!< 12 Pre-solver linear velocity
	PxReal			invMass;				//!< 16 inverse mass
	PxVec3			angularVelocity;		//!< 28 Pre-solver angular velocity
	PxReal			reportThreshold;		//!< 32 contact force threshold
	PxMat33			sqrtInvInertia;			//!< 68 inverse inertia in world space
	PxReal			penBiasClamp;			//!< 72 the penetration bias clamp
	PxU32			nodeIndex;				//!< 76 the node idx of this solverBodyData. Used by solver to reference between solver bodies and island bodies. Not required by immediate mode
	PxReal			maxContactImpulse;		//!< 80 the max contact impulse
	PxTransform		body2World;				//!< 108 the body's transform
	PxU16			pad;					//!< 112 pad

	PX_FORCE_INLINE PxReal projectVelocity(const PxVec3& lin, const PxVec3& ang)	const
	{
		return linearVelocity.dot(lin) + angularVelocity.dot(ang);
	}
};
PX_COMPILE_TIME_ASSERT(0 == (sizeof(PxSolverBodyData) & 15));

//----------------------------------
/**
\brief A header that defines the size of a specific batch of constraints (of same type and without dependencies)
*/
struct PxConstraintBatchHeader
{
	PxU32	startIndex;			//!< Start index for this batch
	PxU16	stride;				//!< Number of constraints in this batch (range: 1-4)
	PxU16	constraintType;		//!< The type of constraint this batch references
};

/**
\brief Constraint descriptor used inside the solver
*/
struct PxSolverConstraintDesc
{
	static const PxU16 RIGID_BODY = 0xffff;

	enum ConstraintType
	{
		eCONTACT_CONSTRAINT,	//!< Defines this pair is a contact constraint
		eJOINT_CONSTRAINT		//!< Defines this pair is a joint constraint
	};

	union
	{
		PxSolverBody*		bodyA;			//!< bodyA pointer
		PxTGSSolverBodyVel*	tgsBodyA;		//!< bodyA pointer
		void*				articulationA;	//!< Articulation pointer for body A
	};

	union
	{
		PxSolverBody*		bodyB;			//!< BodyB pointer
		PxTGSSolverBodyVel*	tgsBodyB;		//!< BodyB pointer
		void*				articulationB;	//!< Articulation pointer for body B
	};
	PxU32	bodyADataIndex;			//!< Body A's index into the SolverBodyData array
	PxU32	bodyBDataIndex;			//!< Body B's index into the SolverBodyData array

	PxU32	linkIndexA;				//!< Link index defining which link in Articulation A this constraint affects. If not an articulation, must be NO_LINK
	PxU32	linkIndexB;				//!< Link index defining which link in Articulation B this constraint affects. If not an articulation, must be NO_LINK
	PxU8*	constraint;				//!< Pointer to the constraint rows to be solved
	void*	writeBack;				//!< Pointer to the writeback structure results for this given constraint are to be written to
	
	PxU16	progressA;				//!< Internal progress counter
	PxU16	progressB;				//!< Internal progress counter
	PxU16	constraintLengthOver16;	//!< constraintLength/16, max constraint length is 1MB, allows PxSolverConstraintDesc to fit in 32 bytes
	PxU8	padding[10];
};

/**
\brief Data structure used for preparing constraints before solving them
*/
struct PxSolverConstraintPrepDescBase
{
	enum BodyState
	{
		eDYNAMIC_BODY = 1 << 0,
		eSTATIC_BODY = 1 << 1,
		eKINEMATIC_BODY = 1 << 2,
		eARTICULATION = 1 << 3
	};

	PxConstraintInvMassScale invMassScales;	//!< In: The local mass scaling for this pair.

	PxSolverConstraintDesc* desc;			//!< Output: The PxSolverConstraintDesc filled in by contact prep

	const PxSolverBody* body0;				//!< In: The first body. Stores velocity information. Unused unless contact involves articulations.
	const PxSolverBody* body1;				//!< In: The second body. Stores velocity information. Unused unless contact involves articulations.

	const PxSolverBodyData* data0;			//!< In: The first PxSolverBodyData. Stores mass and miscellaneous information for the first body. 
	const PxSolverBodyData* data1;			//!< In: The second PxSolverBodyData. Stores mass and miscellaneous information for the second body

	PxTransform bodyFrame0;					//!< In: The world-space transform of the first body.
	PxTransform bodyFrame1;					//!< In: The world-space transform of the second body.

	BodyState bodyState0;					//!< In: Defines what kind of actor the first body is
	BodyState bodyState1;					//!< In: Defines what kind of actor the second body is
};

/**
\brief Data structure used for preparing constraints before solving them
*/
struct PxSolverConstraintPrepDesc : public PxSolverConstraintPrepDescBase
{
	PX_ALIGN(16, Px1DConstraint* rows);		//!< The start of the constraint rows
	PxU32 numRows;							//!< The number of rows

	PxReal linBreakForce, angBreakForce;	//!< Break forces
	PxReal minResponseThreshold;			//!< The minimum response threshold
	void* writeback;						//!< Pointer to constraint writeback structure. Reports back joint breaking. If not required, set to NULL.
	bool disablePreprocessing;				//!< Disable joint pre-processing. Pre-processing can improve stability but under certain circumstances, e.g. when some invInertia rows are zero/almost zero, can cause instabilities.	
	bool improvedSlerp;						//!< Use improved slerp model
	bool driveLimitsAreForces;				//!< Indicates whether drive limits are forces
	bool extendedLimits;					//!< Indicates whether we want to use extended limits
	bool disableConstraint;					//!< Disables constraint

	PxVec3p body0WorldOffset;				//!< Body0 world offset
};

/**
\brief Data structure used for preparing constraints before solving them
*/
struct PxSolverContactDesc : public PxSolverConstraintPrepDescBase
{
	void* shapeInteraction;					//!< Pointer to shape interaction. Used for force threshold reports in solver. Set to NULL if using immediate mode.
	PxContactPoint* contacts;				//!< The start of the contacts for this pair
	PxU32 numContacts;						//!< The total number of contacts this pair references.

	bool hasMaxImpulse;						//!< Defines whether this pairs has maxImpulses clamping enabled
	bool disableStrongFriction;				//!< Defines whether this pair disables strong friction (sticky friction correlation)
	bool hasForceThresholds;				//!< Defines whether this pair requires force thresholds	

	PxReal restDistance;					//!< A distance at which the solver should aim to hold the bodies separated. Default is 0
	PxReal maxCCDSeparation;				//!< A distance used to configure speculative CCD behavior. Default is PX_MAX_F32. Set internally in PhysX for bodies with eENABLE_SPECULATIVE_CCD on. Do not set directly!

	PxU8* frictionPtr;						//!< InOut: Friction patch correlation data. Set each frame by solver. Can be retained for improved behavior or discarded each frame.
	PxU8 frictionCount;						//!< The total number of friction patches in this pair

	PxReal* contactForces;					//!< Out: A buffer for the solver to write applied contact forces to.

	PxU32 startFrictionPatchIndex;			//!< Start index of friction patch in the correlation buffer. Set by friction correlation
	PxU32 numFrictionPatches;				//!< Total number of friction patches in this pair. Set by friction correlation

	PxU32 startContactPatchIndex;			//!< The start index of this pair's contact patches in the correlation buffer. For internal use only
	PxU16 numContactPatches;				//!< Total number of contact patches.
	PxU16 axisConstraintCount;				//!< Axis constraint count. Defines how many constraint rows this pair has produced. Useful for statistical purposes.

	PxReal offsetSlop;						//!< Slop value used to snap contact line of action back in-line with the COM.
	//PxU8 pad[16 - sizeof(void*)];
};

class PxConstraintAllocator
{
public:
	/**
	\brief Allocates constraint data. It is the application's responsibility to release this memory after PxSolveConstraints has completed.
	\param[in] byteSize Allocation size in bytes
	\return The allocated memory. This address must be 16-byte aligned.
	*/
	virtual PxU8* reserveConstraintData(const PxU32 byteSize) = 0;

	/**
	\brief Allocates friction data. Friction data can be retained by the application for a given pair and provided as an input to PxSolverContactDesc to improve simulation stability.
	It is the application's responsibility to release this memory. If this memory is released, the application should ensure it does not pass pointers to this memory to PxSolverContactDesc.
	\param[in] byteSize Allocation size in bytes
	\return The allocated memory. This address must be 4-byte aligned.
	*/
	virtual PxU8* reserveFrictionData(const PxU32 byteSize) = 0;

	virtual ~PxConstraintAllocator() {}
};

/** \addtogroup physics
@{ */
struct PxArticulationAxis
{
	enum Enum
	{
		eTWIST = 0,		//!< Rotational about eX
		eSWING1 = 1,	//!< Rotational about eY
		eSWING2 = 2,	//!< Rotational about eZ
		eX = 3,			//!< Linear in eX
		eY = 4,			//!< Linear in eY
		eZ = 5,			//!< Linear in eZ
		eCOUNT = 6
	};
};

PX_FLAGS_OPERATORS(PxArticulationAxis::Enum, PxU8)

struct PxArticulationMotion
{
	enum Enum
	{
		eLOCKED = 0,	//!< Locked axis, i.e. degree of freedom (DOF)
		eLIMITED = 1,	//!< Limited DOF - set limits of joint DOF together with this flag, see PxArticulationJointReducedCoordinate::setLimitParams
		eFREE = 2		//!< Free DOF
	};
};

typedef PxFlags<PxArticulationMotion::Enum, PxU8> PxArticulationMotions;
PX_FLAGS_OPERATORS(PxArticulationMotion::Enum, PxU8)

struct PxArticulationJointType
{
	enum Enum
	{
		eFIX = 0,					//!< All joint axes, i.e. degrees of freedom (DOFs) locked
		ePRISMATIC = 1,				//!< Single linear DOF, e.g. cart on a rail
		eREVOLUTE = 2,				//!< Single rotational DOF, e.g. an elbow joint or a rotational motor, position wrapped at 2pi radians
		eREVOLUTE_UNWRAPPED = 3,	//!< Single rotational DOF, e.g. an elbow joint or a rotational motor, position not wrapped
		eSPHERICAL = 4,				//!< Ball and socket joint with two or three DOFs
		eUNDEFINED = 5
	};
};

struct PxArticulationFlag
{
	enum Enum
	{
		eFIX_BASE = (1 << 0),				//!< Set articulation base to be fixed.
		eDRIVE_LIMITS_ARE_FORCES = (1<<1),	//!< Limits for drive effort are forces and torques rather than impulses, see PxArticulationDrive::maxForce.
		eDISABLE_SELF_COLLISION = (1<<2),	//!< Disable collisions between the articulation's links (note that parent/child collisions are disabled internally in either case).
		eCOMPUTE_JOINT_FORCES = (1<<3)		//!< Enable in order to be able to query joint solver (i.e. constraint) forces using PxArticulationCache::jointSolverForces.
	};
};

typedef PxFlags<PxArticulationFlag::Enum, PxU8> PxArticulationFlags;
PX_FLAGS_OPERATORS(PxArticulationFlag::Enum, PxU8)

struct PxArticulationDriveType
{
	enum Enum
	{
		eFORCE = 0,			//!< The output of the implicit spring drive controller is a force/torque.
		eACCELERATION = 1,	//!< The output of the implicit spring drive controller is a joint acceleration (use this to get (spatial)-inertia-invariant behavior of the drive).
		eTARGET = 2,		//!< Sets the drive gains internally to track a target position almost kinematically (i.e. with very high drive gains).
		eVELOCITY = 3,		//!< Sets the drive gains internally to track a target velocity almost kinematically (i.e. with very high drive gains).
		eNONE = 4
	};
};

/**
\brief Data structure to set articulation joint limits.

- The lower limit should be strictly smaller than the higher limit. If the limits should be equal, use PxArticulationMotion::eLOCKED
and an appropriate offset in the parent/child joint frames.
- The limit units are linear units (equivalent to scene units) for a translational axis, or radians for a rotational axis.

@see PxArticulationJointReducedCoordinate::setLimitParams, PxArticulationReducedCoordinate
*/
struct PxArticulationLimit
{
	PxArticulationLimit(){}

	PxArticulationLimit(const PxReal low_, const PxReal high_)
	{
		low = low_;
		high = high_;
	}

	/**
	\brief The lower limit on the joint axis position.

	<b>Range:</b> [-PX_MAX_F32, high)<br>
	<b>Default:</b> 0.0f<br>
	*/
	PxReal low;

	/**
	\brief The higher limit on the joint axis position.

	<b>Range:</b> (low, PX_MAX_F32]<br>
	<b>Default:</b> 0.0f<br>
	*/
	PxReal high;
};

/**
\brief Data structure for articulation joint drive configuration.

@see PxArticulationJointReducedCoordinate::setDriveParams, PxArticulationReducedCoordinate
*/
struct PxArticulationDrive
{
	PxArticulationDrive(){}

	PxArticulationDrive(const PxReal stiffness_, const PxReal damping_, const PxReal maxForce_, PxArticulationDriveType::Enum driveType_=PxArticulationDriveType::eFORCE)
	{
		stiffness = stiffness_;
		damping = damping_;
		maxForce = maxForce_;
		driveType = driveType_;
	}
	
	/**
	\brief The drive stiffness, i.e. the proportional gain of the implicit PD controller.

	See manual for further information, and the drives' implicit spring-damper (i.e. PD control) implementation in particular.

	<b>Units:</b> (distance = linear scene units)<br>
	Rotational axis: torque/rad if driveType = PxArticulationDriveType::eFORCE; or (rad/s^2)/rad if driveType = PxArticulationDriveType::eACCELERATION<br>
	Translational axis: force/distance if driveType = PxArticulationDriveType::eFORCE; or (distance/s^2)/distance if driveType = PxArticulationDriveType::eACCELERATION<br>
	<b>Range:</b> [0, PX_MAX_F32]<br>
	<b>Default:</b> 0.0f<br>
	*/
	PxReal stiffness;

	/**
	\brief The drive damping, i.e. the derivative gain of the implicit PD controller.

	See manual for further information, and the drives' implicit spring-damper (i.e. PD control) implementation in particular.

	<b>Units:</b> (distance = linear scene units)<br>
	Rotational axis: torque/(rad/s) if driveType = PxArticulationDriveType::eFORCE; or (rad/s^2)/(rad/s) if driveType = PxArticulationDriveType::eACCELERATION<br>
	Translational axis: force/(distance/s) if driveType = PxArticulationDriveType::eFORCE; or (distance/s^2)/(distance/s) if driveType = PxArticulationDriveType::eACCELERATION<br>
	<b>Range:</b> [0, PX_MAX_F32]<br>
	<b>Default:</b> 0.0f<br>
	*/
	PxReal damping;
	
	/**
	\brief The drive force limit.

	- The limit is enforced regardless of the drive type #PxArticulationDriveType.
	- The limit corresponds to a force (linear axis) or torque (rotational axis) if PxArticulationFlag::eDRIVE_LIMITS_ARE_FORCES is set, and to an impulse (force|torque * dt) otherwise.

	<b>Range:</b> [0, PX_MAX_F32]<br>
	<b>Default:</b> 0.0f<br>

	@see PxArticulationFlag::eDRIVE_LIMITS_ARE_FORCES
	*/
	PxReal maxForce;
	
	/**
	\brief The drive type.

	@see PxArticulationDriveType
	*/
	PxArticulationDriveType::Enum driveType;
};
/** @} */

struct PxTGSSolverBodyVel
{
	PX_ALIGN(16, PxVec3) linearVelocity;	//12
	PxU16			nbStaticInteractions;	//14 Used to accumulate the number of static interactions
	PxU16			maxDynamicPartition;	//16 Used to accumulate the max partition of dynamic interactions
	PxVec3			angularVelocity;		//28
	PxU32			partitionMask;			//32 Used in partitioning as a bit-field
	PxVec3			deltaAngDt;				//44
	PxReal			maxAngVel;				//48
	PxVec3			deltaLinDt;				//60
	PxU16			lockFlags;				//62
	bool			isKinematic;			//63
	PxU8			pad;					//64

	PX_FORCE_INLINE PxReal projectVelocity(const PxVec3& lin, const PxVec3& ang)	const
	{
		return linearVelocity.dot(lin) + angularVelocity.dot(ang);
	}
};

//Needed only by prep, integration and 1D constraints
struct PxTGSSolverBodyTxInertia
{
	PxTransform deltaBody2World;
	PxMat33 sqrtInvInertia;							//!< inverse inertia in world space
};

struct PxTGSSolverBodyData
{
	PX_ALIGN(16, PxVec3) originalLinearVelocity;	//!< Pre-solver linear velocity.
	PxReal maxContactImpulse;						//!< The max contact impulse.
	PxVec3 originalAngularVelocity;					//!< Pre-solver angular velocity
	PxReal penBiasClamp;							//!< The penetration bias clamp.

	PxReal invMass;									//!< Inverse mass.
	PxU32 nodeIndex;								//!< The node idx of this solverBodyData. Used by solver to reference between solver bodies and island bodies. Not required by immediate mode.
	PxReal reportThreshold;							//!< Contact force threshold.
	PxU32 pad;

	PxReal projectVelocity(const PxVec3& linear, const PxVec3& angular) const
	{
		return originalLinearVelocity.dot(linear) + originalAngularVelocity.dot(angular);
	}
};

struct PxTGSSolverConstraintPrepDescBase
{
	PxConstraintInvMassScale invMassScales;		//!< In: The local mass scaling for this pair.

	PxSolverConstraintDesc* desc;				//!< Output: The PxSolverConstraintDesc filled in by contact prep

	const PxTGSSolverBodyVel* body0;			//!< In: The first body. Stores velocity information. Unused unless contact involves articulations.
	const PxTGSSolverBodyVel* body1;			//!< In: The second body. Stores velocity information. Unused unless contact involves articulations.

	const PxTGSSolverBodyTxInertia* body0TxI;	//!< In: The first PxTGSSolverBodyTxInertia. Stores the delta body to world transform and sqrtInvInertia for first body.
	const PxTGSSolverBodyTxInertia* body1TxI;   //!< In: The second PxTGSSolverBodyTxInertia. Stores the delta body to world transform and sqrtInvInertia for second body.

	const PxTGSSolverBodyData* bodyData0;		//!< In: The first PxTGSSolverBodyData. Stores mass and miscellaneous information for the first body. 
	const PxTGSSolverBodyData* bodyData1;		//!< In: The second PxTGSSolverBodyData. Stores mass and miscellaneous information for the second body.

	PxTransform bodyFrame0;						//!< In: The world-space transform of the first body.
	PxTransform bodyFrame1;						//!< In: The world-space transform of the second body.

	PxSolverContactDesc::BodyState bodyState0;	//!< In: Defines what kind of actor the first body is
	PxSolverContactDesc::BodyState bodyState1;	//!< In: Defines what kind of actor the second body is
};

struct PxTGSSolverConstraintPrepDesc : public PxTGSSolverConstraintPrepDescBase
{
	Px1DConstraint* rows;					//!< The start of the constraint rows
	PxU32 numRows;							//!< The number of rows

	PxReal linBreakForce, angBreakForce;	//!< Break forces
	PxReal minResponseThreshold;			//!< The minimum response threshold
	void* writeback;						//!< Pointer to constraint writeback structure. Reports back joint breaking. If not required, set to NULL.
	bool disablePreprocessing;				//!< Disable joint pre-processing. Pre-processing can improve stability but under certain circumstances, e.g. when some invInertia rows are zero/almost zero, can cause instabilities.	
	bool improvedSlerp;						//!< Use improved slerp model
	bool driveLimitsAreForces;				//!< Indicates whether drive limits are forces
	bool extendedLimits;					//!< Indicates whether extended limits are used
	bool disableConstraint;					//!< Disables constraint

	PxVec3p body0WorldOffset;				//!< Body0 world offset
	PxVec3p cA2w;							//!< Location of anchor point A in world space
	PxVec3p cB2w;							//!< Location of anchor point B in world space
};

struct PxTGSSolverContactDesc : public PxTGSSolverConstraintPrepDescBase
{
	void* shapeInteraction;					//!< Pointer to shape interaction. Used for force threshold reports in solver. Set to NULL if using immediate mode.
	PxContactPoint* contacts;				//!< The start of the contacts for this pair
	PxU32 numContacts;						//!< The total number of contacts this pair references.

	bool hasMaxImpulse;						//!< Defines whether this pairs has maxImpulses clamping enabled
	bool disableStrongFriction;				//!< Defines whether this pair disables strong friction (sticky friction correlation)
	bool hasForceThresholds;				//!< Defines whether this pair requires force thresholds	

	PxReal restDistance;					//!< A distance at which the solver should aim to hold the bodies separated. Default is 0
	PxReal maxCCDSeparation;				//!< A distance used to configure speculative CCD behavior. Default is PX_MAX_F32. Set internally in PhysX for bodies with eENABLE_SPECULATIVE_CCD on. Do not set directly!

	PxU8* frictionPtr;						//!< InOut: Friction patch correlation data. Set each frame by solver. Can be retained for improved behavior or discarded each frame.
	PxU8 frictionCount;						//!< The total number of friction patches in this pair

	PxReal* contactForces;					//!< Out: A buffer for the solver to write applied contact forces to.

	PxU32 startFrictionPatchIndex;			//!< Start index of friction patch in the correlation buffer. Set by friction correlation
	PxU32 numFrictionPatches;				//!< Total number of friction patches in this pair. Set by friction correlation

	PxU32 startContactPatchIndex;			//!< The start index of this pair's contact patches in the correlation buffer. For internal use only
	PxU16 numContactPatches;				//!< Total number of contact patches.
	PxU16 axisConstraintCount;				//!< Axis constraint count. Defines how many constraint rows this pair has produced. Useful for statistical purposes.

	PxReal maxImpulse;						//!< The maximum impulse the solver is allowed to introduce for this pair of bodies.

	PxReal torsionalPatchRadius;			//!< This defines the radius of the contact patch used to apply torsional friction.
	PxReal minTorsionalPatchRadius;         //!< This defines the minimum radius of the contact patch used to apply torsional friction.
	PxReal offsetSlop;						//!< Slop value used to snap contact line of action back in-line with the COM.
};

#if !PX_DOXYGEN
}
#endif

#if PX_VC
#pragma warning(pop)
#endif

#endif

