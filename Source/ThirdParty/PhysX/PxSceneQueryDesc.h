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

#ifndef PX_SCENE_QUERY_DESC_H
#define PX_SCENE_QUERY_DESC_H
/** \addtogroup physics
@{
*/

#include "PxPhysXConfig.h"
#include "geometry/PxBVHBuildStrategy.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxSceneQuerySystem;

/**
\brief Pruning structure used to accelerate scene queries.

eNONE uses a simple data structure that consumes less memory than the alternatives,
but generally has slower query performance.

eDYNAMIC_AABB_TREE usually provides the fastest queries. However there is a
constant per-frame management cost associated with this structure. How much work should
be done per frame can be tuned via the #PxSceneQueryDesc::dynamicTreeRebuildRateHint
parameter.

eSTATIC_AABB_TREE is typically used for static objects. It is the same as the
dynamic AABB tree, without the per-frame overhead. This can be a good choice for static
objects, if no static objects are added, moved or removed after the scene has been
created. If there is no such guarantee (e.g. when streaming parts of the world in and out),
then the dynamic version is a better choice even for static objects.
*/
struct PxPruningStructureType
{
	enum Enum
	{
		eNONE,					//!< Using a simple data structure
		eDYNAMIC_AABB_TREE,		//!< Using a dynamic AABB tree
		eSTATIC_AABB_TREE,		//!< Using a static AABB tree

		eLAST
	};
};

/**
\brief Secondary pruning structure used for newly added objects in dynamic trees.

Dynamic trees (PxPruningStructureType::eDYNAMIC_AABB_TREE) are slowly rebuilt
over several frames. A secondary pruning structure holds and manages objects
added to the scene while this rebuild is in progress.

eNONE ignores newly added objects. This means that for a number of frames (roughly
defined by PxSceneQueryDesc::dynamicTreeRebuildRateHint) newly added objects will
be ignored by scene queries. This can be acceptable when streaming large worlds, e.g.
when the objects added at the boundaries of the game world don't immediately need to be
visible from scene queries (it would be equivalent to streaming that data in a few frames
later). The advantage of this approach is that there is no CPU cost associated with
inserting the new objects in the scene query data structures, and no extra runtime cost
when performing queries.

eBUCKET uses a structure similar to PxPruningStructureType::eNONE. Insertion is fast but
query cost can be high.

eINCREMENTAL uses an incremental AABB-tree, with no direct PxPruningStructureType equivalent.
Query time is fast but insertion cost can be high.

eBVH uses a PxBVH structure. This usually offers the best overall performance.
*/
struct PxDynamicTreeSecondaryPruner
{
	enum Enum
	{
		eNONE,			//!< no secondary pruner, new objects aren't visible to SQ for a few frames
		eBUCKET	,		//!< bucket-based secondary pruner, faster updates, slower query time
		eINCREMENTAL,	//!< incremental-BVH secondary pruner, faster query time, slower updates
		eBVH,			//!< PxBVH-based secondary pruner, good overall performance

		eLAST
	};
};

/**
\brief Scene query update mode

This enum controls what work is done when the scene query system is updated. The updates traditionally happen when PxScene::fetchResults
is called. This function then calls PxSceneQuerySystem::finalizeUpdates, where the update mode is used.

fetchResults/finalizeUpdates will sync changed bounds during simulation and update the scene query bounds in pruners, this work is mandatory.

eBUILD_ENABLED_COMMIT_ENABLED does allow to execute the new AABB tree build step during fetchResults/finalizeUpdates, additionally
the pruner commit is called where any changes are applied. During commit PhysX refits the dynamic scene query tree and if a new tree
was built and the build finished the tree is swapped with current AABB tree. 

eBUILD_ENABLED_COMMIT_DISABLED does allow to execute the new AABB tree build step during fetchResults/finalizeUpdates. Pruner commit
is not called, this means that refit will then occur during the first scene query following fetchResults/finalizeUpdates, or may be forced
by the method PxScene::flushQueryUpdates() / PxSceneQuerySystemBase::flushUpdates().

eBUILD_DISABLED_COMMIT_DISABLED no further scene query work is executed. The scene queries update needs to be called manually, see
PxScene::sceneQueriesUpdate (see that function's doc for the equivalent PxSceneQuerySystem sequence). It is recommended to call
PxScene::sceneQueriesUpdate right after fetchResults/finalizeUpdates as the pruning structures are not updated.
*/
struct PxSceneQueryUpdateMode
{
	enum Enum
	{
		eBUILD_ENABLED_COMMIT_ENABLED,		//!< Both scene query build and commit are executed.
		eBUILD_ENABLED_COMMIT_DISABLED,		//!< Scene query build only is executed.
		eBUILD_DISABLED_COMMIT_DISABLED		//!< No work is done, no update of scene queries
	};
};

/**
\brief Descriptor class for scene query system. See #PxSceneQuerySystem.
*/
class PxSceneQueryDesc
{
public:

	/**
	\brief Defines the structure used to store static objects (PxRigidStatic actors).

	There are usually a lot more static actors than dynamic actors in a scene, so they are stored
	in a separate structure. The idea is that when dynamic actors move each frame, the static structure
	remains untouched and does not need updating.

	<b>Default:</b> PxPruningStructureType::eDYNAMIC_AABB_TREE

	\note Only PxPruningStructureType::eSTATIC_AABB_TREE and PxPruningStructureType::eDYNAMIC_AABB_TREE are allowed here.

	@see PxPruningStructureType PxSceneSQSystem.getStaticStructure()
	*/
	PxPruningStructureType::Enum	staticStructure;

	/**
	\brief Defines the structure used to store dynamic objects (non-PxRigidStatic actors).

	<b>Default:</b> PxPruningStructureType::eDYNAMIC_AABB_TREE

	@see PxPruningStructureType PxSceneSQSystem.getDynamicStructure()
	*/
	PxPruningStructureType::Enum	dynamicStructure;

	/**
	\brief Hint for how much work should be done per simulation frame to rebuild the pruning structures.

	This parameter gives a hint on the distribution of the workload for rebuilding the dynamic AABB tree
	pruning structure #PxPruningStructureType::eDYNAMIC_AABB_TREE. It specifies the desired number of simulation frames
	the rebuild process should take. Higher values will decrease the workload per frame but the pruning
	structure will get more and more outdated the longer the rebuild takes (which can make
	scene queries less efficient).

	\note Only used for #PxPruningStructureType::eDYNAMIC_AABB_TREE pruning structures.

	\note Both staticStructure & dynamicStructure can use a PxPruningStructureType::eDYNAMIC_AABB_TREE, in which case
	this parameter is used for both.

	\note This parameter gives only a hint. The rebuild process might still take more or less time depending on the
	number of objects involved.

	<b>Range:</b> [4, PX_MAX_U32)<br>
	<b>Default:</b> 100

	@see PxSceneQuerySystemBase.setDynamicTreeRebuildRateHint() PxSceneQuerySystemBase.getDynamicTreeRebuildRateHint()
	*/
	PxU32	dynamicTreeRebuildRateHint;

	/**
	\brief Secondary pruner for dynamic tree.

	This is used for PxPruningStructureType::eDYNAMIC_AABB_TREE structures, to control how objects added to the system
	at runtime are managed.

	\note Both staticStructure & dynamicStructure can use a PxPruningStructureType::eDYNAMIC_AABB_TREE, in which case
	this parameter is used for both.

	<b>Default:</b> PxDynamicTreeSecondaryPruner::eINCREMENTAL

	@see PxDynamicTreeSecondaryPruner
	*/
	PxDynamicTreeSecondaryPruner::Enum dynamicTreeSecondaryPruner;

	/**
	\brief Build strategy for PxSceneQueryDesc::staticStructure.

	This parameter is used to refine / control the build strategy of PxSceneQueryDesc::staticStructure. This is only
	used with PxPruningStructureType::eDYNAMIC_AABB_TREE and PxPruningStructureType::eSTATIC_AABB_TREE.

	<b>Default:</b> PxBVHBuildStrategy::eFAST

	@see PxBVHBuildStrategy PxSceneQueryDesc::staticStructure
	*/
	PxBVHBuildStrategy::Enum	staticBVHBuildStrategy;

	/**
	\brief Build strategy for PxSceneQueryDesc::dynamicStructure.

	This parameter is used to refine / control the build strategy of PxSceneQueryDesc::dynamicStructure. This is only
	used with PxPruningStructureType::eDYNAMIC_AABB_TREE and PxPruningStructureType::eSTATIC_AABB_TREE.

	<b>Default:</b> PxBVHBuildStrategy::eFAST

	@see PxBVHBuildStrategy PxSceneQueryDesc::dynamicStructure
	*/
	PxBVHBuildStrategy::Enum	dynamicBVHBuildStrategy;

	/**
	\brief Number of objects per node for PxSceneQueryDesc::staticStructure.

	This parameter is used to refine / control the number of objects per node for PxSceneQueryDesc::staticStructure.
	This is only used with PxPruningStructureType::eDYNAMIC_AABB_TREE and PxPruningStructureType::eSTATIC_AABB_TREE.

	This parameter has an impact on how quickly the structure gets built, and on the per-frame cost of maintaining
	the structure. Increasing this value gives smaller AABB-trees that use less memory, are faster to build and
	update, but it can lead to slower queries.

	<b>Default:</b> 4

	@see PxSceneQueryDesc::staticStructure
	*/
	PxU32	staticNbObjectsPerNode;

	/**
	\brief Number of objects per node for PxSceneQueryDesc::dynamicStructure.

	This parameter is used to refine / control the number of objects per node for PxSceneQueryDesc::dynamicStructure.
	This is only used with PxPruningStructureType::eDYNAMIC_AABB_TREE and PxPruningStructureType::eSTATIC_AABB_TREE.

	This parameter has an impact on how quickly the structure gets built, and on the per-frame cost of maintaining
	the structure. Increasing this value gives smaller AABB-trees that use less memory, are faster to build and
	update, but it can lead to slower queries.

	<b>Default:</b> 4

	@see PxSceneQueryDesc::dynamicStructure
	*/
	PxU32	dynamicNbObjectsPerNode;

	/**
	\brief Defines the scene query update mode.

	<b>Default:</b> PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_ENABLED

	@see PxSceneQuerySystemBase.setUpdateMode() PxSceneQuerySystemBase.getUpdateMode()
	*/
	PxSceneQueryUpdateMode::Enum sceneQueryUpdateMode;

public:
	/**
	\brief constructor sets to default.
	*/	
	PX_INLINE PxSceneQueryDesc();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault();

	/**
	\brief Returns true if the descriptor is valid.
	\return true if the current settings are valid.
	*/
	PX_INLINE bool isValid() const;
};

PX_INLINE PxSceneQueryDesc::PxSceneQueryDesc():
	staticStructure				(PxPruningStructureType::eDYNAMIC_AABB_TREE),
	dynamicStructure			(PxPruningStructureType::eDYNAMIC_AABB_TREE),
	dynamicTreeRebuildRateHint	(100),
	dynamicTreeSecondaryPruner	(PxDynamicTreeSecondaryPruner::eINCREMENTAL),
	staticBVHBuildStrategy		(PxBVHBuildStrategy::eFAST),
	dynamicBVHBuildStrategy		(PxBVHBuildStrategy::eFAST),
	staticNbObjectsPerNode		(4),
	dynamicNbObjectsPerNode		(4),
	sceneQueryUpdateMode		(PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_ENABLED)
{
}

PX_INLINE void PxSceneQueryDesc::setToDefault()
{
	*this = PxSceneQueryDesc();
}

PX_INLINE bool PxSceneQueryDesc::isValid() const
{
	if(staticStructure!=PxPruningStructureType::eSTATIC_AABB_TREE && staticStructure!=PxPruningStructureType::eDYNAMIC_AABB_TREE)
		return false;

	if(dynamicTreeRebuildRateHint < 4)
		return false;

	return true;
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
