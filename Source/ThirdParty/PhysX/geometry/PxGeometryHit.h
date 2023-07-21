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

#ifndef PX_GEOMETRY_HIT_H
#define PX_GEOMETRY_HIT_H
/** \addtogroup scenequery
@{
*/
#include "foundation/PxVec3.h"
#include "foundation/PxFlags.h"
#include "common/PxPhysXCommonConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Scene query and geometry query behavior flags.

PxHitFlags are used for 3 different purposes:

1) To request hit fields to be filled in by scene queries (such as hit position, normal, face index or UVs).
2) Once query is completed, to indicate which fields are valid (note that a query may produce more valid fields than requested).
3) To specify additional options for the narrow phase and mid-phase intersection routines.

All these flags apply to both scene queries and geometry queries (PxGeometryQuery).

@see PxRaycastHit PxSweepHit PxOverlapHit PxScene.raycast PxScene.sweep PxScene.overlap PxGeometryQuery PxFindFaceIndex
*/
struct PxHitFlag
{
	enum Enum
	{
		ePOSITION					= (1<<0),	//!< "position" member of #PxQueryHit is valid
		eNORMAL						= (1<<1),	//!< "normal" member of #PxQueryHit is valid
		eUV							= (1<<3),	//!< "u" and "v" barycentric coordinates of #PxQueryHit are valid. Not applicable to sweep queries.
		eASSUME_NO_INITIAL_OVERLAP	= (1<<4),	//!< Performance hint flag for sweeps when it is known upfront there's no initial overlap.
												//!< NOTE: using this flag may cause undefined results if shapes are initially overlapping.
		eANY_HIT					= (1<<5),	//!< Report any first hit. Used for geometries that contain more than one primitive. For meshes,
												//!< if neither eMESH_MULTIPLE nor eANY_HIT is specified, a single closest hit will be reported.
		eMESH_MULTIPLE				= (1<<6),	//!< Report all hits for meshes rather than just the first. Not applicable to sweep queries.
		eMESH_ANY					= eANY_HIT,	//!< @deprecated Deprecated, please use eANY_HIT instead.
		eMESH_BOTH_SIDES			= (1<<7),	//!< Report hits with back faces of mesh triangles. Also report hits for raycast
												//!< originating on mesh surface and facing away from the surface normal. Not applicable to sweep queries.
												//!< Please refer to the user guide for heightfield-specific differences.
		ePRECISE_SWEEP				= (1<<8),	//!< Use more accurate but slower narrow phase sweep tests.
												//!< May provide better compatibility with PhysX 3.2 sweep behavior.
		eMTD						= (1<<9),	//!< Report the minimum translation depth, normal and contact point.
		eFACE_INDEX					= (1<<10),	//!< "face index" member of #PxQueryHit is valid

		eDEFAULT					= ePOSITION|eNORMAL|eFACE_INDEX,

		/** \brief Only this subset of flags can be modified by pre-filter. Other modifications will be discarded. */
		eMODIFIABLE_FLAGS			= eMESH_MULTIPLE|eMESH_BOTH_SIDES|eASSUME_NO_INITIAL_OVERLAP|ePRECISE_SWEEP
	};
};

/**
\brief collection of set bits defined in PxHitFlag.

@see PxHitFlag
*/
PX_FLAGS_TYPEDEF(PxHitFlag, PxU16)

/**
\brief Scene query hit information.
*/
struct PxQueryHit
{
	PX_INLINE			PxQueryHit() : faceIndex(0xFFFFffff) {}

	/**
	Face index of touched triangle, for triangle meshes, convex meshes and height fields.

	\note This index will default to 0xFFFFffff value for overlap queries.
	\note Please refer to the user guide for more details for sweep queries.
	\note This index is remapped by mesh cooking. Use #PxTriangleMesh::getTrianglesRemap() to convert to original mesh index.
	\note For convex meshes use #PxConvexMesh::getPolygonData() to retrieve touched polygon data.
	*/
	PxU32				faceIndex;
};

/**
\brief Scene query hit information for raycasts and sweeps returning hit position and normal information.

::PxHitFlag flags can be passed to scene query functions, as an optimization, to cause the SDK to
only generate specific members of this structure.
*/
struct PxLocationHit : PxQueryHit
{
	PX_INLINE			PxLocationHit() : flags(0), position(PxVec3(0)), normal(PxVec3(0)), distance(PX_MAX_REAL)	{}

	/**
	\note For raycast hits: true for shapes overlapping with raycast origin.
	\note For sweep hits: true for shapes overlapping at zero sweep distance.

	@see PxRaycastHit PxSweepHit
	*/
	PX_INLINE bool		hadInitialOverlap() const { return (distance <= 0.0f); }

	// the following fields are set in accordance with the #PxHitFlags
	PxHitFlags			flags;		//!< Hit flags specifying which members contain valid values.
	PxVec3				position;	//!< World-space hit position (flag: #PxHitFlag::ePOSITION)
	PxVec3				normal;		//!< World-space hit normal (flag: #PxHitFlag::eNORMAL)

	/**
	\brief	Distance to hit.
	\note	If the eMTD flag is used, distance will be a negative value if shapes are overlapping indicating the penetration depth.
	\note	Otherwise, this value will be >= 0 */
	PxF32				distance;
};

/**
\brief Stores results of raycast queries.

::PxHitFlag flags can be passed to raycast function, as an optimization, to cause the SDK to only compute specified members of this
structure.

Some members like barycentric coordinates are currently only computed for triangle meshes and height fields, but next versions
might provide them in other cases. The client code should check #flags to make sure returned values are valid.

@see PxScene.raycast 
*/
struct PxGeomRaycastHit : PxLocationHit
{
	PX_INLINE			PxGeomRaycastHit() : u(0.0f), v(0.0f)	{}

	// the following fields are set in accordance with the #PxHitFlags

	PxReal	u, v;			//!< barycentric coordinates of hit point, for triangle mesh and height field (flag: #PxHitFlag::eUV)
};

/**
\brief Stores results of overlap queries.

@see PxScene.overlap 
*/
struct PxGeomOverlapHit : PxQueryHit
{
	PX_INLINE			PxGeomOverlapHit() {}
};

/**
\brief Stores results of sweep queries.

@see PxScene.sweep
*/
struct PxGeomSweepHit : PxLocationHit
{
	PX_INLINE			PxGeomSweepHit() {}
};

/**
\brief Pair of indices, typically either object or triangle indices.
*/
struct PxGeomIndexPair
{
    PX_FORCE_INLINE PxGeomIndexPair()													{}
    PX_FORCE_INLINE PxGeomIndexPair(PxU32	_id0, PxU32 _id1) : id0(_id0), id1(_id1)	{}

	PxU32	id0, id1;
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
