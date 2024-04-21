#pragma once
#if COMPILE_WITH_PHYSX
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Physics/PhysicsTransform.h"
#include "Types.h"
#include <PhysX\extensions\PxGjkQueryExt.h>

struct CylinderCallbacks : PxCustomGeometry::Callbacks
{
    DECLARE_CUSTOM_GEOMETRY_TYPE
private:
    static PxConvexMesh* LowPolyCylinder;

    PxReal radius;
    PxReal halfHeight;
public:
    /**
    \brief Constructor, initializes to a capsule with passed radius and half height.
    */
    CylinderCallbacks(PxPhysics* PhysX,PxReal radius_ = 0.0f, PxReal halfHeight_ = 0.0f);


    /**
    \brief Return local bounds.

    \param[in] geometry		This geometry.

    \return  Bounding box in the geometry local space.
    */
    virtual PxBounds3 getLocalBounds(const PxGeometry& geometry) const;

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
        PxContactBuffer& contactBuffer) const;

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
        PxReal maxDist, PxHitFlags hitFlags, PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride, PxRaycastThreadContext* threadContext) const;

    /**
    \brief Overlap. Test if geometries overlap.

    \param[in] geom0			This custom geometry
    \param[in] pose0			This custom geometry pose
    \param[in] geom1			The other geometry
    \param[in] pose1			The other geometry pose
    \param[in] threadContext	Optional user-defined per-thread context.

    \return True if there is overlap. False otherwise.
    */
    virtual bool overlap(const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxOverlapThreadContext* threadContext) const;

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
        PxGeomSweepHit& sweepHit, PxHitFlags hitFlags, const PxReal inflation, PxSweepThreadContext* threadContext) const;

    /**
    \brief Visualize custom geometry for debugging. Optional.

    \param[in] geometry		This geometry.
    \param[in] out			Render output.
    \param[in] absPose		Geometry absolute transform.
    \param[in] cullbox		Region to visualize.
    */
    virtual void visualize(const PxGeometry& geometry, PxRenderOutput& out, const PxTransform& absPose, const PxBounds3& cullbox) const;

    /**
    \brief Compute custom geometry mass properties. For geometries usable with dynamic rigidbodies.

    \param[in] geometry			This geometry.
    \param[out] massProperties	Mass properties to compute.
    */
    virtual void computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const;

    /**
    \brief Compatible with PhysX's PCM feature. Allows to optimize contact generation.

    \param[in] geometry				This geometry.
    \param[out] breakingThreshold	The threshold to trigger contacts re-generation.
    */
    virtual bool usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const;
};
#endif
