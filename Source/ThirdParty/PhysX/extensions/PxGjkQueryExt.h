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

#ifndef PX_GJK_QUERY_EXT_H
#define PX_GJK_QUERY_EXT_H

#include "geometry/PxGjkQuery.h"
#include "geometry/PxGeometry.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxSphereGeometry;
class PxCapsuleGeometry;
class PxBoxGeometry;
class PxConvexMeshGeometry;
class PxContactBuffer;
class PxConvexMesh;

/**
\brief Pre-made support mapping for built-in convex geometry types.
*/
class PxGjkQueryExt
{
public:
	/**
	\brief Pre-made support mapping for a sphere
	*/
	struct SphereSupport : PxGjkQuery::Support
	{
		PxReal radius;

		/**
		\brief Default constructor
		*/
		SphereSupport();
		/**
		\brief Constructs a SphereSupport for a sphere radius
		*/
		SphereSupport(PxReal radius);
		/**
		\brief Constructs a SphereSupport for a PxSphereGeometry
		*/
		SphereSupport(const PxSphereGeometry& geom);

		virtual PxReal getMargin() const;
		virtual PxVec3 supportLocal(const PxVec3& dir) const;
	};

	/**
	\brief Pre-made support mapping for a capsule
	*/
	struct CapsuleSupport : PxGjkQuery::Support
	{
		PxReal radius, halfHeight;

		/**
		\brief Default constructor
		*/
		CapsuleSupport();
		/**
		\brief Constructs a CapsuleSupport for capsule radius and halfHeight
		*/
		CapsuleSupport(PxReal radius, PxReal halfHeight);
		/**
		\brief Constructs a CapsuleSupport for a PxCapsuleGeometry
		*/
		CapsuleSupport(const PxCapsuleGeometry& geom);

		virtual PxReal getMargin() const;
		virtual PxVec3 supportLocal(const PxVec3& dir) const;
	};

	/**
	\brief Pre-made support mapping for a box
	*/
	struct BoxSupport : PxGjkQuery::Support
	{
		PxVec3 halfExtents;
		PxReal margin;

		/**
		\brief Default constructor
		*/
		BoxSupport();
		/**
		\brief Constructs a BoxSupport for a box halfExtents with optional margin
		*/
		BoxSupport(const PxVec3& halfExtents, PxReal margin = 0);
		/**
		\brief Constructs a BoxSupport for a PxBoxGeometry
		*/
		BoxSupport(const PxBoxGeometry& box, PxReal margin = 0);

		virtual PxReal getMargin() const;
		virtual PxVec3 supportLocal(const PxVec3& dir) const;
	};

	/**
	\brief Pre-made support mapping for a convex mesh
	*/
	struct ConvexMeshSupport : PxGjkQuery::Support
	{
		const PxConvexMesh* convexMesh;
		PxVec3 scale;
		PxQuat scaleRotation;
		PxReal margin;

		/**
		\brief Default constructor
		*/
		ConvexMeshSupport();
		/**
		\brief Constructs a BoxSupport for a PxConvexMesh
		*/
		ConvexMeshSupport(const PxConvexMesh& convexMesh, const PxVec3& scale = PxVec3(1), const PxQuat& scaleRotation = PxQuat(PxIdentity), PxReal margin = 0);
		/**
		\brief Constructs a BoxSupport for a PxConvexMeshGeometry
		*/
		ConvexMeshSupport(const PxConvexMeshGeometry& convexMesh, PxReal margin = 0);

		virtual PxReal getMargin() const;
		virtual PxVec3 supportLocal(const PxVec3& dir) const;
	};

	/**
	\brief Pre-made support mapping for any PhysX's convex geometry (sphere, capsule, box, convex mesh)
	*/
	struct ConvexGeomSupport : PxGjkQuery::Support
	{
		/**
		\brief Default constructor
		*/
		ConvexGeomSupport();
		/**
		\brief Constructs a BoxSupport for a PxGeometry
		*/
		ConvexGeomSupport(const PxGeometry& geom, PxReal margin = 0);
		/**
		\brief Destructor
		*/
		~ConvexGeomSupport();

		/**
		\brief Returns false if ConvexGeomSupport was constructed from non-convex geometry
		*/
		bool isValid() const;

		virtual PxReal getMargin() const;
		virtual PxVec3 supportLocal(const PxVec3& dir) const;

	private:
		PxGeometryType::Enum mType;
		union {
			void* alignment;
			PxU8 sphere[sizeof(SphereSupport)];
			PxU8 capsule[sizeof(CapsuleSupport)];
			PxU8 box[sizeof(BoxSupport)];
			PxU8 convexMesh[sizeof(ConvexMeshSupport)];
		} mSupport;
	};

	/**
	\brief Generates a contact point between two shapes using GJK-EPA algorithm

	\param[in] a				Shape A support mapping
	\param[in] b				Shape B support mapping
	\param[in] poseA			Shape A transformation
	\param[in] poseB			Shape B transformation
	\param[in] contactDistance	The distance at which contacts begin to be generated between the shapes
	\param[in] toleranceLength	The toleranceLength. Used for scaling distance-based thresholds internally to produce appropriate results given simulations in different units
	\param[out] contactBuffer	A buffer to store the contact

	\return						True if there is a contact.
	*/
	static bool generateContacts(const PxGjkQuery::Support& a, const PxGjkQuery::Support& b, const PxTransform& poseA, const PxTransform& poseB,
		PxReal contactDistance, PxReal toleranceLength, PxContactBuffer& contactBuffer);

};

#if !PX_DOXYGEN
}
#endif

#endif
