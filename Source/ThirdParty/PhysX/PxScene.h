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

#ifndef PX_SCENE_H
#define PX_SCENE_H
/** \addtogroup physics
@{
*/

#include "PxSceneQuerySystem.h"
#include "PxSceneDesc.h"
#include "PxVisualizationParameter.h"
#include "PxSimulationStatistics.h"
#include "PxClient.h"
#include "task/PxTask.h"
#include "PxArticulationFlag.h"
#include "PxSoftBodyFlag.h"
#include "PxHairSystemFlag.h"
#include "PxActorData.h"
#include "PxParticleSystemFlag.h"
#include "PxParticleSolverType.h"

#include "pvd/PxPvdSceneClient.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxCollection;
class PxConstraint;
class PxSimulationEventCallback;
class PxPhysics;
class PxAggregate;
class PxRenderBuffer;
class PxArticulationReducedCoordinate;
class PxParticleSystem;

struct PxContactPairHeader;

typedef PxU8 PxDominanceGroup;

class PxPvdSceneClient;

class PxSoftBody;
class PxFEMCloth;
class PxHairSystem;

/**
\brief Expresses the dominance relationship of a contact.
For the time being only three settings are permitted:

(1, 1), (0, 1), and (1, 0).

@see getDominanceGroup() PxDominanceGroup PxScene::setDominanceGroupPair()
*/	
struct PxDominanceGroupPair
{
	PxDominanceGroupPair(PxU8 a, PxU8 b) 
		: dominance0(a), dominance1(b) {}
	PxU8 dominance0;
	PxU8 dominance1;
};


/**
\brief Identifies each type of actor for retrieving actors from a scene.

\note #PxArticulationLink objects are not supported. Use the #PxArticulationReducedCoordinate object to retrieve all its links.

@see PxScene::getActors(), PxScene::getNbActors()
*/
struct PxActorTypeFlag
{
	enum Enum
	{
		/**
		\brief A static rigid body
		@see PxRigidStatic
		*/
		eRIGID_STATIC		= (1 << 0),

		/**
		\brief A dynamic rigid body
		@see PxRigidDynamic
		*/
		eRIGID_DYNAMIC		= (1 << 1)
	};
};

/**
\brief Collection of set bits defined in PxActorTypeFlag.

@see PxActorTypeFlag
*/
typedef PxFlags<PxActorTypeFlag::Enum,PxU16> PxActorTypeFlags;
PX_FLAGS_OPERATORS(PxActorTypeFlag::Enum,PxU16)

class PxActor;

/**
\brief Broad-phase callback to receive broad-phase related events.

Each broadphase callback object is associated with a PxClientID. It is possible to register different
callbacks for different clients. The callback functions are called this way:
- for shapes/actors, the callback assigned to the actors' clients are used
- for aggregates, the callbacks assigned to clients from aggregated actors  are used

\note SDK state should not be modified from within the callbacks. In particular objects should not
be created or destroyed. If state modification is needed then the changes should be stored to a buffer
and performed after the simulation step.

<b>Threading:</b> It is not necessary to make this class thread safe as it will only be called in the context of the
user thread.

@see PxSceneDesc PxScene.setBroadPhaseCallback() PxScene.getBroadPhaseCallback()
*/
class PxBroadPhaseCallback
{
	public:
	virtual				~PxBroadPhaseCallback()	{}

	/**
	\brief Out-of-bounds notification.
		
	This function is called when an object leaves the broad-phase.

	\param[in] shape	Shape that left the broad-phase bounds
	\param[in] actor	Owner actor
	*/
	virtual		void	onObjectOutOfBounds(PxShape& shape, PxActor& actor) = 0;

	/**
	\brief Out-of-bounds notification.
		
	This function is called when an aggregate leaves the broad-phase.

	\param[in] aggregate	Aggregate that left the broad-phase bounds
	*/
	virtual		void	onObjectOutOfBounds(PxAggregate& aggregate) = 0;
};

/** 
 \brief A scene is a collection of bodies and constraints which can interact.

 The scene simulates the behavior of these objects over time. Several scenes may exist 
 at the same time, but each body or constraint is specific to a scene 
 -- they may not be shared.

 @see PxSceneDesc PxPhysics.createScene() release()
*/
class PxScene : public PxSceneSQSystem
{
	protected:
	
	/************************************************************************************************/

	/** @name Basics
	*/
	//@{
	
								PxScene() : userData(NULL)	{}
	virtual						~PxScene()					{}

	public:

	/**
	\brief Deletes the scene.

	Removes any actors and constraint shaders from this scene
	(if the user hasn't already done so).

	Be sure	to not keep a reference to this object after calling release.
	Avoid release calls while the scene is simulating (in between simulate() and fetchResults() calls).
	
	@see PxPhysics.createScene() 
	*/
	virtual		void			release() = 0;

	/**
	\brief Sets a scene flag. You can only set one flag at a time.

	\note Not all flags are mutable and changing some will result in an error. Please check #PxSceneFlag to see which flags can be changed.

	@see PxSceneFlag
	*/
	virtual		void			setFlag(PxSceneFlag::Enum flag, bool value) = 0;

	/**
	\brief Get the scene flags.

	\return The scene flags. See #PxSceneFlag

	@see PxSceneFlag
	*/
	virtual		PxSceneFlags	getFlags() const = 0;

	/**
	\brief Set new scene limits. 

	\note Increase the maximum capacity of various data structures in the scene. The new capacities will be 
	at least as large as required to deal with the objects currently in the scene. Further, these values 
	are for preallocation and do not represent hard limits.

	\param[in] limits Scene limits.
	@see PxSceneLimits
	*/
	virtual void				setLimits(const PxSceneLimits& limits) = 0;

	/**
	\brief Get current scene limits.
	\return Current scene limits.
	@see PxSceneLimits
	*/
	virtual PxSceneLimits		getLimits() const = 0;

	/**
	\brief Call this method to retrieve the Physics SDK.

	\return The physics SDK this scene is associated with.

	@see PxPhysics
	*/
	virtual	PxPhysics&			getPhysics() = 0;

	/**
	\brief Retrieves the scene's internal timestamp, increased each time a simulation step is completed.

	\return scene timestamp
	*/
	virtual	PxU32				getTimestamp()	const	= 0;

	//@}
	/************************************************************************************************/

	/** @name Add/Remove Articulations
	*/
	//@{
	/**
	\brief Adds an articulation to this scene.

	\note If the articulation is already assigned to a scene (see #PxArticulationReducedCoordinate::getScene), the call is ignored and an error is issued.

	\param[in] articulation The articulation to add to the scene.
	\return True if success

	@see PxArticulationReducedCoordinate
	*/
	virtual	bool				addArticulation(PxArticulationReducedCoordinate& articulation) = 0;

	/**
	\brief Removes an articulation from this scene.

	\note If the articulation is not part of this scene (see #PxArticulationReducedCoordinate::getScene), the call is ignored and an error is issued. 
	
	\note If the articulation is in an aggregate it will be removed from the aggregate.

	\param[in] articulation The articulation to remove from the scene.
	\param[in] wakeOnLostTouch Specifies whether touching objects from the previous frame should get woken up in the next frame.
	Only applies to PxArticulationReducedCoordinate and PxRigidActor types.

	@see PxArticulationReducedCoordinate, PxAggregate
	*/
	virtual	void				removeArticulation(PxArticulationReducedCoordinate& articulation, bool wakeOnLostTouch = true) = 0;

	//@}
	/************************************************************************************************/

	/** @name Add/Remove Actors
	*/
	//@{
	/**
	\brief Adds an actor to this scene.
	
	\note If the actor is already assigned to a scene (see #PxActor::getScene), the call is ignored and an error is issued.
	\note If the actor has an invalid constraint, in checked builds the call is ignored and an error is issued.

	\note You can not add individual articulation links (see #PxArticulationLink) to the scene. Use #addArticulation() instead.

	\note If the actor is a PxRigidActor then each assigned PxConstraint object will get added to the scene automatically if
	it connects to another actor that is part of the scene already. 

	\note When a BVH is provided the actor shapes are grouped together. 
	The scene query pruning structure inside PhysX SDK will store/update one
	bound per actor. The scene queries against such an actor will query actor
	bounds and then make a local space query against the provided BVH, which is in actor's local space.

	\param[in] actor	Actor to add to scene.
	\param[in] bvh		BVH for actor shapes.
	\return True if success

	@see PxActor, PxConstraint::isValid(), PxBVH
	*/
	virtual	bool				addActor(PxActor& actor, const PxBVH* bvh = NULL) = 0;

	/**
	\brief Adds actors to this scene. Only supports actors of type PxRigidStatic and PxRigidDynamic.

	\note This method only supports actors of type PxRigidStatic and PxRigidDynamic. For other actors, use addActor() instead.
	For articulation links, use addArticulation().

	\note If one of the actors is already assigned to a scene (see #PxActor::getScene), the call is ignored and an error is issued.

	\note If an actor in the array contains an invalid constraint, in checked builds the call is ignored and an error is issued.
	\note If an actor in the array is a PxRigidActor then each assigned PxConstraint object will get added to the scene automatically if
	it connects to another actor that is part of the scene already.

	\note this method is optimized for high performance. 

	\param[in] actors Array of actors to add to scene.
	\param[in] nbActors Number of actors in the array.
	\return True if success

	@see PxActor, PxConstraint::isValid()
	*/
	virtual	bool				addActors(PxActor*const* actors, PxU32 nbActors) = 0;

	/**
	\brief Adds a pruning structure together with its actors to this scene. Only supports actors of type PxRigidStatic and PxRigidDynamic.

	\note This method only supports actors of type PxRigidStatic and PxRigidDynamic. For other actors, use addActor() instead.
	For articulation links, use addArticulation().

	\note If an actor in the pruning structure contains an invalid constraint, in checked builds the call is ignored and an error is issued.
	\note For all actors in the pruning structure each assigned PxConstraint object will get added to the scene automatically if
	it connects to another actor that is part of the scene already.

	\note This method is optimized for high performance.

	\note Merging a PxPruningStructure into an active scene query optimization AABB tree might unbalance the tree. A typical use case for 
	PxPruningStructure is a large world scenario where blocks of closely positioned actors get streamed in. The merge process finds the 
	best node in the active scene query optimization AABB tree and inserts the PxPruningStructure. Therefore using PxPruningStructure 
	for actors scattered throughout the world will result in an unbalanced tree.

	\param[in] pruningStructure Pruning structure for a set of actors.
	\return True if success

	@see PxPhysics::createPruningStructure, PxPruningStructure
	*/
	virtual	bool				addActors(const PxPruningStructure& pruningStructure) = 0;

	/**
	\brief Removes an actor from this scene.

	\note If the actor is not part of this scene (see #PxActor::getScene), the call is ignored and an error is issued.

	\note You can not remove individual articulation links (see #PxArticulationLink) from the scene. Use #removeArticulation() instead.

	\note If the actor is a PxRigidActor then all assigned PxConstraint objects will get removed from the scene automatically.

	\note If the actor is in an aggregate it will be removed from the aggregate.

	\param[in] actor Actor to remove from scene.
	\param[in] wakeOnLostTouch Specifies whether touching objects from the previous frame should get woken up in the next frame. Only applies to PxArticulationReducedCoordinate and PxRigidActor types.

	@see PxActor, PxAggregate
	*/
	virtual	void				removeActor(PxActor& actor, bool wakeOnLostTouch = true) = 0;

	/**
	\brief Removes actors from this scene. Only supports actors of type PxRigidStatic and PxRigidDynamic.

	\note This method only supports actors of type PxRigidStatic and PxRigidDynamic. For other actors, use removeActor() instead.
	For articulation links, use removeArticulation().

	\note If some actor is not part of this scene (see #PxActor::getScene), the actor remove is ignored and an error is issued.

	\note You can not remove individual articulation links (see #PxArticulationLink) from the scene. Use #removeArticulation() instead.

	\note If the actor is a PxRigidActor then all assigned PxConstraint objects will get removed from the scene automatically.

	\param[in] actors Array of actors to add to scene.
	\param[in] nbActors Number of actors in the array.
	\param[in] wakeOnLostTouch Specifies whether touching objects from the previous frame should get woken up in the next frame. Only applies to PxArticulationReducedCooridnate and PxRigidActor types.

	@see PxActor
	*/
	virtual	void				removeActors(PxActor*const* actors, PxU32 nbActors, bool wakeOnLostTouch = true) = 0;

	/**
	\brief Adds an aggregate to this scene.
	
	\note If the aggregate is already assigned to a scene (see #PxAggregate::getScene), the call is ignored and an error is issued.
	\note If the aggregate contains an actor with an invalid constraint, in checked builds the call is ignored and an error is issued.

	\note If the aggregate already contains actors, those actors are added to the scene as well.

	\param[in] aggregate Aggregate to add to scene.
	\return True if success
	
	@see PxAggregate, PxConstraint::isValid()
	*/
    virtual	bool				addAggregate(PxAggregate& aggregate)	= 0;

	/**
	\brief Removes an aggregate from this scene.

	\note If the aggregate is not part of this scene (see #PxAggregate::getScene), the call is ignored and an error is issued.

	\note If the aggregate contains actors, those actors are removed from the scene as well.

	\param[in] aggregate Aggregate to remove from scene.
	\param[in] wakeOnLostTouch Specifies whether touching objects from the previous frame should get woken up in the next frame. Only applies to PxArticulationReducedCoordinate and PxRigidActor types.

	@see PxAggregate
	*/
	virtual	void				removeAggregate(PxAggregate& aggregate, bool wakeOnLostTouch = true)	= 0;

	/**
	\brief Adds objects in the collection to this scene.

	This function adds the following types of objects to this scene: PxRigidActor (except PxArticulationLink), PxAggregate, PxArticulationReducedCoordinate. 
	This method is typically used after deserializing the collection in order to populate the scene with deserialized objects.

	\note If the collection contains an actor with an invalid constraint, in checked builds the call is ignored and an error is issued.

	\param[in] collection Objects to add to this scene. See #PxCollection
	\return True if success

	@see PxCollection, PxConstraint::isValid()
	*/
	virtual	bool				addCollection(const PxCollection& collection) = 0;
	//@}
	/************************************************************************************************/

	/** @name Contained Object Retrieval
	*/
	//@{

	/**
	\brief Retrieve the number of actors of certain types in the scene. For supported types, see PxActorTypeFlags.

	\param[in] types Combination of actor types.
	\return the number of actors.

	@see getActors()
	*/
	virtual	PxU32				getNbActors(PxActorTypeFlags types) const = 0;

	/**
	\brief Retrieve an array of all the actors of certain types in the scene. For supported types, see PxActorTypeFlags.

	\param[in] types Combination of actor types to retrieve.
	\param[out] userBuffer The buffer to receive actor pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first actor pointer to be retrieved
	\return Number of actors written to the buffer.

	@see getNbActors()
	*/
	virtual	PxU32				getActors(PxActorTypeFlags types, PxActor** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const	= 0;

	/**
	\brief Queries the PxScene for a list of the PxActors whose transforms have been 
	updated during the previous simulation step. Only includes actors of type PxRigidDynamic and PxArticulationLink.

	\note PxSceneFlag::eENABLE_ACTIVE_ACTORS must be set.

	\note Do not use this method while the simulation is running. Calls to this method while the simulation is running will be ignored and NULL will be returned.

	\param[out] nbActorsOut The number of actors returned.

	\return A pointer to the list of active PxActors generated during the last call to fetchResults().

	@see PxActor
	*/
	virtual PxActor**		getActiveActors(PxU32& nbActorsOut) = 0;

	/**
	\brief Retrieve the number of soft bodies in the scene.

	\return the number of soft bodies.

	@see getActors()
	*/
	virtual	PxU32				getNbSoftBodies() const = 0;

	/**
	\brief Retrieve an array of all the soft bodies in the scene.

	\param[out] userBuffer The buffer to receive actor pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first actor pointer to be retrieved
	\return Number of actors written to the buffer.

	@see getNbActors()
	*/
	virtual	PxU32				getSoftBodies(PxSoftBody** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Retrieve the number of particle systems of the requested type in the scene.

	\param[in] type The particle system type. See PxParticleSolverType. Only one type can be requested per function call.
	\return the number particle systems.

	See getParticleSystems(), PxParticleSolverType
	*/
	virtual PxU32				getNbParticleSystems(PxParticleSolverType::Enum type) const = 0;

	/**
	\brief Retrieve an array of all the particle systems of the requested type in the scene.

	\param[in] type The particle system type. See PxParticleSolverType. Only one type can be requested per function call.
	\param[out] userBuffer The buffer to receive particle system pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first particle system pointer to be retrieved
	\return Number of particle systems written to the buffer.

	See getNbParticleSystems(), PxParticleSolverType
	*/
	virtual	PxU32				getParticleSystems(PxParticleSolverType::Enum type, PxParticleSystem** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Retrieve the number of FEM cloths in the scene.
	\warning Feature under development, only for internal usage.

	\return the number of FEM cloths.

	See getFEMCloths()
	*/
	virtual PxU32				getNbFEMCloths() const = 0;

	/**
	\brief Retrieve an array of all the FEM cloths in the scene.
	\warning Feature under development, only for internal usage.

	\param[out] userBuffer The buffer to write the FEM cloth pointers to
	\param[in] bufferSize Size of the provided user buffer
	\param[in] startIndex Index of first FEM cloth pointer to be retrieved
	\return Number of FEM cloths written to the buffer
	*/
	virtual PxU32				getFEMCloths(PxFEMCloth** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Retrieve the number of hair systems in the scene.
	\warning Feature under development, only for internal usage.
	\return the number of hair systems
	@see getActors()
	*/
	virtual	PxU32				getNbHairSystems() const = 0;

	/**
	\brief Retrieve an array of all the hair systems in the scene.
	\warning Feature under development, only for internal usage.

	\param[out] userBuffer The buffer to write the actor pointers to
	\param[in] bufferSize Size of the provided user buffer
	\param[in] startIndex Index of first actor pointer to be retrieved
	\return Number of actors written to the buffer
	*/
	virtual	PxU32				getHairSystems(PxHairSystem** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Returns the number of articulations in the scene.

	\return the number of articulations in this scene.

	@see getArticulations()
	*/
	virtual PxU32				getNbArticulations() const = 0;

	/**
	\brief Retrieve all the articulations in the scene.

	\param[out] userBuffer The buffer to receive articulations pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first articulations pointer to be retrieved
	\return Number of articulations written to the buffer.

	@see getNbArticulations()
	*/
	virtual	PxU32				getArticulations(PxArticulationReducedCoordinate** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const = 0;

	/**
	\brief Returns the number of constraint shaders in the scene.

	\return the number of constraint shaders in this scene.

	@see getConstraints()
	*/
	virtual PxU32				getNbConstraints()	const	= 0;

	/**
	\brief Retrieve all the constraint shaders in the scene.

	\param[out] userBuffer The buffer to receive constraint shader pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first constraint pointer to be retrieved
	\return Number of constraint shaders written to the buffer.

	@see getNbConstraints()
	*/
	virtual	PxU32				getConstraints(PxConstraint** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const = 0;

	/**
	\brief Returns the number of aggregates in the scene.

	\return the number of aggregates in this scene.

	@see getAggregates()
	*/
	virtual			PxU32		getNbAggregates()	const	= 0;

	/**
	\brief Retrieve all the aggregates in the scene.

	\param[out] userBuffer The buffer to receive aggregates pointers.
	\param[in] bufferSize Size of provided user buffer.
	\param[in] startIndex Index of first aggregate pointer to be retrieved
	\return Number of aggregates written to the buffer.

	@see getNbAggregates()
	*/
	virtual			PxU32		getAggregates(PxAggregate** userBuffer, PxU32 bufferSize, PxU32 startIndex=0)	const	= 0;

	//@}
	/************************************************************************************************/

	/** @name Dominance
	*/
	//@{

	/**
	\brief Specifies the dominance behavior of contacts between two actors with two certain dominance groups.
	
	It is possible to assign each actor to a dominance groups using #PxActor::setDominanceGroup().

	With dominance groups one can have all contacts created between actors act in one direction only. This is useful, for example, if you
	want an object to push debris out of its way and be unaffected,while still responding physically to forces and collisions
	with non-debris objects.
	
	Whenever a contact between two actors (a0, a1) needs to be solved, the groups (g0, g1) of both
	actors are retrieved. Then the PxDominanceGroupPair setting for this group pair is retrieved with getDominanceGroupPair(g0, g1).
	
	In the contact, PxDominanceGroupPair::dominance0 becomes the dominance setting for a0, and 
	PxDominanceGroupPair::dominance1 becomes the dominance setting for a1. A dominanceN setting of 1.0f, the default, 
	will permit aN to be pushed or pulled by a(1-N) through the contact. A dominanceN setting of 0.0f, will however 
	prevent aN to be pushed by a(1-N) via the contact. Thus, a PxDominanceGroupPair of (1.0f, 0.0f) makes 
	the interaction one-way.
	
	
	The matrix sampled by getDominanceGroupPair(g1, g2) is initialised by default such that:
	
	if g1 == g2, then (1.0f, 1.0f) is returned
	if g1 <  g2, then (0.0f, 1.0f) is returned
	if g1 >  g2, then (1.0f, 0.0f) is returned
	
	In other words, we permit actors in higher groups to be pushed around by actors in lower groups by default.
		
	These settings should cover most applications, and in fact not overriding these settings may likely result in higher performance.
	
	It is not possible to make the matrix asymetric, or to change the diagonal. In other words: 
	
	* it is not possible to change (g1, g2) if (g1==g2)	
	* if you set 
	
	(g1, g2) to X, then (g2, g1) will implicitly and automatically be set to ~X, where:
	
	~(1.0f, 1.0f) is (1.0f, 1.0f)
	~(0.0f, 1.0f) is (1.0f, 0.0f)
	~(1.0f, 0.0f) is (0.0f, 1.0f)
	
	These two restrictions are to make sure that contacts between two actors will always evaluate to the same dominance
	setting, regardless of the order of the actors.
	
	Dominance settings are currently specified as floats 0.0f or 1.0f because in the future we may permit arbitrary 
	fractional settings to express 'partly-one-way' interactions.
		
	<b>Sleeping:</b> Does <b>NOT</b> wake actors up automatically.

	@see getDominanceGroupPair() PxDominanceGroup PxDominanceGroupPair PxActor::setDominanceGroup() PxActor::getDominanceGroup()
	*/
	virtual void				setDominanceGroupPair(
									PxDominanceGroup group1, PxDominanceGroup group2, const PxDominanceGroupPair& dominance) = 0;

	/**
	\brief Samples the dominance matrix.

	@see setDominanceGroupPair() PxDominanceGroup PxDominanceGroupPair PxActor::setDominanceGroup() PxActor::getDominanceGroup()
	*/
	virtual PxDominanceGroupPair getDominanceGroupPair(PxDominanceGroup group1, PxDominanceGroup group2) const = 0;

	//@}
	/************************************************************************************************/

	/** @name Dispatcher
	*/
	//@{

	/**
	\brief Return the cpu dispatcher that was set in PxSceneDesc::cpuDispatcher when creating the scene with PxPhysics::createScene

	@see PxSceneDesc::cpuDispatcher, PxPhysics::createScene
	*/
	virtual PxCpuDispatcher* getCpuDispatcher() const = 0;

	/**
	\brief Return the CUDA context manager that was set in PxSceneDesc::cudaContextManager when creating the scene with PxPhysics::createScene

	<b>Platform specific:</b> Applies to PC GPU only.

	@see PxSceneDesc::cudaContextManager, PxPhysics::createScene
	*/
	virtual PxCudaContextManager* getCudaContextManager() const = 0;

	//@}
	/************************************************************************************************/
	/** @name Multiclient
	*/
	//@{
	/**
	\brief Reserves a new client ID.
	
	PX_DEFAULT_CLIENT is always available as the default clientID.
	Additional clients are returned by this function. Clients cannot be released once created. 
	An error is reported when more than a supported number of clients (currently 128) are created. 

	@see PxClientID
	*/
	virtual PxClientID			createClient() = 0;

	//@}

	/************************************************************************************************/

	/** @name Callbacks
	*/
	//@{

	/**
	\brief Sets a user notify object which receives special simulation events when they occur.

	\note Do not set the callback while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[in] callback User notification callback. See #PxSimulationEventCallback.

	@see PxSimulationEventCallback getSimulationEventCallback
	*/
	virtual void				setSimulationEventCallback(PxSimulationEventCallback* callback) = 0;

	/**
	\brief Retrieves the simulationEventCallback pointer set with setSimulationEventCallback().

	\return The current user notify pointer. See #PxSimulationEventCallback.

	@see PxSimulationEventCallback setSimulationEventCallback()
	*/
	virtual PxSimulationEventCallback*	getSimulationEventCallback() const = 0;

	/**
	\brief Sets a user callback object, which receives callbacks on all contacts generated for specified actors.

	\note Do not set the callback while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[in] callback Asynchronous user contact modification callback. See #PxContactModifyCallback.
	*/
	virtual void				setContactModifyCallback(PxContactModifyCallback* callback) = 0;

	/**
	\brief Sets a user callback object, which receives callbacks on all CCD contacts generated for specified actors.

	\note Do not set the callback while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[in] callback Asynchronous user contact modification callback. See #PxCCDContactModifyCallback.
	*/
	virtual void				setCCDContactModifyCallback(PxCCDContactModifyCallback* callback) = 0;

	/**
	\brief Retrieves the PxContactModifyCallback pointer set with setContactModifyCallback().

	\return The current user contact modify callback pointer. See #PxContactModifyCallback.

	@see PxContactModifyCallback setContactModifyCallback()
	*/
	virtual PxContactModifyCallback*	getContactModifyCallback() const = 0;

	/**
	\brief Retrieves the PxCCDContactModifyCallback pointer set with setContactModifyCallback().

	\return The current user contact modify callback pointer. See #PxContactModifyCallback.

	@see PxContactModifyCallback setContactModifyCallback()
	*/
	virtual PxCCDContactModifyCallback*	getCCDContactModifyCallback() const = 0;

	/**
	\brief Sets a broad-phase user callback object.

	\note Do not set the callback while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[in] callback	Asynchronous broad-phase callback. See #PxBroadPhaseCallback.
	*/
	virtual void				setBroadPhaseCallback(PxBroadPhaseCallback* callback) = 0;

	/**
	\brief Retrieves the PxBroadPhaseCallback pointer set with setBroadPhaseCallback().

	\return The current broad-phase callback pointer. See #PxBroadPhaseCallback.

	@see PxBroadPhaseCallback setBroadPhaseCallback()
	*/
	virtual PxBroadPhaseCallback* getBroadPhaseCallback()	const = 0;

	//@}
	/************************************************************************************************/

	/** @name Collision Filtering
	*/
	//@{

	/**
	\brief Sets the shared global filter data which will get passed into the filter shader.

	\note It is the user's responsibility to ensure that changing the shared global filter data does not change the filter output value for existing pairs. 
	      If the filter output for existing pairs does change nonetheless then such a change will not take effect until the pair gets refiltered. 
		  resetFiltering() can be used to explicitly refilter the pairs of specific objects.

	\note The provided data will get copied to internal buffers and this copy will be used for filtering calls.

	\note Do not use this method while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[in] data The shared global filter shader data.
	\param[in] dataSize Size of the shared global filter shader data (in bytes).

	@see getFilterShaderData() PxSceneDesc.filterShaderData PxSimulationFilterShader
	*/
	virtual void				setFilterShaderData(const void* data, PxU32 dataSize) = 0;

	/**
	\brief Gets the shared global filter data in use for this scene.

	\note The reference points to a copy of the original filter data specified in #PxSceneDesc.filterShaderData or provided by #setFilterShaderData().

	\return Shared filter data for filter shader.

	@see getFilterShaderDataSize() setFilterShaderData() PxSceneDesc.filterShaderData PxSimulationFilterShader
	*/
	virtual	const void*			getFilterShaderData() const = 0;

	/**
	\brief Gets the size of the shared global filter data (#PxSceneDesc.filterShaderData)

	\return Size of shared filter data [bytes].

	@see getFilterShaderData() PxSceneDesc.filterShaderDataSize PxSimulationFilterShader
	*/
	virtual	PxU32				getFilterShaderDataSize() const = 0;

	/**
	\brief Gets the custom collision filter shader in use for this scene.

	\return Filter shader class that defines the collision pair filtering.

	@see PxSceneDesc.filterShader PxSimulationFilterShader
	*/
	virtual	PxSimulationFilterShader	getFilterShader() const = 0;

	/**
	\brief Gets the custom collision filter callback in use for this scene.

	\return Filter callback class that defines the collision pair filtering.

	@see PxSceneDesc.filterCallback PxSimulationFilterCallback
	*/
	virtual	PxSimulationFilterCallback*	getFilterCallback() const = 0;

	/**
	\brief Marks the object to reset interactions and re-run collision filters in the next simulation step.
	
	This call forces the object to remove all existing collision interactions, to search anew for existing contact
	pairs and to run the collision filters again for found collision pairs.

	\note The operation is supported for PxRigidActor objects only.

	\note All persistent state of existing interactions will be lost and can not be retrieved even if the same collison pair
	is found again in the next step. This will mean, for example, that you will not get notified about persistent contact
	for such an interaction (see #PxPairFlag::eNOTIFY_TOUCH_PERSISTS), the contact pair will be interpreted as newly found instead.

	\note Lost touch contact reports will be sent for every collision pair which includes this shape, if they have
	been requested through #PxPairFlag::eNOTIFY_TOUCH_LOST or #PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST.

	\note This is an expensive operation, don't use it if you don't have to.

	\note Can be used to retrieve collision pairs that were killed by the collision filters (see #PxFilterFlag::eKILL)

	\note It is invalid to use this method if the actor has not been added to a scene already.

	\note It is invalid to use this method if PxActorFlag::eDISABLE_SIMULATION is set.

	\note Do not use this method while the simulation is running.

	<b>Sleeping:</b> Does wake up the actor.

	\param[in] actor The actor for which to re-evaluate interactions.
	\return True if success

	@see PxSimulationFilterShader PxSimulationFilterCallback
	*/
	virtual bool				resetFiltering(PxActor& actor) = 0;

	/**
	\brief Marks the object to reset interactions and re-run collision filters for specified shapes in the next simulation step.
	
	This is a specialization of the resetFiltering(PxActor& actor) method and allows to reset interactions for specific shapes of
	a PxRigidActor.

	\note Do not use this method while the simulation is running.

	<b>Sleeping:</b> Does wake up the actor.

	\param[in] actor The actor for which to re-evaluate interactions.
	\param[in] shapes The shapes for which to re-evaluate interactions.
	\param[in] shapeCount Number of shapes in the list.

	@see PxSimulationFilterShader PxSimulationFilterCallback
	*/
	virtual bool				resetFiltering(PxRigidActor& actor, PxShape*const* shapes, PxU32 shapeCount) = 0;

	/**
	\brief Gets the pair filtering mode for kinematic-kinematic pairs.

	\return Filtering mode for kinematic-kinematic pairs.

	@see PxPairFilteringMode PxSceneDesc
	*/
	virtual	PxPairFilteringMode::Enum	getKinematicKinematicFilteringMode()	const	= 0;

	/**
	\brief Gets the pair filtering mode for static-kinematic pairs.

	\return Filtering mode for static-kinematic pairs.

	@see PxPairFilteringMode PxSceneDesc
	*/
	virtual	PxPairFilteringMode::Enum	getStaticKinematicFilteringMode()		const	= 0;

	//@}
	/************************************************************************************************/

	/** @name Simulation
	*/
	//@{
	/**
 	\brief Advances the simulation by an elapsedTime time.
	
	\note Large elapsedTime values can lead to instabilities. In such cases elapsedTime
	should be subdivided into smaller time intervals and simulate() should be called
	multiple times for each interval.

	Calls to simulate() should pair with calls to fetchResults():
 	Each fetchResults() invocation corresponds to exactly one simulate()
 	invocation; calling simulate() twice without an intervening fetchResults()
 	or fetchResults() twice without an intervening simulate() causes an error
 	condition.
 
 	scene->simulate();
 	...do some processing until physics is computed...
 	scene->fetchResults();
 	...now results of run may be retrieved.


	\param[in] elapsedTime Amount of time to advance simulation by. The parameter has to be larger than 0, else the resulting behavior will be undefined. <b>Range:</b> (0, PX_MAX_F32)
	\param[in] completionTask if non-NULL, this task will have its refcount incremented in simulate(), then
	decremented when the scene is ready to have fetchResults called. So the task will not run until the
	application also calls removeReference().
	\param[in] scratchMemBlock a memory region for physx to use for temporary data during simulation. This block may be reused by the application
	after fetchResults returns. Must be aligned on a 16-byte boundary
	\param[in] scratchMemBlockSize the size of the scratch memory block. Must be a multiple of 16K.
	\param[in] controlSimulation if true, the scene controls its PxTaskManager simulation state. Leave
    true unless the application is calling the PxTaskManager start/stopSimulation() methods itself.
	\return True if success

	@see fetchResults() checkResults()
	*/
	virtual	bool				simulate(PxReal elapsedTime, physx::PxBaseTask* completionTask = NULL,
									void* scratchMemBlock = 0, PxU32 scratchMemBlockSize = 0, bool controlSimulation = true) = 0;

	/**
 	\brief Performs dynamics phase of the simulation pipeline.
	
	\note Calls to advance() should follow calls to fetchCollision(). An error message will be issued if this sequence is not followed.

	\param[in] completionTask if non-NULL, this task will have its refcount incremented in advance(), then
	decremented when the scene is ready to have fetchResults called. So the task will not run until the
	application also calls removeReference().
	\return True if success
	*/
	virtual	bool				advance(physx::PxBaseTask* completionTask = 0) = 0;

	/**
	\brief Performs collision detection for the scene over elapsedTime
	
	\note Calls to collide() should be the first method called to simulate a frame.


	\param[in] elapsedTime Amount of time to advance simulation by. The parameter has to be larger than 0, else the resulting behavior will be undefined. <b>Range:</b> (0, PX_MAX_F32)
	\param[in] completionTask if non-NULL, this task will have its refcount incremented in collide(), then
	decremented when the scene is ready to have fetchResults called. So the task will not run until the
	application also calls removeReference().
	\param[in] scratchMemBlock a memory region for physx to use for temporary data during simulation. This block may be reused by the application
	after fetchResults returns. Must be aligned on a 16-byte boundary
	\param[in] scratchMemBlockSize the size of the scratch memory block. Must be a multiple of 16K.
	\param[in] controlSimulation if true, the scene controls its PxTaskManager simulation state. Leave
    true unless the application is calling the PxTaskManager start/stopSimulation() methods itself.
	\return True if success
	*/
	virtual	bool				collide(PxReal elapsedTime, physx::PxBaseTask* completionTask = 0, void* scratchMemBlock = 0,
									PxU32 scratchMemBlockSize = 0, bool controlSimulation = true) = 0;  
	
	/**
	\brief This checks to see if the simulation run has completed.

	This does not cause the data available for reading to be updated with the results of the simulation, it is simply a status check.
	The bool will allow it to either return immediately or block waiting for the condition to be met so that it can return true
	
	\param[in] block When set to true will block until the condition is met.
	\return True if the results are available.

	@see simulate() fetchResults()
	*/
	virtual	bool				checkResults(bool block = false) = 0;

	/**
	This method must be called after collide() and before advance(). It will wait for the collision phase to finish. If the user makes an illegal simulation call, the SDK will issue an error
	message.

	\param[in] block When set to true will block until the condition is met, which is collision must finish running.
	*/
	virtual	bool				fetchCollision(bool block = false)	= 0;			

	/**
	This is the big brother to checkResults() it basically does the following:
	
	\code
	if ( checkResults(block) )
	{
		fire appropriate callbacks
		swap buffers
		return true
	}
	else
		return false

	\endcode

	\param[in] block When set to true will block until results are available.
	\param[out] errorState Used to retrieve hardware error codes. A non zero value indicates an error.
	\return True if the results have been fetched.

	@see simulate() checkResults()
	*/
	virtual	bool				fetchResults(bool block = false, PxU32* errorState = 0)	= 0;

	/**
	This call performs the first section of fetchResults, and returns a pointer to the contact streams output by the simulation. It can be used to process contact pairs in parallel, which is often a limiting factor
	for fetchResults() performance. 

	After calling this function and processing the contact streams, call fetchResultsFinish(). Note that writes to the simulation are not
	permitted between the start of fetchResultsStart() and the end of fetchResultsFinish().

	\param[in] block When set to true will block until results are available.
	\param[out] contactPairs an array of pointers to contact pair headers
	\param[out] nbContactPairs the number of contact pairs
	\return True if the results have been fetched.

	@see simulate() checkResults() fetchResults() fetchResultsFinish()
	*/
	virtual	bool				fetchResultsStart(const PxContactPairHeader*& contactPairs, PxU32& nbContactPairs, bool block = false) = 0;

	/**
	This call processes all event callbacks in parallel. It takes a continuation task, which will be executed once all callbacks have been processed.

	This is a utility function to make it easier to process callbacks in parallel using the PhysX task system. It can only be used in conjunction with 
	fetchResultsStart(...) and fetchResultsFinish(...)

	\param[in] continuation The task that will be executed once all callbacks have been processed.
	*/
	virtual void				processCallbacks(physx::PxBaseTask* continuation) = 0;

	/**
	This call performs the second section of fetchResults.

	It must be called after fetchResultsStart() returns and contact reports have been processed.

	Note that once fetchResultsFinish() has been called, the contact streams returned in fetchResultsStart() will be invalid.

	\param[out] errorState Used to retrieve hardware error codes. A non zero value indicates an error.

	@see simulate() checkResults() fetchResults() fetchResultsStart()
	*/
	virtual	void				fetchResultsFinish(PxU32* errorState = 0) = 0;

	/**
	This call performs the synchronization of particle system data copies.
	 */
	virtual void				fetchResultsParticleSystem() = 0;

	/**
	\brief Clear internal buffers and free memory.

	This method can be used to clear buffers and free internal memory without having to destroy the scene. Can be useful if
	the physics data gets streamed in and a checkpoint with a clean state should be created.

	\note It is not allowed to call this method while the simulation is running. The call will fail.
	
	\param[in] sendPendingReports When set to true pending reports will be sent out before the buffers get cleaned up (for instance lost touch contact/trigger reports due to deleted objects).
	*/
	virtual	void				flushSimulation(bool sendPendingReports = false) = 0;
	
	/**
	\brief Sets a constant gravity for the entire scene.

	\note Do not use this method while the simulation is running.

	<b>Sleeping:</b> Does <b>NOT</b> wake the actor up automatically.

	\param[in] vec A new gravity vector(e.g. PxVec3(0.0f,-9.8f,0.0f) ) <b>Range:</b> force vector

	@see PxSceneDesc.gravity getGravity()
	*/
	virtual void				setGravity(const PxVec3& vec) = 0;

	/**
	\brief Retrieves the current gravity setting.

	\return The current gravity for the scene.

	@see setGravity() PxSceneDesc.gravity
	*/
	virtual PxVec3				getGravity() const = 0;

	/**
	\brief Set the bounce threshold velocity.  Collision speeds below this threshold will not cause a bounce.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::bounceThresholdVelocity, getBounceThresholdVelocity
	*/
	virtual void				setBounceThresholdVelocity(const PxReal t) = 0;

	/**
	\brief Return the bounce threshold velocity.

	@see PxSceneDesc.bounceThresholdVelocity, setBounceThresholdVelocity
	*/
	virtual PxReal				getBounceThresholdVelocity() const = 0;

	/**
	\brief Sets the maximum number of CCD passes

	\note Do not use this method while the simulation is running.

	\param[in] ccdMaxPasses Maximum number of CCD passes

	@see PxSceneDesc.ccdMaxPasses getCCDMaxPasses()
	*/
	virtual void				setCCDMaxPasses(PxU32 ccdMaxPasses) = 0;

	/**
	\brief Gets the maximum number of CCD passes.

	\return The maximum number of CCD passes.

	@see PxSceneDesc::ccdMaxPasses setCCDMaxPasses()
	*/
	virtual PxU32				getCCDMaxPasses() const = 0;	

	/**
	\brief Set the maximum CCD separation.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::ccdMaxSeparation, getCCDMaxSeparation
	*/
	virtual void				setCCDMaxSeparation(const PxReal t) = 0;

	/**
	\brief Gets the maximum CCD separation.

	\return The maximum CCD separation.

	@see PxSceneDesc::ccdMaxSeparation setCCDMaxSeparation()
	*/
	virtual PxReal				getCCDMaxSeparation() const = 0;

	/**
	\brief Set the CCD threshold.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::ccdThreshold, getCCDThreshold
	*/
	virtual void				setCCDThreshold(const PxReal t) = 0;

	/**
	\brief Gets the CCD threshold.

	\return The CCD threshold.

	@see PxSceneDesc::ccdThreshold setCCDThreshold()
	*/
	virtual PxReal				getCCDThreshold() const = 0;

	/**
	\brief Set the max bias coefficient.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::maxBiasCoefficient, getMaxBiasCoefficient
	*/
	virtual void				setMaxBiasCoefficient(const PxReal t) = 0;

	/**
	\brief Gets the max bias coefficient.

	\return The max bias coefficient.

	@see PxSceneDesc::maxBiasCoefficient setMaxBiasCoefficient()
	*/
	virtual PxReal				getMaxBiasCoefficient() const = 0;

	/**
	\brief Set the friction offset threshold.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::frictionOffsetThreshold, getFrictionOffsetThreshold
	*/
	virtual void				setFrictionOffsetThreshold(const PxReal t) = 0;

	/**
	\brief Gets the friction offset threshold.

	@see PxSceneDesc::frictionOffsetThreshold,  setFrictionOffsetThreshold
	*/
	virtual PxReal				getFrictionOffsetThreshold() const = 0;

	/**
	\brief Set the friction correlation distance.

	\note Do not use this method while the simulation is running.

	@see PxSceneDesc::frictionCorrelationDistance, getFrictionCorrelationDistance
	*/
	virtual void				setFrictionCorrelationDistance(const PxReal t) = 0;

	/**
	\brief Gets the friction correlation distance.

	@see PxSceneDesc::frictionCorrelationDistance,  setFrictionCorrelationDistance
	*/
	virtual PxReal				getFrictionCorrelationDistance() const = 0;

	/**
	\brief Return the friction model.
	@see PxFrictionType, PxSceneDesc::frictionType
	*/
	virtual PxFrictionType::Enum	getFrictionType() const = 0;

	/**
	\brief Return the solver model.
	@see PxSolverType, PxSceneDesc::solverType
	*/
	virtual PxSolverType::Enum	getSolverType()	const = 0;

	//@}
	/************************************************************************************************/

	/** @name Visualization and Statistics
	*/
	//@{
	/**
	\brief Function that lets you set debug visualization parameters.

	Returns false if the value passed is out of range for usage specified by the enum.

	\note Do not use this method while the simulation is running.

	\param[in] param	Parameter to set. See #PxVisualizationParameter
	\param[in] value	The value to set, see #PxVisualizationParameter for allowable values. Setting to zero disables visualization for the specified property, setting to a positive value usually enables visualization and defines the scale factor.
	\return False if the parameter is out of range.

	@see getVisualizationParameter PxVisualizationParameter getRenderBuffer()
	*/
	virtual bool				setVisualizationParameter(PxVisualizationParameter::Enum param, PxReal value) = 0;

	/**
	\brief Function that lets you query debug visualization parameters.

	\param[in] paramEnum The Parameter to retrieve.
	\return The value of the parameter.

	@see setVisualizationParameter PxVisualizationParameter
	*/
	virtual PxReal				getVisualizationParameter(PxVisualizationParameter::Enum paramEnum) const = 0;

	/**
	\brief Defines a box in world space to which visualization geometry will be (conservatively) culled. Use a non-empty culling box to enable the feature, and an empty culling box to disable it.

	\note Do not use this method while the simulation is running.
	
	\param[in] box the box to which the geometry will be culled. Empty box to disable the feature.
	@see setVisualizationParameter getVisualizationCullingBox getRenderBuffer()
	*/
	virtual void				setVisualizationCullingBox(const PxBounds3& box) = 0;

	/**
	\brief Retrieves the visualization culling box.

	\return the box to which the geometry will be culled.
	@see setVisualizationParameter setVisualizationCullingBox 
	*/
	virtual PxBounds3			getVisualizationCullingBox() const = 0;
	
	/**
	\brief Retrieves the render buffer.
	
	This will contain the results of any active visualization for this scene.

	\note Do not use this method while the simulation is running. Calls to this method while the simulation is running will result in undefined behaviour.

	\return The render buffer.

	@see PxRenderBuffer
	*/
	virtual const PxRenderBuffer& getRenderBuffer() = 0;
	
	/**
	\brief Call this method to retrieve statistics for the current simulation step.

	\note Do not use this method while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\param[out] stats Used to retrieve statistics for the current simulation step.

	@see PxSimulationStatistics
	*/
	virtual	void				getSimulationStatistics(PxSimulationStatistics& stats) const = 0;
	
	//@}
	
	/************************************************************************************************/
	/** @name Broad-phase
	*/
	//@{

	/**
	\brief Returns broad-phase type.

	\return Broad-phase type
	*/
	virtual	PxBroadPhaseType::Enum	getBroadPhaseType()								const = 0;

	/**
	\brief Gets broad-phase caps.

	\param[out]	caps	Broad-phase caps
	\return True if success
	*/
	virtual	bool					getBroadPhaseCaps(PxBroadPhaseCaps& caps)			const = 0;

	/**
	\brief Returns number of regions currently registered in the broad-phase.

	\return Number of regions
	*/
	virtual	PxU32					getNbBroadPhaseRegions()							const = 0;

	/**
	\brief Gets broad-phase regions.

	\param[out]	userBuffer	Returned broad-phase regions
	\param[in]	bufferSize	Size of userBuffer
	\param[in]	startIndex	Index of first desired region, in [0 ; getNbRegions()[
	\return Number of written out regions
	*/
	virtual	PxU32					getBroadPhaseRegions(PxBroadPhaseRegionInfo* userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const	= 0;

	/**
	\brief Adds a new broad-phase region.

	The bounds for the new region must be non-empty, otherwise an error occurs and the call is ignored.

	Note that by default, objects already existing in the SDK that might touch this region will not be automatically
	added to the region. In other words the newly created region will be empty, and will only be populated with new
	objects when they are added to the simulation, or with already existing objects when they are updated.

	It is nonetheless possible to override this default behavior and let the SDK populate the new region automatically
	with already existing objects overlapping the incoming region. This has a cost though, and it should only be used
	when the game can not guarantee that all objects within the new region will be added to the simulation after the
	region itself.

	Objects automatically move from one region to another during their lifetime. The system keeps tracks of what
	regions a given object is in. It is legal for an object to be in an arbitrary number of regions. However if an
	object leaves all regions, or is created outside of all regions, several things happen:
		- collisions get disabled for this object
		- if a PxBroadPhaseCallback object is provided, an "out-of-bounds" event is generated via that callback
		- if a PxBroadPhaseCallback object is not provided, a warning/error message is sent to the error stream

	If an object goes out-of-bounds and user deletes it during the same frame, neither the out-of-bounds event nor the
	error message is generated.

	\param[in]	region			User-provided region data
	\param[in]	populateRegion	Automatically populate new region with already existing objects overlapping it
	\return Handle for newly created region, or 0xffffffff in case of failure.
	\see	PxBroadPhaseRegion PxBroadPhaseCallback
	*/
	virtual	PxU32					addBroadPhaseRegion(const PxBroadPhaseRegion& region, bool populateRegion=false)		= 0;

	/**
	\brief Removes a new broad-phase region.

	If the region still contains objects, and if those objects do not overlap any region any more, they are not
	automatically removed from the simulation. Instead, the PxBroadPhaseCallback::onObjectOutOfBounds notification
	is used for each object. Users are responsible for removing the objects from the simulation if this is the
	desired behavior.

	If the handle is invalid, or if a valid handle is removed twice, an error message is sent to the error stream.

	\param[in]	handle	Region's handle, as returned by PxScene::addBroadPhaseRegion.
	\return True if success
	*/
	virtual	bool					removeBroadPhaseRegion(PxU32 handle)				= 0;

	//@}

	/************************************************************************************************/

	/** @name Threads and Memory
	*/
	//@{

	/**
	\brief Get the task manager associated with this scene

	\return the task manager associated with the scene
	*/
	virtual PxTaskManager*			getTaskManager() const = 0;

	/**
	\brief Lock the scene for reading from the calling thread.

	When the PxSceneFlag::eREQUIRE_RW_LOCK flag is enabled lockRead() must be 
	called before any read calls are made on the scene.

	Multiple threads may read at the same time, no threads may read while a thread is writing.
	If a call to lockRead() is made while another thread is holding a write lock 
	then the calling thread will be blocked until the writing thread calls unlockWrite().

	\note Lock upgrading is *not* supported, that means it is an error to
	call lockRead() followed by lockWrite().

	\note Recursive locking is supported but each lockRead() call must be paired with an unlockRead().

	\param file String representing the calling file, for debug purposes
	\param line The source file line number, for debug purposes
	*/
	virtual void lockRead(const char* file=NULL, PxU32 line=0) = 0;

	/** 
	\brief Unlock the scene from reading.

	\note Each unlockRead() must be paired with a lockRead() from the same thread.
	*/
	virtual void unlockRead() = 0;

	/**
	\brief Lock the scene for writing from this thread.

	When the PxSceneFlag::eREQUIRE_RW_LOCK flag is enabled lockWrite() must be 
	called before any write calls are made on the scene.

	Only one thread may write at a time and no threads may read while a thread is writing.
	If a call to lockWrite() is made and there are other threads reading then the 
	calling thread will be blocked until the readers complete.

	Writers have priority. If a thread is blocked waiting to write then subsequent calls to 
	lockRead() from other threads will be blocked until the writer completes.

	\note If multiple threads are waiting to write then the thread that is first
	granted access depends on OS scheduling.

	\note Recursive locking is supported but each lockWrite() call must be paired 
	with an unlockWrite().	

	\note If a thread has already locked the scene for writing then it may call
	lockRead().

	\param file String representing the calling file, for debug purposes
	\param line The source file line number, for debug purposes
	*/
	virtual void lockWrite(const char* file=NULL, PxU32 line=0) = 0;

	/**
	\brief Unlock the scene from writing.

	\note Each unlockWrite() must be paired with a lockWrite() from the same thread.
	*/
	virtual void unlockWrite() = 0;
	
	/**
	\brief set the cache blocks that can be used during simulate(). 
	
	Each frame the simulation requires memory to store contact, friction, and contact cache data. This memory is used in blocks of 16K.
	Each frame the blocks used by the previous frame are freed, and may be retrieved by the application using PxScene::flushSimulation()

	This call will force allocation of cache blocks if the numBlocks parameter is greater than the currently allocated number
	of blocks, and less than the max16KContactDataBlocks parameter specified at scene creation time.

	\note Do not use this method while the simulation is running.

	\param[in] numBlocks The number of blocks to allocate.	

	@see PxSceneDesc.nbContactDataBlocks PxSceneDesc.maxNbContactDataBlocks flushSimulation() getNbContactDataBlocksUsed getMaxNbContactDataBlocksUsed
	*/
	virtual         void				setNbContactDataBlocks(PxU32 numBlocks) = 0;

	/**
	\brief get the number of cache blocks currently used by the scene 

	This function may not be called while the scene is simulating

	\return the number of cache blocks currently used by the scene

	@see PxSceneDesc.nbContactDataBlocks PxSceneDesc.maxNbContactDataBlocks flushSimulation() setNbContactDataBlocks() getMaxNbContactDataBlocksUsed()
	*/
	virtual         PxU32				getNbContactDataBlocksUsed() const = 0;

	/**
	\brief get the maximum number of cache blocks used by the scene 

	This function may not be called while the scene is simulating

	\return the maximum number of cache blocks everused by the scene

	@see PxSceneDesc.nbContactDataBlocks PxSceneDesc.maxNbContactDataBlocks flushSimulation() setNbContactDataBlocks() getNbContactDataBlocksUsed()
	*/
	virtual         PxU32				getMaxNbContactDataBlocksUsed() const = 0;

	/**
	\brief Return the value of PxSceneDesc::contactReportStreamBufferSize that was set when creating the scene with PxPhysics::createScene

	@see PxSceneDesc::contactReportStreamBufferSize, PxPhysics::createScene
	*/
	virtual			PxU32				getContactReportStreamBufferSize() const = 0;

	/**
	\brief Sets the number of actors required to spawn a separate rigid body solver thread.

	\note Do not use this method while the simulation is running.

	\param[in] solverBatchSize Number of actors required to spawn a separate rigid body solver thread.

	@see PxSceneDesc.solverBatchSize getSolverBatchSize()
	*/
	virtual	void						setSolverBatchSize(PxU32 solverBatchSize) = 0;

	/**
	\brief Retrieves the number of actors required to spawn a separate rigid body solver thread.

	\return Current number of actors required to spawn a separate rigid body solver thread.

	@see PxSceneDesc.solverBatchSize setSolverBatchSize()
	*/
	virtual PxU32						getSolverBatchSize() const = 0;

	/**
	\brief Sets the number of articulations required to spawn a separate rigid body solver thread.

	\note Do not use this method while the simulation is running.

	\param[in] solverBatchSize Number of articulations required to spawn a separate rigid body solver thread.

	@see PxSceneDesc.solverBatchSize getSolverArticulationBatchSize()
	*/
	virtual	void						setSolverArticulationBatchSize(PxU32 solverBatchSize) = 0;

	/**
	\brief Retrieves the number of articulations required to spawn a separate rigid body solver thread.

	\return Current number of articulations required to spawn a separate rigid body solver thread.

	@see PxSceneDesc.solverBatchSize setSolverArticulationBatchSize()
	*/
	virtual PxU32						getSolverArticulationBatchSize() const = 0;
	
	//@}

	/**
	\brief Returns the wake counter reset value.

	\return Wake counter reset value

	@see PxSceneDesc.wakeCounterResetValue
	*/
	virtual	PxReal						getWakeCounterResetValue() const = 0;

	/**
	\brief Shift the scene origin by the specified vector.

	The poses of all objects in the scene and the corresponding data structures will get adjusted to reflect the new origin location
	(the shift vector will get subtracted from all object positions).

	\note It is the user's responsibility to keep track of the summed total origin shift and adjust all input/output to/from PhysX accordingly.

	\note Do not use this method while the simulation is running. Calls to this method while the simulation is running will be ignored.

	\note Make sure to propagate the origin shift to other dependent modules (for example, the character controller module etc.).

	\note This is an expensive operation and we recommend to use it only in the case where distance related precision issues may arise in areas far from the origin.

	\param[in] shift Translation vector to shift the origin by.
	*/
	virtual	void					shiftOrigin(const PxVec3& shift) = 0;

	/**
	\brief Returns the Pvd client associated with the scene.
	\return the client, NULL if no PVD supported.
	*/
	virtual PxPvdSceneClient*		getScenePvdClient() = 0;

	/**
	\brief Copy GPU articulation data from the internal GPU buffer to a user-provided device buffer.
	\param[in] data User-provided gpu data buffer which should be sized appropriately for the particular data that is requested. Further details provided in the user guide.
	\param[in] index User-provided gpu index buffer. This buffer stores the articulation indices which the user wants to copy.
	\param[in] dataType Enum specifying the type of data the user wants to read back from the articulations.
	\param[in] nbCopyArticulations Number of articulations that data should be copied from.
	\param[in] copyEvent User-provided event for the articulation stream to signal when the data copy to the user buffer has completed.
	*/
	virtual		void				copyArticulationData(void* data, void* index, PxArticulationGpuDataType::Enum dataType, const PxU32 nbCopyArticulations, void* copyEvent = NULL) = 0;
	
	/**
	\brief Apply GPU articulation data from a user-provided device buffer to the internal GPU buffer.
	\param[in] data User-provided gpu data buffer which should be sized appropriately for the particular data that is requested. Further details provided in the user guide.
	\param[in] index User-provided gpu index buffer. This buffer stores the articulation indices which the user wants to write to.
	\param[in] dataType Enum specifying the type of data the user wants to write to the articulations.
	\param[in] nbUpdatedArticulations Number of articulations that data should be written to.
	\param[in] waitEvent User-provided event for the articulation stream to wait for data.
	\param[in] signalEvent User-provided event for the articulation stream to signal when the data read from the user buffer has completed.
	*/
	virtual		void				applyArticulationData(void* data, void* index, PxArticulationGpuDataType::Enum dataType, const PxU32 nbUpdatedArticulations, void* waitEvent = NULL, void* signalEvent = NULL) = 0;

	/**
	\brief Copy GPU softbody data from the internal GPU buffer to a user-provided device buffer.
	\param[in] data User-provided gpu buffer containing a pointer to another gpu buffer for every softbody to process
	\param[in] dataSizes The size of every buffer in bytes
	\param[in] softBodyIndices User provided gpu index buffer. This buffer stores the softbody index which the user want to copy.
	\param[in] maxSize The largest size stored in dataSizes. Used internally to decide how many threads to launch for the copy process.
	\param[in] flag Flag defining which data the user wants to read back from the softbody system
	\param[in] nbCopySoftBodies The number of softbodies to be copied.
	\param[in] copyEvent User-provided event for the user to sync data
	*/
	virtual		void				copySoftBodyData(void** data, void* dataSizes, void* softBodyIndices, PxSoftBodyDataFlag::Enum flag, const PxU32 nbCopySoftBodies, const PxU32 maxSize, void* copyEvent = NULL) = 0;


	/**
	\brief Apply user-provided data to the internal softbody system.
	\param[in] data User-provided gpu buffer containing a pointer to another gpu buffer for every softbody to process
	\param[in] dataSizes The size of every buffer in bytes	
	\param[in] softBodyIndices User provided gpu index buffer. This buffer stores the updated softbody index.
	\param[in] flag Flag defining which data the user wants to write to the softbody system
	\param[in] maxSize The largest size stored in dataSizes. Used internally to decide how many threads to launch for the copy process. 
	\param[in] nbUpdatedSoftBodies The number of updated softbodies
	\param[in] applyEvent User-provided event for the softbody stream to wait for data
	*/
	virtual		void				applySoftBodyData(void** data, void* dataSizes, void* softBodyIndices, PxSoftBodyDataFlag::Enum flag, const PxU32 nbUpdatedSoftBodies, const PxU32 maxSize, void* applyEvent = NULL) = 0;

	/**
	\brief Copy contact data from the internal GPU buffer to a user-provided device buffer.

	\note The contact data contains pointers to internal state and is only valid until the next call to simulate().

	\param[in] data User-provided gpu data buffer, which should be the size of PxGpuContactPair * numContactPairs
	\param[in] maxContactPairs  The maximum number of pairs that the buffer can contain
	\param[in] numContactPairs The actual number of contact pairs that were written
	\param[in] copyEvent User-provided event for the user to sync data
	*/
	virtual		void				copyContactData(void* data, const PxU32 maxContactPairs, void* numContactPairs, void* copyEvent = NULL) = 0;

	/**
	\brief Copy GPU rigid body data from the internal GPU buffer to a user-provided device buffer.
	\param[in] data User-provided gpu data buffer which should nbCopyActors * sizeof(PxGpuBodyData). The only data it can copy is PxGpuBodyData.
	\param[in] index User provided node PxGpuActorPair buffer. This buffer stores pairs of indices: the PxNodeIndex corresponding to the rigid body and an index corresponding to the location in the user buffer that this value should be placed.
	\param[in] nbCopyActors The number of rigid body to be copied.
	\param[in] copyEvent User-provided event for the user to sync data.
	*/
	virtual		void				copyBodyData(PxGpuBodyData* data, PxGpuActorPair* index, const PxU32 nbCopyActors, void* copyEvent = NULL) = 0;

	/**
	\brief Apply user-provided data to rigid body.
	\param[in] data User-provided gpu data buffer which should be sized appropriately for the particular data that is requested. Further details provided in the user guide.
	\param[in] index User provided PxGpuActorPair buffer. This buffer stores pairs of indices: the PxNodeIndex corresponding to the rigid body and an index corresponding to the location in the user buffer that this value should be placed.
	\param[in] flag Flag defining which data the user wants to write to the rigid body
	\param[in] nbUpdatedActors The number of updated rigid body
	\param[in] waitEvent User-provided event for the rigid body stream to wait for data
	\param[in] signalEvent User-provided event for the rigid body stream to signal when the read from the user buffer has completed
	*/
	virtual		void				applyActorData(void* data, PxGpuActorPair* index, PxActorCacheFlag::Enum flag, const PxU32 nbUpdatedActors, void* waitEvent = NULL, void* signalEvent = NULL) = 0;

	/**
	\brief Compute dense Jacobian matrices for specified articulations on the GPU.

	The size of Jacobians can vary by articulation, since it depends on the number of links, degrees-of-freedom, and whether the base is fixed.

	The size is determined using these formulas:
	nCols = (fixedBase ? 0 : 6) + dofCount
	nRows = (fixedBase ? 0 : 6) + (linkCount - 1) * 6;

	The user must ensure that adequate space is provided for each Jacobian matrix.

	\param[in] indices User-provided gpu buffer of (index, data) pairs. The entries map a GPU articulation index to a GPU block of memory where the returned Jacobian will be stored.
	\param[in] nbIndices The number of (index, data) pairs provided.
	\param[in] computeEvent User-provided event for the user to sync data.
	*/
	virtual		void				computeDenseJacobians(const PxIndexDataPair* indices, PxU32 nbIndices, void* computeEvent) = 0;

	/**
	\brief Compute the joint-space inertia matrices that maps joint accelerations to joint forces: forces = M * accelerations on the GPU.

	The size of matrices can vary by articulation, since it depends on the number of links and degrees-of-freedom.

	The size is determined using this formula:
	sizeof(float) * dofCount * dofCount

	The user must ensure that adequate space is provided for each mass matrix.

	\param[in] indices User-provided gpu buffer of (index, data) pairs. The entries map a GPU articulation index to a GPU block of memory where the returned matrix will be stored.
	\param[in] nbIndices The number of (index, data) pairs provided.
	\param[in] computeEvent User-provided event for the user to sync data.
	*/
	virtual		void				computeGeneralizedMassMatrices(const PxIndexDataPair* indices, PxU32 nbIndices, void* computeEvent) = 0;

	/**
	\brief Computes the joint DOF forces required to counteract gravitational forces for the given articulation pose.

	The size of the result can vary by articulation, since it depends on the number of links and degrees-of-freedom.

	The size is determined using this formula:
	sizeof(float) * dofCount

	The user must ensure that adequate space is provided for each articulation.

	\param[in] indices User-provided gpu buffer of (index, data) pairs. The entries map a GPU articulation index to a GPU block of memory where the returned matrix will be stored.
	\param[in] nbIndices The number of (index, data) pairs provided.
	\param[in] computeEvent User-provided event for the user to sync data.
	*/
	virtual		void				computeGeneralizedGravityForces(const PxIndexDataPair* indices, PxU32 nbIndices, void* computeEvent) = 0;

	/**
	\brief Computes the joint DOF forces required to counteract coriolis and centrifugal forces for the given articulation pose.

	The size of the result can vary by articulation, since it depends on the number of links and degrees-of-freedom.

	The size is determined using this formula:
	sizeof(float) * dofCount

	The user must ensure that adequate space is provided for each articulation.

	\param[in] indices User-provided gpu buffer of (index, data) pairs. The entries map a GPU articulation index to a GPU block of memory where the returned matrix will be stored.
	\param[in] nbIndices The number of (index, data) pairs provided.
	\param[in] computeEvent User-provided event for the user to sync data.
	*/
	virtual		void				computeCoriolisAndCentrifugalForces(const PxIndexDataPair* indices, PxU32 nbIndices, void* computeEvent) = 0;

	virtual		PxgDynamicsMemoryConfig getGpuDynamicsConfig() const = 0;

	/**
	\brief Apply user-provided data to particle buffers.

	This function should be used if the particle buffer flags are already on the device. Otherwise, use PxParticleBuffer::raiseFlags()
	from the CPU.

	This assumes the data has been changed directly in the PxParticleBuffer.

	\param[in] indices User-provided index buffer that indexes into the BufferIndexPair and flags list.
	\param[in] bufferIndexPair User-provided index pair buffer specifying the unique id and GPU particle system for each PxParticleBuffer. See PxGpuParticleBufferIndexPair.
	\param[in] flags Flags to mark what data needs to be updated. See PxParticleBufferFlags. 
	\param[in] nbUpdatedBuffers The number of particle buffers to update.
	\param[in] waitEvent User-provided event for the particle stream to wait for data.
	\param[in] signalEvent User-provided event for the particle stream to signal when the data read from the user buffer has completed.
	*/
	virtual		void				applyParticleBufferData(const PxU32* indices, const PxGpuParticleBufferIndexPair* bufferIndexPair, const PxParticleBufferFlags* flags, PxU32 nbUpdatedBuffers, void* waitEvent = NULL, void* signalEvent = NULL) = 0;

	void*	userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
