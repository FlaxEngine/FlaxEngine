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

#ifndef PX_TRIANGLE_MESH_H
#define PX_TRIANGLE_MESH_H
/** \addtogroup geomutils 
@{ */

#include "foundation/PxVec3.h"
#include "foundation/PxBounds3.h"
#include "common/PxPhysXCommonConfig.h"
#include "common/PxBase.h"
#include "foundation/PxUserAllocated.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Mesh midphase structure. This enum is used to select the desired acceleration structure for midphase queries
 (i.e. raycasts, overlaps, sweeps vs triangle meshes).

 The PxMeshMidPhase::eBVH33 structure is the one used in recent PhysX versions (up to PhysX 3.3). It has great performance and is
 supported on all platforms. It is deprecated since PhysX 5.x.

 The PxMeshMidPhase::eBVH34 structure is a revisited implementation introduced in PhysX 3.4. It can be significantly faster both
 in terms of cooking performance and runtime performance.
*/
struct PxMeshMidPhase
{
	enum Enum
	{
		eBVH33 = 0,		//!< Default midphase mesh structure, as used up to PhysX 3.3 (deprecated)
		eBVH34 = 1,		//!< New midphase mesh structure, introduced in PhysX 3.4

		eLAST
	};
};

/**
\brief Flags for the mesh geometry properties.

Used in ::PxTriangleMeshFlags.
*/
struct PxTriangleMeshFlag
{
	enum Enum
	{
		e16_BIT_INDICES	= (1<<1),	//!< The triangle mesh has 16bits vertex indices.
		eADJACENCY_INFO	= (1<<2),	//!< The triangle mesh has adjacency information build.
		ePREFER_NO_SDF_PROJ = (1<<3)//!< Indicates that this mesh would preferably not be the mesh projected for mesh-mesh collision. This can indicate that the mesh is not well tessellated.
	};
};

/**
\brief collection of set bits defined in PxTriangleMeshFlag.

@see PxTriangleMeshFlag
*/
typedef PxFlags<PxTriangleMeshFlag::Enum,PxU8> PxTriangleMeshFlags;
PX_FLAGS_OPERATORS(PxTriangleMeshFlag::Enum,PxU8)

/**

\brief A triangle mesh, also called a 'polygon soup'.

It is represented as an indexed triangle list. There are no restrictions on the
triangle data. 

To avoid duplicating data when you have several instances of a particular 
mesh positioned differently, you do not use this class to represent a 
mesh object directly. Instead, you create an instance of this mesh via
the PxTriangleMeshGeometry and PxShape classes.

<h3>Creation</h3>

To create an instance of this class call PxPhysics::createTriangleMesh(),
and release() to delete it. This is only possible
once you have released all of its PxShape instances.


<h3>Visualizations:</h3>
\li #PxVisualizationParameter::eCOLLISION_AABBS
\li #PxVisualizationParameter::eCOLLISION_SHAPES
\li #PxVisualizationParameter::eCOLLISION_AXES
\li #PxVisualizationParameter::eCOLLISION_FNORMALS
\li #PxVisualizationParameter::eCOLLISION_EDGES

@see PxTriangleMeshDesc PxTriangleMeshGeometry PxShape PxPhysics.createTriangleMesh()
*/
class PxTriangleMesh : public PxRefCounted
{
	public:
	/**
	\brief Returns the number of vertices.
	\return	number of vertices
	@see getVertices()
	*/
	virtual	PxU32	getNbVertices()	const	= 0;

	/**
	\brief Returns the vertices.
	\return	array of vertices
	@see getNbVertices()
	*/
	virtual	const PxVec3*	getVertices()	const	= 0;

	/**
	\brief Returns all mesh vertices for modification.

	This function will return the vertices of the mesh so that their positions can be changed in place.
	After modifying the vertices you must call refitBVH for the refitting to actually take place.
	This function maintains the old mesh topology (triangle indices).	

	\return  inplace vertex coordinates for each existing mesh vertex.

	\note It is recommended to use this feature for scene queries only.
	\note Size of array returned is equal to the number returned by getNbVertices().
	\note This function operates on cooked vertex indices.
	\note This means the index mapping and vertex count can be different from what was provided as an input to the cooking routine.
	\note To achieve unchanged 1-to-1 index mapping with orignal mesh data (before cooking) please use the following cooking flags:
	\note eWELD_VERTICES = 0, eDISABLE_CLEAN_MESH = 1.
	\note It is also recommended to make sure that a call to validateTriangleMesh returns true if mesh cleaning is disabled.
	@see getNbVertices()
	@see refitBVH()	
	*/
	virtual PxVec3*	getVerticesForModification() = 0;

	/**
	\brief Refits BVH for mesh vertices.

	This function will refit the mesh BVH to correctly enclose the new positions updated by getVerticesForModification.
	Mesh BVH will not be reoptimized by this function so significantly different new positions will cause significantly reduced performance.	

	\return New bounds for the entire mesh.

	\note For PxMeshMidPhase::eBVH34 trees the refit operation is only available on non-quantized trees (see PxBVH34MidphaseDesc::quantized)
	\note PhysX does not keep a mapping from the mesh to mesh shapes that reference it.
	\note Call PxShape::setGeometry on each shape which references the mesh, to ensure that internal data structures are updated to reflect the new geometry.
	\note PxShape::setGeometry does not guarantee correct/continuous behavior when objects are resting on top of old or new geometry.
	\note It is also recommended to make sure that a call to validateTriangleMesh returns true if mesh cleaning is disabled.
	\note Active edges information will be lost during refit, the rigid body mesh contact generation might not perform as expected.
	@see getNbVertices()
	@see getVerticesForModification()
	@see PxBVH34MidphaseDesc::quantized
	*/
	virtual PxBounds3	refitBVH() = 0;

	/**
	\brief Returns the number of triangles.
	\return	number of triangles
	@see getTriangles() getTrianglesRemap()
	*/
	virtual	PxU32	getNbTriangles()	const	= 0;

	/**
	\brief Returns the triangle indices.

	The indices can be 16 or 32bit depending on the number of triangles in the mesh.
	Call getTriangleMeshFlags() to know if the indices are 16 or 32 bits.

	The number of indices is the number of triangles * 3.

	\return	array of triangles
	@see getNbTriangles() getTriangleMeshFlags() getTrianglesRemap()
	*/
	virtual	const void*	getTriangles()	const	= 0;

	/**
	\brief Reads the PxTriangleMesh flags.
	
	See the list of flags #PxTriangleMeshFlag

	\return The values of the PxTriangleMesh flags.

	@see PxTriangleMesh
	*/
	virtual	PxTriangleMeshFlags		getTriangleMeshFlags()	const = 0;

	/**
	\brief Returns the triangle remapping table.

	The triangles are internally sorted according to various criteria. Hence the internal triangle order
	does not always match the original (user-defined) order. The remapping table helps finding the old
	indices knowing the new ones:

		remapTable[ internalTriangleIndex ] = originalTriangleIndex

	\return	the remapping table (or NULL if 'PxCookingParams::suppressTriangleMeshRemapTable' has been used)
	@see getNbTriangles() getTriangles() PxCookingParams::suppressTriangleMeshRemapTable
	*/
	virtual	const PxU32*	getTrianglesRemap()	const	= 0;

	/**	
	\brief Decrements the reference count of a triangle mesh and releases it if the new reference count is zero.	
	
	@see PxPhysics.createTriangleMesh()
	*/
	virtual void	release()	= 0;

	/**
	\brief Returns material table index of given triangle

	This function takes a post cooking triangle index.

	\param[in] triangleIndex (internal) index of desired triangle
	\return Material table index, or 0xffff if no per-triangle materials are used
	*/
	virtual	PxMaterialTableIndex	getTriangleMaterialIndex(PxTriangleID triangleIndex)	const	= 0;

	/**
	\brief Returns the local-space (vertex space) AABB from the triangle mesh.

	\return	local-space bounds
	*/
	virtual	PxBounds3	getLocalBounds()	const	= 0;

	/**
	\brief Returns the local-space Signed Distance Field for this mesh if it has one.
	\return local-space SDF.
	*/
	virtual const PxReal*	getSDF() const = 0;

	/**
	\brief Returns the resolution of the local-space dense SDF.
	*/
	virtual void getSDFDimensions(PxU32& numX, PxU32& numY, PxU32& numZ) const = 0;

	/**
	\brief Sets whether this mesh should be preferred for SDF projection.

	By default, meshes are flagged as preferring projection and the decisions on which mesh to project is based on the triangle and vertex 
	count. The model with the fewer triangles is projected onto the SDF of the more detailed mesh.
	If one of the meshes is set to prefer SDF projection (default) and the other is set to not prefer SDF projection, model flagged as 
	preferring SDF projection will be projected onto the model flagged as not preferring, regardless of the detail of the respective meshes.
	Where both models are flagged as preferring no projection, the less detailed model will be projected as before.

	\param[in] preferProjection Indicates if projection is preferred
	*/
	virtual void setPreferSDFProjection(bool preferProjection) = 0;

	/**
	\brief Returns whether this mesh prefers SDF projection.
	\return whether this mesh prefers SDF projection.
	*/
	virtual bool getPreferSDFProjection() const = 0;

	/**
	\brief Returns the mass properties of the mesh assuming unit density.

	The following relationship holds between mass and volume:

		mass = volume * density

	The mass of a unit density mesh is equal to its volume, so this function returns the volume of the mesh.

	Similarly, to obtain the localInertia of an identically shaped object with a uniform density of d, simply multiply the
	localInertia of the unit density mesh by d.

	\param[out] mass The mass of the mesh assuming unit density.
	\param[out] localInertia The inertia tensor in mesh local space assuming unit density.
	\param[out] localCenterOfMass Position of center of mass (or centroid) in mesh local space.
	*/
	virtual	void getMassInformation(PxReal& mass, PxMat33& localInertia, PxVec3& localCenterOfMass)	const = 0;

protected:
	PX_INLINE			PxTriangleMesh(PxType concreteType, PxBaseFlags baseFlags) : PxRefCounted(concreteType, baseFlags)	{}
	PX_INLINE			PxTriangleMesh(PxBaseFlags baseFlags) : PxRefCounted(baseFlags)										{}
	virtual				~PxTriangleMesh() {}

	virtual	bool		isKindOf(const char* name) const { return !::strcmp("PxTriangleMesh", name) || PxRefCounted::isKindOf(name); }
};

/**

\brief A triangle mesh containing the PxMeshMidPhase::eBVH33 structure.

@see PxMeshMidPhase
@deprecated
*/
class PX_DEPRECATED PxBVH33TriangleMesh : public PxTriangleMesh
{
	public:
protected:
	PX_INLINE			PxBVH33TriangleMesh(PxType concreteType, PxBaseFlags baseFlags) : PxTriangleMesh(concreteType, baseFlags) {}
	PX_INLINE			PxBVH33TriangleMesh(PxBaseFlags baseFlags) : PxTriangleMesh(baseFlags) {}
	virtual				~PxBVH33TriangleMesh() {}
	virtual	bool		isKindOf(const char* name) const { return !::strcmp("PxBVH33TriangleMesh", name) || PxTriangleMesh::isKindOf(name); }
};

/**

\brief A triangle mesh containing the PxMeshMidPhase::eBVH34 structure.

@see PxMeshMidPhase
*/
class PxBVH34TriangleMesh : public PxTriangleMesh
{
	public:
protected:
	PX_INLINE			PxBVH34TriangleMesh(PxType concreteType, PxBaseFlags baseFlags) : PxTriangleMesh(concreteType, baseFlags) {}
	PX_INLINE			PxBVH34TriangleMesh(PxBaseFlags baseFlags) : PxTriangleMesh(baseFlags) {}
	virtual				~PxBVH34TriangleMesh() {}
	virtual	bool		isKindOf(const char* name) const { return !::strcmp("PxBVH34TriangleMesh", name) || PxTriangleMesh::isKindOf(name); }
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
