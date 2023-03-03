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

#ifndef PX_BROAD_PHASE_H
#define PX_BROAD_PHASE_H
/** \addtogroup physics
@{
*/

#include "PxPhysXConfig.h"
#include "foundation/PxBounds3.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxBaseTask;
	class PxCudaContextManager;

	/**
	\brief Broad phase algorithm used in the simulation

	eSAP is a good generic choice with great performance when many objects are sleeping. Performance
	can degrade significantly though, when all objects are moving, or when large numbers of objects
	are added to or removed from the broad phase. This algorithm does not need world bounds to be
	defined in order to work.

	eMBP is an alternative broad phase algorithm that does not suffer from the same performance
	issues as eSAP when all objects are moving or when inserting large numbers of objects. However
	its generic performance when many objects are sleeping might be inferior to eSAP, and it requires
	users to define world bounds in order to work.

	eABP is a revisited implementation of MBP, which automatically manages broad-phase regions.
	It offers the convenience of eSAP (no need to define world bounds or regions) and the performance
	of eMBP when a lot of objects are moving. While eSAP can remain faster when most objects are
	sleeping and eMBP can remain faster when it uses a large number of properly-defined regions,
	eABP often gives the best performance on average and the best memory usage.

	ePABP is a parallel implementation of ABP. It can often be the fastest (CPU) broadphase, but it
	can use more memory than ABP.

	eGPU is a GPU implementation of the incremental sweep and prune approach. Additionally, it uses a ABP-style
	initial pair generation approach to avoid large spikes when inserting shapes. It not only has the advantage 
	of traditional SAP approch which is good for when many objects are sleeping, but due to being fully parallel, 
	it also is great when lots of shapes are moving or for runtime pair insertion and removal. It can become a 
	performance bottleneck if there are a very large number of shapes roughly projecting to the same values
	on a given axis. If the scene has a very large number of shapes in an actor, e.g. a humanoid, it is recommended
	to use an aggregate to represent multi-shape or multi-body actors to minimize stress placed on the broad phase.
	*/
	struct PxBroadPhaseType
	{
		enum Enum
		{
			eSAP,	//!< 3-axes sweep-and-prune
			eMBP,	//!< Multi box pruning
			eABP,	//!< Automatic box pruning
			ePABP,	//!< Parallel automatic box pruning
			eGPU,	//!< GPU broad phase
			eLAST
		};
	};

	/**
	\brief "Region of interest" for the broad-phase.

	This is currently only used for the PxBroadPhaseType::eMBP broad-phase, which requires zones or regions to be defined
	when the simulation starts in order to work. Regions can overlap and be added or removed at runtime, but at least one
	region needs to be defined when the scene is created.

	If objects that do no overlap any region are inserted into the scene, they will not be added to the broad-phase and
	thus collisions will be disabled for them. A PxBroadPhaseCallback out-of-bounds notification will be sent for each one
	of those objects.

	The total number of regions is limited by PxBroadPhaseCaps::mMaxNbRegions.

	The number of regions has a direct impact on performance and memory usage, so it is recommended to experiment with
	various settings to find the best combination for your game. A good default setup is to start with global bounds
	around the whole world, and subdivide these bounds into 4*4 regions. The PxBroadPhaseExt::createRegionsFromWorldBounds
	function can do that for you.

	@see PxBroadPhaseCallback PxBroadPhaseExt.createRegionsFromWorldBounds
	*/
	struct PxBroadPhaseRegion
	{
		PxBounds3	mBounds;		//!< Region's bounds
		void*		mUserData;	//!< Region's user-provided data
	};

	/**
	\brief Information & stats structure for a region
	*/
	struct PxBroadPhaseRegionInfo
	{
		PxBroadPhaseRegion	mRegion;			//!< User-provided region data
		PxU32				mNbStaticObjects;	//!< Number of static objects in the region
		PxU32				mNbDynamicObjects;	//!< Number of dynamic objects in the region
		bool				mActive;			//!< True if region is currently used, i.e. it has not been removed
		bool				mOverlap;			//!< True if region overlaps other regions (regions that are just touching are not considering overlapping)
	};

	/**
	\brief Caps class for broad phase.
	*/
	struct PxBroadPhaseCaps
	{
		PxU32	mMaxNbRegions;	//!< Max number of regions supported by the broad-phase (0 = explicit regions not needed)
	};

	/**
	\brief Broadphase descriptor.

	This structure is used to create a standalone broadphase. It captures all the parameters needed to
	initialize a broadphase.

	For the GPU broadphase (PxBroadPhaseType::eGPU) it is necessary to provide a CUDA context manager.

	The kinematic filtering flags are currently not supported by the GPU broadphase. They are used to
	dismiss pairs that involve kinematic objects directly within the broadphase.

	\see	PxCreateBroadPhase
	*/
	class PxBroadPhaseDesc
	{
		public:
		PxBroadPhaseDesc(PxBroadPhaseType::Enum type = PxBroadPhaseType::eLAST) :
			mType						(type),
			mContextID					(0),
			mContextManager				(NULL),
			mFoundLostPairsCapacity		(256 * 1024),
			mDiscardStaticVsKinematic	(false),
			mDiscardKinematicVsKinematic(false)
		{}
	
		PxBroadPhaseType::Enum	mType;							//!< Desired broadphase implementation
		PxU64					mContextID;						//!< Context ID for profiler. See PxProfilerCallback.

		PxCudaContextManager*	mContextManager;				//!< (GPU) CUDA context manager, must be provided for PxBroadPhaseType::eGPU.
		PxU32					mFoundLostPairsCapacity;		//!< (GPU) Capacity of found and lost buffers allocated in GPU global memory. This is used for the found/lost pair reports in the BP.

		bool					mDiscardStaticVsKinematic;		//!< Static-vs-kinematic filtering flag. Not supported by PxBroadPhaseType::eGPU.
		bool					mDiscardKinematicVsKinematic;	//!< kinematic-vs-kinematic filtering flag. Not supported by PxBroadPhaseType::eGPU.

		PX_INLINE	bool		isValid()	const
		{
			if(PxU32(mType)>=PxBroadPhaseType::eLAST)
				return false;

			if(mType==PxBroadPhaseType::eGPU && !mContextManager)
				return false;

			return true;
		}
	};

	typedef PxU32 PxBpIndex;						//!< Broadphase index. Indexes bounds, groups and distance arrays.
	typedef PxU32 PxBpFilterGroup;					//!< Broadphase filter group.
	#define PX_INVALID_BP_FILTER_GROUP	0xffffffff	//!< Invalid broadphase filter group

	/**
	\brief Retrieves the filter group for static objects.

	Mark static objects with this group when adding them to the broadphase.
	Overlaps between static objects will not be detected. All static objects
	should have the same group.

	\return		Filter group for static objects.
	\see		PxBpFilterGroup
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API	PxBpFilterGroup	PxGetBroadPhaseStaticFilterGroup();

	/**
	\brief Retrieves a filter group for dynamic objects.

	Mark dynamic objects with this group when adding them to the broadphase.
	Each dynamic object must have an ID, and overlaps between dynamic objects that have
	the same ID will not be detected. This is useful to dismiss overlaps between shapes
	of the same (compound) actor directly within the broadphase.

	\param		id [in]		ID/Index of dynamic object
	\return		Filter group for the object.
	\see		PxBpFilterGroup
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API	PxBpFilterGroup	PxGetBroadPhaseDynamicFilterGroup(PxU32 id);

	/**
	\brief Retrieves a filter group for kinematic objects.

	Mark kinematic objects with this group when adding them to the broadphase.
	Each kinematic object must have an ID, and overlaps between kinematic objects that have
	the same ID will not be detected.

	\param		id [in]		ID/Index of kinematic object
	\return		Filter group for the object.
	\see		PxBpFilterGroup
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API	PxBpFilterGroup	PxGetBroadPhaseKinematicFilterGroup(PxU32 id);

	/**
	\brief Broadphase data update structure.

	This structure is used to update the low-level broadphase (PxBroadPhase). All added, updated and removed objects
	must be batched and submitted at once to the broadphase.

	Broadphase objects have bounds, a filtering group, and a distance. With the low-level broadphase the data must be
	externally managed by the clients of the broadphase API, and passed to the update function.

	The provided bounds are non-inflated "base" bounds that can be further extended by the broadphase using the passed
	distance value. These can be contact offsets, or dynamically updated distance values for e.g. speculative contacts.
	Either way they are optional and can be left to zero. The broadphase implementations efficiently combine the base
	bounds with the per-object distance values at runtime.

	The per-object filtering groups are used to discard some pairs directly within the broadphase, which is more
	efficient than reporting the pairs and culling them in a second pass.

	\see	PxBpFilterGroup PxBpIndex PxBounds3 PxBroadPhase::update
	*/
	class PxBroadPhaseUpdateData
	{
		public:

		PxBroadPhaseUpdateData(	const PxBpIndex* created, PxU32 nbCreated,
								const PxBpIndex* updated, PxU32 nbUpdated,
								const PxBpIndex* removed, PxU32 nbRemoved,
								const PxBounds3* bounds, const PxBpFilterGroup* groups, const float* distances,
								PxU32 capacity) :
			mCreated	(created),	mNbCreated	(nbCreated),
			mUpdated	(updated),	mNbUpdated	(nbUpdated),
			mRemoved	(removed),	mNbRemoved	(nbRemoved),
			mBounds		(bounds),	mGroups		(groups),	mDistances	(distances),
			mCapacity	(capacity)
		{
		}

		PxBroadPhaseUpdateData(const PxBroadPhaseUpdateData& other) :
			mCreated	(other.mCreated),	mNbCreated	(other.mNbCreated),
			mUpdated	(other.mUpdated),	mNbUpdated	(other.mNbUpdated),
			mRemoved	(other.mRemoved),	mNbRemoved	(other.mNbRemoved),
			mBounds		(other.mBounds),	mGroups		(other.mGroups),	mDistances	(other.mDistances),
			mCapacity	(other.mCapacity)
		{
		}

		PxBroadPhaseUpdateData& operator=(const PxBroadPhaseUpdateData& other);

		const PxBpIndex*		mCreated;	//!< Indices of created objects.
		const PxU32				mNbCreated;	//!< Number of created objects.

		const PxBpIndex*		mUpdated;	//!< Indices of updated objects.
		const PxU32				mNbUpdated;	//!< Number of updated objects.

		const PxBpIndex*		mRemoved;	//!< Indices of removed objects.
		const PxU32				mNbRemoved;	//!< Number of removed objects.

		const PxBounds3*		mBounds;	//!< (Persistent) array of bounds.
		const PxBpFilterGroup*	mGroups;	//!< (Persistent) array of groups.
		const float*			mDistances;	//!< (Persistent) array of distances.
		const PxU32				mCapacity;	//!< Capacity of bounds / groups / distance buffers.
	};

	/**
	\brief Broadphase pair.

	A pair of indices returned by the broadphase for found or lost pairs.

	\see	PxBroadPhaseResults
	*/
	struct PxBroadPhasePair
	{
		PxBpIndex	mID0;	//!< Index of first object
		PxBpIndex	mID1;	//!< Index of second object
	};

	/**
	\brief Broadphase results.

	Set of found and lost pairs after a broadphase update.

	\see	PxBroadPhasePair PxBroadPhase::fetchResults PxAABBManager::fetchResults
	*/
	struct PxBroadPhaseResults
	{
		PxBroadPhaseResults() : mNbCreatedPairs(0), mCreatedPairs(NULL), mNbDeletedPairs(0), mDeletedPairs(NULL)	{}

		PxU32					mNbCreatedPairs;	//!< Number of new/found/created pairs.
		const PxBroadPhasePair*	mCreatedPairs;		//!< Array of new/found/created pairs.

		PxU32					mNbDeletedPairs;	//!< Number of lost/deleted pairs.
		const PxBroadPhasePair*	mDeletedPairs;		//!< Array of lost/deleted pairs.
	};

	/**
	\brief Broadphase regions.

	An API to manage broadphase regions. Only needed for the MBP broadphase (PxBroadPhaseType::eMBP).

	\see	PxBroadPhase::getRegions()
	*/
	class PxBroadPhaseRegions
	{
		protected:
				PxBroadPhaseRegions()	{}
		virtual	~PxBroadPhaseRegions()	{}
		public:

		/**
		\brief Returns number of regions currently registered in the broad-phase.

		\return Number of regions
		*/
		virtual	PxU32	getNbRegions()	const	= 0;

		/**
		\brief Gets broad-phase regions.

		\param	userBuffer	[out]	Returned broad-phase regions
		\param	bufferSize	[in]	Size of provided userBuffer.
		\param	startIndex	[in]	Index of first desired region, in [0 ; getNbRegions()[
		\return Number of written out regions.
		\see	PxBroadPhaseRegionInfo
		*/
		virtual	PxU32	getRegions(PxBroadPhaseRegionInfo* userBuffer, PxU32 bufferSize, PxU32 startIndex=0)	const	= 0;

		/**
		\brief Adds a new broad-phase region.

		The total number of regions is limited to PxBroadPhaseCaps::mMaxNbRegions. If that number is exceeded, the call is ignored.

		The newly added region will be automatically populated with already existing objects that touch it, if the
		'populateRegion' parameter is set to true. Otherwise the newly added region will be empty, and it will only be
		populated with objects when those objects are added to the simulation, or updated if they already exist.

		Using 'populateRegion=true' has a cost, so it is best to avoid it if possible. In particular it is more efficient
		to create the empty regions first (with populateRegion=false) and then add the objects afterwards (rather than
		the opposite).

		Objects automatically move from one region to another during their lifetime. The system keeps tracks of what
		regions a given object is in. It is legal for an object to be in an arbitrary number of regions. However if an
		object leaves all regions, or is created outside of all regions, several things happen:
			- collisions get disabled for this object
			- the object appears in the getOutOfBoundsObjects() array

		If an out-of-bounds object, whose collisions are disabled, re-enters a valid broadphase region, then collisions
		are re-enabled for that object.

		\param	region			[in] User-provided region data
		\param	populateRegion	[in] True to automatically populate the newly added region with existing objects touching it
		\param	bounds			[in] User-managed array of bounds
		\param	distances		[in] User-managed array of distances

		\return Handle for newly created region, or 0xffffffff in case of failure.
		\see	PxBroadPhaseRegion getOutOfBoundsObjects()
		*/
		virtual	PxU32	addRegion(const PxBroadPhaseRegion& region, bool populateRegion, const PxBounds3* bounds, const float* distances)	= 0;

		/**
		\brief Removes a broad-phase region.

		If the region still contains objects, and if those objects do not overlap any region any more, they are not
		automatically removed from the simulation. Instead, the PxBroadPhaseCallback::onObjectOutOfBounds notification
		is used for each object. Users are responsible for removing the objects from the simulation if this is the
		desired behavior.

		If the handle is invalid, or if a valid handle is removed twice, an error message is sent to the error stream.

		\param	handle	[in] Region's handle, as returned by addRegion
		\return True if success
		*/
		virtual	bool	removeRegion(PxU32 handle)	= 0;

		/*
		\brief Return the number of objects that are not in any region.
		*/
		virtual	PxU32	getNbOutOfBoundsObjects()	const	= 0;

		/*
		\brief Return an array of objects that are not in any region.
		*/
		virtual	const PxU32*	getOutOfBoundsObjects()	const	= 0;
	};

	/**
	\brief Low-level broadphase API.

	This low-level API only supports batched updates and leaves most of the data management to its clients.

	This is useful if you want to use the broadphase with your own memory buffers. Note however that the GPU broadphase
	works best with buffers allocated in CUDA memory. The getAllocator() function returns an allocator that is compatible
	with the selected broadphase. It is recommended to allocate and deallocate the broadphase data (bounds, groups, distances)
	using this allocator.

	Important note: it must be safe to load 4 bytes past the end of the provided bounds array.
	
	The high-level broadphase API (PxAABBManager) is an easier-to-use interface that automatically deals with these requirements.

	\see	PxCreateBroadPhase
	*/
	class PxBroadPhase
	{
		protected:
				PxBroadPhase()	{}
		virtual	~PxBroadPhase()	{}
		public:

		/*
		\brief Releases the broadphase.
		*/
		virtual	void	release()	= 0;

		/**
		\brief Gets the broadphase type.

		\return	Broadphase type.
		\see	PxBroadPhaseType::Enum
		*/
		virtual	PxBroadPhaseType::Enum	getType()	const	= 0;

		/**
		\brief Gets broad-phase caps.

		\param	caps	[out] Broad-phase caps
		\see	PxBroadPhaseCaps
		*/
		virtual	void	getCaps(PxBroadPhaseCaps& caps)	const	= 0;

		/**
		\brief Retrieves the regions API if applicable.

		For broadphases that do not use explicit user-defined regions, this call returns NULL.

		\return	Region API, or NULL.
		\see	PxBroadPhaseRegions
		*/
		virtual	PxBroadPhaseRegions*	getRegions()	= 0;

		/**
		\brief Retrieves the broadphase allocator.

		User-provided buffers should ideally be allocated with this allocator, for best performance.
		This is especially true for the GPU broadphases, whose buffers need to be allocated in CUDA
		host memory.

		\return	The broadphase allocator.
		\see	PxAllocatorCallback
		*/
		virtual	PxAllocatorCallback*	getAllocator()	= 0;

		/**
		\brief Retrieves the profiler's context ID.

		\return	The context ID.
		\see	PxBroadPhaseDesc
		*/
		virtual	PxU64	getContextID()	const	= 0;

		/**
		\brief Sets a scratch buffer

		Some broadphases might take advantage of a scratch buffer to limit runtime allocations.

		All broadphases still work without providing a scratch buffer, this is an optional function
		that can potentially reduce runtime allocations.

		\param	scratchBlock	[in] The scratch buffer
		\param	size			[in] Size of the scratch buffer in bytes
		*/
		virtual	void	setScratchBlock(void* scratchBlock, PxU32 size)	= 0;

		/**
		\brief Updates the broadphase and computes the lists of created/deleted pairs.

		The provided update data describes changes to objects since the last broadphase update.

		To benefit from potentially multithreaded implementations, it is necessary to provide a continuation
		task to the function. It is legal to pass NULL there, but the underlying (CPU) implementations will
		then run single-threaded.

		\param	updateData		[in] The update data
		\param	continuation	[in] Continuation task to enable multi-threaded implementations, or NULL.
		\see	PxBroadPhaseUpdateData PxBaseTask
		*/
		virtual	void	update(const PxBroadPhaseUpdateData& updateData, PxBaseTask* continuation=NULL)	= 0;

		/**
		\brief Retrieves the broadphase results after an update.

		This should be called once after each update call to retrieve the results of the broadphase. The
		results are incremental, i.e. the system only returns new and lost pairs, not all current pairs.

		\param	results		[out] The broadphase results
		\see	PxBroadPhaseResults
		*/
		virtual	void	fetchResults(PxBroadPhaseResults& results)	= 0;

		/**
		\brief Helper for single-threaded updates.

		This short helper function performs a single-theaded update and reports the results in a single call.

		\param	results		[out] The broadphase results
		\param	updateData	[in] The update data
		\see	PxBroadPhaseUpdateData PxBroadPhaseResults
		*/
		PX_FORCE_INLINE	void	update(PxBroadPhaseResults& results, const PxBroadPhaseUpdateData& updateData)
		{
			update(updateData);
			fetchResults(results);
		}
	};

	/**
	\brief Broadphase factory function.

	Use this function to create a new standalone broadphase.

	\param	desc	[in] Broadphase descriptor
	\return	Newly created broadphase, or NULL
	\see	PxBroadPhase PxBroadPhaseDesc
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API	PxBroadPhase*	PxCreateBroadPhase(const PxBroadPhaseDesc& desc);

	/**
	\brief High-level broadphase API.

	The low-level broadphase API (PxBroadPhase) only supports batched updates and has a few non-trivial
	requirements for managing the bounds data.

	The high-level broadphase API (PxAABBManager) is an easier-to-use one-object-at-a-time API that
	automatically deals with the quirks of the PxBroadPhase data management.

	\see	PxCreateAABBManager
	*/
	class PxAABBManager
	{
		protected:
				PxAABBManager()		{}
		virtual	~PxAABBManager()	{}
		public:

		/*
		\brief Releases the AABB manager.
		*/
		virtual	void	release()	= 0;

		/**
		\brief Retrieves the underlying broadphase.

		\return	The managed broadphase.
		\see	PxBroadPhase
		*/
		virtual	PxBroadPhase&	getBroadPhase()	= 0;

		/**
		\brief Retrieves the managed bounds.

		This is needed as input parameters to functions like PxBroadPhaseRegions::addRegion.

		\return	The managed object bounds.
		\see	PxBounds3
		*/
		virtual	const PxBounds3*	getBounds()	const	= 0;

		/**
		\brief Retrieves the managed distances.

		This is needed as input parameters to functions like PxBroadPhaseRegions::addRegion.

		\return	The managed object distances.
		*/
		virtual	const float*	getDistances()	const	= 0;

		/**
		\brief Retrieves the managed filter groups.

		\return	The managed object groups.
		*/
		virtual	const PxBpFilterGroup*	getGroups()	const	= 0;

		/**
		\brief Retrieves the managed buffers' capacity.

		Bounds, distances and groups buffers have the same capacity.

		\return	The managed buffers' capacity.
		*/
		virtual	PxU32	getCapacity()	const	= 0;

		/**
		\brief Adds an object to the manager.

		Objects' indices are externally managed, i.e. they must be provided by users (as opposed to handles
		that could be returned by this manager). The design allows users to identify an object by a single ID,
		and use the same ID in multiple sub-systems.

		\param	index		[in] The object's index
		\param	bounds		[in] The object's bounds
		\param	group		[in] The object's filter group
		\param	distance	[in] The object's distance (optional)
		\see	PxBpIndex PxBounds3 PxBpFilterGroup
		*/
		virtual	void	addObject(PxBpIndex index, const PxBounds3& bounds, PxBpFilterGroup group, float distance=0.0f)	= 0;

		/**
		\brief Removes an object from the manager.

		\param	index	[in] The object's index
		\see	PxBpIndex
		*/
		virtual	void	removeObject(PxBpIndex index)	= 0;

		/**
		\brief Updates an object in the manager.

		This call can update an object's bounds, distance, or both.
		It is not possible to update an object's filter group.

		\param	index		[in] The object's index
		\param	bounds		[in] The object's updated bounds, or NULL
		\param	distance	[in] The object's updated distance, or NULL
		\see	PxBpIndex PxBounds3
		*/
		virtual	void	updateObject(PxBpIndex index, const PxBounds3* bounds=NULL, const float* distance=NULL)	= 0;

		/**
		\brief Updates the broadphase and computes the lists of created/deleted pairs.

		The data necessary for updating the broadphase is internally computed by the AABB manager.

		To benefit from potentially multithreaded implementations, it is necessary to provide a continuation
		task to the function. It is legal to pass NULL there, but the underlying (CPU) implementations will
		then run single-threaded.

		\param	continuation	[in] Continuation task to enable multi-threaded implementations, or NULL.
		\see	PxBaseTask
		*/
		virtual	void	update(PxBaseTask* continuation=NULL)	= 0;

		/**
		\brief Retrieves the broadphase results after an update.

		This should be called once after each update call to retrieve the results of the broadphase. The
		results are incremental, i.e. the system only returns new and lost pairs, not all current pairs.

		\param	results		[out] The broadphase results
		\see	PxBroadPhaseResults
		*/
		virtual	void	fetchResults(PxBroadPhaseResults& results)	= 0;

		/**
		\brief Helper for single-threaded updates.

		This short helper function performs a single-theaded update and reports the results in a single call.

		\param	results		[out] The broadphase results
		\see	PxBroadPhaseResults
		*/
		PX_FORCE_INLINE	void	update(PxBroadPhaseResults& results)
		{
			update();
			fetchResults(results);
		}
	};

	/**
	\brief AABB manager factory function.

	Use this function to create a new standalone high-level broadphase.

	\param	broadphase	[in] The broadphase that will be managed by the AABB manager
	\return	Newly created AABB manager, or NULL
	\see	PxAABBManager PxBroadPhase
	*/
	PX_C_EXPORT PX_PHYSX_CORE_API	PxAABBManager*	PxCreateAABBManager(PxBroadPhase& broadphase);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
