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

#ifndef PX_NEW_SCENE_QUERY_SYSTEM_H
#define PX_NEW_SCENE_QUERY_SYSTEM_H
/** \addtogroup extensions
  @{
*/

#include "PxSceneQuerySystem.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief A custom scene query system.

	This is an example of a custom scene query system. It augments the PxSceneQuerySystem API to support an arbitrary number
	of "pruners", instead of the usual hardcoded two.

	It might not be possible to support the whole PxSceneQuerySystem API in this context. See the source code for details.

	@see PxSceneQuerySystem
	*/
	class PxCustomSceneQuerySystem : public PxSceneQuerySystem
	{
		public:
				PxCustomSceneQuerySystem()	{}
		virtual	~PxCustomSceneQuerySystem()	{}

		/**
		\brief Adds a pruner to the system.

		The internal PhysX scene-query system uses two regular pruners (one for static shapes, one for dynamic shapes) and an optional
		compound pruner. Our custom scene query system supports an arbitrary number of regular pruners.

		This can be useful to reduce the load on each pruner, in particular during updates, when internal trees are rebuilt in the
		background. On the other hand this implementation simply iterates over all created pruners to perform queries, so their cost
		might increase if a large number of pruners is used.

		In any case this serves as an example of how the PxSceneQuerySystem API can be used to customize scene queries.

		\param[in] primaryType		Desired primary (main) type for the new pruner
		\param[in] secondaryType	Secondary type when primary type is PxPruningStructureType::eDYNAMIC_AABB_TREE.
		\param[in] preallocated		Optional number of preallocated shapes in the new pruner

		\return	A pruner index

		@see PxCustomSceneQuerySystem PxSceneQueryUpdateMode PxCustomSceneQuerySystemAdapter PxSceneDesc::sceneQuerySystem
		*/
		virtual	PxU32	addPruner(PxPruningStructureType::Enum primaryType, PxDynamicTreeSecondaryPruner::Enum secondaryType, PxU32 preallocated=0)	= 0;

		/**
		\brief Start custom build-steps for all pruners

		This function is used in combination with customBuildstep() and finishCustomBuildstep() to let users take control
		of the pruners' build-step & commit calls - basically the pruners' update functions. These functions should be used
		with the PxSceneQueryUpdateMode::eBUILD_DISABLED_COMMIT_DISABLED update mode, otherwise the build-steps will happen
		automatically in fetchResults. For N pruners it can be more efficient to use these custom build-step functions to
		perform the updates in parallel:

		- call startCustomBuildstep() first (one synchronous call)
		- for each pruner, call customBuildstep() (asynchronous calls from multiple threads)
		- once it is done, call finishCustomBuildstep() to finish the update (synchronous call)

		The multi-threaded update is more efficient here than what it is in PxScene, because the "flushShapes()" call is
		also multi-threaded (while it is not in PxScene).

		Note that users are responsible for locks here, and these calls should not overlap with other SQ calls. In particular
		one should not add new objects to the SQ system or perform queries while these calls are happening.

		\return	The number of pruners in the system.

		@see customBuildstep finishCustomBuildstep PxSceneQueryUpdateMode
		*/
		virtual	PxU32	startCustomBuildstep()	= 0;

		/**
		\brief Perform a custom build-step for a given pruner.

		\param[in] index	Pruner index (should be between 0 and the number returned by startCustomBuildstep)

		@see startCustomBuildstep finishCustomBuildstep
		*/
		virtual	void	customBuildstep(PxU32 index)	= 0;

		/**
		\brief Finish custom build-steps

		Call this function once after all the customBuildstep() calls are done.

		@see startCustomBuildstep customBuildstep
		*/
		virtual	void	finishCustomBuildstep()	= 0;
	};

	/**
	\brief An adapter class to customize the object-to-pruner mapping.

	In the regular PhysX code static shapes went to the static pruner, and dynamic shapes went to the
	dynamic pruner.

	This class is a replacement for this mapping when N user-defined pruners are involved.
	*/
	class PxCustomSceneQuerySystemAdapter
	{
		public:
						PxCustomSceneQuerySystemAdapter()	{}
		virtual			~PxCustomSceneQuerySystemAdapter()	{}

		/**
		\brief Gets a pruner index for an actor/shape.

		This user-defined function tells the system in which pruner a given actor/shape should go.

		\note The returned index must be valid, i.e. it must have been previously returned to users by PxCustomSceneQuerySystem::addPruner.

		\param[in] actor	The actor
		\param[in] shape	The shape

		\return	A pruner index for this actor/shape.

		@see PxRigidActor PxShape PxCustomSceneQuerySystem::addPruner
		*/
		virtual	PxU32	getPrunerIndex(const PxRigidActor& actor, const PxShape& shape)	const	= 0;

		/**
		\brief Pruner filtering callback.

		This will be called for each query to validate whether it should process a given pruner.

		\param[in] prunerIndex	The index of currently processed pruner
		\param[in] context		The query context
		\param[in] filterData	The query's filter data
		\param[in] filterCall	The query's filter callback

		\return	True to process the pruner, false to skip it entirely
		*/
		virtual	bool	processPruner(PxU32 prunerIndex, const PxQueryThreadContext* context, const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall) const = 0;
	};

	/**
	\brief Creates a custom scene query system.

	This is similar to PxCreateExternalSceneQuerySystem, except this function creates a PxCustomSceneQuerySystem object.
	It can be plugged to PxScene the same way, via PxSceneDesc::sceneQuerySystem.

	\param[in] sceneQueryUpdateMode		Desired update mode
	\param[in] contextID				Context ID parameter, sent to the profiler
	\param[in] adapter					Adapter class implementing our extended API
	\param[in] usesTreeOfPruners		True to keep pruners themselves in a BVH, which might increase query performance if a lot of pruners are involved

	\return	A custom SQ system instance

	@see PxCustomSceneQuerySystem PxSceneQueryUpdateMode PxCustomSceneQuerySystemAdapter PxSceneDesc::sceneQuerySystem
	*/
	PxCustomSceneQuerySystem* PxCreateCustomSceneQuerySystem(PxSceneQueryUpdateMode::Enum sceneQueryUpdateMode, PxU64 contextID, const PxCustomSceneQuerySystemAdapter& adapter, bool usesTreeOfPruners=false);

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
