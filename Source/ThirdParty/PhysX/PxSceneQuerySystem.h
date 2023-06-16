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

#ifndef PX_SCENE_QUERY_SYSTEM_H
#define PX_SCENE_QUERY_SYSTEM_H
/** \addtogroup physics
@{ */

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxBitMap.h"
#include "foundation/PxTransform.h"
#include "PxSceneQueryDesc.h"
#include "PxQueryReport.h"
#include "PxQueryFiltering.h"
#include "geometry/PxGeometryQueryFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxBaseTask;
	class PxRenderOutput;
	class PxGeometry;
	class PxRigidActor;
	class PxShape;
	class PxBVH;
	class PxPruningStructure;

	/**
	\brief Built-in enum for default PxScene pruners

	This is passed as a pruner index to various functions in the following APIs.

	@see PxSceneQuerySystemBase::forceRebuildDynamicTree PxSceneQuerySystem::preallocate
	@see PxSceneQuerySystem::visualize PxSceneQuerySystem::sync PxSceneQuerySystem::prepareSceneQueryBuildStep
	*/
	enum PxScenePrunerIndex
	{
		PX_SCENE_PRUNER_STATIC		= 0,
		PX_SCENE_PRUNER_DYNAMIC		= 1,
		PX_SCENE_COMPOUND_PRUNER	= 0xffffffff
	};

	/**
	\brief Base class for the scene-query system.

	Methods defined here are common to both the traditional PxScene API and the PxSceneQuerySystem API.

	@see PxScene PxSceneQuerySystem
	*/
	class PxSceneQuerySystemBase
	{
		protected:
						PxSceneQuerySystemBase()	{}
		virtual			~PxSceneQuerySystemBase()	{}

		public:

		/** @name Scene Query
		*/
		//@{

		/**
		\brief Sets the rebuild rate of the dynamic tree pruning structures.

		\param[in] dynamicTreeRebuildRateHint Rebuild rate of the dynamic tree pruning structures.

		@see PxSceneQueryDesc.dynamicTreeRebuildRateHint getDynamicTreeRebuildRateHint() forceRebuildDynamicTree()
		*/
		virtual	void	setDynamicTreeRebuildRateHint(PxU32 dynamicTreeRebuildRateHint) = 0;

		/**
		\brief Retrieves the rebuild rate of the dynamic tree pruning structures.

		\return The rebuild rate of the dynamic tree pruning structures.

		@see PxSceneQueryDesc.dynamicTreeRebuildRateHint setDynamicTreeRebuildRateHint() forceRebuildDynamicTree()
		*/
		virtual PxU32	getDynamicTreeRebuildRateHint() const = 0;

		/**
		\brief Forces dynamic trees to be immediately rebuilt.

		\param[in] prunerIndex	Index of pruner containing the dynamic tree to rebuild

		\note PxScene will call this function with the PX_SCENE_PRUNER_STATIC or PX_SCENE_PRUNER_DYNAMIC value.

		@see PxSceneQueryDesc.dynamicTreeRebuildRateHint setDynamicTreeRebuildRateHint() getDynamicTreeRebuildRateHint()
		*/
		virtual void	forceRebuildDynamicTree(PxU32 prunerIndex)	= 0;

		/**
		\brief Sets scene query update mode	

		\param[in] updateMode	Scene query update mode.

		@see PxSceneQueryUpdateMode::Enum
		*/
		virtual	void	setUpdateMode(PxSceneQueryUpdateMode::Enum updateMode)	= 0;

		/**
		\brief Gets scene query update mode	

		\return Current scene query update mode.

		@see PxSceneQueryUpdateMode::Enum
		*/
		virtual	PxSceneQueryUpdateMode::Enum	getUpdateMode()	const	= 0;

		/**
		\brief Retrieves the system's internal scene query timestamp, increased each time a change to the
		static scene query structure is performed.

		\return scene query static timestamp
		*/
		virtual	PxU32	getStaticTimestamp()	const	= 0;

		/**
		\brief Flushes any changes to the scene query representation.

		This method updates the state of the scene query representation to match changes in the scene state.

		By default, these changes are buffered until the next query is submitted. Calling this function will not change
		the results from scene queries, but can be used to ensure that a query will not perform update work in the course of 
		its execution.
	
		A thread performing updates will hold a write lock on the query structure, and thus stall other querying threads. In multithread
		scenarios it can be useful to explicitly schedule the period where this lock may be held for a significant period, so that
		subsequent queries issued from multiple threads will not block.
		*/
		virtual	void	flushUpdates()	= 0;

		/**
		\brief Performs a raycast against objects in the scene, returns results in a PxRaycastBuffer object
		or via a custom user callback implementation inheriting from PxRaycastCallback.

		\note	Touching hits are not ordered.
		\note	Shooting a ray from within an object leads to different results depending on the shape type. Please check the details in user guide article SceneQuery. User can ignore such objects by employing one of the provided filter mechanisms.

		\param[in] origin		Origin of the ray.
		\param[in] unitDir		Normalized direction of the ray.
		\param[in] distance		Length of the ray. Has to be in the [0, inf) range.
		\param[out] hitCall		Raycast hit buffer or callback object used to report raycast hits.
		\param[in] hitFlags		Specifies which properties per hit should be computed and returned via the hit callback.
		\param[in] filterData	Filtering data passed to the filter shader. 
		\param[in] filterCall	Custom filtering logic (optional). Only used if the corresponding #PxQueryFlag flags are set. If NULL, all hits are assumed to be blocking.
		\param[in] cache		Cached hit shape (optional). Ray is tested against cached shape first. If no hit is found the ray gets queried against the scene.
								Note: Filtering is not executed for a cached shape if supplied; instead, if a hit is found, it is assumed to be a blocking hit.
								Note: Using past touching hits as cache will produce incorrect behavior since the cached hit will always be treated as blocking.
		\param[in] queryFlags	Optional flags controlling the query.

		\return True if any touching or blocking hits were found or any hit was found in case PxQueryFlag::eANY_HIT was specified.

		@see PxRaycastCallback PxRaycastBuffer PxQueryFilterData PxQueryFilterCallback PxQueryCache PxRaycastHit PxQueryFlag PxQueryFlag::eANY_HIT PxGeometryQueryFlag
		*/
		virtual bool	raycast(const PxVec3& origin, const PxVec3& unitDir, const PxReal distance,
								PxRaycastCallback& hitCall, PxHitFlags hitFlags = PxHitFlag::eDEFAULT,
								const PxQueryFilterData& filterData = PxQueryFilterData(), PxQueryFilterCallback* filterCall = NULL,
								const PxQueryCache* cache = NULL, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

		/**
		\brief Performs a sweep test against objects in the scene, returns results in a PxSweepBuffer object
		or via a custom user callback implementation inheriting from PxSweepCallback.
	
		\note	Touching hits are not ordered.
		\note	If a shape from the scene is already overlapping with the query shape in its starting position,
				the hit is returned unless eASSUME_NO_INITIAL_OVERLAP was specified.

		\param[in] geometry		Geometry of object to sweep (supported types are: box, sphere, capsule, convex).
		\param[in] pose			Pose of the sweep object.
		\param[in] unitDir		Normalized direction of the sweep.
		\param[in] distance		Sweep distance. Needs to be in [0, inf) range and >0 if eASSUME_NO_INITIAL_OVERLAP was specified. Will be clamped to PX_MAX_SWEEP_DISTANCE.
		\param[out] hitCall		Sweep hit buffer or callback object used to report sweep hits.
		\param[in] hitFlags		Specifies which properties per hit should be computed and returned via the hit callback.
		\param[in] filterData	Filtering data and simple logic.
		\param[in] filterCall	Custom filtering logic (optional). Only used if the corresponding #PxQueryFlag flags are set. If NULL, all hits are assumed to be blocking.
		\param[in] cache		Cached hit shape (optional). Sweep is performed against cached shape first. If no hit is found the sweep gets queried against the scene.
								Note: Filtering is not executed for a cached shape if supplied; instead, if a hit is found, it is assumed to be a blocking hit.
								Note: Using past touching hits as cache will produce incorrect behavior since the cached hit will always be treated as blocking.
		\param[in] inflation	This parameter creates a skin around the swept geometry which increases its extents for sweeping. The sweep will register a hit as soon as the skin touches a shape, and will return the corresponding distance and normal.
								Note: ePRECISE_SWEEP doesn't support inflation. Therefore the sweep will be performed with zero inflation.	
		\param[in] queryFlags	Optional flags controlling the query.
	
		\return True if any touching or blocking hits were found or any hit was found in case PxQueryFlag::eANY_HIT was specified.

		@see PxSweepCallback PxSweepBuffer PxQueryFilterData PxQueryFilterCallback PxSweepHit PxQueryCache PxGeometryQueryFlag
		*/
		virtual bool	sweep(	const PxGeometry& geometry, const PxTransform& pose, const PxVec3& unitDir, const PxReal distance,
								PxSweepCallback& hitCall, PxHitFlags hitFlags = PxHitFlag::eDEFAULT,
								const PxQueryFilterData& filterData = PxQueryFilterData(), PxQueryFilterCallback* filterCall = NULL,
								const PxQueryCache* cache = NULL, const PxReal inflation = 0.0f, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

		/**
		\brief Performs an overlap test of a given geometry against objects in the scene, returns results in a PxOverlapBuffer object
		or via a custom user callback implementation inheriting from PxOverlapCallback.
	
		\note Filtering: returning eBLOCK from user filter for overlap queries will cause a warning (see #PxQueryHitType).

		\param[in] geometry		Geometry of object to check for overlap (supported types are: box, sphere, capsule, convex).
		\param[in] pose			Pose of the object.
		\param[out] hitCall		Overlap hit buffer or callback object used to report overlap hits.
		\param[in] filterData	Filtering data and simple logic. See #PxQueryFilterData #PxQueryFilterCallback
		\param[in] filterCall	Custom filtering logic (optional). Only used if the corresponding #PxQueryFlag flags are set. If NULL, all hits are assumed to overlap.
		\param[in] cache		Cached hit shape (optional). Overlap is performed against cached shape first. If no hit is found the overlap gets queried against the scene.
		\param[in] queryFlags	Optional flags controlling the query.
		Note: Filtering is not executed for a cached shape if supplied; instead, if a hit is found, it is assumed to be a blocking hit.
		Note: Using past touching hits as cache will produce incorrect behavior since the cached hit will always be treated as blocking.

		\return True if any touching or blocking hits were found or any hit was found in case PxQueryFlag::eANY_HIT was specified.

		\note eBLOCK should not be returned from user filters for overlap(). Doing so will result in undefined behavior, and a warning will be issued.
		\note If the PxQueryFlag::eNO_BLOCK flag is set, the eBLOCK will instead be automatically converted to an eTOUCH and the warning suppressed.

		@see PxOverlapCallback PxOverlapBuffer PxHitFlags PxQueryFilterData PxQueryFilterCallback PxGeometryQueryFlag
		*/
		virtual bool	overlap(const PxGeometry& geometry, const PxTransform& pose, PxOverlapCallback& hitCall,
								const PxQueryFilterData& filterData = PxQueryFilterData(), PxQueryFilterCallback* filterCall = NULL,
								const PxQueryCache* cache = NULL, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;
		//@}
	};

	/**
	\brief Traditional SQ system for PxScene.

	Methods defined here are only available through the traditional PxScene API.
	Thus PxSceneSQSystem effectively captures the scene-query related part of the PxScene API.

	@see PxScene PxSceneQuerySystemBase
	*/
	class PxSceneSQSystem : public PxSceneQuerySystemBase
	{
		protected:
					PxSceneSQSystem()	{}
		virtual		~PxSceneSQSystem()	{}

		public:

		/** @name Scene Query
		*/
		//@{

		/**
		\brief Sets scene query update mode	

		\param[in] updateMode	Scene query update mode.

		@see PxSceneQueryUpdateMode::Enum
		*/
		PX_FORCE_INLINE	void	setSceneQueryUpdateMode(PxSceneQueryUpdateMode::Enum updateMode)	{ setUpdateMode(updateMode);	}

		/**
		\brief Gets scene query update mode	

		\return Current scene query update mode.

		@see PxSceneQueryUpdateMode::Enum
		*/
		PX_FORCE_INLINE	PxSceneQueryUpdateMode::Enum	getSceneQueryUpdateMode()	const	{ return getUpdateMode();	}

		/**
		\brief Retrieves the scene's internal scene query timestamp, increased each time a change to the
		static scene query structure is performed.

		\return scene query static timestamp
		*/
		PX_FORCE_INLINE	PxU32	getSceneQueryStaticTimestamp()	const	{ return getStaticTimestamp();	}

		/**
		\brief Flushes any changes to the scene query representation.

		@see flushUpdates
		*/
		PX_FORCE_INLINE	void	flushQueryUpdates()	{ flushUpdates();	}

		/**
		\brief Forces dynamic trees to be immediately rebuilt.

		\param[in] rebuildStaticStructure	True to rebuild the dynamic tree containing static objects
		\param[in] rebuildDynamicStructure	True to rebuild the dynamic tree containing dynamic objects

		@see PxSceneQueryDesc.dynamicTreeRebuildRateHint setDynamicTreeRebuildRateHint() getDynamicTreeRebuildRateHint()
		*/
		PX_FORCE_INLINE void	forceDynamicTreeRebuild(bool rebuildStaticStructure, bool rebuildDynamicStructure)
		{
			if(rebuildStaticStructure)
				forceRebuildDynamicTree(PX_SCENE_PRUNER_STATIC);
			if(rebuildDynamicStructure)
				forceRebuildDynamicTree(PX_SCENE_PRUNER_DYNAMIC);
		}

		/**
		\brief Return the value of PxSceneQueryDesc::staticStructure that was set when creating the scene with PxPhysics::createScene

		@see PxSceneQueryDesc::staticStructure, PxPhysics::createScene
		*/
		virtual	PxPruningStructureType::Enum getStaticStructure() const = 0;

		/**
		\brief Return the value of PxSceneQueryDesc::dynamicStructure that was set when creating the scene with PxPhysics::createScene

		@see PxSceneQueryDesc::dynamicStructure, PxPhysics::createScene
		*/
		virtual PxPruningStructureType::Enum getDynamicStructure() const = 0;

		/**
		\brief Executes scene queries update tasks.

		This function will refit dirty shapes within the pruner and will execute a task to build a new AABB tree, which is
		build on a different thread. The new AABB tree is built based on the dynamic tree rebuild hint rate. Once
		the new tree is ready it will be commited in next fetchQueries call, which must be called after.

		This function is equivalent to the following PxSceneQuerySystem calls:
		Synchronous calls:
			- PxSceneQuerySystemBase::flushUpdates()
			- handle0 = PxSceneQuerySystem::prepareSceneQueryBuildStep(PX_SCENE_PRUNER_STATIC)
			- handle1 = PxSceneQuerySystem::prepareSceneQueryBuildStep(PX_SCENE_PRUNER_DYNAMIC)
		Asynchronous calls:
			- PxSceneQuerySystem::sceneQueryBuildStep(handle0);
			- PxSceneQuerySystem::sceneQueryBuildStep(handle1);

		This function is part of the PxSceneSQSystem interface because it uses the PxScene task system under the hood. But
		it calls PxSceneQuerySystem functions, which are independent from this system and could be called in a similar
		fashion by a separate, possibly user-defined task manager.

		\note If PxSceneQueryUpdateMode::eBUILD_DISABLED_COMMIT_DISABLED is used, it is required to update the scene queries
		using this function.

		\param[in] completionTask if non-NULL, this task will have its refcount incremented in sceneQueryUpdate(), then
		decremented when the scene is ready to have fetchQueries called. So the task will not run until the
		application also calls removeReference().
		\param[in] controlSimulation if true, the scene controls its PxTaskManager simulation state. Leave
		true unless the application is calling the PxTaskManager start/stopSimulation() methods itself.

		@see PxSceneQueryUpdateMode::eBUILD_DISABLED_COMMIT_DISABLED
		*/
		virtual void	sceneQueriesUpdate(PxBaseTask* completionTask = NULL, bool controlSimulation = true)	= 0;

		/**
		\brief This checks to see if the scene queries update has completed.

		This does not cause the data available for reading to be updated with the results of the scene queries update, it is simply a status check.
		The bool will allow it to either return immediately or block waiting for the condition to be met so that it can return true
	
		\param[in] block When set to true will block until the condition is met.
		\return True if the results are available.

		@see sceneQueriesUpdate() fetchResults()
		*/
		virtual	bool	checkQueries(bool block = false) = 0;

		/**
		This method must be called after sceneQueriesUpdate. It will wait for the scene queries update to finish. If the user makes an illegal scene queries update call, 
		the SDK will issue an error	message.

		If a new AABB tree build finished, then during fetchQueries the current tree within the pruning structure is swapped with the new tree. 

		\param[in] block When set to true will block until the condition is met, which is tree built task must finish running.
		*/
		virtual	bool	fetchQueries(bool block = false)	= 0;	
		//@}
	};

	typedef PxU32	PxSQCompoundHandle;
	typedef PxU32	PxSQPrunerHandle;
	typedef void*	PxSQBuildStepHandle;

	/**
	\brief Scene-queries external sub-system for PxScene-based objects.

	The default PxScene has hardcoded support for 2 regular pruners + 1 compound pruner, but these interfaces
	should work with multiple pruners.

	Regular shapes are traditional PhysX shapes that belong to an actor. That actor can be a compound, i.e. it has
	more than one shape. *All of these go to the regular pruners*. This is important because it might be misleading:
	by default all shapes go to one of the two regular pruners, even shapes that belong to compound actors.

	For compound actors, adding all the actor's shapes individually to the SQ system can be costly, since all the
	corresponding bounds will always move together and remain close together - that can put a lot of stress on the
	code that updates the SQ spatial structures. In these cases it can be more efficient to add the compound's bounds
	(i.e. the actor's bounds) to the system, as the first level of a bounds hierarchy. The scene queries would then
	be performed against the actor's bounds first, and only visit the shapes' bounds second. This is only useful
	for actors that have more than one shape, i.e. compound actors. Such actors added to the SQ system are thus
	called "SQ compounds". These objects are managed by the "compound pruner", which is only used when an explicit
	SQ compound is added to the SQ system via the addSQCompound call. So in the end one has to distinguish between:

	- a "compound shape", which is added to regular pruners as its own individual entity.
	- an "SQ compound shape", which is added to the compound pruner as a subpart of an SQ compound actor.

	A compound shape has an invalid compound ID, since it does not belong to an SQ compound.
	An SQ compound shape has a valid compound ID, that identifies its SQ compound owner.

	@see PxScene PxSceneQuerySystemBase
	*/
	class PxSceneQuerySystem : public PxSceneQuerySystemBase
	{
		protected:
						PxSceneQuerySystem()	{}
		virtual			~PxSceneQuerySystem()	{}
		public:

		/**
		\brief Decrements the reference count of the object and releases it if the new reference count is zero.		
		*/
		virtual	void	release()	= 0;

		/**
		\brief Acquires a counted reference to this object.

		This method increases the reference count of the object by 1. Decrement the reference count by calling release()
		*/
		virtual	void	acquireReference()	= 0;

		/**
		\brief Preallocates internal arrays to minimize the amount of reallocations.

		The system does not prevent more allocations than given numbers. It is legal to not call this function at all,
		or to add more shapes to the system than the preallocated amounts.

		\param[in] prunerIndex	Index of pruner to preallocate (PX_SCENE_PRUNER_STATIC, PX_SCENE_PRUNER_DYNAMIC or PX_SCENE_COMPOUND_PRUNER when called from PxScene).
		\param[in] nbShapes		Expected number of (regular) shapes
		*/
		virtual	void	preallocate(PxU32 prunerIndex, PxU32 nbShapes)	= 0;

		/**
		\brief Frees internal memory that may not be in-use anymore.

		This is an entry point for reclaiming transient memory allocated at some point by the SQ system,
		but which wasn't been immediately freed for performance reason. Calling this function might free
		some memory, but it might also produce a new set of allocations in the next frame.
		*/
		virtual	void	flushMemory()	= 0;

		/**
		\brief Adds a shape to the SQ system.

		The same function is used to add either a regular shape, or a SQ compound shape.

		\param[in] actor				The shape's actor owner
		\param[in] shape				The shape itself
		\param[in] bounds				Shape bounds, in world-space for regular shapes, in local-space for SQ compound shapes.
		\param[in] transform			Shape transform, in world-space for regular shapes, in local-space for SQ compound shapes.
		\param[in] compoundHandle		Handle of SQ compound owner, or NULL for regular shapes.
		\param[in] hasPruningStructure	True if the shape is part of a pruning structure. The structure will be merged later, adding the objects will not invalidate the pruner.

		@see merge() PxPruningStructure
		*/
		virtual	void	addSQShape(	const PxRigidActor& actor, const PxShape& shape, const PxBounds3& bounds,
									const PxTransform& transform, const PxSQCompoundHandle* compoundHandle=NULL, bool hasPruningStructure=false)	= 0;

		/**
		\brief Removes a shape from the SQ system.

		The same function is used to remove either a regular shape, or a SQ compound shape.

		\param[in] actor	The shape's actor owner
		\param[in] shape	The shape itself
		*/
		virtual	void	removeSQShape(const PxRigidActor& actor, const PxShape& shape)	= 0;

		/**
		\brief Updates a shape in the SQ system.

		The same function is used to update either a regular shape, or a SQ compound shape.

		The transforms are eager-evaluated, but the bounds are lazy-evaluated. This means that
		the updated transform has to be passed to the update function, while the bounds are automatically
		recomputed by the system whenever needed.

		\param[in] actor		The shape's actor owner
		\param[in] shape		The shape itself
		\param[in] transform	New shape transform, in world-space for regular shapes, in local-space for SQ compound shapes.
		*/
		virtual	void	updateSQShape(const PxRigidActor& actor, const PxShape& shape, const PxTransform& transform)	= 0;

		/**
		\brief Adds a compound to the SQ system.

		\param[in] actor		The compound actor
		\param[in] shapes		The compound actor's shapes
		\param[in] bvh			BVH structure containing the compound's shapes in local space
		\param[in] transforms	Shape transforms, in local-space

		\return	SQ compound handle

		@see PxBVH PxCooking::createBVH
		*/
		virtual	PxSQCompoundHandle	addSQCompound(const PxRigidActor& actor, const PxShape** shapes, const PxBVH& bvh, const PxTransform* transforms)	= 0;

		/**
		\brief Removes a compound from the SQ system.

		\param[in] compoundHandle	SQ compound handle (returned by addSQCompound)
		*/
		virtual	void	removeSQCompound(PxSQCompoundHandle compoundHandle)	= 0;

		/**
		\brief Updates a compound in the SQ system.

		The compound structures are immediately updated when the call occurs.

		\param[in] compoundHandle		SQ compound handle (returned by addSQCompound)
		\param[in] compoundTransform	New actor/compound transform, in world-space
		*/
		virtual	void	updateSQCompound(PxSQCompoundHandle compoundHandle, const PxTransform& compoundTransform)	= 0;

		/**
		\brief Shift the data structures' origin by the specified vector.

		Please refer to the notes of the similar function in PxScene.

		\param[in] shift	Translation vector to shift the origin by.
		*/
		virtual	void	shiftOrigin(const PxVec3& shift)	= 0;

		/**
		\brief Visualizes the system's internal data-structures, for debugging purposes.

		\param[in] prunerIndex	Index of pruner to visualize (PX_SCENE_PRUNER_STATIC, PX_SCENE_PRUNER_DYNAMIC or PX_SCENE_COMPOUND_PRUNER when called from PxScene).

		\param[out] out			Filled with render output data

		@see PxRenderOutput
		*/
		virtual	void	visualize(PxU32 prunerIndex, PxRenderOutput& out)	const	= 0;

		/**
		\brief Merges a pruning structure with the SQ system's internal pruners.

		\param[in] pruningStructure		The pruning structure to merge

		@see PxPruningStructure
		*/
		virtual	void	merge(const PxPruningStructure& pruningStructure)	= 0;

		/**
		\brief Shape to SQ-pruner-handle mapping function.

		This function finds and returns the SQ pruner handle associated with a given (actor/shape) couple
		that was previously added to the system. This is needed for the sync function.

		\param[in] actor		The shape's actor owner
		\param[in] shape		The shape itself
		\param[out] prunerIndex	Index of pruner the shape belongs to

		\return Associated SQ pruner handle.
		*/
		virtual	PxSQPrunerHandle	getHandle(const PxRigidActor& actor, const PxShape& shape, PxU32& prunerIndex)	const	= 0;

		/**
		\brief Synchronizes the scene-query system with another system that references the same objects.

		This function is used when the scene-query objects also exist in another system that can also update them. For example the scene-query objects
		(used for raycast, overlap or sweep queries) might be driven by equivalent objects in an external rigid-body simulation engine. In this case
		the rigid-body simulation engine computes the new poses and transforms, and passes them to the scene-query system using this function. It is
		more efficient than calling updateSQShape on each object individually, since updateSQShape would end up recomputing the bounds already available
		in the rigid-body engine.

		\param[in] prunerIndex		Index of pruner being synched (PX_SCENE_PRUNER_DYNAMIC for regular PhysX usage)
		\param[in] handles			Handles of updated objects
		\param[in] indices			Bounds & transforms indices of updated objects, i.e. object handles[i] has bounds[indices[i]] and transforms[indices[i]]
		\param[in] bounds			Array of bounds for all objects (not only updated bounds)
		\param[in] transforms		Array of transforms for all objects (not only updated transforms)
		\param[in] count			Number of updated objects
		\param[in] ignoredIndices	Optional bitmap of ignored indices, i.e. update is skipped if ignoredIndices[indices[i]] is set.

		@see PxBounds3 PxTransform32 PxBitMap
		*/
		virtual	void	sync(PxU32 prunerIndex, const PxSQPrunerHandle* handles, const PxU32* indices, const PxBounds3* bounds, const PxTransform32* transforms, PxU32 count, const PxBitMap& ignoredIndices)	= 0;

		/**
		\brief Finalizes updates made to the SQ system.

		This function should be called after updates have been made to the SQ system, to fully reflect the changes
		inside the internal pruners. In particular it should be called:
		- after calls to updateSQShape
		- after calls to sync

		This function:
		- recomputes bounds of manually updated shapes (i.e. either regular or SQ compound shapes modified by updateSQShape)
		- updates dynamic pruners (refit operations)
		- incrementally rebuilds AABB-trees

		The amount of work performed in this function depends on PxSceneQueryUpdateMode.

		@see PxSceneQueryUpdateMode updateSQShape() sync()
		*/
		virtual	void	finalizeUpdates()	= 0;

		/**
		\brief Prepares asynchronous build step.

		This is directly called (synchronously) by PxSceneSQSystem::sceneQueriesUpdate(). See the comments there.

		This function is called to let the system execute any necessary synchronous operation before the
		asynchronous sceneQueryBuildStep() function is called.

		If there is any work to do for the specific pruner, the function returns a pruner-specific handle that
		will be passed to the corresponding, asynchronous sceneQueryBuildStep function.

		\return	A pruner-specific handle that will be sent to sceneQueryBuildStep if there is any work to do, i.e. to execute the corresponding sceneQueryBuildStep() call.

		\param[in] prunerIndex		Index of pruner being built. (PX_SCENE_PRUNER_STATIC or PX_SCENE_PRUNER_DYNAMIC when called by PxScene).

		\return	Null if there is no work to do, otherwise a pruner-specific handle.

		@see PxSceneSQSystem::sceneQueriesUpdate sceneQueryBuildStep
		*/
		virtual	PxSQBuildStepHandle	prepareSceneQueryBuildStep(PxU32 prunerIndex)	= 0;

		/**
		\brief Executes asynchronous build step.

		This is directly called (asynchronously) by PxSceneSQSystem::sceneQueriesUpdate(). See the comments there.

		This function incrementally builds the internal trees/pruners. It is called asynchronously, i.e. this can be
		called from different threads for building multiple trees at the same time.
		
		\param[in] handle	Pruner-specific handle previously returned by the prepareSceneQueryBuildStep function.

		@see PxSceneSQSystem::sceneQueriesUpdate prepareSceneQueryBuildStep
		*/
		virtual	void	sceneQueryBuildStep(PxSQBuildStepHandle handle)	= 0;
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
