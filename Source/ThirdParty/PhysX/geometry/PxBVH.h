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

#ifndef PX_BVH_H
#define PX_BVH_H
/** \addtogroup geomutils
@{
*/

#include "common/PxBase.h"
#include "foundation/PxTransform.h"
#include "foundation/PxBounds3.h"
#include "geometry/PxGeometryQueryFlags.h"
#include "geometry/PxReportCallback.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxGeometry;

/**
\brief Class representing a bounding volume hierarchy.

PxBVH can be provided to PxScene::addActor. In this case the scene query 
pruning structure inside PhysX SDK will store/update one bound per actor. 
The scene queries against such an actor will query actor bounds and then 
make a local space query against the provided BVH, which is in actor's
local space.

PxBVH can also be used as a standalone data-structure for arbitrary
purposes, unrelated to PxScene / PxActor.

@see PxScene::addActor
*/
class PxBVH : public PxBase
{
public:

	/**
	\brief Raycast test against a BVH.

	\param[in] origin		The origin of the ray.
	\param[in] unitDir		Normalized direction of the ray.
	\param[in] maxDist		Maximum ray length, has to be in the [0, inf) range
	\param[in] maxHits		Max number of returned hits = size of 'rayHits' buffer
	\param[out] rayHits		Raycast hits information, bounds indices 
	\return Number of hits  
	@deprecated
	*/
	PX_DEPRECATED	virtual PxU32	raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal maxDist, PxU32 maxHits, PxU32* PX_RESTRICT rayHits) const = 0;

	/**
	\brief Sweep test against a BVH.

	\param[in] aabb			The axis aligned bounding box to sweep
	\param[in] unitDir		Normalized direction of the sweep.
	\param[in] maxDist		Maximum sweep length, has to be in the [0, inf) range
	\param[in] maxHits		Max number of returned hits = size of 'sweepHits' buffer
	\param[out] sweepHits	Sweep hits information, bounds indices 
	\return Number of hits 
	@deprecated
	*/
	PX_DEPRECATED	virtual PxU32	sweep(const PxBounds3& aabb, const PxVec3& unitDir, PxReal maxDist, PxU32 maxHits, PxU32* PX_RESTRICT sweepHits) const = 0;

	/**
	\brief AABB overlap test against a BVH.

	\param[in] aabb			The axis aligned bounding box		
	\param[in] maxHits		Max number of returned hits = size of 'overlapHits' buffer
	\param[out] overlapHits	Overlap hits information, bounds indices 
	\return Number of hits 
	@deprecated
	*/
	PX_DEPRECATED	virtual PxU32	overlap(const PxBounds3& aabb, PxU32 maxHits, PxU32* PX_RESTRICT overlapHits) const = 0;

	struct RaycastCallback
	{
						RaycastCallback()	{}
		virtual			~RaycastCallback()	{}

		// Reports one raycast or sweep hit.
		// boundsIndex	[in]		Index of touched bounds
		// distance		[in/out]	Impact distance. Shrinks the ray if written out.
		// return false to abort the query
		virtual bool	reportHit(PxU32 boundsIndex, PxReal& distance) = 0;
	};

	struct OverlapCallback
	{
						OverlapCallback()	{}
		virtual			~OverlapCallback()	{}

		// Reports one overlap hit.
		// boundsIndex	[in] Index of touched bounds
		// return false to abort the query
		virtual bool	reportHit(PxU32 boundsIndex) = 0;
	};

	struct TraversalCallback
	{
						TraversalCallback()		{}
		virtual			~TraversalCallback()	{}

		// Reports one visited node.
		// bounds	[in] node bounds
		// return true to continue traversing this branch
		virtual bool	visitNode(const PxBounds3& bounds) = 0;

		// Reports one validated leaf node. Called on leaf nodes after visitNode returns true on them.
		// nbPrims	[in] number of primitives in the node
		// prims	[in] primitives in the node (nbPrims entries)
		// return false to abort the query
		virtual bool	reportLeaf(PxU32 nbPrims, const PxU32* prims) = 0;
	};

	/**
	\brief Raycast test against a BVH.

	\param[in] origin		The origin of the ray.
	\param[in] unitDir		Normalized direction of the ray.
	\param[in] maxDist		Maximum ray length, has to be in the [0, inf) range
	\param[in] cb			Raycast callback, called once per hit
	\param[in] queryFlags	Optional flags controlling the query.
	\return false if query has been aborted
	*/
	virtual	bool				raycast(const PxVec3& origin, const PxVec3& unitDir, float maxDist, RaycastCallback& cb, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

	/**
	\brief Sweep test against a BVH.

	\param[in] geom			The query volume
	\param[in] pose			The pose of the query volume
	\param[in] unitDir		Normalized direction of the sweep.
	\param[in] maxDist		Maximum sweep length, has to be in the [0, inf) range
	\param[in] cb			Raycast callback, called once per hit
	\param[in] queryFlags	Optional flags controlling the query.
	\return false if query has been aborted
	*/
	virtual bool				sweep(const PxGeometry& geom, const PxTransform& pose, const PxVec3& unitDir, float maxDist, RaycastCallback& cb, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

	/**
	\brief Overlap test against a BVH.

	\param[in] geom			The query volume
	\param[in] pose			The pose of the query volume
	\param[in] cb			Overlap callback, called once per hit
	\param[in] queryFlags	Optional flags controlling the query.
	\return false if query has been aborted
	*/
	virtual	bool				overlap(const PxGeometry& geom, const PxTransform& pose, OverlapCallback& cb, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

	/**
	\brief Frustum culling test against a BVH.
	
	This is similar in spirit to an overlap query using a convex object around the frustum.
	However this specialized query has better performance, and can support more than the 6 planes
	of a frustum, which can be useful in portal-based engines.

	On the other hand this test only returns a conservative number of bounds, i.e. some of the returned
	bounds may actually be outside the frustum volume, close to it but not touching it. This is usually
	an ok performance trade-off when the function is used for view-frustum culling.

	\param[in] nbPlanes		Number of planes. Only 32 planes max are supported.
	\param[in] planes		Array of planes, should be in the same space as the BVH.
	\param[in] cb			Overlap callback, called once per visible object
	\param[in] queryFlags	Optional flags controlling the query.
	\return false if query has been aborted
	*/
	virtual	bool				cull(PxU32 nbPlanes, const PxPlane* planes, OverlapCallback& cb, PxGeometryQueryFlags queryFlags = PxGeometryQueryFlag::eDEFAULT) const = 0;

	/**
	\brief Returns the number of bounds in the BVH.

	You can use #getBounds() to retrieve the bounds.

	\note These are the user-defined bounds passed to the BVH builder, not the internal bounds around each BVH node.

	\return Number of bounds in the BVH.

	@see getBounds() getBoundsForModification()
	*/
	virtual PxU32				getNbBounds() const = 0;
	
	/**
	\brief Retrieve the read-only bounds in the BVH.

	\note These are the user-defined bounds passed to the BVH builder, not the internal bounds around each BVH node.

	@see PxBounds3 getNbBounds() getBoundsForModification()
	*/
	virtual const PxBounds3*	getBounds() const = 0;

	/**
	\brief Retrieve the bounds in the BVH.
	
	These bounds can be modified. Call refit() after modifications are done.

	\note These are the user-defined bounds passed to the BVH builder, not the internal bounds around each BVH node.

	@see PxBounds3 getNbBounds() getBounds() refit() updateBounds() partialRefit()
	*/
	PX_FORCE_INLINE	PxBounds3*	getBoundsForModification()
								{
									return const_cast<PxBounds3*>(getBounds());
								}

	/**
	\brief Refit the BVH.

	This function "refits" the tree, i.e. takes the new (leaf) bounding boxes into account and
	recomputes all the BVH bounds accordingly. This is an O(n) operation with n = number of bounds in the BVH.

	This works best with minor bounds modifications, i.e. when the bounds remain close to their initial values.
	With large modifications the tree quality degrades more and more, and subsequent query performance suffers.
	It might be a better strategy to create a brand new BVH if bounds change drastically.

	This function refits the whole tree after an arbitrary number of bounds have potentially been modified by
	users (via getBoundsForModification()). If you only have a small number of bounds to update, it might be
	more efficient to use setBounds() and partialRefit() instead.
	
	@see getNbBounds() getBoundsForModification() updateBounds() partialRefit()
	*/
	virtual	void				refit()	= 0;

	/**
	\brief Update single bounds.

	This is an alternative to getBoundsForModification() / refit(). If you only have a small set of bounds to
	update, it can be inefficient to call the refit() function, because it refits the whole BVH.

	Instead, one can update individual bounds with this updateBounds() function. It sets the new bounds and
	marks the corresponding BVH nodes for partial refit. Once all the individual bounds have been updated,
	call partialRefit() to only refit the subset of marked nodes.
	
	\param[in] boundsIndex	Index of updated bounds. Valid range is between 0 and getNbBounds().
	\param[in] newBounds	Updated bounds.

	\return true if success

	@see getNbBounds() getBoundsForModification() refit() partialRefit()
	*/
	virtual	bool				updateBounds(PxU32 boundsIndex, const PxBounds3& newBounds)	= 0;

	/**
	\brief Refits subset of marked nodes.

	This is an alternative to the refit() function, to be called after updateBounds() calls.
	See updateBounds() for details.
	
	@see getNbBounds() getBoundsForModification() refit() updateBounds()
	*/
	virtual	void				partialRefit()	= 0;

	/**
	\brief Generic BVH traversal function.

	This can be used to implement custom BVH traversal functions if provided ones are not enough.
	In particular this can be used to visualize the tree's bounds.

	\param[in] cb	Traversal callback, called for each visited node
	\return false if query has been aborted
	*/
	virtual	bool				traverse(TraversalCallback& cb)	const	= 0;

	virtual	const char*			getConcreteTypeName() const	{ return "PxBVH";	}
protected:
	PX_INLINE					PxBVH(PxType concreteType, PxBaseFlags baseFlags) : PxBase(concreteType, baseFlags)	{}
	PX_INLINE					PxBVH(PxBaseFlags baseFlags) : PxBase(baseFlags)									{}
	virtual						~PxBVH()																			{}

	virtual	bool				isKindOf(const char* name) const { return !::strcmp("PxBVH", name) || PxBase::isKindOf(name); }
};

	struct PxGeomIndexPair;

	/**
	\brief BVH-vs-BVH overlap test

	This function returns pairs of box indices that belong to both the first & second input bvhs.

	\param[in] callback			The callback object used to report results
	\param[in] bvh0				First bvh
	\param[in] bvh1				Second bvh
	\return true if an overlap has been detected

	@see PxBVH PxReportCallback
	*/
	PX_C_EXPORT PX_PHYSX_COMMON_API bool PX_CALL_CONV PxFindOverlap(PxReportCallback<PxGeomIndexPair>& callback, const PxBVH& bvh0, const PxBVH& bvh1);

//! @cond
	/**
	 * @deprecated
	 */
	typedef PX_DEPRECATED PxBVH PxBVHStructure;
//! @endcond

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
