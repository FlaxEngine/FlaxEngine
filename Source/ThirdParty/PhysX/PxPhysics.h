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

#ifndef PX_PHYSICS_H
#define PX_PHYSICS_H

/** \addtogroup physics
@{
*/

#include "PxPhysXConfig.h"
#include "PxDeletionListener.h"
#include "foundation/PxTransform.h"
#include "PxShape.h"
#include "PxAggregate.h"
#include "PxBuffer.h"
#include "PxParticleSystem.h"
#include "foundation/PxPreprocessor.h"
#if PX_ENABLE_FEATURES_UNDER_CONSTRUCTION
#include "PxFEMCloth.h"
#include "PxHairSystem.h"
#endif

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxScene;
class PxSceneDesc;
class PxTolerancesScale;
class PxPvd;
class PxOmniPvd;
class PxInsertionCallback;

class PxRigidActor;
class PxConstraintConnector;
struct PxConstraintShaderTable;

class PxGeometry;
class PxFoundation;

class PxPruningStructure;
class PxBVH;
/**
 * @deprecated
 */
typedef PX_DEPRECATED PxBVH PxBVHStructure;

class PxParticleClothBuffer;
class PxParticleRigidBuffer;

class PxSoftBodyMesh;

/**
\brief Abstract singleton factory class used for instancing objects in the Physics SDK.

In addition you can use PxPhysics to set global parameters which will effect all scenes and create
objects that can be shared across multiple scenes.

You can get an instance of this class by calling PxCreateBasePhysics() or PxCreatePhysics() with pre-registered modules.

@see PxCreatePhysics() PxCreateBasePhysics() PxScene
*/
class PxPhysics
{
public:

	/** @name Basics
	*/
	//@{

	virtual ~PxPhysics() {}

	/**
	\brief Destroys the instance it is called on.

	Use this release method to destroy an instance of this class. Be sure
	to not keep a reference to this object after calling release.
	Avoid release calls while a scene is simulating (in between simulate() and fetchResults() calls).

	Note that this must be called once for each prior call to PxCreatePhysics, as
	there is a reference counter. Also note that you mustn't destroy the PxFoundation instance (holding the allocator, error callback etc.)
	until after the reference count reaches 0 and the SDK is actually removed.

	Releasing an SDK will also release any objects created through it (scenes, triangle meshes, convex meshes, heightfields, shapes etc.),
	provided the user hasn't already done so.

	\note Releasing the PxPhysics instance is a prerequisite to releasing the PxFoundation instance.

	@see PxCreatePhysics() PxFoundation
	*/
	virtual	void release() = 0;

	/**
	\brief Retrieves the Foundation instance.
	\return A reference to the Foundation object.
	*/
	virtual PxFoundation& getFoundation() = 0;

	/**
	\brief Retrieves the PxOmniPvd instance if there is one registered with PxPhysics.
	\return A pointer to a PxOmniPvd object.
	*/
	virtual PxOmniPvd* getOmniPvd() = 0;

	/**
	\brief Creates an aggregate with the specified maximum size and filtering hint.

	The previous API used "bool enableSelfCollision" which should now silently evaluates
	to a PxAggregateType::eGENERIC aggregate with its self-collision bit.

	Use PxAggregateType::eSTATIC or PxAggregateType::eKINEMATIC for aggregates that will
	only contain static or kinematic actors. This provides faster filtering when used in
	combination with PxPairFilteringMode.

	\param	[in] maxActor		The maximum number of actors that may be placed in the aggregate.
	\param	[in] maxShape		The maximum number of shapes that may be placed in the aggregate.
	\param	[in] filterHint		The aggregate's filtering hint.
	\return The new aggregate.

	@see PxAggregate PxAggregateFilterHint PxAggregateType PxPairFilteringMode
	*/
	virtual	PxAggregate* createAggregate(PxU32 maxActor, PxU32 maxShape, PxAggregateFilterHint filterHint) = 0;

	/**
	\brief Creates an aggregate with the specified maximum size and filtering hint.

	The previous API used "bool enableSelfCollision" which should now silently evaluates
	to a PxAggregateType::eGENERIC aggregate with its self-collision bit.

	Use PxAggregateType::eSTATIC or PxAggregateType::eKINEMATIC for aggregates that will
	only contain static or kinematic actors. This provides faster filtering when used in
	combination with PxPairFilteringMode.

	\note This variation of the method is not compatible with GPU rigid bodies.

	\param	[in] maxActor		The maximum number of actors that may be placed in the aggregate.
	\param	[in] filterHint		The aggregate's filtering hint.
	\return The new aggregate.

	@see PxAggregate PxAggregateFilterHint PxAggregateType PxPairFilteringMode
	@deprecated
	*/
	PX_FORCE_INLINE PX_DEPRECATED PxAggregate* createAggregate(PxU32 maxActor, PxAggregateFilterHint filterHint)
	{
		return createAggregate(maxActor, PX_MAX_U32, filterHint);
	}

	/**
	\brief Returns the simulation tolerance parameters.
	\return The current simulation tolerance parameters.
	*/
	virtual const PxTolerancesScale& getTolerancesScale() const = 0;

	//@}
	/** @name Meshes
	*/
	//@{

	/**
	\brief Creates a triangle mesh object.

	This can then be instanced into #PxShape objects.

	\param	[in] stream	The triangle mesh stream.
	\return The new triangle mesh.

	@see PxTriangleMesh PxMeshPreprocessingFlag PxTriangleMesh.release() PxInputStream PxTriangleMeshFlag
	*/
	virtual PxTriangleMesh* createTriangleMesh(PxInputStream& stream) = 0;

	/**
	\brief Return the number of triangle meshes that currently exist.

	\return Number of triangle meshes.

	@see getTriangleMeshes()
	*/
	virtual PxU32 getNbTriangleMeshes() const = 0;

	/**
	\brief Writes the array of triangle mesh pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the triangle meshes in the array is not specified.

	\param	[out] userBuffer	The buffer to receive triangle mesh pointers.
	\param	[in]  bufferSize	The number of triangle mesh pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first mesh pointer to be retrieved.
	\return The number of triangle mesh pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbTriangleMeshes() PxTriangleMesh
	*/
	virtual	PxU32 getTriangleMeshes(PxTriangleMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;


	//@}
	/** @name Tetrahedron Meshes
	*/
	//@{

	/**
	\brief Creates a tetrahedron mesh object.

	This can then be instanced into #PxShape objects.

	\param[in] stream The tetrahedron mesh stream.
	\return The new tetrahedron mesh.

	@see PxTetrahedronMesh PxMeshPreprocessingFlag PxTetrahedronMesh.release() PxInputStream PxTriangleMeshFlag
	*/
	virtual PxTetrahedronMesh* createTetrahedronMesh(PxInputStream& stream) = 0;

	/**
	\brief Creates a softbody mesh object.

	\param[in] stream The softbody mesh stream.
	\return The new softbody mesh.

	@see createTetrahedronMesh
	*/
	virtual PxSoftBodyMesh* createSoftBodyMesh(PxInputStream& stream) = 0;

	/**
	\brief Return the number of tetrahedron meshes that currently exist.

	\return Number of tetrahedron meshes.

	@see getTetrahedronMeshes()
	*/
	virtual PxU32 getNbTetrahedronMeshes() const = 0;

	/**
	\brief Writes the array of tetrahedron mesh pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the tetrahedron meshes in the array is not specified.

	\param[out] userBuffer The buffer to receive tetrahedron mesh pointers.
	\param[in] bufferSize The number of tetrahedron mesh pointers which can be stored in the buffer.
	\param[in] startIndex Index of first mesh pointer to be retrieved.
	\return The number of tetrahedron mesh pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbTetrahedronMeshes() PxTetrahedronMesh
	*/
	virtual	PxU32 getTetrahedronMeshes(PxTetrahedronMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a heightfield object from previously cooked stream.

	This can then be instanced into #PxShape objects.

	\param	[in] stream	The heightfield mesh stream.
	\return The new heightfield.

	@see PxHeightField PxHeightField.release() PxInputStream PxRegisterHeightFields
	*/
	virtual PxHeightField* createHeightField(PxInputStream& stream) = 0;

	/**
	\brief Return the number of heightfields that currently exist.

	\return Number of heightfields.

	@see getHeightFields()
	*/
	virtual PxU32 getNbHeightFields() const = 0;

	/**
	\brief Writes the array of heightfield pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the heightfields in the array is not specified.

	\param	[out] userBuffer	The buffer to receive heightfield pointers.
	\param	[in]  bufferSize	The number of heightfield pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first heightfield pointer to be retrieved.
	\return The number of heightfield pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbHeightFields() PxHeightField
	*/
	virtual	PxU32 getHeightFields(PxHeightField** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a convex mesh object.

	This can then be instanced into #PxShape objects.

	\param	[in] stream	The stream to load the convex mesh from.
	\return The new convex mesh.

	@see PxConvexMesh PxConvexMesh.release() PxInputStream createTriangleMesh() PxConvexMeshGeometry PxShape
	*/
	virtual PxConvexMesh* createConvexMesh(PxInputStream& stream) = 0;

	/**
	\brief Return the number of convex meshes that currently exist.

	\return Number of convex meshes.

	@see getConvexMeshes()
	*/
	virtual PxU32 getNbConvexMeshes() const = 0;

	/**
	\brief Writes the array of convex mesh pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the convex meshes in the array is not specified.

	\param	[out] userBuffer	The buffer to receive convex mesh pointers.
	\param	[in]  bufferSize	The number of convex mesh pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first convex mesh pointer to be retrieved.
	\return The number of convex mesh pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbConvexMeshes() PxConvexMesh
	*/
	virtual	PxU32 getConvexMeshes(PxConvexMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a bounding volume hierarchy.

	\param	[in] stream	The stream to load the BVH from.
	\return The new BVH.

	@see PxBVH PxInputStream
	*/
	virtual PxBVH* createBVH(PxInputStream& stream) = 0;

	/**
	 * @deprecated
	 */
	PX_DEPRECATED PX_FORCE_INLINE PxBVH*	createBVHStructure(PxInputStream& stream)
											{
												return createBVH(stream);
											}

	/**
	\brief Return the number of bounding volume hierarchies that currently exist.

	\return Number of bounding volume hierarchies.

	@see PxBVH getBVHs()
	*/
	virtual PxU32 getNbBVHs() const = 0;

	/**
	 * @deprecated
	 */
	PX_DEPRECATED PX_FORCE_INLINE PxU32		getNbBVHStructures() const
											{
												return getNbBVHs();
											}

	/**
	\brief Writes the array of bounding volume hierarchy pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the BVHs in the array is not specified.

	\param	[out] userBuffer	The buffer to receive BVH pointers.
	\param	[in]  bufferSize	The number of BVH pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first BVH pointer to be retrieved.
	\return The number of BVH pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbBVHs() PxBVH
	*/
	virtual	PxU32 getBVHs(PxBVH** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	 * @deprecated
	 */
	PX_DEPRECATED PX_FORCE_INLINE PxU32	getBVHStructures(PxBVHStructure** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const
										{
											return getBVHs(userBuffer, bufferSize, startIndex);
										}

	//@}
	/** @name Scenes
	*/
	//@{

	/**
	\brief Creates a scene.

	\note Every scene uses a Thread Local Storage slot. This imposes a platform specific limit on the
	number of scenes that can be created.

	\param	[in] sceneDesc	Scene descriptor. See #PxSceneDesc
	\return The new scene object.

	@see PxScene PxScene.release() PxSceneDesc
	*/
	virtual PxScene* createScene(const PxSceneDesc& sceneDesc) = 0;

	/**
	\brief Gets number of created scenes.

	\return The number of scenes created.

	@see getScenes()
	*/
	virtual PxU32 getNbScenes() const = 0;

	/**
	\brief Writes the array of scene pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the scene pointers in the array is not specified.

	\param	[out] userBuffer	The buffer to receive scene pointers.
	\param	[in]  bufferSize	The number of scene pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first scene pointer to be retrieved.
	\return The number of scene pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbScenes() PxScene
	*/
	virtual	PxU32 getScenes(PxScene** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	//@}
	/** @name Actors
	*/
	//@{

	/**
	\brief Creates a static rigid actor with the specified pose and all other fields initialized
	to their default values.

	\param	[in] pose	The initial pose of the actor. Must be a valid transform.

	@see PxRigidStatic
	*/
	virtual PxRigidStatic* createRigidStatic(const PxTransform& pose) = 0;

	/**
	\brief Creates a dynamic rigid actor with the specified pose and all other fields initialized
	to their default values.

	\param	[in] pose	The initial pose of the actor. Must be a valid transform.

	@see PxRigidDynamic
	*/
	virtual PxRigidDynamic* createRigidDynamic(const PxTransform& pose) = 0;

	/**
	\brief Creates a pruning structure from actors.

	\note Every provided actor needs at least one shape with the eSCENE_QUERY_SHAPE flag set.
	\note Both static and dynamic actors can be provided.
	\note It is not allowed to pass in actors which are already part of a scene.
	\note Articulation links cannot be provided.

	\param	[in] actors		Array of actors to add to the pruning structure. Must be non NULL.
	\param	[in] nbActors	Number of actors in the array. Must be >0.
	\return Pruning structure created from given actors, or NULL if any of the actors did not comply with the above requirements.
	@see PxActor PxPruningStructure
	*/
	virtual PxPruningStructure* createPruningStructure(PxRigidActor*const* actors, PxU32 nbActors) = 0;

	//@}
	/** @name Shapes
	*/
	//@{

	/**
	\brief Creates a shape which may be attached to multiple actors

	The shape will be created with a reference count of 1.

	\param	[in] geometry		The geometry for the shape
	\param	[in] material		The material for the shape
	\param	[in] isExclusive	Whether this shape is exclusive to a single actor or maybe be shared
	\param	[in] shapeFlags		The PxShapeFlags to be set
	\return The shape

	\note Shared shapes are not mutable when they are attached to an actor

	@see PxShape
	*/
	PX_FORCE_INLINE	PxShape* createShape(	const PxGeometry& geometry,
											const PxMaterial& material,
											bool isExclusive = false,
											PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE)
	{
		PxMaterial* materialPtr = const_cast<PxMaterial*>(&material);
		return createShape(geometry, &materialPtr, 1, isExclusive, shapeFlags);
	}

	/**
	\brief Creates a shape which may be attached to one or more softbody actors

	The shape will be created with a reference count of 1.

	\param	[in] geometry		The geometry for the shape
	\param	[in] material		The material for the shape
	\param	[in] isExclusive	Whether this shape is exclusive to a single actor or maybe be shared
	\param	[in] shapeFlags		The PxShapeFlags to be set
	\return The shape

	\note Shared shapes are not mutable when they are attached to an actor

	@see PxShape
	*/
	PX_FORCE_INLINE	PxShape* createShape(	const PxGeometry& geometry,
											const PxFEMSoftBodyMaterial& material,
											bool isExclusive = false,
											PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE)
	{
		PxFEMSoftBodyMaterial* materialPtr = const_cast<PxFEMSoftBodyMaterial*>(&material);
		return createShape(geometry, &materialPtr, 1, isExclusive, shapeFlags);
	}

#if PX_ENABLE_FEATURES_UNDER_CONSTRUCTION
	/**
	\brief Creates a shape which may be attached to one or more FEMCloth actors

	The shape will be created with a reference count of 1.

	\param	[in] geometry		The geometry for the shape
	\param	[in] material		The material for the shape
	\param	[in] isExclusive	Whether this shape is exclusive to a single actor or maybe be shared
	\param	[in] shapeFlags		The PxShapeFlags to be set
	\return The shape

	\note Shared shapes are not mutable when they are attached to an actor

	@see PxShape
	*/
	PX_FORCE_INLINE	PxShape* createShape(	const PxGeometry& geometry,
											const PxFEMClothMaterial& material,
											bool isExclusive = false,
											PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE)
	{
		PxFEMClothMaterial* materialPtr = const_cast<PxFEMClothMaterial*>(&material);
		return createShape(geometry, &materialPtr, 1, isExclusive, shapeFlags);
	}
#endif

	/**
	\brief Creates a shape which may be attached to multiple actors

	The shape will be created with a reference count of 1.

	\param	[in] geometry		The geometry for the shape
	\param	[in] materials		The materials for the shape
	\param	[in] materialCount	The number of materials
	\param	[in] isExclusive	Whether this shape is exclusive to a single actor or may be shared
	\param	[in] shapeFlags		The PxShapeFlags to be set
	\return The shape

	\note Shared shapes are not mutable when they are attached to an actor
	\note Shapes created from *SDF* triangle-mesh geometries do not support more than one material.

	@see PxShape
	*/
	virtual PxShape* createShape(	const PxGeometry& geometry,
									PxMaterial*const * materials,
									PxU16 materialCount,
									bool isExclusive = false,
									PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE) = 0;

	virtual PxShape* createShape(	const PxGeometry& geometry,
									PxFEMSoftBodyMaterial*const * materials,
									PxU16 materialCount,
									bool isExclusive = false,
									PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE) = 0;

	virtual PxShape* createShape(	const PxGeometry& geometry,
									PxFEMClothMaterial*const * materials,
									PxU16 materialCount,
									bool isExclusive = false,
									PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE) = 0;

	/**
	\brief Return the number of shapes that currently exist.

	\return Number of shapes.

	@see getShapes()
	*/
	virtual PxU32 getNbShapes() const = 0;

	/**
	\brief Writes the array of shape pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the shapes in the array is not specified.

	\param	[out] userBuffer	The buffer to receive shape pointers.
	\param	[in]  bufferSize	The number of shape pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first shape pointer to be retrieved
	\return The number of shape pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbShapes() PxShape
	*/
	virtual	PxU32 getShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	//@}
	/** @name Constraints and Articulations
	*/
	//@{

	/**
	\brief Creates a constraint shader.

	\note A constraint shader will get added automatically to the scene the two linked actors belong to. Either, but not both, of actor0 and actor1 may
	be NULL to denote attachment to the world.

	\param	[in] actor0		The first actor
	\param	[in] actor1		The second actor
	\param	[in] connector	The connector object, which the SDK uses to communicate with the infrastructure for the constraint
	\param	[in] shaders	The shader functions for the constraint
	\param	[in] dataSize	The size of the data block for the shader

	\return The new constraint shader.

	@see PxConstraint
	*/
	virtual PxConstraint* createConstraint(PxRigidActor* actor0, PxRigidActor* actor1, PxConstraintConnector& connector, const PxConstraintShaderTable& shaders, PxU32 dataSize) = 0;

	/**
	\brief Creates a reduced-coordinate articulation with all fields initialized to their default values.

	\return the new articulation

	@see PxArticulationReducedCoordinate, PxRegisterArticulationsReducedCoordinate
	*/
	virtual PxArticulationReducedCoordinate* createArticulationReducedCoordinate() = 0;


	/**
	\brief Creates a FEM-based cloth with all fields initialized to their default values.
	\warning Feature under development, only for internal usage.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\return the new FEM-cloth

	@see PxFEMCloth
	*/
	virtual PxFEMCloth* createFEMCloth(PxCudaContextManager& cudaContextManager) = 0;

	/**
	\brief Creates a FEM-based soft body with all fields initialized to their default values.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\return the new soft body

	@see PxSoftBody
	*/
	virtual PxSoftBody* createSoftBody(PxCudaContextManager& cudaContextManager) = 0;

	/**
	\brief Creates a hair system with all fields initialized to their default values.
	\warning Feature under development, only for internal usage.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\return the new hair system

	@see PxHairSystem
	*/
	virtual PxHairSystem* createHairSystem(PxCudaContextManager& cudaContextManager) = 0;

	/**
	\brief Creates a particle system with a position-based dynamics (PBD) solver.

	A PBD particle system can be used to simulate particle systems with fluid and granular particles. It also allows simulating cloth using
	mass-spring constraints and rigid bodies by shape matching the bodies with particles.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\param[in] maxNeighborhood The maximum number of particles considered in neighborhood-based particle interaction calculations (e.g. fluid density constraints).
	\return the new particle system

	@see PxPBDParticleSystem
	*/
	virtual PxPBDParticleSystem* createPBDParticleSystem(PxCudaContextManager& cudaContextManager, PxU32 maxNeighborhood = 96) = 0;

	/**
	\brief Creates a particle system with a fluid-implicit particle solver (FLIP).
	\warning Feature under development, only for internal usage.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\return the new particle system

	@see PxFLIPParticleSystem
	*/
	virtual PxFLIPParticleSystem* createFLIPParticleSystem(PxCudaContextManager& cudaContextManager) = 0;

	/**
	\brief Creates a particle system with a material-point-method solver (MPM).
	\warning Feature under development, only for internal usage.

	A MPM particle system can be used to simulate fluid dynamics and deformable body effects using particles.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\return the new particle system

	@see PxMPMParticleSystem
	*/
	virtual PxMPMParticleSystem* createMPMParticleSystem(PxCudaContextManager& cudaContextManager) = 0;

	/**
	\brief Creates a customizable particle system to simulate effects that are not supported by PhysX natively (e.g. molecular dynamics).
	\warning Feature under development, only for internal usage.

	\param[in] cudaContextManager The PxCudaContextManager this instance is tied to.
	\param[in] maxNeighborhood The maximum number of particles considered in neighborhood-based particle interaction calculations (e.g. fluid density constraints).
	\return the new particle system

	@see PxCustomParticleSystem
	*/
	virtual PxCustomParticleSystem* createCustomParticleSystem(PxCudaContextManager& cudaContextManager, PxU32 maxNeighborhood) = 0;

	/**
	\brief Create a buffer for reading and writing data across host and device memory spaces.

	\param[in] byteSize The size of the buffer in bytes.
	\param[in] bufferType The memory space of the buffer.
	\param[in] cudaContextManager The PxCudaContextManager this buffer is tied to.
	\return PxBuffer instance.

	@see PxBuffer
	*/
	virtual PxBuffer* createBuffer(PxU64 byteSize, PxBufferType::Enum bufferType, PxCudaContextManager* cudaContextManager) = 0;

	/**
	\brief Create particle buffer to simulate fluid/granular material.

	\param[in] maxParticles The maximum number of particles in this buffer.
	\param[in] maxVolumes The maximum number of volumes in this buffer. See PxParticleVolume.
	\param[in] cudaContextManager The PxCudaContextManager this buffer is tied to.
	\return PxParticleBuffer instance

	@see PxParticleBuffer
	*/
	virtual PxParticleBuffer* createParticleBuffer(PxU32 maxParticles, PxU32 maxVolumes, PxCudaContextManager* cudaContextManager) = 0;

	/**
	\brief Create a particle buffer for fluid dynamics with diffuse particles. Diffuse particles are used to simulate fluid effects
	such as foam, spray and bubbles.

	\param[in] maxParticles The maximum number of particles in this buffer.
	\param[in] maxVolumes The maximum number of volumes in this buffer. See #PxParticleVolume.
	\param[in] maxDiffuseParticles The max number of diffuse particles int this buffer.
	\param[in] cudaContextManager The PxCudaContextManager this buffer is tied to.
	\return PxParticleAndDiffuseBuffer instance

	@see PxParticleAndDiffuseBuffer, PxDiffuseParticleParams
	*/
	virtual PxParticleAndDiffuseBuffer* createParticleAndDiffuseBuffer(PxU32 maxParticles, PxU32 maxVolumes, PxU32 maxDiffuseParticles, PxCudaContextManager* cudaContextManager) = 0;

	/**
	\brief Create a particle buffer to simulate particle cloth.

	\param[in] maxParticles The maximum number of particles in this buffer.
	\param[in] maxNumVolumes The maximum number of volumes in this buffer. See #PxParticleVolume.
	\param[in] maxNumCloths The maximum number of cloths in this buffer. See #PxParticleCloth.
	\param[in] maxNumTriangles The maximum number of triangles for aerodynamics.
	\param[in] maxNumSprings The maximum number of springs to connect particles. See #PxParticleSpring.
	\param[in] cudaContextManager The PxCudaContextManager this buffer is tied to.
	\return PxParticleClothBuffer instance

	@see PxParticleClothBuffer
	*/
	virtual PxParticleClothBuffer* createParticleClothBuffer(PxU32 maxParticles, PxU32 maxNumVolumes, PxU32 maxNumCloths, PxU32 maxNumTriangles, PxU32 maxNumSprings, PxCudaContextManager* cudaContextManager) = 0;
	
	/**
	\brief Create a particle buffer to simulate rigid bodies using shape matching with particles.

	\param[in] maxParticles The maximum number of particles in this buffer.
	\param[in] maxNumVolumes The maximum number of volumes in this buffer. See #PxParticleVolume.
	\param[in] maxNumRigids The maximum number of rigid bodies this buffer is used to simulate.
	\param[in] cudaContextManager The PxCudaContextManager this buffer is tied to.
	\return PxParticleRigidBuffer instance

	@see PxParticleRigidBuffer
	*/
	virtual PxParticleRigidBuffer* createParticleRigidBuffer(PxU32 maxParticles, PxU32 maxNumVolumes, PxU32 maxNumRigids, PxCudaContextManager* cudaContextManager) = 0;

	//@}
	/** @name Materials
	*/
	//@{

	/**
	\brief Creates a new rigid body material with certain default properties.

	\return The new rigid body material.

	\param	[in] staticFriction		The coefficient of static friction
	\param	[in] dynamicFriction	The coefficient of dynamic friction
	\param	[in] restitution		The coefficient of restitution

	@see PxMaterial
	*/
	virtual PxMaterial* createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution) = 0;

	/**
	\brief Return the number of rigid body materials that currently exist.

	\return Number of rigid body materials.

	@see getMaterials()
	*/
	virtual PxU32 getNbMaterials() const = 0;

	/**
	\brief Writes the array of rigid body material pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbMaterials() PxMaterial
	*/
	virtual PxU32 getMaterials(PxMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a new FEM soft body material with certain default properties.

	\return The new FEM material.

	\param	[in] youngs					The young's modulus
	\param	[in] poissons				The poissons's ratio
	\param	[in] dynamicFriction		The dynamic friction coefficient

	@see PxFEMSoftBodyMaterial
	*/
	virtual PxFEMSoftBodyMaterial* createFEMSoftBodyMaterial(PxReal youngs, PxReal poissons, PxReal dynamicFriction) = 0;

	/**
	\brief Return the number of FEM soft body materials that currently exist.

	\return Number of FEM materials.

	@see getFEMSoftBodyMaterials()
	*/
	virtual PxU32 getNbFEMSoftBodyMaterials() const = 0;

	/**
	\brief Writes the array of FEM soft body material pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbFEMSoftBodyMaterials() PxFEMSoftBodyMaterial
	*/
	virtual PxU32 getFEMSoftBodyMaterials(PxFEMSoftBodyMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a new FEM cloth material with certain default properties.
	\warning Feature under development, only for internal usage.

	\return The new FEM material.

	\param	[in] youngs					The young's modulus
	\param	[in] poissons				The poissons's ratio
	\param	[in] dynamicFriction		The dynamic friction coefficient

	@see PxFEMClothMaterial
	*/
	virtual PxFEMClothMaterial* createFEMClothMaterial(PxReal youngs, PxReal poissons, PxReal dynamicFriction) = 0;

	/**
	\brief Return the number of FEM cloth materials that currently exist.

	\return Number of FEM cloth materials.

	@see getFEMClothMaterials()
	*/
	virtual PxU32 getNbFEMClothMaterials() const = 0;

	/**
	\brief Writes the array of FEM cloth material pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbFEMClothMaterials() PxFEMClothMaterial
	*/
	virtual PxU32 getFEMClothMaterials(PxFEMClothMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a new PBD material with certain default properties.

	\param	[in]  friction				The friction parameter
	\param	[in]  damping				The velocity damping parameter
	\param	[in]  adhesion				The adhesion parameter
	\param	[in]  viscosity				The viscosity parameter
	\param	[in]  vorticityConfinement	The vorticity confinement coefficient
	\param	[in]  surfaceTension		The surface tension coefficient
	\param	[in]  cohesion				The cohesion parameter
	\param	[in]  lift					The lift parameter
	\param	[in]  drag					The drag parameter
	\param	[in]  cflCoefficient		The Courant-Friedrichs-Lewy(cfl) coefficient
	\param	[in]  gravityScale			The gravity scale
	\return The new PBD material.

	@see PxPBDMaterial
	*/
	virtual PxPBDMaterial* createPBDMaterial(PxReal friction, PxReal damping, PxReal adhesion, PxReal viscosity, PxReal vorticityConfinement, PxReal surfaceTension, PxReal cohesion, PxReal lift, PxReal drag, PxReal cflCoefficient = 1.f, PxReal gravityScale = 1.f) = 0;

	/**
	\brief Return the number of PBD materials that currently exist.

	\return Number of PBD materials.

	@see getPBDMaterials()
	*/
	virtual PxU32 getNbPBDMaterials() const = 0;

	/**
	\brief Writes the array of PBD material pointers to a user buffer.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbPBDMaterials() PxPBDMaterial
	*/
	virtual PxU32 getPBDMaterials(PxPBDMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;
	
	/**
	\brief Creates a new FLIP material with certain default properties.
	\warning Feature under development, only for internal usage.

	\param	[in]  friction				The friction parameter
	\param	[in]  damping				The velocity damping parameter
	\param	[in]  adhesion				The maximum velocity magnitude of particles
	\param	[in]  viscosity				The viscosity parameter
	\param	[in]  gravityScale			The gravity scale
	\return The new FLIP material.

	@see PxFLIPMaterial
	*/
	virtual PxFLIPMaterial* createFLIPMaterial(PxReal friction, PxReal damping, PxReal adhesion, PxReal viscosity, PxReal gravityScale = 1.f) = 0;

	/**
	\brief Return the number of FLIP materials that currently exist.
	\warning Feature under development, only for internal usage.

	\return Number of FLIP materials.

	@see getFLIPMaterials()
	*/
	virtual PxU32 getNbFLIPMaterials() const = 0;

	/**
	\brief Writes the array of FLIP material pointers to a user buffer.
	\warning Feature under development, only for internal usage.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbFLIPMaterials() PxFLIPMaterial
	*/
	virtual PxU32 getFLIPMaterials(PxFLIPMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;
	
	/**
	\brief Creates a new MPM material with certain default properties.
	\warning Feature under development, only for internal usage.
	
	\param	[in]  friction						The friction parameter
	\param	[in]  damping						The velocity damping parameter
	\param	[in]  adhesion						The maximum velocity magnitude of particles
	\param	[in]  isPlastic						True if plastic
	\param	[in]  youngsModulus					The Young's modulus
	\param	[in]  poissons						The Poissons's ratio
	\param	[in]  hardening						The hardening parameter
	\param	[in]  criticalCompression			The critical compression parameter
	\param	[in]  criticalStretch				The critical stretch parameter
	\param	[in]  tensileDamageSensitivity		The tensile damage sensitivity parameter
	\param	[in]  compressiveDamageSensitivity	The compressive damage sensitivity parameter
	\param	[in]  attractiveForceResidual		The attractive force residual parameter
	\param	[in]  gravityScale					The gravity scale
	\return The new MPM material.

	@see PxMPMMaterial
	*/
	virtual PxMPMMaterial* createMPMMaterial(PxReal friction, PxReal damping, PxReal adhesion, bool isPlastic, PxReal youngsModulus, PxReal poissons, PxReal hardening, PxReal criticalCompression, PxReal criticalStretch, PxReal tensileDamageSensitivity, PxReal compressiveDamageSensitivity, PxReal attractiveForceResidual, PxReal gravityScale = 1.0f) = 0;

	/**
	\brief Return the number of MPM materials that currently exist.
	\warning Feature under development, only for internal usage.

	\return Number of MPM materials.

	@see getMPMMaterials()
	*/
	virtual PxU32 getNbMPMMaterials() const = 0;

	/**
	\brief Writes the array of MPM material pointers to a user buffer.
	\warning Feature under development, only for internal usage.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbMPMMaterials() PxMPMMaterial
	*/
	virtual PxU32 getMPMMaterials(PxMPMMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;

	/**
	\brief Creates a new material for custom particle systems.
	\warning Feature under development, only for internal usage.

	\param[in] gpuBuffer A pointer to a GPU buffer containing material parameters.
	\return the new material.
	*/
	virtual PxCustomMaterial* createCustomMaterial(void* gpuBuffer) = 0;

	/**
	\brief Return the number of custom materials that currently exist.
	\warning Feature under development, only for internal usage.

	\return Number of custom materials.

	@see getCustomMaterials()
	*/
	virtual PxU32 getNbCustomMaterials() const = 0;

	/**
	\brief Writes the array of custom material pointers to a user buffer.
	\warning Feature under development, only for internal usage.

	Returns the number of pointers written.

	The ordering of the materials in the array is not specified.

	\param	[out] userBuffer	The buffer to receive material pointers.
	\param	[in]  bufferSize	The number of material pointers which can be stored in the buffer.
	\param	[in]  startIndex	Index of first material pointer to be retrieved.
	\return The number of material pointers written to userBuffer, this should be less or equal to bufferSize.

	@see getNbCustomMaterials() PxCustomMaterial
	*/
	virtual PxU32 getCustomMaterials(PxCustomMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex = 0) const = 0;


	//@}
	/** @name Deletion Listeners
	*/
	//@{

	/**
	\brief Register a deletion listener. Listeners will be called whenever an object is deleted.

	It is illegal to register or unregister a deletion listener while deletions are being processed.

	\note By default a registered listener will receive events from all objects. Set the restrictedObjectSet parameter to true on registration and use #registerDeletionListenerObjects to restrict the received events to specific objects.

	\note The deletion events are only supported on core PhysX objects. In general, objects in extension modules do not provide this functionality, however, in the case of PxJoint objects, the underlying PxConstraint will send the events.

	\param	[in] observer				Observer object to send notifications to.
	\param	[in] deletionEvents			The deletion event types to get notified of.
	\param	[in] restrictedObjectSet	If false, the deletion listener will get events from all objects, else the objects to receive events from have to be specified explicitly through #registerDeletionListenerObjects.

	@see PxDeletionListener unregisterDeletionListener
	*/
	virtual void registerDeletionListener(PxDeletionListener& observer, const PxDeletionEventFlags& deletionEvents, bool restrictedObjectSet = false) = 0;

	/**
	\brief Unregister a deletion listener.

	It is illegal to register or unregister a deletion listener while deletions are being processed.

	\param	[in] observer	Observer object to stop sending notifications to.

	@see PxDeletionListener registerDeletionListener
	*/
	virtual void unregisterDeletionListener(PxDeletionListener& observer) = 0;

	/**
	\brief Register specific objects for deletion events.

	This method allows for a deletion listener to limit deletion events to specific objects only.

	\note It is illegal to register or unregister objects while deletions are being processed.

	\note The deletion listener has to be registered through #registerDeletionListener() and configured to support restricted object sets prior to this method being used.

	\param	[in] observer			Observer object to send notifications to.
	\param	[in] observables		List of objects for which to receive deletion events. Only PhysX core objects are supported. In the case of PxJoint objects, the underlying PxConstraint can be used to get the events.
	\param	[in] observableCount	Size of the observables list.

	@see PxDeletionListener unregisterDeletionListenerObjects
	*/
	virtual void registerDeletionListenerObjects(PxDeletionListener& observer, const PxBase* const* observables, PxU32 observableCount) = 0;

	/**
	\brief Unregister specific objects for deletion events.

	This method allows to clear previously registered objects for a deletion listener (see #registerDeletionListenerObjects()).

	\note It is illegal to register or unregister objects while deletions are being processed.

	\note The deletion listener has to be registered through #registerDeletionListener() and configured to support restricted object sets prior to this method being used.

	\param	[in] observer			Observer object to stop sending notifications to.
	\param	[in] observables		List of objects for which to not receive deletion events anymore.
	\param	[in] observableCount	Size of the observables list.

	@see PxDeletionListener registerDeletionListenerObjects
	*/
	virtual void unregisterDeletionListenerObjects(PxDeletionListener& observer, const PxBase* const* observables, PxU32 observableCount) = 0;

	/**
	\brief Gets PxPhysics object insertion interface.

	The insertion interface is needed for PxCreateTriangleMesh, PxCooking::createTriangleMesh etc., this allows runtime mesh creation.

	@see PxCreateTriangleMesh PxCreateHeightField PxCreateTetrahedronMesh PxCreateBVH
	     PxCooking::createTriangleMesh PxCooking::createHeightfield PxCooking::createTetrahedronMesh PxCooking::createBVH
	*/
	virtual PxInsertionCallback& getPhysicsInsertionCallback() = 0;

	//@}
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/**
\brief Enables the usage of the reduced coordinate articulations feature. This function is called automatically inside PxCreatePhysics().
On resource constrained platforms, it is possible to call PxCreateBasePhysics() and then NOT call this function
to save on code memory if your application does not use reduced coordinate articulations. In this case the linker should strip out
the relevant implementation code from the library. If you need to use reduced coordinate articulations but not some other optional
component, you shoud call PxCreateBasePhysics() followed by this call.

@deprecated
*/
PX_DEPRECATED PX_C_EXPORT PX_PHYSX_CORE_API void PX_CALL_CONV PxRegisterArticulationsReducedCoordinate(physx::PxPhysics& physics);

/**
\brief Enables the usage of the heightfield feature.

This call will link the default 'unified' implementation of heightfields which is identical to the narrow phase of triangle meshes.
This function is called automatically inside PxCreatePhysics().

On resource constrained platforms, it is possible to call PxCreateBasePhysics() and then NOT call this function
to save on code memory if your application does not use heightfields. In this case the linker should strip out
the relevant implementation code from the library. If you need to use heightfield but not some other optional
component, you shoud call PxCreateBasePhysics() followed by this call.

You must call this function at a time where no ::PxScene instance exists, typically before calling PxPhysics::createScene().
This is to prevent a change to the heightfield implementation code at runtime which would have undefined results.

Calling PxCreateBasePhysics() and then attempting to create a heightfield shape without first calling
::PxRegisterHeightFields(), will result in an error.

@deprecated
*/
PX_DEPRECATED PX_C_EXPORT PX_PHYSX_CORE_API void PX_CALL_CONV PxRegisterHeightFields(physx::PxPhysics& physics);

/**
\brief Creates an instance of the physics SDK with minimal additional components registered

Creates an instance of this class. May not be a class member to avoid name mangling.
Pass the constant #PX_PHYSICS_VERSION as the argument.
There may be only one instance of this class per process. Calling this method after an instance
has been created already will result in an error message and NULL will be returned.

\param version Version number we are expecting (should be #PX_PHYSICS_VERSION)
\param foundation Foundation instance (see PxFoundation)
\param scale values used to determine default tolerances for objects at creation time
\param trackOutstandingAllocations true if you want to track memory allocations
			so a debugger connection partway through your physics simulation will get
			an accurate map of everything that has been allocated so far. This could have a memory
			and performance impact on your simulation hence it defaults to off.
\param pvd When pvd points to a valid PxPvd instance (PhysX Visual Debugger), a connection to the specified PxPvd instance is created.
			If pvd is NULL no connection will be attempted.
\param omniPvd When omniPvd points to a valid PxOmniPvd instance PhysX will sample its internal structures to the defined OmniPvd output streams
			set in the PxOmniPvd object.
\return PxPhysics instance on success, NULL if operation failed

@see PxPhysics, PxFoundation, PxTolerancesScale, PxPvd, PxOmniPvd

@deprecated
*/
PX_DEPRECATED PX_C_EXPORT PX_PHYSX_CORE_API physx::PxPhysics* PX_CALL_CONV PxCreateBasePhysics(	physx::PxU32 version,
																					physx::PxFoundation& foundation,
																					const physx::PxTolerancesScale& scale,
																					bool trackOutstandingAllocations = false,
																					physx::PxPvd* pvd = NULL,
																					physx::PxOmniPvd* omniPvd = NULL);

/**
\brief Creates an instance of the physics SDK.

Creates an instance of this class. May not be a class member to avoid name mangling.
Pass the constant #PX_PHYSICS_VERSION as the argument.
There may be only one instance of this class per process. Calling this method after an instance
has been created already will result in an error message and NULL will be returned.

Calling this will register all optional code modules (Articulations and HeightFields), preparing them for use.
If you do not need some of these modules, consider calling PxCreateBasePhysics() instead and registering needed
modules manually.

\param version Version number we are expecting (should be #PX_PHYSICS_VERSION)
\param foundation Foundation instance (see PxFoundation)
\param scale values used to determine default tolerances for objects at creation time
\param trackOutstandingAllocations true if you want to track memory allocations
			so a debugger connection partway through your physics simulation will get
			an accurate map of everything that has been allocated so far. This could have a memory
			and performance impact on your simulation hence it defaults to off.
\param pvd When pvd points to a valid PxPvd instance (PhysX Visual Debugger), a connection to the specified PxPvd instance is created.
			If pvd is NULL no connection will be attempted.
\param omniPvd When omniPvd points to a valid PxOmniPvd instance PhysX will sample its internal structures to the defined OmniPvd output streams
			set in the PxOmniPvd object.
\return PxPhysics instance on success, NULL if operation failed

@see PxPhysics, PxCreateBasePhysics, PxRegisterArticulationsReducedCoordinate, PxRegisterHeightFields
*/
PX_INLINE physx::PxPhysics* PxCreatePhysics(physx::PxU32 version,
											physx::PxFoundation& foundation,
											const physx::PxTolerancesScale& scale,
											bool trackOutstandingAllocations = false,
											physx::PxPvd* pvd = NULL,
											physx::PxOmniPvd* omniPvd = NULL)
{
	physx::PxPhysics* physics = PxCreateBasePhysics(version, foundation, scale, trackOutstandingAllocations, pvd, omniPvd);
	if (!physics)
		return NULL;

	PxRegisterArticulationsReducedCoordinate(*physics);
	PxRegisterHeightFields(*physics);

	return physics;
}


/**
\brief Retrieves the Physics SDK after it has been created.

Before using this function the user must call #PxCreatePhysics() or #PxCreateBasePhysics().

\note The behavior of this method is undefined if the Physics SDK instance has not been created already.
*/
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

PX_C_EXPORT PX_PHYSX_CORE_API physx::PxPhysics& PX_CALL_CONV PxGetPhysics();

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/** @} */
#endif
