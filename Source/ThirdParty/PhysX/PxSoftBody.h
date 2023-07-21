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

#ifndef PX_SOFT_BODY_H
#define PX_SOFT_BODY_H
/** \addtogroup physics
@{ */

#include "PxFEMParameter.h"
#include "PxActor.h"
#include "PxConeLimitedConstraint.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4435)
#endif

	class PxCudaContextManager;
	class PxBuffer;
	class PxTetrahedronMesh;
	class PxSoftBodyAuxData;
	class PxFEMCloth;
	class PxParticleBuffer;

	/**
	\brief The maximum tetrahedron index supported in the model.
	*/
	#define	PX_MAX_TETID	0x000fffff

	/**
	\brief Identifies input and output buffers for PxSoftBody.

	@see PxSoftBodyData::readData(), PxSoftBodyData::writeData(), PxBuffer.
	*/
	struct PxSoftBodyData
	{
		enum Enum
		{
			eNONE = 0,

			ePOSITION_INVMASS = 1 << 0,		//!< Flag to request access to the collision mesh's positions; read only @see PxSoftBody::writeData
			eSIM_POSITION_INVMASS = 1 << 2,	//!< Flag to request access to the simulation mesh's positions and inverse masses
			eSIM_VELOCITY = 1 << 3,			//!< Flag to request access to the simulation mesh's velocities and inverse masses
			eSIM_KINEMATIC_TARGET = 1 << 4,	//!< Flag to request access to the simulation mesh's kinematic target position

			eALL = ePOSITION_INVMASS | eSIM_POSITION_INVMASS | eSIM_VELOCITY | eSIM_KINEMATIC_TARGET
		};
	};

	typedef PxFlags<PxSoftBodyData::Enum, PxU32> PxSoftBodyDataFlags;

	/**
	\brief Flags to enable or disable special modes of a SoftBody
	*/
	struct PxSoftBodyFlag
	{
		enum Enum
		{
			eDISABLE_SELF_COLLISION = 1 << 0,		//!< Determines if self collision will be detected and resolved
			eCOMPUTE_STRESS_TENSOR = 1 << 1,		//!< Enables computation of a Cauchy stress tensor for every tetrahedron in the simulation mesh. The tensors can be accessed through the softbody direct API
			eENABLE_CCD = 1 << 2,					//!< Enables support for continuous collision detection
			eDISPLAY_SIM_MESH = 1 << 3,				//!< Enable debug rendering to display the simulation mesh
			eKINEMATIC = 1 << 4,					//!< Enables support for kinematic motion of the collision and simulation mesh.
			ePARTIALLY_KINEMATIC = 1 << 5			//!< Enables partially kinematic motion of the collisios and simulation mesh.
		};
	};

	typedef PxFlags<PxSoftBodyFlag::Enum, PxU32> PxSoftBodyFlags;

	/**
	\brief Represents a FEM softbody including everything to calculate its definition like geometry and material properties
	*/
	class PxSoftBody : public PxActor
	{
	public:

		virtual							~PxSoftBody() {}

		/**
		\brief Set a single softbody flag

		\param[in] flag The flag to set or clear
		\param[in] val The new state of the flag
		*/
		virtual		void				setSoftBodyFlag(PxSoftBodyFlag::Enum flag, bool val) = 0;
		
		/**
		\brief Set the softbody flags

		\param[in] flags The new softbody flags
		*/
		virtual		void				setSoftBodyFlags(PxSoftBodyFlags flags) = 0;

		/**
		\brief Get the softbody flags

		\return The softbody flags
		*/
		virtual		PxSoftBodyFlags		getSoftBodyFlag() const = 0;

		/**
		\brief Set parameter for FEM internal solve

		\param[in] parameters The FEM parameters
		*/
		virtual void					setParameter(const PxFEMParameters parameters) = 0;

		/**
		\brief Get parameter for FEM internal solve

		\return The FEM parameters
		*/
		virtual PxFEMParameters			getParameter() const = 0;

		/**
		\brief Issues a read command to the PxSoftBody.

		Read operations are scheduled and then flushed in PxScene::simulate().
		Read operations are known to be finished when PxBuffer::map() returns.

		PxSoftBodyData::ePOSITION_INVMASS, PxSoftBodyData::eSIM_POSITION_INVMASS and PxSoftBodyData::eSIM_VELOCITY can be read from the PxSoftBody.

		The softbody class offers internal cpu buffers that can be used to hold the data. The cpu buffers are accessible through getPositionInvMassCPU(),
		getSimPositionInvMassCPU() and getSimVelocityInvMassCPU().

		\param[in] flags Specifies which PxSoftBody data to read from.
		\param[in] buffer Specifies buffer to which data is written to.
		\param[in] flush If set to true the command gets executed immediately, otherwise it will get executed the next time copy commands are flushed.

		@see writeData(), PxBuffer, PxSoftBodyData
		*/
		virtual		void				readData(PxSoftBodyData::Enum flags, PxBuffer& buffer, bool flush = false) = 0;

		/**
		\brief Issues a read command to the PxSoftBody.

		Read operations are scheduled and then flushed in PxScene::simulate().
		Read operations are known to be finished when PxBuffer::map() returns.

		PxSoftBodyData::ePOSITION_INVMASS, PxSoftBodyData::eSIM_POSITION_INVMASS and PxSoftBodyData::eSIM_VELOCITY can be read from the PxSoftBody.

		The data to read from the GPU is written to the corresponding cpu buffer that a softbody provides. Those cpu buffers are accessible through
		getPositionInvMassCPU(), getSimPositionInvMassCPU() or getSimVelocityInvMassCPU().

		\param[in] flags Specifies which PxSoftBody data to read from.
		\param[in] flush If set to true the command gets executed immediately, otherwise it will get executed the next time copy commands are flushed.

		@see writeData(), PxSoftBodyData
		*/
		virtual		void				readData(PxSoftBodyData::Enum flags, bool flush = false) = 0;

		/**
		\brief Issues a write command to the PxSoftBody.

		Write operations are scheduled and then flushed in PxScene::simulate().
		Write operations are known to be finished when PxScene::fetchResult() returns.

		PxSoftBodyData::eSIM_POSITION_INVMASS and PxSoftBodyData::eSIM_VELOCITY can be written to the PxSoftBody. PxSoftBodyData::ePOSITION_INVMASS is read only,
		because the collision-mesh vertices are driven by the simulation-mesh vertices, which can be written to with PxSoftBodyData::eSIM_POSITION_INVMASS.

		The softbody class offers internal cpu buffers that can be used to hold the data. The cpu buffers are accessible through getPositionInvMassCPU(),
		getSimPositionInvMassCPU() and getSimVelocityInvMassCPU(). Consider to use the PxSoftBodyExt::commit() extension method if all buffers should get written.

		\param[in] flags Specifies which PxSoftBody data to write to.
		\param[in] buffer Specifies buffer from which data is read.
		\param[in] flush If set to true the command gets executed immediately, otherwise it will get executed the next time copy commands are flushed.

		@see readData(), PxBuffer, PxSoftBodyData, PxSoftBodyExt::commit
		*/
		virtual		void				writeData(PxSoftBodyData::Enum flags, PxBuffer& buffer, bool flush = false) = 0;

		/**
		\brief Issues a write command to the PxSoftBody.

		Write operations are scheduled and then flushed in PxScene::simulate().
		Write operations are known to be finished when PxScene::fetchResult() returns.

		PxSoftBodyData::eSIM_POSITION_INVMASS and PxSoftBodyData::eSIM_VELOCITY can be written to the PxSoftBody. PxSoftBodyData::ePOSITION_INVMASS is read only,
		because the collision-mesh vertices are driven by the simulation-mesh vertices, which can be written to with PxSoftBodyData::eSIM_POSITION_INVMASS.

		The data to write to the GPU is taken from the corresponding cpu buffer that a softbody provides. Those cpu buffers are accessible through
		getPositionInvMassCPU(), getSimPositionInvMassCPU() or getSimVelocityInvMassCPU().

		\param[in] flags Specifies which PxSoftBody data to write to.
		\param[in] flush If set to true the command gets executed immediately, otherwise it will get executed the next time copy commands are flushed.

		@see readData(), PxSoftBodyData
		*/
		virtual		void				writeData(PxSoftBodyData::Enum flags, bool flush = false) = 0;

		/**
		\brief Return the cuda context manager

		\return The cuda context manager
		*/
		virtual		PxCudaContextManager*	getCudaContextManager() const = 0;

		/**
		\brief Sets the wake counter for the soft body.

		The wake counter value determines the minimum amount of time until the soft body can be put to sleep. Please note
		that a soft body will not be put to sleep if any vertex velocity is above the specified threshold
		or if other awake objects are touching it.

		\note Passing in a positive value will wake the soft body up automatically.

		<b>Default:</b> 0.4 (which corresponds to 20 frames for a time step of 0.02)

		\param[in] wakeCounterValue Wake counter value. <b>Range:</b> [0, PX_MAX_F32)

		@see isSleeping() getWakeCounter()
		*/
		virtual		void				setWakeCounter(PxReal wakeCounterValue) = 0;

		/**
		\brief Returns the wake counter of the soft body.

		\return The wake counter of the soft body.

		@see isSleeping() setWakeCounter()
		*/
		virtual		PxReal				getWakeCounter() const = 0;

		/**
		\brief Returns true if this soft body is sleeping.

		When an actor does not move for a period of time, it is no longer simulated in order to save time. This state
		is called sleeping. However, because the object automatically wakes up when it is either touched by an awake object,
		or a sleep-affecting property is changed by the user, the entire sleep mechanism should be transparent to the user.

		A soft body can only go to sleep if all vertices are ready for sleeping. A soft body is guaranteed to be awake
		if at least one of the following holds:

		\li The wake counter is positive (@see setWakeCounter()).
		\li The velocity of any vertex is above the sleep threshold.

		If a soft body is sleeping, the following state is guaranteed:

		\li The wake counter is zero.
		\li The linear velocity of all vertices is zero.

		When a soft body gets inserted into a scene, it will be considered asleep if all the points above hold, else it will
		be treated as awake.

		\note It is invalid to use this method if the soft body has not been added to a scene already.

		\return True if the soft body is sleeping.

		@see isSleeping()
		*/
		virtual		bool				isSleeping() const = 0;

		/**
		\brief Sets the solver iteration counts for the body.

		The solver iteration count determines how accurately deformation and contacts are resolved.
		If you are having trouble with softbodies that are not as stiff as they should be, then
		setting a higher position iteration count may improve the behavior.

		If intersecting bodies are being depenetrated too violently, increase the number of velocity
		iterations.

		<b>Default:</b> 4 position iterations, 1 velocity iteration

		\param[in] minPositionIters Minimal number of position iterations the solver should perform for this body. <b>Range:</b> [1,255]
		\param[in] minVelocityIters Minimal number of velocity iterations the solver should perform for this body. <b>Range:</b> [1,255]

		@see getSolverIterationCounts()
		*/
		virtual		void				setSolverIterationCounts(PxU32 minPositionIters, PxU32 minVelocityIters = 1) = 0;

		/**
		\brief Retrieves the solver iteration counts.

		@see setSolverIterationCounts()
		*/
		virtual		void				getSolverIterationCounts(PxU32& minPositionIters, PxU32& minVelocityIters) const = 0;


		/**
		\brief Retrieves the shape pointer belonging to the actor.

		\return Pointer to the collision mesh's shape
		@see PxShape getNbShapes() PxShape::release()
		*/
		virtual		PxShape*			getShape() = 0;


		/**
		\brief Retrieve the collision mesh pointer.

		Allows to access the geometry of the tetrahedral mesh used to perform collision detection

		\return Pointer to the collision mesh
		*/
		virtual PxTetrahedronMesh*		getCollisionMesh() = 0;

		/**
		\brief Retrieves the simulation mesh pointer.

		Allows to access the geometry of the tetrahedral mesh used to compute the object's deformation

		\return Pointer to the simulation mesh
		*/
		virtual PxTetrahedronMesh*		getSimulationMesh() = 0;

		/**
		\brief Retrieves the simulation state pointer.

		Allows to access the additional data of the simulation mesh (inverse mass, rest state etc.).
		The geometry part of the data is stored in the simulation mesh.

		\return Pointer to the simulation state
		*/
		virtual PxSoftBodyAuxData* getSoftBodyAuxData() = 0;


		/**
		\brief Attaches a shape

		Attaches the shape to use for collision detection

		\param[in] shape The shape to use for collisions

		\return Returns true if the operation was successful
		*/
		virtual		bool				attachShape(PxShape& shape) = 0;

		/**
		\brief Attaches a simulation mesh

		Attaches the simulation mesh (geometry) and a state containing inverse mass, rest pose
		etc. required to compute the softbody deformation.

		\param[in] simulationMesh The tetrahedral mesh used to compute the softbody's deformation
		\param[in] softBodyAuxData A state that contain a mapping from simulation to collision mesh, volume information etc.

		\return Returns true if the operation was successful
		*/
		virtual		bool				attachSimulationMesh(PxTetrahedronMesh& simulationMesh, PxSoftBodyAuxData& softBodyAuxData) = 0;

		/**
		\brief Detaches the shape

		Detaches the shape used for collision detection.

		@see void detachSimulationMesh()
		*/
		virtual     void				detachShape() = 0;

		/**
		\brief Detaches the simulation mesh

		Detaches the simulation mesh and simulation state used to compute the softbody deformation.

		@see void detachShape()
		*/
		virtual		void				detachSimulationMesh() = 0;

		/**
		\brief Releases the softbody

		Releases the softbody and frees its resources.
		*/
		virtual		void				release() = 0;

		/**
		\brief Creates a collision filter between a particle and a tetrahedron in the soft body's collision mesh.

		\param[in] particlesystem The particle system used for the collision filter
		\param[in] buffer The PxParticleBuffer to which the particle belongs to.
		\param[in] particleId The particle whose collisions with the tetrahedron in the soft body are filtered.
		\param[in] tetId The tetradedron in the soft body that is filtered. If tetId is PX_MAX_TETID, this particle will filter against all tetrahedra in this soft body
		*/
		virtual		void				addParticleFilter(PxPBDParticleSystem* particlesystem, const PxParticleBuffer* buffer, PxU32 particleId, PxU32 tetId) = 0;
		
		/**
		\brief Removes a collision filter between a particle and a tetrahedron in the soft body's collision mesh.

		\param[in] particlesystem The particle system used for the collision filter
		\param[in] buffer The PxParticleBuffer to which the particle belongs to.
		\param[in] particleId The particle whose collisions with the tetrahedron in the soft body are filtered.
		\param[in] tetId The tetrahedron in the soft body is filtered.
		*/
		virtual		void				removeParticleFilter(PxPBDParticleSystem* particlesystem, const PxParticleBuffer* buffer, PxU32 particleId, PxU32 tetId) = 0;
		
		/**
		\brief Creates an attachment between a particle and a soft body.
		Be aware that destroying the particle system before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the particle system does not.

		\param[in] particlesystem The particle system used for the attachment
		\param[in] buffer The PxParticleBuffer to which the particle belongs to.
		\param[in] particleId The particle that is attached to a tetrahedron in the soft body's collision mesh.
		\param[in] tetId The tetrahedron in the soft body's collision mesh to attach the particle to.
		\param[in] barycentric The barycentric coordinates of the particle attachment position with respect to the tetrahedron specified with tetId.
		\return Returns a handle that identifies the attachment created. This handle can be used to release the attachment later
		*/
		virtual		PxU32				addParticleAttachment(PxPBDParticleSystem* particlesystem, const PxParticleBuffer* buffer, PxU32 particleId, PxU32 tetId, const PxVec4& barycentric) = 0;
		
		
		/**
		\brief Removes an attachment between a particle and a soft body.
		Be aware that destroying the particle system before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the particle system does not.
		
		\param[in] particlesystem The particle system used for the attachment
		\param[in] handle Index that identifies the attachment. This handle gets returned by the addParticleAttachment when the attachment is created
		*/
		virtual		void				removeParticleAttachment(PxPBDParticleSystem* particlesystem, PxU32 handle) = 0;

		/**
		\brief Creates a collision filter between a vertex in a soft body and a rigid body.

		\param[in] actor The rigid body actor used for the collision filter
		\param[in] vertId The index of a vertex in the softbody's collision mesh whose collisions with the rigid body are filtered.
		*/
		virtual		void				addRigidFilter(PxRigidActor* actor, PxU32 vertId) = 0;

		/**
		\brief Removes a collision filter between a vertex in a soft body and a rigid body.

		\param[in] actor The rigid body actor used for the collision filter
		\param[in] vertId The index of a vertex in the softbody's collision mesh whose collisions with the rigid body are filtered.
		*/
		virtual		void				removeRigidFilter(PxRigidActor* actor, PxU32 vertId) = 0;

		/**
		\brief Creates a rigid attachment between a soft body and a rigid body.
		Be aware that destroying the rigid body before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the rigid body does not.

		This method attaches a vertex on the soft body collision mesh to the rigid body.

		\param[in] actor The rigid body actor used for the attachment
		\param[in] vertId The index of a vertex in the softbody's collision mesh that gets attached to the rigid body.
		\param[in] actorSpacePose The location of the attachment point expressed in the rigid body's coordinate system.
		\param[in] constraint The user defined cone distance limit constraint to limit the movement between a vertex in the soft body and rigid body.
		\return Returns a handle that identifies the attachment created. This handle can be used to relese the attachment later
		*/
		virtual		PxU32					addRigidAttachment(PxRigidActor* actor, PxU32 vertId, const PxVec3& actorSpacePose, PxConeLimitedConstraint* constraint = NULL) = 0;

		/**
		\brief Releases a rigid attachment between a soft body and a rigid body.
		Be aware that destroying the rigid body before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the rigid body does not.

		This method removes a previously-created attachment between a vertex of the soft body collision mesh and the rigid body.

		\param[in] actor The rigid body actor used for the attachment
		\param[in] handle Index that identifies the attachment. This handle gets returned by the addRigidAttachment when the attachment is created
		*/
		virtual		void					removeRigidAttachment(PxRigidActor* actor, PxU32 handle) = 0;

		/**
		\brief Creates collision filter between a tetrahedron in a soft body and a rigid body.

		\param[in] actor The rigid body actor used for collision filter
		\param[in] tetIdx The index of a tetrahedron in the softbody's collision mesh whose collisions with the rigid body is filtered.
		*/
		virtual		void					addTetRigidFilter(PxRigidActor* actor, PxU32 tetIdx) = 0;
		
		/**
		\brief Removes collision filter between a tetrahedron in a soft body and a rigid body.

		\param[in] actor The rigid body actor used for collision filter
		\param[in] tetIdx The index of a tetrahedron in the softbody's collision mesh whose collisions with the rigid body is filtered.
		*/
		virtual		void					removeTetRigidFilter(PxRigidActor* actor, PxU32 tetIdx) = 0;

		/**
		\brief Creates a rigid attachment between a soft body and a rigid body.
		Be aware that destroying the rigid body before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the rigid body does not.

		This method attaches a point inside a tetrahedron of the collision to the rigid body.

		\param[in] actor The rigid body actor used for the attachment
		\param[in] tetIdx The index of a tetrahedron in the softbody's collision mesh that contains the point to be attached to the rigid body
		\param[in] barycentric The barycentric coordinates of the attachment point inside the tetrahedron specified by tetIdx
		\param[in] actorSpacePose The location of the attachment point expressed in the rigid body's coordinate system.
		\param[in] constraint The user defined cone distance limit constraint to limit the movement between a tet and rigid body.
		\return Returns a handle that identifies the attachment created. This handle can be used to release the attachment later
		*/
		virtual		PxU32					addTetRigidAttachment(PxRigidActor* actor, PxU32 tetIdx, const PxVec4& barycentric, const PxVec3& actorSpacePose, PxConeLimitedConstraint* constraint = NULL) = 0;

		/**
		\brief Creates collision filter between a tetrahedron in a soft body and a tetrahedron in another soft body.

		\param[in] otherSoftBody The other soft body actor used for collision filter
		\param[in] otherTetIdx The index of the tetrahedron in the other softbody's collision mesh to be filtered.
		\param[in] tetIdx1 The index of the tetrahedron in the softbody's collision mesh to be filtered.
		*/
		virtual		void					addSoftBodyFilter(PxSoftBody* otherSoftBody, PxU32 otherTetIdx, PxU32 tetIdx1) = 0;

		/**
		\brief Removes collision filter between a tetrahedron in a soft body and a tetrahedron in other soft body.

		\param[in] otherSoftBody The other soft body actor used for collision filter
		\param[in] otherTetIdx The index of the other tetrahedron in the other softbody's collision mesh whose collision with the tetrahedron with the soft body is filtered.
		\param[in] tetIdx1 The index of the tetrahedron in the softbody's collision mesh whose collision with the other tetrahedron with the other soft body is filtered.
		*/
		virtual		void					removeSoftBodyFilter(PxSoftBody* otherSoftBody, PxU32 otherTetIdx, PxU32 tetIdx1) = 0;

		/**
		\brief Creates collision filters between a tetrahedron in a soft body with another soft body.

		\param[in] otherSoftBody The other soft body actor used for collision filter
		\param[in] otherTetIndices The indices of the tetrahedron in the other softbody's collision mesh to be filtered.
		\param[in] tetIndices The indices of the tetrahedron of the softbody's collision mesh to be filtered.
		\param[in] tetIndicesSize The size of tetIndices.
		*/
		virtual		void					addSoftBodyFilters(PxSoftBody* otherSoftBody, PxU32* otherTetIndices, PxU32* tetIndices, PxU32 tetIndicesSize) = 0;

		/**
		\brief Removes collision filters between a tetrahedron in a soft body with another soft body.

		\param[in] otherSoftBody The other soft body actor used for collision filter
		\param[in] otherTetIndices The indices of the tetrahedron in the other softbody's collision mesh to be filtered.
		\param[in] tetIndices The indices of the tetrahedron of the softbody's collision mesh to be filtered.
		\param[in] tetIndicesSize The size of tetIndices.
		*/
		virtual		void					removeSoftBodyFilters(PxSoftBody* otherSoftBody, PxU32* otherTetIndices, PxU32* tetIndices, PxU32 tetIndicesSize) = 0;

		/**
		\brief Creates an attachment between two soft bodies.

		This method attaches a point inside a tetrahedron of the collision mesh to a point in another soft body's tetrahedron collision mesh.

		\param[in] softbody0 The soft body actor used for the attachment
		\param[in] tetIdx0 The index of a tetrahedron in the other soft body that contains the point to be attached to the soft body
		\param[in] tetBarycentric0 The barycentric coordinates of the attachment point inside the tetrahedron specified by tetIdx0
		\param[in] tetIdx1 The index of a tetrahedron in the softbody's collision mesh that contains the point to be attached to the softbody0
		\param[in] tetBarycentric1 The barycentric coordinates of the attachment point inside the tetrahedron specified by tetIdx1
		\param[in] constraint The user defined cone distance limit constraint to limit the movement between tets.
		\return Returns a handle that identifies the attachment created. This handle can be used to release the attachment later
		*/
		virtual		PxU32					addSoftBodyAttachment(PxSoftBody* softbody0, PxU32 tetIdx0, const PxVec4& tetBarycentric0, PxU32 tetIdx1, const PxVec4& tetBarycentric1,
											PxConeLimitedConstraint* constraint = NULL) = 0;

		/**
		\brief Releases an attachment between a soft body and the other soft body.
		Be aware that destroying the soft body before destroying the attachment is illegal and may cause a crash.

		This method removes a previously-created attachment between a point inside a tetrahedron of the collision mesh to a point in another soft body's tetrahedron collision mesh.

		\param[in] softbody0 The softbody actor used for the attachment.
		\param[in] handle Index that identifies the attachment. This handle gets returned by the addSoftBodyAttachment when the attachment is created.
		*/
		virtual		void					removeSoftBodyAttachment(PxSoftBody* softbody0, PxU32 handle) = 0;

		/**
		\brief Creates collision filter between a tetrahedron in a soft body and a triangle in a cloth.
		\warning Feature under development, only for internal usage.

		\param[in] cloth The cloth actor used for collision filter
		\param[in] triIdx The index of the triangle in the cloth mesh to be filtered.
		\param[in] tetIdx The index of the tetrahedron in the softbody's collision mesh to be filtered.
		*/
		virtual		void					addClothFilter(PxFEMCloth* cloth, PxU32 triIdx, PxU32 tetIdx) = 0;

		/**
		\brief Removes collision filter between a tetrahedron in a soft body and a triangle in a cloth.
		\warning Feature under development, only for internal usage.

		\param[in] cloth The cloth actor used for collision filter
		\param[in] triIdx The index of the triangle in the cloth mesh to be filtered.
		\param[in] tetIdx The index of the tetrahedron in the softbody's collision mesh to be filtered.
		*/
		virtual		void					removeClothFilter(PxFEMCloth* cloth, PxU32 triIdx, PxU32 tetIdx) = 0;


		/**
		\brief Creates an attachment between a soft body and a cloth.
		Be aware that destroying the rigid body before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the cloth does not.

		This method attaches a point inside a tetrahedron of the collision mesh to a cloth.

		\warning Feature under development, only for internal usage.

		\param[in] cloth The cloth actor used for the attachment
		\param[in] triIdx The index of a triangle in the cloth mesh that contains the point to be attached to the soft body
		\param[in] triBarycentric The barycentric coordinates of the attachment point inside the triangle specified by triangleIdx
		\param[in] tetIdx The index of a tetrahedron in the softbody's collision mesh that contains the point to be attached to the cloth
		\param[in] tetBarycentric The barycentric coordinates of the attachment point inside the tetrahedron specified by tetIdx
		\param[in] constraint The user defined cone distance limit constraint to limit the movement between a triangle in the fem cloth and a tet in the soft body.
		\return Returns a handle that identifies the attachment created. This handle can be used to release the attachment later
		*/
		virtual		PxU32					addClothAttachment(PxFEMCloth* cloth, PxU32 triIdx, const PxVec4& triBarycentric, PxU32 tetIdx, const PxVec4& tetBarycentric,
											PxConeLimitedConstraint* constraint = NULL) = 0;

		/**
		\brief Releases an attachment between a cloth and a soft body.
		Be aware that destroying the cloth before destroying the attachment is illegal and may cause a crash.
		The soft body keeps track of these attachments but the cloth does not.

		This method removes a previously-created attachment between a point inside a collision mesh tetrahedron and a point inside a cloth mesh.

		\warning Feature under development, only for internal usage.

		\param[in] cloth The cloth actor used for the attachment
		\param[in] handle Index that identifies the attachment. This handle gets returned by the addClothAttachment when the attachment is created
		*/
		virtual		void					removeClothAttachment(PxFEMCloth* cloth, PxU32 handle) = 0;

		/**
		\brief Access to the vertices of the simulation mesh on the host

		Each element uses 4 float values containing position and inverseMass per vertex [x, y, z, inverseMass]
		The inverse mass must match the inverse mass in the simVelocityCPU buffer at the same index. A copy of this value
		is stored in the simVelocityCPU buffer to allow for faster access on the GPU. If the inverse masses in those two buffers
		don't match, the simulation may produce wrong results

		Allows to access the CPU buffer of the simulation mesh's vertices

		\return The buffer that contains the simulation mesh's vertex positions (x, y, z) and the inverse mass as 4th component
		*/
		virtual		PxBuffer*				getSimPositionInvMassCPU() = 0;

		/**
		\brief Access to the vertices of the simulation mesh on the host

		Each element uses 4 float values containing position and inverseMass per vertex [x, y, z, inverseMass]
		The inverse mass must match the inverse mass in the simVelocityCPU buffer at the same index. A copy of this value
		is stored in the simVelocityCPU buffer to allow for faster access on the GPU. If the inverse masses in those two buffers
		don't match, the simulation may produce wrong results

		Allows to access the CPU buffer of the simulation mesh's vertices

		\return The buffer that contains the simulation mesh's vertex positions (x, y, z) and the inverse mass as 4th component
		*/
		virtual		PxBuffer*				getKinematicTargetCPU() = 0;
		/**
		\brief Access to the velocities of the simulation mesh on the host

		Each element uses 4 float values containing velocity and inverseMass per vertex [x, y, z, inverseMass]
		The inverse mass must match the inverse mass in the simPositionInvMassCPU buffer at the same index. A copy of this value
		is stored in the simPositionInvMassCPU buffer to allow for faster access on the GPU. If the inverse masses in those two buffers
		don't match, the simulation may produce wrong results

		Allows to access the CPU buffer of the simulation mesh's vertices

		\return The buffer that contains the simulation mesh's velocities (x, y, z) and the inverse mass as 4th component
		*/
		virtual		PxBuffer*				getSimVelocityInvMassCPU() = 0;

		/**
		\brief Access to the vertices of the collision mesh on the host

		Each element uses 4 float values containing position and inverseMass per vertex [x, y, z, inverseMass]
		The inverse mass on the collision mesh has no effect, it can be set to an arbitrary value.

		Allows to access the CPU buffer of the collision mesh's vertices

		\return The buffer that contains the collision mesh's vertex positions (x, y, z) and the inverse mass as 4th component
		*/
		virtual		PxBuffer*				getPositionInvMassCPU() = 0;

		/**
		\brief Access to the rest vertices of the collision mesh on the host

		Each element uses 4 float values containing position and inverseMass per vertex [x, y, z, inverseMass]
		The inverse mass on the collision mesh has no effect, it can be set to an arbitrary value.

		Allows to access the CPU buffer of the collision mesh's rest vertices

		\return The buffer that contains the collision mesh's rest vertex positions (x, y, z) and the inverse mass as 4th component
		*/
		virtual		PxBuffer*				getRestPositionInvMassCPU() = 0;

		/**
		\brief Retrieves the axis aligned bounding box enclosing the soft body.

		\note It is not allowed to use this method while the simulation is running (except during PxScene::collide(),
		in PxContactModifyCallback or in contact report callbacks).

		\param[in] inflation  Scale factor for computed world bounds. Box extents are multiplied by this value.

		\return The soft body's bounding box.

		@see PxBounds3
		*/
		virtual		PxBounds3		getWorldBounds(float inflation = 1.01f) const = 0;

		/**
		\brief Returns the GPU soft body index.

		\return The GPU index, or 0xFFFFFFFF if the soft body is not in a scene.
		*/
		virtual		PxU32			getGpuSoftBodyIndex() = 0;
	
		virtual		const char*		getConcreteTypeName() const PX_OVERRIDE { return "PxSoftBody";  }


	protected:
		PX_INLINE					PxSoftBody(PxType concreteType, PxBaseFlags baseFlags) : PxActor(concreteType, baseFlags) {}
		PX_INLINE					PxSoftBody(PxBaseFlags baseFlags) : PxActor(baseFlags) {}
		virtual		bool			isKindOf(const char* name) const PX_OVERRIDE { return !::strcmp("PxSoftBody", name) || PxActor::isKindOf(name); }
	};

#if PX_VC
#pragma warning(pop)
#endif


#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
