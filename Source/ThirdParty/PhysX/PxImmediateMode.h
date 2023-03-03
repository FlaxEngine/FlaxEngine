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

#ifndef PX_IMMEDIATE_MODE_H
#define PX_IMMEDIATE_MODE_H
/** \addtogroup immediatemode
@{ */

#include "PxPhysXConfig.h"
#include "foundation/PxMemory.h"
#include "solver/PxSolverDefs.h"
#include "collision/PxCollisionDefs.h"
#include "PxArticulationReducedCoordinate.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxCudaContextManager;
	class PxBaseTask;
	class PxGeometry;

#if !PX_DOXYGEN
namespace immediate
{
#endif

	typedef void* PxArticulationHandle;

	/**
	\brief Structure to store linear and angular components of spatial vector
	*/
	struct PxSpatialVector
	{
		PxVec3 top;
		PxReal pad0;
		PxVec3 bottom;
		PxReal pad1;
	};

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
		virtual bool recordContacts(const PxContactPoint* contactPoints, const PxU32 nbContacts, const PxU32 index) = 0;

		virtual ~PxContactRecorder(){}
	};

	/**
	\brief Constructs a PxSolverBodyData structure based on rigid body properties. Applies gravity, damping and clamps maximum velocity.
	\param	[in] inRigidData		The array rigid body properties
	\param	[out] outSolverBodyData	The array of solverBodyData produced to represent these bodies
	\param	[in] nbBodies			The total number of solver bodies to create
	\param	[in] gravity			The gravity vector
	\param	[in] dt					The timestep
	\param  [in] gyroscopicForces	Indicates whether gyroscopic forces should be integrated
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructSolverBodies(const PxRigidBodyData* inRigidData, PxSolverBodyData* outSolverBodyData, PxU32 nbBodies, const PxVec3& gravity, PxReal dt, bool gyroscopicForces = false);

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
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32 PxBatchConstraints(	const PxSolverConstraintDesc* solverConstraintDescs, PxU32 nbConstraints, PxSolverBody* solverBodies, PxU32 nbBodies,
															PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
															PxArticulationHandle* articulations=NULL, PxU32 nbArticulations=0);

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
	\param	[out] Z							Temporary buffer for impulse propagation.

	\return a boolean to define if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateContactConstraints(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxSolverContactDesc* contactDescs,
		PxConstraintAllocator& allocator, PxReal invDt, PxReal bounceThreshold, PxReal frictionOffsetThreshold, PxReal correlationDistance,
		PxSpatialVector* Z = NULL);

	/**
	\brief Creates a set of joint constraint blocks. Note that, depending the results of PxBatchConstraints, the batchHeader may refer to up to 4 solverConstraintDescs
	\param	[in] batchHeaders	The array of batch headers to be processed.
	\param	[in] nbHeaders		The total number of batch headers to process.
	\param	[in] jointDescs		An array of constraint prep descs defining the properties of the constraints being created.
	\param	[in] allocator		An allocator callback to allocate constraint data.
	\param	[in] dt				The timestep.
	\param	[in] invDt			The inverse timestep.
	\param	[out] Z				Temporary buffer for impulse propagation.
	\return a boolean indicating if this method was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraints(PxConstraintBatchHeader* batchHeaders, PxU32 nbHeaders, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, PxSpatialVector* Z, PxReal dt, PxReal invDt);

	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxConstraint** param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed.
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows.
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created.
	\param	[in] allocator		An allocator callback to allocate constraint data.
	\param	[in] dt				The timestep.
	\param	[in] invDt			The inverse timestep.
	\param	[out] Z				Temporary buffer for impulse propagation. 
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithShaders(PxConstraintBatchHeader* batchHeaders, PxU32 nbBatchHeaders, PxConstraint** constraints, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, PxReal dt, PxReal invDt, PxSpatialVector* Z = NULL);

	struct PxImmediateConstraint
	{
		PxConstraintSolverPrep	prep;
		const void*				constantBlock;
	};

	/**
	\brief Creates a set of joint constraint blocks. This function runs joint shaders defined inside PxImmediateConstraint* param, fills in joint row information in jointDescs and then calls PxCreateJointConstraints.
	\param	[in] batchHeaders	The set of batchHeaders to be processed.
	\param	[in] nbBatchHeaders	The number of batch headers to process.
	\param	[in] constraints	The set of constraints to be used to produce constraint rows.
	\param	[in,out] jointDescs	An array of constraint prep descs defining the properties of the constraints being created.
	\param	[in] allocator		An allocator callback to allocate constraint data.
	\param	[in] dt				The timestep.
	\param	[in] invDt			The inverse timestep.
	\param	[out] Z				Temporary buffer for impulse propagation.
	\return a boolean indicating if this method was successful or not.
	@see PxCreateJointConstraints
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithImmediateShaders(PxConstraintBatchHeader* batchHeaders, PxU32 nbBatchHeaders, PxImmediateConstraint* constraints, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, PxReal dt, PxReal invDt, PxSpatialVector* Z = NULL);

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
	\param	[out] Z						Temporary buffer for impulse propagation 
	\param	[out] deltaV				Temporary buffer for velocity change
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxSolveConstraints(const PxConstraintBatchHeader* batchHeaders, PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
		const PxSolverBody* solverBodies, PxVec3* linearMotionVelocity, PxVec3* angularMotionVelocity, PxU32 nbSolverBodies, PxU32 nbPositionIterations, PxU32 nbVelocityIterations,
		float dt=0.0f, float invDt=0.0f, PxU32 nbSolverArticulations=0, PxArticulationHandle* solverArticulations=NULL, PxSpatialVector* Z = NULL, PxSpatialVector* deltaV = NULL);

	/**
	\brief Integrates a rigid body, returning the new velocities and transforms. After this function has been called, solverBodyData stores all the body's velocity data.

	\param	[in,out] solverBodyData		The array of solver body data to be integrated
	\param	[in] solverBody				The bodies' linear and angular velocities
	\param	[in] linearMotionVelocity	The bodies' linear motion velocity array
	\param	[in] angularMotionState		The bodies' angular motion velocity array
	\param	[in] nbBodiesToIntegrate	The total number of bodies to integrate
	\param	[in] dt						The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxIntegrateSolverBodies(PxSolverBodyData* solverBodyData, PxSolverBody* solverBody, const PxVec3* linearMotionVelocity, const PxVec3* angularMotionState, PxU32 nbBodiesToIntegrate, PxReal dt);

	/**
	\brief Performs contact generation for a given pair of geometries at the specified poses. Produced contacts are stored in the provided contact recorder. Information is cached in PxCache structure
	to accelerate future contact generation between pairs. This cache data is valid only as long as the memory provided by PxCacheAllocator has not been released/re-used. Recommendation is to 
	retain that data for a single simulation frame, discarding cached data after 2 frames. If the cached memory has been released/re-used prior to the corresponding pair having contact generation
	performed again, it is the application's responsibility to reset the PxCache.

	\param	[in] geom0				Array of geometries to perform collision detection on.
	\param	[in] geom1				Array of geometries to perform collision detection on
	\param	[in] pose0				Array of poses associated with the corresponding entry in the geom0 array
	\param	[in] pose1				Array of poses associated with the corresponding entry in the geom1 array
	\param	[in,out] contactCache	Array of contact caches associated with each pair geom0[i] + geom1[i]
	\param	[in] nbPairs			The total number of pairs to process
	\param	[out] contactRecorder	A callback that is called to record contacts for each pair that detects contacts
	\param	[in] contactDistance	The distance at which contacts begin to be generated between the pairs
	\param	[in] meshContactMargin	The mesh contact margin.
	\param	[in] toleranceLength	The toleranceLength. Used for scaling distance-based thresholds internally to produce appropriate results given simulations in different units
	\param	[in] allocator			A callback to allocate memory for the contact cache

	\return a boolean indicating if the function was successful or not.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxGenerateContacts(	const PxGeometry* const * geom0, const PxGeometry* const * geom1, const PxTransform* pose0, const PxTransform* pose1,
															PxCache* contactCache, PxU32 nbPairs, PxContactRecorder& contactRecorder,
															PxReal contactDistance, PxReal meshContactMargin, PxReal toleranceLength, PxCacheAllocator& allocator);

	/**
	\brief Register articulation-related solver functions. This is equivalent to PxRegisterArticulationsReducedCoordinate() for PxScene-level articulations.
	Call this first to enable reduced coordinates articulations in immediate mode.

	@see PxRegisterArticulationsReducedCoordinate
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxRegisterImmediateArticulations();

	struct PxArticulationJointDataRC
	{
		PxTransform						parentPose;
		PxTransform						childPose;
		PxArticulationMotion::Enum		motion[PxArticulationAxis::eCOUNT];
		PxArticulationLimit				limits[PxArticulationAxis::eCOUNT];
		PxArticulationDrive				drives[PxArticulationAxis::eCOUNT];
		PxReal							targetPos[PxArticulationAxis::eCOUNT];
		PxReal							targetVel[PxArticulationAxis::eCOUNT];
		PxReal							armature[PxArticulationAxis::eCOUNT];
		PxReal							jointPos[PxArticulationAxis::eCOUNT];
		PxReal							jointVel[PxArticulationAxis::eCOUNT];
		PxReal							frictionCoefficient;
		PxReal							maxJointVelocity;
		PxArticulationJointType::Enum	type;

		void	initData()
		{
			parentPose			= PxTransform(PxIdentity);
			childPose			= PxTransform(PxIdentity);
			frictionCoefficient	= 0.05f;
			maxJointVelocity	= 100.0f;
			type				= PxArticulationJointType::eUNDEFINED;	// For root

			for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
			{
				motion[i] = PxArticulationMotion::eLOCKED;
				limits[i] = PxArticulationLimit(0.0f, 0.0f);
				drives[i] = PxArticulationDrive(0.0f, 0.0f, 0.0f);
				armature[i] = 0.0f;
				jointPos[i] = 0.0f;
				jointVel[i] = 0.0f;
			}
			PxMemSet(targetPos, 0xff, sizeof(PxReal)*PxArticulationAxis::eCOUNT);
			PxMemSet(targetVel, 0xff, sizeof(PxReal)*PxArticulationAxis::eCOUNT);
		}
	};

	struct PxArticulationDataRC
	{
		PxArticulationFlags		flags;
	};

	struct PxArticulationLinkMutableDataRC
	{
		PxVec3	inverseInertia;
		float	inverseMass;
		float	linearDamping;
		float	angularDamping;
		float	maxLinearVelocitySq;
		float	maxAngularVelocitySq;
		float	cfmScale;
		bool	disableGravity;

		void	initData()
		{
			inverseInertia			= PxVec3(1.0f);
			inverseMass				= 1.0f;
			linearDamping			= 0.05f;
			angularDamping			= 0.05f;
			maxLinearVelocitySq		= 100.0f * 100.0f;
			maxAngularVelocitySq	= 50.0f * 50.0f;
			cfmScale				= 0.025f;
			disableGravity			= false;
		}
	};

	struct PxArticulationLinkDerivedDataRC
	{
		PxTransform	pose;
		PxVec3		linearVelocity;
		PxVec3		angularVelocity;
	};

	struct PxArticulationLinkDataRC : PxArticulationLinkMutableDataRC
	{
		PxArticulationLinkDataRC()	{ PxArticulationLinkDataRC::initData();	}

		void	initData()
		{
			pose = PxTransform(PxIdentity);

			PxArticulationLinkMutableDataRC::initData();
			inboundJoint.initData();
		}

		PxArticulationJointDataRC	inboundJoint;
		PxTransform					pose;
	};

	typedef void* PxArticulationCookie;

	struct PxArticulationLinkCookie
	{
		PxArticulationCookie	articulation;
		PxU32					linkId;
	};

	struct PxCreateArticulationLinkCookie : PxArticulationLinkCookie
	{
		PX_FORCE_INLINE	PxCreateArticulationLinkCookie(PxArticulationCookie art=NULL, PxU32 id=0xffffffff)
		{
			articulation	= art;
			linkId			= id;
		}
	};

	struct PxArticulationLinkHandle
	{
		PX_FORCE_INLINE			PxArticulationLinkHandle(PxArticulationHandle art=NULL, const PxU32 id=0xffffffff) : articulation(art), linkId(id) {}

		PxArticulationHandle	articulation;
		PxU32					linkId;
	};

	/**
	\brief Begin creation of an immediate-mode reduced-coordinate articulation.

	Returned cookie must be used to add links to the articulation, and to complete creating the articulation.

	The cookie is a temporary ID for the articulation, only valid until PxEndCreateArticulationRC is called.

	\param	[in] data	Articulation data

	\return	Articulation cookie

	@see PxAddArticulationLink PxEndCreateArticulationRC
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxArticulationCookie PxBeginCreateArticulationRC(const PxArticulationDataRC& data);

	/**
	\brief Add a link to the articulation.

	All links must be added before the articulation is completed. It is not possible to add a new link at runtime.

	Returned cookie is a temporary ID for the link, only valid until PxEndCreateArticulationRC is called.

	\param	[in] articulation	Cookie value returned by PxBeginCreateArticulationRC
	\param	[in] parent			Parent for the new link, or NULL if this is the root link
	\param	[in] data			Link data

	\return	Link cookie

	@see PxBeginCreateArticulationRC PxEndCreateArticulationRC
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxArticulationLinkCookie PxAddArticulationLink(PxArticulationCookie articulation, const PxArticulationLinkCookie* parent, const PxArticulationLinkDataRC& data);

	/**
	\brief End creation of an immediate-mode reduced-coordinate articulation.

	This call completes the creation of the articulation. All involved cookies become unsafe to use after that point.

	The links are actually created in this function, and it returns the actual link handles to users. The given buffer should be large enough
	to contain as many links as created between the PxBeginCreateArticulationRC & PxEndCreateArticulationRC calls, i.e.
	if N calls were made to PxAddArticulationLink, the buffer should be large enough to contain N handles.

	\param	[in] articulation	Cookie value returned by PxBeginCreateArticulationRC
	\param	[out] linkHandles	Articulation link handles of all created articulation links
	\param	[in] bufferSize		Size of linkHandles buffer. Must match internal expected number of articulation links.

	\return	Articulation handle, or NULL if creation failed

	@see PxAddArticulationLink PxEndCreateArticulationRC
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxArticulationHandle PxEndCreateArticulationRC(PxArticulationCookie articulation, PxArticulationLinkHandle* linkHandles, PxU32 bufferSize);

	/**
	\brief Releases an immediate-mode reduced-coordinate articulation.
	\param	[in] articulation	Articulation handle

	@see PxCreateFeatherstoneArticulation
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxReleaseArticulation(PxArticulationHandle articulation);

	/**
	\brief Creates an articulation cache.
	\param	[in] articulation	Articulation handle
	\return	Articulation cache

	@see PxReleaseArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxArticulationCache* PxCreateArticulationCache(PxArticulationHandle articulation);

	/**
	\brief Copy the internal data of the articulation to the cache
	\param[in] articulation	Articulation handle.
	\param[in] cache		Articulation data
	\param[in] flag			Indicates which values of the articulation system are copied to the cache

	@see createCache PxApplyArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxCopyInternalStateToArticulationCache(PxArticulationHandle articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag);
	
	/**
	\brief Apply the user defined data in the cache to the articulation system
	\param[in] articulation Articulation handle.
	\param[in] cache 		Articulation data.
	\param[in] flag			Defines which values in the cache will be applied to the articulation

	@see createCache PxCopyInternalStateToArticulationCache
	*/	
	PX_C_EXPORT PX_PHYSX_CORE_API void PxApplyArticulationCache(PxArticulationHandle articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag);

	/**
	\brief Release an articulation cache

	\param[in] cache	The cache to release

	@see PxCreateArticulationCache PxCopyInternalStateToArticulationCache PxCopyInternalStateToArticulationCache
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxReleaseArticulationCache(PxArticulationCache& cache);

	/**
	\brief Retrieves non-mutable link data from a link handle.
	The data here is computed by the articulation code but cannot be directly changed by users.
	\param	[in] link	Link handle
	\param	[out] data	Link data
	\return	True if success

	@see PxGetAllLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxGetLinkData(const PxArticulationLinkHandle& link, PxArticulationLinkDerivedDataRC& data);

	/**
	\brief Retrieves non-mutable link data from an articulation handle (all links).
	The data here is computed by the articulation code but cannot be directly changed by users.
	\param	[in] articulation	Articulation handle
	\param	[out] data			Link data for N links, or NULL to just retrieve the number of links.
	\return	Number of links in the articulation = number of link data structure written to the data array.

	@see PxGetLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32 PxGetAllLinkData(const PxArticulationHandle articulation, PxArticulationLinkDerivedDataRC* data);

	/**
	\brief Retrieves mutable link data from a link handle.
	\param	[in] link	Link handle
	\param	[out] data	Data for this link
	\return	True if success

	@see PxSetMutableLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxGetMutableLinkData(const PxArticulationLinkHandle& link, PxArticulationLinkMutableDataRC& data);

	/**
	\brief Sets mutable link data for given link.
	\param	[in] link	Link handle
	\param	[in] data	Data for this link
	\return	True if success

	@see PxGetMutableLinkData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxSetMutableLinkData(const PxArticulationLinkHandle& link, const PxArticulationLinkMutableDataRC& data);

	/**
	\brief Retrieves joint data from a link handle.
	\param	[in] link	Link handle
	\param	[out] data	Joint data for this link
	\return	True if success

	@see PxSetJointData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxGetJointData(const PxArticulationLinkHandle& link, PxArticulationJointDataRC& data);

	/**
	\brief Sets joint data for given link.
	\param	[in] link	Link handle
	\param	[in] data	Joint data for this link
	\return	True if success

	@see PxGetJointData
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxSetJointData(const PxArticulationLinkHandle& link, const PxArticulationJointDataRC& data);

	/**
	\brief Computes unconstrained velocities for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] gravity		Gravity vector
	\param	[in] dt				Timestep
	\param	[in] invLengthScale 1/lengthScale from PxTolerancesScale.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxComputeUnconstrainedVelocities(PxArticulationHandle articulation, const PxVec3& gravity, const PxReal dt, const PxReal invLengthScale);

	/**
	\brief Updates bodies for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] dt				Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxUpdateArticulationBodies(PxArticulationHandle articulation, const PxReal dt);

	/**
	\brief Computes unconstrained velocities for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] gravity		Gravity vector
	\param	[in] dt				Timestep/numPosIterations
	\param	[in] totalDt		Timestep
	\param	[in] invDt			1/(Timestep/numPosIterations)
	\param	[in] invTotalDt		1/Timestep
	\param	[in] invLengthScale 1/lengthScale from PxTolerancesScale.
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxComputeUnconstrainedVelocitiesTGS(	PxArticulationHandle articulation, const PxVec3& gravity, const PxReal dt,
																			const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal invLengthScale);

	/**
	\brief Updates bodies for a given articulation.
	\param	[in] articulation	Articulation handle
	\param	[in] dt				Timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxUpdateArticulationBodiesTGS(PxArticulationHandle articulation, const PxReal dt);

	/**
	\brief Constructs a PxSolverBodyData structure based on rigid body properties. Applies gravity, damping and clamps maximum velocity.
	\param	[in] inRigidData		The array rigid body properties
	\param	[out] outSolverBodyVel	The array of PxTGSSolverBodyVel structures produced to represent these bodies
	\param	[out] outSolverBodyTxInertia The array of PxTGSSolverBodyTxInertia produced to represent these bodies
	\param	[out] outSolverBodyData	The array of PxTGSolverBodyData produced to represent these bodies
	\param	[in] nbBodies			The total number of solver bodies to create
	\param	[in] gravity			The gravity vector
	\param	[in] dt					The timestep
	\param	[in] gyroscopicForces	Indicates whether gyroscopic forces should be integrated
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxConstructSolverBodiesTGS(const PxRigidBodyData* inRigidData, PxTGSSolverBodyVel* outSolverBodyVel, PxTGSSolverBodyTxInertia* outSolverBodyTxInertia, PxTGSSolverBodyData* outSolverBodyData, const PxU32 nbBodies, const PxVec3& gravity, const PxReal dt, const bool gyroscopicForces = false);

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
	PX_C_EXPORT PX_PHYSX_CORE_API PxU32 PxBatchConstraintsTGS(	const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxTGSSolverBodyVel* solverBodies, const PxU32 nbBodies,
																PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
																PxArticulationHandle* articulations = NULL, const PxU32 nbArticulations = 0);

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
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateContactConstraintsTGS(	PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxTGSSolverContactDesc* contactDescs,
																		PxConstraintAllocator& allocator, const PxReal invDt, const PxReal invTotalDt, const PxReal bounceThreshold,
																		const PxReal frictionOffsetThreshold, const PxReal correlationDistance);

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
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsTGS(	PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders,
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
	PX_C_EXPORT PX_PHYSX_CORE_API bool PxCreateJointConstraintsWithShadersTGS(	PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxConstraint** constraints, PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator,
																				const PxReal dt, const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale);

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
	\param	[out] Z						Temporary buffer for impulse propagation (only if articulations are used, size should be at least as large as the maximum number of links in any articulations being simulated)
	\param	[out] deltaV				Temporary buffer for velocity change (only if articulations are used, size should be at least as large as the maximum number of links in any articulations being simulated)
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxSolveConstraintsTGS(	const PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
																PxTGSSolverBodyVel* solverBodies, PxTGSSolverBodyTxInertia* txInertias, const PxU32 nbSolverBodies, const PxU32 nbPositionIterations, const PxU32 nbVelocityIterations,
																const float dt, const float invDt, const PxU32 nbSolverArticulations = 0, PxArticulationHandle* solverArticulations = NULL, PxSpatialVector* Z = NULL, PxSpatialVector* deltaV = NULL);

	/**
	\brief Integrates a rigid body, returning the new velocities and transforms. After this function has been called, solverBody stores all the body's velocity data.

	\param	[in,out] solverBody			The array of solver bodies to be integrated
	\param	[in] txInertia				The delta pose and inertia terms
	\param	[in,out] poses				The original poses of the bodies. Updated to be the new poses of the bodies
	\param	[in] nbBodiesToIntegrate	The total number of bodies to integrate
	\param	[in] dt						The timestep
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API void PxIntegrateSolverBodiesTGS(PxTGSSolverBodyVel* solverBody, PxTGSSolverBodyTxInertia* txInertia, PxTransform* poses, const PxU32 nbBodiesToIntegrate, const PxReal dt);

	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxArticulationJointDataRC			PxFeatherstoneArticulationJointData;
	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxArticulationLinkDataRC			PxFeatherstoneArticulationLinkData;
	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxArticulationDataRC				PxFeatherstoneArticulationData;
	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxArticulationLinkMutableDataRC	PxMutableLinkData;
	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxArticulationLinkDerivedDataRC	PxLinkData;

#if !PX_DOXYGEN
}
#endif

#if !PX_DOXYGEN
}
#endif

/** @} */
#endif

