//
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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PX_PHYSICS_IMMEDIATE_MODE
#define PX_PHYSICS_IMMEDIATE_MODE
/** \addtogroup immediatemode
@{ */

#include "PxPhysXConfig.h"
#include "solver/PxSolverDefs.h"
#include "collision/PxCollisionDefs.h"
#include "PxArticulationReducedCoordinate.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if !PX_DOXYGEN
namespace immediate
{
#endif

	/**
	\brief Structure to store rigid body properties
	*/
	struct PxRigidBodyData
	{
		PX_ALIGN(16, PxVec3 linearVelocity);	//!< 12 Linear velocity
		PxReal invMass;							//!< 16 Inverse mass
		PxVec3 angularVelocity;					//!< 28 Angular velocity
		PxReal maxDepenetrationVelocity;		//!< 32 Maximum de-penetration velocity
		PxVec3 invInertia;						//!< 44 Mass-space inverse interia diagonal vector
		PxReal maxContactImpulse;				//!< 48 Maximum permissable contact impulse
		PxTransform body2World;					//!< 76 World space transform
		PxReal linearDamping;					//!< 80 Linear damping coefficient
		PxReal angularDamping;					//!< 84 Angular damping coefficient
		PxReal maxLinearVelocitySq;				//!< 88 Squared maximum linear velocity
		PxReal maxAngularVelocitySq;			//!< 92 Squared maximum angular velocity
		PxU32 pad;								//!< 96 Padding for 16-byte alignment
	};

	/**
	\brief Callback class to record contact points produced by immediate::PxGenerateContacts
	*/
	class PxContactRecorder
	{
	public:
		/**
		\brief Method to record new contacts
		\param	[in] contactPoints	The contact points produced
		\param	[in] nbContacts		The number of contact points produced
		\param	[in] index			The index of this pair. This is an index from 0-N-1 identifying which pair this relates to from within the array of pairs passed to PxGenerateContacts
		\return a boolean to indicate if this callback successfully stored the contacts or not.
		*/
		virtual bool recordContacts(const Gu::ContactPoint* contactPoints, const PxU32 nbContacts, const PxU32 index) = 0;

		virtual ~PxContactRecorder(){}
	};

	/**
	\brief Constructs a PxSolverBodyData structure based on rigid body properties. Applies gravity, damping and clamps maximum velocity.
	\param	[in] inRigidData		The array rigid body properties
	\param	[out] outSolverBodyData	The array of solverBodyData produced to repreent these bodies
	\param	[in] nbBodies			The total number of solver bodies to create
	\param	[in] gravity			The gravity vector
	\param	[in] dt					The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructSolverBodies(const PxRigidBodyData* inRigidData, PxSolverBodyData* outSolverBodyData, const PxU32 nbBodies, const PxVec3& gravity, const PxReal dt);

	/**
	\brief Constructs a PxSolverBodyData structure for a static body at a given pose.
	\param	[in] globalPose			The pose of this static actor
	\param	[out] solverBodyData	The solver body representation of this static actor
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructStaticSolverBody(const PxTransform& globalPose, PxSolverBodyData& solverBodyData);

	/**
	\brief Groups together sets of independent PxSolverConstraintDesc objects to be solved using SIMD SOA approach.
	\param	[in] solverConstraintDescs		The set of solver constraint descs to batch
	\param	[in] nbConstraints				The number of constraints to batch
	\param	[in,out] solverBodies			The array of solver bodies that the constraints reference. Some fields in these structures are written to as scratch memory for the batching.
	\param	[in] nbBodies					The number of bodies
	\param	[out] outBatchHeaders			The batch headers produced by this batching process. This array must have at least 1 entry per input constraint
	\param	[out] outOrderedConstraintDescs	A reordered copy of the constraint descs. This array is referenced by the constraint batches. This array must have at least 1 entry per input constraint.
	\param	[in,out] articulations			The array of articulations that the constraints reference. Some fields in these structures are written to as scratch memory for the batching.
	\param	[in] nbArticulations			The number of articulations
	\return The total number of batches produced. This should be less than or equal to nbConstraints.

	\note	This method considers all bodies within the range [0, nbBodies-1] to be valid dynamic bodies. A given dynamic body can only be referenced in a batch once. Static or kinematic bodies can be
			referenced multiple times within a batch safely because constraints do not affect their velocities. The batching will implicitly consider any bodies outside of the range [0, nbBodies-1] to be 
			infinite mass (static or kinematic). This means that either appending static/kinematic to the end of the array of bodies or placing static/kinematic bodies at before the start body pointer
			will ensure that the minimum number of batches are produced.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32 PxBatchConstraints(	const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxSolverBody* solverBodies, const PxU32 nbBodies,
															PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
															Dy::ArticulationV** articulations=NULL, const PxU32 nbArticulations=0);

	/**	
	\brief Creates a set of contact constraint blocks. Note that, depending the results of PxBatchConstraints, each batchHeader may refer to up to 4 solverConstraintDescs.
	This function will allocate both constraint and friction patch data via the PxConstraintAllocator provided. Constraint data is only valid until PxSolveConstraints has completed. 
	Friction data is to be retained and provided by the application for friction correlation.

	\param	[in] batchHeaders				Array of batch headers to process
	\param	[in] nbHeaders					The total number of headers
	\param	[in] contactDescs				An array of contact descs defining the pair and contact properties of each respective contacting pair
	\param	[in] allocator					An allocator callback to allocate constraint and friction memory
	\param	[in] invDt						The inverse timestep
	\param	[in] bounceThreshold			The bounce threshold. Relative velocities below this will be solved by bias only. Relative velocities above this will be solved by restitution. If restitution is zero
											then these pairs will always be solved by bias.
	\param	[in] frictionOffsetThreshold	The friction offset threshold. Contacts whose separations are below this threshold can generate friction constraints.
	\param	[in] correlationDistance		The correlation distance used by friction correlation to identify whether a friction patch is broken on the grounds of relation separation.

	\return a boolean to define if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateContactConstraints(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxSolverContactDesc* contactDescs,
		PxConstraintAllocator& allocator, const PxReal invDt, const PxReal bounceThreshold, const PxReal frictionOffsetThreshold, const PxReal correlationDistance);

	/**
	\brief Creates a set of joint constraint blocks. Note that, depending the results of PxBatchConstraints, the batchHeader may refer to up to 4 solverConstraintDescs
	\param	[in] batchHeaders	The array of batch headers to be processed
	\param	[in] nbHeaders		The total number of batch headers to process
	\param	[in] jointDescs		An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The timestep
	\param	[in] invDt			The inverse timestep
	\return a boolean indicating if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraints(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt);

	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxConstraint** param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The timestep
	\param	[in] invDt			The inverse timestep
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithShaders(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxConstraint** constraints, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt);

	struct PxImmediateConstraint
	{
		PxConstraintSolverPrep	prep;
		const void*				constantBlock;
	};
	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxImmediateConstraint* param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The timestep
	\param	[in] invDt			The inverse timestep
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithImmediateShaders(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxImmediateConstraint* constraints, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt);

	/**
	\brief Iteratively solves the set of constraints defined by the provided PxConstraintBatchHeader and PxSolverConstraintDesc structures. Updates deltaVelocities inside the PxSolverBody structures. Produces resulting linear and angular motion velocities.
	\param	[in] batchHeaders			The set of batch headers to be solved
	\param	[in] nbBatchHeaders			The total number of batch headers to be solved
	\param	[in] solverConstraintDescs	The reordererd set of solver constraint descs referenced by the batch headers
	\param	[in,out] solverBodies		The set of solver bodies the bodies reference
	\param	[out] linearMotionVelocity	The resulting linear motion velocity
	\param	[out] angularMotionVelocity	The resulting angular motion velocity.
	\param	[in] nbSolverBodies			The total number of solver bodies
	\param	[in] nbPositionIterations	The number of position iterations to run
	\param	[in] nbVelocityIterations	The number of velocity iterations to run
	\param	[in] dt						Timestep. Only needed if articulations are sent to the function.
	\param	[in] invDt					Inverse timestep. Only needed if articulations are sent to the function.
	\param	[in] nbSolverArticulations	Number of articulations to solve constraints for.
	\param	[in] solverArticulations	Array of articulations to solve constraints for.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxSolveConstraints(const PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
		const PxSolverBody* solverBodies, PxVec3* linearMotionVelocity, PxVec3* angularMotionVelocity, const PxU32 nbSolverBodies, const PxU32 nbPositionIterations, const PxU32 nbVelocityIterations,
		const float dt=0.0f, const float invDt=0.0f, const PxU32 nbSolverArticulations=0, Dy::ArticulationV** solverArticulations=NULL);

	/**
	\brief Integrates a rigid body, returning the new velocities and transforms. After this function has been called, solverBodyData stores all the body's velocity data.

	\param	[in,out] solverBodyData		The array of solver body data to be integrated
	\param	[in] solverBody				The bodies' linear and angular velocities
	\param	[in] linearMotionVelocity	The bodies' linear motion velocity array
	\param	[in] angularMotionState		The bodies' angular motion velocity array
	\param	[in] nbBodiesToIntegrate	The total number of bodies to integrate
	\param	[in] dt						The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxIntegrateSolverBodies(PxSolverBodyData* solverBodyData, PxSolverBody* solverBody, const PxVec3* linearMotionVelocity, const PxVec3* angularMotionState, const PxU32 nbBodiesToIntegrate, const PxReal dt);

	/**
	\brief Performs contact generation for a given pair of geometries at the specified poses. Produced contacts are stored in the provided Gu::ContactBuffer. Information is cached in PxCache structure
	to accelerate future contact generation between pairs. This cache data is valid only as long as the memory provided by PxCacheAllocator has not been released/re-used. Recommendation is to 
	retain that data for a single simulation frame, discarding cached data after 2 frames. If the cached memory has been released/re-used prior to the corresponding pair having contact generation
	performed again, it is the application's responsibility to reset the PxCache.

	\param	[in] geom0				Array of geometries to perform collision detection on.
	\param	[in] geom1				Array of geometries to perform collision detection on
	\param	[in] pose0				Array of poses associated with the corresponding entry in the geom0 array
	\param	[in] pose1				Array of poses associated with the corresponding entry in the geom1 array
	\param	[in,out] contactCache	Array of contact caches associated with each pair geom0[i] + geom1[i]
	\param	[in] nbPairs			The total number of pairs to process
	\param	[in] contactRecorder	A callback that is called to record contacts for each pair that detects contacts
	\param	[in] contactDistance	The distance at which contacts begin to be generated between the pairs
	\param	[in] meshContactMargin	The mesh contact margin.
	\param	[in] toleranceLength	The toleranceLength. Used for scaling distance-based thresholds internally to produce appropriate results given simulations in different units
	\param	[in] allocator			A callback to allocate memory for the contact cache

	\return a boolean indicating if the function was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxGenerateContacts(const PxGeometry* const * geom0, const PxGeometry* const * geom1, const PxTransform* pose0, const PxTransform* pose1, PxCache* contactCache, const PxU32 nbPairs, PxContactRecorder& contactRecorder,
		const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength, PxCacheAllocator& allocator);

	/**
	\brief Register articulation-related solver functions. This is equivalent to PxRegisterArticulationsReducedCoordinate() for PxScene-level articulations.
	Call this first to enable reduced coordinates articulations in immediate mode.

	@see PxRegisterArticulationsReducedCoordinate
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxRegisterImmediateArticulations();

	struct PxArticulationJointData
	{
		PxTransform	parentPose;
		PxTransform	childPose;
	};

	struct PxFeatherstoneArticulationJointData : PxArticulationJointData
	{
		PxArticulationJointType::Enum	type;
		PxArticulationMotion::Enum		motion[PxArticulationAxis::eCOUNT];
		PxArticulationLimit				limits[PxArticulationAxis::eCOUNT];
		PxArticulationDrive				drives[PxArticulationAxis::eCOUNT];
		PxReal							targetPos[PxArticulationAxis::eCOUNT];
		PxReal							targetVel[PxArticulationAxis::eCOUNT];
		PxReal							frictionCoefficient;
		PxReal							maxJointVelocity;
	};

	struct PxFeatherstoneArticulationData
	{
		PxArticulationFlags		flags;
	};

	struct PxMutableLinkData
	{
		PxVec3	inverseInertia;
		float	inverseMass;
		float	linearDamping;
		float	angularDamping;
		float	maxLinearVelocitySq;
		float	maxAngularVelocitySq;
		bool	disableGravity;
	};

	struct PxLinkData
	{
		PxTransform	pose;
		PxVec3		linearVelocity;
		PxVec3		angularVelocity;
	};

	struct PxFeatherstoneArticulationLinkData : PxMutableLinkData
	{
		PxFeatherstoneArticulationLinkData()	{ initData();	}

		void	initData()
		{
			inboundJoint.type					= PxArticulationJointType::eUNDEFINED;	// For root
			inboundJoint.parentPose				= PxTransform(PxIdentity);
			inboundJoint.childPose				= PxTransform(PxIdentity);
			inboundJoint.frictionCoefficient	= 0.05f;
			inboundJoint.maxJointVelocity		= 100.0f;

			pose					= PxTransform(PxIdentity);
			parent					= 0;
			inverseInertia			= PxVec3(1.0f);
			inverseMass				= 1.0f;
			linearDamping			= 0.05f;
			angularDamping			= 0.05f;
			maxLinearVelocitySq		= 100.0f * 100.0f;
			maxAngularVelocitySq	= 50.0f * 50.0f;
			disableGravity			= false;

			for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
			{
				inboundJoint.motion[i] = PxArticulationMotion::eLOCKED;
				inboundJoint.limits[i].low = inboundJoint.limits[i].high = 0.0f;
				inboundJoint.drives[i].stiffness = 0.0f;
				inboundJoint.drives[i].damping = 0.0f;
				inboundJoint.drives[i].maxForce = 0.0f;
				inboundJoint.drives[i].driveType = PxArticulationDriveType::eFORCE;
			}
			memset(inboundJoint.targetPos, 0xff, sizeof(PxReal)*PxArticulationAxis::eCOUNT);
			memset(inboundJoint.targetVel, 0xff, sizeof(PxReal)*PxArticulationAxis::eCOUNT);
		}

		PxFeatherstoneArticulationJointData	inboundJoint;

		// Non-mutable link data
		PxTransform					pose;
		Dy::ArticulationLinkHandle	parent;
	};

	/**
	\brief Creates an immediate-mode reduced-coordinate articulation.
	\param	[in] data	Articulation data
	\return	Articulation handle

	@see PxReleaseArticulation
	*/
	// PT: Design note: I use Dy::ArticulationV* to be consistent with PxSolverDefs.h
	PX_C_EXPORT PX_PHYSX_CORE_API Dy::ArticulationV*	PxCreateFeatherstoneArticulation(const PxFeatherstoneArticulationData& data);

	/**
	\brief Releases an immediate-mode reduced-coordinate articulation.
	\param	[in] articulation	Articulation handle

	@see PxCreateFeatherstoneArticulation
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void	PxReleaseArticulation(Dy::ArticulationV* articulation);

	/**
	\brief Creates an articulation cache.
	\param	[in] articulation	Articulation handle
	\return	Articulation cache

	@see PxReleaseArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxArticulationCache* PxCreateArticulationCache(Dy::ArticulationV* articulation);
	

	/**
	\brief Copy the internal data of the articulation to the cache
	\param[in] articulation		articulation handle.
	\param[in] cache	Articulation data
	\param[in] flag		Indicates which values of the articulation system are copied to the cache

	@see createCache PxApplyArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxCopyInternalStateToArticulationCache(Dy::ArticulationV* articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag);
	
	/**
	\brief Apply the user defined data in the cache to the articulation system
	\param[in] articulation 	articulation handle.
	\param[in] cache 	Articulation data.
	\param[in] flag		Defines which values in the cache will be applied to the articulation

	@see createCache PxCopyInternalStateToArticulationCache
	*/	
	PX_C_EXPORT PX_PHYSX_CORE_API void PxApplyArticulationCache(Dy::ArticulationV* articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag);

	/**
	\brief Release an articulation cache

	\param[in] cache	The cache to release

	@see PxCreateArticulationCache PxCopyInternalStateToArticulationCache PxCopyInternalStateToArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxReleaseArticulationCache(PxArticulationCache& cache);

	/**
	\brief Adds a link to an immediate-mode reduced-coordinate articulation. The articulation becomes the link's owner/parent.
	\param	[in] articulation	Articulation handle
	\param	[in] data			Link data
	\param	[in] isLastLink		Hint to tell the system whether the added link is the last one or not.
								This is a minor optimization, it is also ok to always use either true or false here.
	\return	Articulation link handle
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API Dy::ArticulationLinkHandle	PxAddArticulationLink(Dy::ArticulationV* articulation, const PxFeatherstoneArticulationLinkData& data, bool isLastLink=false);

	/**
	\brief Retrieves owner/parent articulation handle from a link handle.
	\param	[in] link			Link handle
	\return	Articulation handle
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API Dy::ArticulationV*	PxGetLinkArticulation(const Dy::ArticulationLinkHandle link);

	/**
	\brief Retrieves link index from a link handle.
	\param	[in] link			Link handle
	\return	Link index
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32	PxGetLinkIndex(const Dy::ArticulationLinkHandle link);

	/**
	\brief Retrieves non-mutable link data from a link handle.
	The data here is computed by the articulation code but cannot be directly changed by users.
	\param	[in] link			Link handle
	\param	[out] data			Link data
	\return	True if success

	@see PxGetAllLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool	PxGetLinkData(const Dy::ArticulationLinkHandle link, PxLinkData& data);

	/**
	\brief Retrieves non-mutable link data from an articulation handle (all links).
	The data here is computed by the articulation code but cannot be directly changed by users.
	\param	[in] articulation	Articulation handle
	\param	[out] data			Link data for N links, or NULL to just retrieve the number of links.
	\return	Number of links in the articulation = number of link data structure written to the data array.

	@see PxGetLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32	PxGetAllLinkData(const Dy::ArticulationV* articulation, PxLinkData* data);

	/**
	\brief Retrieves mutable link data from a link handle.
	\param	[in] link	Link handle
	\param	[out] data	Data for this link
	\return	True if success

	@see PxSetMutableLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool	PxGetMutableLinkData(const Dy::ArticulationLinkHandle link, PxMutableLinkData& data);

	/**
	\brief Sets mutable link data for given link.
	\param	[in] link	Link handle
	\param	[in] data	Data for this link
	\return	True if success

	@see PxGetMutableLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool	PxSetMutableLinkData(Dy::ArticulationLinkHandle link, const PxMutableLinkData& data);

	/**
	\brief Retrieves joint data from a link handle.
	\param	[in] link	Link handle
	\param	[out] data	Joint data for this link
	\return	True if success

	@see PxSetJointData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool	PxGetJointData(const Dy::ArticulationLinkHandle link, PxFeatherstoneArticulationJointData& data);

	/**
	\brief Sets joint data for given link.
	\param	[in] link	Link handle
	\param	[in] data	Joint data for this link
	\return	True if success

	@see PxGetJointData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool	PxSetJointData(Dy::ArticulationLinkHandle link, const PxFeatherstoneArticulationJointData& data);

	/**
	\brief Computes unconstrained velocities for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] gravity		Gravity vector
	\param	[in] dt				Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void	PxComputeUnconstrainedVelocities(Dy::ArticulationV* articulation, const PxVec3& gravity, const PxReal dt);

	/**
	\brief Updates bodies for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] dt				Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void	PxUpdateArticulationBodies(Dy::ArticulationV* articulation, const PxReal dt);

	/**
	\brief Computes unconstrained velocities for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] gravity		Gravity vector
	\param	[in] dt				Timestep/numPosIterations
	\param	[in] totalDt		Timestep
	\param	[in] invDt			1/(Timestep/numPosIterations)
	\param	[in] invTotalDt		1/Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void	PxComputeUnconstrainedVelocitiesTGS(Dy::ArticulationV* articulation, const PxVec3& gravity, const PxReal dt, const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt);

	/**
	\brief Updates bodies for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] dt				Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void	PxUpdateArticulationBodiesTGS(Dy::ArticulationV* articulation, const PxReal dt);


	/**
	\brief Constructs a PxSolverBodyData structure based on rigid body properties. Applies gravity, damping and clamps maximum velocity.
	\param	[in] inRigidData		The array rigid body properties
	\param	[out] outSolverBodyVel	The array of PxTGSSolverBodyVel structures produced to represent these bodies
	\param	[out] outSolverBodyTxInertia The array of PxTGSSolverBodyTxInertia produced to represent these bodies
	\param	[out] outSolverBodyData	The array of PxTGSolverBodyData produced to repreent these bodies
	\param	[in] nbBodies			The total number of solver bodies to create
	\param	[in] gravity			The gravity vector
	\param	[in] dt					The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructSolverBodiesTGS(const PxRigidBodyData* inRigidData, PxTGSSolverBodyVel* outSolverBodyVel, PxTGSSolverBodyTxInertia* outSolverBodyTxInertia, PxTGSSolverBodyData* outSolverBodyData, const PxU32 nbBodies, const PxVec3& gravity, const PxReal dt);

	/**
	\brief Constructs a PxSolverBodyData structure for a static body at a given pose.
	\param	[in] globalPose			The pose of this static actor
	\param	[out] solverBodyVel		The velocity component of this body (will be zero)
	\param	[out] solverBodyTxInertia The intertia and transform delta component of this body (will be zero)
	\param	[out] solverBodyData	The solver body representation of this static actor
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructStaticSolverBodyTGS(const PxTransform& globalPose, PxTGSSolverBodyVel& solverBodyVel, PxTGSSolverBodyTxInertia& solverBodyTxInertia, PxTGSSolverBodyData& solverBodyData);

	/**
	\brief Groups together sets of independent PxSolverConstraintDesc objects to be solved using SIMD SOA approach.
	\param	[in] solverConstraintDescs		The set of solver constraint descs to batch
	\param	[in] nbConstraints				The number of constraints to batch
	\param	[in,out] solverBodies			The array of solver bodies that the constraints reference. Some fields in these structures are written to as scratch memory for the batching.
	\param	[in] nbBodies					The number of bodies
	\param	[out] outBatchHeaders			The batch headers produced by this batching process. This array must have at least 1 entry per input constraint
	\param	[out] outOrderedConstraintDescs	A reordered copy of the constraint descs. This array is referenced by the constraint batches. This array must have at least 1 entry per input constraint.
	\param	[in,out] articulations			The array of articulations that the constraints reference. Some fields in these structures are written to as scratch memory for the batching.
	\param	[in] nbArticulations			The number of articulations
	\return The total number of batches produced. This should be less than or equal to nbConstraints.

	\note	This method considers all bodies within the range [0, nbBodies-1] to be valid dynamic bodies. A given dynamic body can only be referenced in a batch once. Static or kinematic bodies can be
	referenced multiple times within a batch safely because constraints do not affect their velocities. The batching will implicitly consider any bodies outside of the range [0, nbBodies-1] to be
	infinite mass (static or kinematic). This means that either appending static/kinematic to the end of the array of bodies or placing static/kinematic bodies at before the start body pointer
	will ensure that the minimum number of batches are produced.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32 PxBatchConstraintsTGS(const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxTGSSolverBodyVel* solverBodies, const PxU32 nbBodies,
		PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
		Dy::ArticulationV** articulations = NULL, const PxU32 nbArticulations = 0);


	/**
	\brief Creates a set of contact constraint blocks. Note that, depending the results of PxBatchConstraints, each batchHeader may refer to up to 4 solverConstraintDescs.
	This function will allocate both constraint and friction patch data via the PxConstraintAllocator provided. Constraint data is only valid until PxSolveConstraints has completed.
	Friction data is to be retained and provided by the application for friction correlation.

	\param	[in] batchHeaders				Array of batch headers to process
	\param	[in] nbHeaders					The total number of headers
	\param	[in] contactDescs				An array of contact descs defining the pair and contact properties of each respective contacting pair
	\param	[in] allocator					An allocator callback to allocate constraint and friction memory
	\param	[in] invDt						The inverse timestep/nbPositionIterations
	\param	[in] invTotalDt					The inverse time-step
	\param	[in] bounceThreshold			The bounce threshold. Relative velocities below this will be solved by bias only. Relative velocities above this will be solved by restitution. If restitution is zero
	then these pairs will always be solved by bias.
	\param	[in] frictionOffsetThreshold	The friction offset threshold. Contacts whose separations are below this threshold can generate friction constraints.
	\param	[in] correlationDistance		The correlation distance used by friction correlation to identify whether a friction patch is broken on the grounds of relation separation.

	\return a boolean to define if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateContactConstraintsTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxTGSSolverContactDesc* contactDescs,
		PxConstraintAllocator& allocator, const PxReal invDt, const PxReal invTotalDt, const PxReal bounceThreshold, const PxReal frictionOffsetThreshold, const PxReal correlationDistance);

	/**
	\brief Creates a set of joint constraint blocks. Note that, depending the results of PxBatchConstraints, the batchHeader may refer to up to 4 solverConstraintDescs
	\param	[in] batchHeaders	The array of batch headers to be processed
	\param	[in] nbHeaders		The total number of batch headers to process
	\param	[in] jointDescs		An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The total time-step/nbPositionIterations
	\param	[in] totalDt		The total time-step
	\param	[in] invDt			The inverse (timestep/nbPositionIterations)
	\param	[in] invTotalDt		The inverse total time-step
	\param	[in] lengthScale	PxToleranceScale::length, i.e. a meter in simulation units
	\return a boolean indicating if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders,
		PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal totalDt, const PxReal invDt,
		const PxReal invTotalDt, const PxReal lengthScale);

	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxConstraint** param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The total time-step/nbPositionIterations
	\param	[in] totalDt		The total time-step
	\param	[in] invDt			The inverse (timestep/nbPositionIterations)
	\param	[in] invTotalDt		The inverse total time-step
	\param	[in] lengthScale	PxToleranceScale::length, i.e. a meter in simulation units
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithShadersTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxConstraint** constraints, PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, 
		const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale);

	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxImmediateConstraint* param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created
	\param	[in] allocator		An allocator callback to allocate constraint data
	\param	[in] dt				The total time-step/nbPositionIterations
	\param	[in] totalDt		The total time-step
	\param	[in] invDt			The inverse (timestep/nbPositionIterations)
	\param	[in] invTotalDt		The inverse total time-step
	\param	[in] lengthScale	PxToleranceScale::length, i.e. a meter in simulation units
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/

	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithImmediateShadersTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxImmediateConstraint* constraints, PxTGSSolverConstraintPrepDesc* jointDescs,
		PxConstraintAllocator& allocator, const PxReal dt, const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale);


	/**
	\brief Iteratively solves the set of constraints defined by the provided PxConstraintBatchHeader and PxSolverConstraintDesc structures. Updates deltaVelocities inside the PxSolverBody structures. Produces resulting linear and angular motion velocities.
	\param	[in] batchHeaders			The set of batch headers to be solved
	\param	[in] nbBatchHeaders			The total number of batch headers to be solved
	\param	[in] solverConstraintDescs	The reordererd set of solver constraint descs referenced by the batch headers
	\param	[in,out] solverBodies		The set of solver bodies the bodies reference
	\param	[in,out] txInertias			The set of solver body TxInertias the bodies reference
	\param	[in] nbSolverBodies			The total number of solver bodies
	\param	[in] nbPositionIterations	The number of position iterations to run
	\param	[in] nbVelocityIterations	The number of velocity iterations to run
	\param	[in] dt						time-step/nbPositionIterations
	\param	[in] invDt					1/(time-step/nbPositionIterations)
	\param	[in] nbSolverArticulations	Number of articulations to solve constraints for.
	\param	[in] solverArticulations	Array of articulations to solve constraints for.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxSolveConstraintsTGS(const PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
		PxTGSSolverBodyVel* solverBodies, PxTGSSolverBodyTxInertia* txInertias, const PxU32 nbSolverBodies, const PxU32 nbPositionIterations, const PxU32 nbVelocityIterations,
		const float dt, const float invDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations);

	/**
	\brief Integrates a rigid body, returning the new velocities and transforms. After this function has been called, solverBody stores all the body's velocity data.

	\param	[in,out] solverBody			The array of solver bodies to be integrated
	\param	[in] txInertia				The delta pose and inertia terms
	\param	[in,out] poses				The original poses of the bodies. Updated to be the new poses of the bodies
	\param	[in] nbBodiesToIntegrate	The total number of bodies to integrate
	\param	[in] dt						The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxIntegrateSolverBodiesTGS(PxTGSSolverBodyVel* solverBody, PxTGSSolverBodyTxInertia* txInertia, PxTransform* poses, const PxU32 nbBodiesToIntegrate, const PxReal dt);


#if !PX_DOXYGEN
}
#endif

#if !PX_DOXYGEN
}
#endif

/** @} */
#endif

