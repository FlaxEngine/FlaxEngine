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

#ifndef PX_CUSTOMGEOMETRY_H
#define PX_CUSTOMGEOMETRY_H
/** \addtogroup geomutils
@{
*/

#include "geometry/PxGeometry.h"
#include "geometry/PxGeometryHit.h"
#include "geometry/PxGeometryQueryContext.h"
#include "foundation/PxFoundationConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	class PxContactBuffer;
	class PxRenderOutput;
	class PxMassProperties;

	/**
	\brief Custom geometry class. This class allows user to create custom geometries by providing a set of virtual callback functions.
	*/
	class PxCustomGeometry : public PxGeometry
	{
	public:
		/**
		\brief For internal use
		*/
		PX_PHYSX_COMMON_API static PxU32 getUniqueID();

		/**
		\brief The type of a custom geometry. Allows to identify a particular kind of it.
		*/
		struct Type
		{
			/**
			\brief Default constructor
			*/
			PX_INLINE Type() : mID(getUniqueID()) {}

			/**
			\brief Default constructor
			*/
			PX_INLINE Type(const Type& t) : mID(t.mID) {}

			/**
			\brief Assigment operator
			*/
			PX_INLINE Type& operator = (const Type& t) { mID = t.mID; return *this; }

			/**
			\brief Equality operator
			*/
			PX_INLINE bool operator == (const Type& t) const { return mID == t.mID; }

			/**
			\brief Inequality operator
			*/
			PX_INLINE bool operator != (const Type& t) const { return mID != t.mID; }

			/**
			\brief Invalid type
			*/
			PX_INLINE static Type INVALID() { PxU32 z(0); return reinterpret_cast<const Type&>(z); }

		private:
			PxU32 mID;
		};

		/**
		\brief Custom geometry callbacks structure. User should inherit this and implement all pure virtual functions.
		*/
		struct Callbacks
		{
			/**
			\brief Return custom type. The type purpose is for user to differentiate custom geometries. Not used by PhysX.

			\return  Unique ID of a custom geometry type.

			\note User should use DECLARE_CUSTOM_GEOMETRY_TYPE and IMPLEMENT_CUSTOM_GEOMETRY_TYPE intead of overwriting this function.
			*/
			virtual Type getCustomType() const = 0;

			/**
			\brief Return local bounds.

			\param[in] geometry		This geometry.

			\return  Bounding box in the geometry local space.
			*/
			virtual PxBounds3 getLocalBounds(const PxGeometry& geometry) const = 0;

			/**
			\brief Contacts generation. Generate collision contacts between two geometries in given poses.

			\param[in] geom0				This custom geometry
			\param[in] geom1				The other geometry
			\param[in] pose0				This custom geometry pose
			\param[in] pose1				The other geometry pose
			\param[in] contactDistance		The distance at which contacts begin to be generated between the pairs
			\param[in] meshContactMargin	The mesh contact margin.
			\param[in] toleranceLength		The toleranceLength. Used for scaling distance-based thresholds internally to produce appropriate results given simulations in different units
			\param[out] contactBuffer		A buffer to write contacts to.

			\return True if there are contacts. False otherwise.
			*/
			virtual bool generateContacts(const PxGeometry& geom0, const PxGeometry& geom1, const PxTransform& pose0, const PxTransform& pose1,
				const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength,
				PxContactBuffer& contactBuffer) const = 0;

			/**
			\brief Raycast. Cast a ray against the geometry in given pose.

			\param[in] origin			Origin of the ray.
			\param[in] unitDir			Normalized direction of the ray.
			\param[in] geom				This custom geometry
			\param[in] pose				This custom geometry pose
			\param[in] maxDist			Length of the ray. Has to be in the [0, inf) range.
			\param[in] hitFlags			Specifies which properties per hit should be computed and returned via the hit callback.
			\param[in] maxHits			max number of returned hits = size of 'rayHits' buffer
			\param[out] rayHits			Ray hits.
			\param[in] stride			Ray hit structure stride.
			\param[in] threadContext	Optional user-defined per-thread context.

			\return Number of hits.
			*/
			virtual PxU32 raycast(const PxVec3& origin, const PxVec3& unitDir, const PxGeometry& geom, const PxTransform& pose,
				PxReal maxDist, PxHitFlags hitFlags, PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride, PxRaycastThreadContext* threadContext) const = 0;

			/**
			\brief Overlap. Test if geometries overlap.

			\param[in] geom0			This custom geometry
			\param[in] pose0			This custom geometry pose
			\param[in] geom1			The other geometry
			\param[in] pose1			The other geometry pose
			\param[in] threadContext	Optional user-defined per-thread context.

			\return True if there is overlap. False otherwise.
			*/
			virtual bool overlap(const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxOverlapThreadContext* threadContext) const = 0;

			/**
			\brief Sweep. Sweep one geometry against the other.

			\param[in] unitDir			Normalized direction of the sweep.
			\param[in] maxDist			Length of the sweep. Has to be in the [0, inf) range.
			\param[in] geom0			This custom geometry
			\param[in] pose0			This custom geometry pose
			\param[in] geom1			The other geometry
			\param[in] pose1			The other geometry pose
			\param[out] sweepHit		Used to report the sweep hit.
			\param[in] hitFlags			Specifies which properties per hit should be computed and returned via the hit callback.
			\param[in] inflation		This parameter creates a skin around the swept geometry which increases its extents for sweeping.
			\param[in] threadContext	Optional user-defined per-thread context.

			\return True if there is hit. False otherwise.
			*/
			virtual bool sweep(const PxVec3& unitDir, const PxReal maxDist,
				const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1,
				PxGeomSweepHit& sweepHit, PxHitFlags hitFlags, const PxReal inflation, PxSweepThreadContext* threadContext) const = 0;

			/**
			\brief Visualize custom geometry for debugging. Optional.

			\param[in] geometry		This geometry.
			\param[in] out			Render output.
			\param[in] absPose		Geometry absolute transform.
			\param[in] cullbox		Region to visualize.
			*/
			virtual void visualize(const PxGeometry& geometry, PxRenderOutput& out, const PxTransform& absPose, const PxBounds3& cullbox) const = 0;

			/**
			\brief Compute custom geometry mass properties. For geometries usable with dynamic rigidbodies.

			\param[in] geometry			This geometry.
			\param[out] massProperties	Mass properties to compute.
			*/
			virtual void computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const = 0;

			/**
			\brief Compatible with PhysX's PCM feature. Allows to optimize contact generation.

			\param[in] geometry				This geometry.
			\param[out] breakingThreshold	The threshold to trigger contacts re-generation.
			*/
			virtual bool usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const = 0;

			/* Destructor */
			virtual ~Callbacks() {}
		};

		/**
		\brief Default constructor.

		Creates an empty object with a NULL callbacks pointer.
		*/
		PX_INLINE PxCustomGeometry() :
			PxGeometry(PxGeometryType::eCUSTOM),
			callbacks(NULL)
		{}

		/**
		\brief Constructor.
		*/
		PX_INLINE PxCustomGeometry(Callbacks& _callbacks) :
			PxGeometry(PxGeometryType::eCUSTOM),
			callbacks(&_callbacks)
		{}

		/**
		\brief Copy constructor.

		\param[in] that		Other object
		*/
		PX_INLINE PxCustomGeometry(const PxCustomGeometry& that) :
			PxGeometry(that),
			callbacks(that.callbacks)
		{}

		/**
		\brief Assignment operator
		*/
		PX_INLINE void operator=(const PxCustomGeometry& that)
		{
			mType = that.mType;
			callbacks = that.callbacks;
		}

		/**
		\brief Returns true if the geometry is valid.

		\return  True if the current settings are valid for shape creation.

		@see PxRigidActor::createShape, PxPhysics::createShape
		*/
		PX_INLINE bool isValid() const;

		/**
		\brief Returns the custom type of the custom geometry.
		*/
		PX_INLINE Type getCustomType() const
		{
			return callbacks ? callbacks->getCustomType() : Type::INVALID();
		}

	public:
		Callbacks* callbacks;		//!< A reference to the callbacks object.
	};

	PX_INLINE bool PxCustomGeometry::isValid() const
	{
		return mType == PxGeometryType::eCUSTOM && callbacks != NULL;
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

/**
\brief Used in pair with IMPLEMENT_CUSTOM_GEOMETRY_TYPE to overwrite Callbacks::getCustomType() callback.
*/
#define DECLARE_CUSTOM_GEOMETRY_TYPE								\
	static ::physx::PxCustomGeometry::Type TYPE();					\
	virtual ::physx::PxCustomGeometry::Type getCustomType() const;

/**
\brief Used in pair with DECLARE_CUSTOM_GEOMETRY_TYPE to overwrite Callbacks::getCustomType() callback.
*/
#define IMPLEMENT_CUSTOM_GEOMETRY_TYPE(CLASS)						\
::physx::PxCustomGeometry::Type CLASS::TYPE()						\
{																	\
	static ::physx::PxCustomGeometry::Type customType;				\
	return customType;												\
}																	\
::physx::PxCustomGeometry::Type CLASS::getCustomType() const		\
{																	\
	return TYPE();													\
}

/** @} */
#endif
