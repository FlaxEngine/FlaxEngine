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

#ifndef PX_CUSTOM_GEOMETRY_EXT_H
#define PX_CUSTOM_GEOMETRY_EXT_H
/** \addtogroup extensions
  @{
*/

#include <geometry/PxCustomGeometry.h>
#include <geometry/PxGjkQuery.h>

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxGeometry;
class PxMassProperties;
class PxGeometryHolder;
struct PxContactPoint;

/**
\brief Pre-made custom geometry callbacks implementations.
*/
class PxCustomGeometryExt
{
public:

	/// \cond PRIVATE
	struct BaseConvexCallbacks : PxCustomGeometry::Callbacks, PxGjkQuery::Support
	{
		float margin;

		BaseConvexCallbacks(float _margin) : margin(_margin) {}

		// override PxCustomGeometry::Callbacks
		virtual PxBounds3 getLocalBounds(const PxGeometry& geometry) const;
		virtual bool generateContacts(const PxGeometry& geom0, const PxGeometry& geom1, const PxTransform& pose0, const PxTransform& pose1,
			const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength,
			PxContactBuffer& contactBuffer) const;
		virtual PxU32 raycast(const PxVec3& origin, const PxVec3& unitDir, const PxGeometry& geom, const PxTransform& pose,
			PxReal maxDist, PxHitFlags hitFlags, PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride, PxRaycastThreadContext*) const;
		virtual bool overlap(const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxOverlapThreadContext*) const;
		virtual bool sweep(const PxVec3& unitDir, const PxReal maxDist,
			const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1,
			PxGeomSweepHit& sweepHit, PxHitFlags hitFlags, const PxReal inflation, PxSweepThreadContext*) const;
		virtual bool usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const;

		// override PxGjkQuery::Support
		virtual PxReal getMargin() const { return margin; }

	protected:

		// Substitute geometry
		virtual bool useSubstituteGeometry(PxGeometryHolder& geom, PxTransform& preTransform, const PxContactPoint& p, const PxTransform& pose0, const PxVec3& pos1) const = 0;
	};
	/// \endcond

	/**
	\brief Cylinder geometry callbacks
	*/
	struct CylinderCallbacks : BaseConvexCallbacks
	{
		/// \brief Cylinder height
		float height;
		/// \brief Cylinder radius
		float radius;
		/// \brief Cylinder axis
		int axis;

		/**
		\brief Construct cylinder geometry callbacks object

		\param[in] height The cylinder height.
		\param[in] radius The cylinder radius.
		\param[in] axis The cylinder axis (0 - X, 1 - Y, 2 - Z).
		\param[in] margin The cylinder margin.
		*/
		CylinderCallbacks(float height, float radius, int axis = 0, float margin = 0);

		/// \cond PRIVATE
		// override PxCustomGeometry::Callbacks
		DECLARE_CUSTOM_GEOMETRY_TYPE
		virtual void visualize(const PxGeometry&, PxRenderOutput&, const PxTransform&, const PxBounds3&) const;
		virtual void computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const;

		// override PxGjkQuery::Support
		virtual PxVec3 supportLocal(const PxVec3& dir) const;

	protected:

		// Substitute geometry
		virtual bool useSubstituteGeometry(PxGeometryHolder& geom, PxTransform& preTransform, const PxContactPoint& p, const PxTransform& pose0, const PxVec3& pos1) const;

		// Radius at height
		float getRadiusAtHeight(float height) const;
		/// \endcond
	};

	/**
	\brief Cone geometry callbacks
	*/
	struct ConeCallbacks : BaseConvexCallbacks
	{
		/// \brief Cone height
		float height;
		/// \brief Cone radius
		float radius;
		/// \brief Cone axis
		int axis;

		/**
		\brief Construct cone geometry callbacks object

		\param[in] height The cylinder height.
		\param[in] radius The cylinder radius.
		\param[in] axis The cylinder axis (0 - X, 1 - Y, 2 - Z).
		\param[in] margin The cylinder margin.
		*/
		ConeCallbacks(float height, float radius, int axis = 0, float margin = 0);

		/// \cond PRIVATE
		// override PxCustomGeometry::Callbacks
		DECLARE_CUSTOM_GEOMETRY_TYPE
		virtual void visualize(const PxGeometry&, PxRenderOutput&, const PxTransform&, const PxBounds3&) const;
		virtual void computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const;

		// override PxGjkQuery::Support
		virtual PxVec3 supportLocal(const PxVec3& dir) const;

	protected:

		// Substitute geometry
		virtual bool useSubstituteGeometry(PxGeometryHolder& geom, PxTransform& preTransform, const PxContactPoint& p, const PxTransform& pose0, const PxVec3& pos1) const;

		// Radius at height
		float getRadiusAtHeight(float height) const;
		/// \endcond
	};
};

#if !PX_DOXYGEN
}
#endif

/** @} */
#endif
