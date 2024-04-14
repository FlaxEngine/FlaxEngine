#if COMPILE_WITH_PHYSX
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Ray.h"

#include "PxCylinderGeometry.h"
#include <extensions/PxMassProperties.h>
#include <extensions/PxCustomGeometryExt.h>
#include <geomutils/PxContactBuffer.h>
#include <collision/PxCollisionDefs.h>
#include <PxImmediateMode.h>

struct ContactRecorder : immediate::PxContactRecorder
{
    PxContactBuffer* contactBuffer;
    ContactRecorder(PxContactBuffer& _contactBuffer) : contactBuffer(&_contactBuffer) {}
    virtual bool recordContacts(const PxContactPoint* contactPoints, PxU32 nbContacts, PxU32 /*index*/)
    {
        for (PxU32 i = 0; i < nbContacts; ++i)
            if (!contactBuffer->contact(contactPoints[i]))
                return false;
        return true;
    }
};
struct ContactCacheAllocator : PxCacheAllocator
{
    PxU8 buffer[1024];
    ContactCacheAllocator() { memset(buffer, 0, sizeof(buffer)); }
    virtual PxU8* allocateCacheData(const PxU32 /*byteSize*/) { return (PxU8*)(size_t(buffer + 0xf) & ~0xf); }
};

IMPLEMENT_CUSTOM_GEOMETRY_TYPE(CylinderCallbacks)

PxBounds3 CylinderCallbacks::getLocalBounds(const PxGeometry& geometry) const
{
    PxVec3 min{ -radius,-halfHeight,-radius };
    PxVec3 max{ radius,halfHeight,radius };
    return PxBounds3(min, max);
}

bool CylinderCallbacks::generateContacts(const PxGeometry& geom0, const PxGeometry& geom1, const PxTransform& pose0, const PxTransform& pose1, const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength, PxContactBuffer& contactBuffer) const
{
    auto type0 = geom0.getType();
    //pxpoint
    PxCapsuleGeometry Geom(radius, halfHeight);
    PxGeometry* pGeom0 = &Geom;

    const PxGeometry* pGeom1 = &geom1;

    PxContactBuffer lcontactBuffer{};
    ContactRecorder contactRecorder(lcontactBuffer);
    PxCache contactCache;
    ContactCacheAllocator contactCacheAllocator;

    auto T = PxTransform(PxVec3(0, 0, 0), PxQuat(0, 0, 0.707107, 0.707107));

    PxTransform p0 = pose0.transform(T);

    if (immediate::PxGenerateContacts(&pGeom0, &pGeom1, &p0, &pose1, &contactCache, 1, contactRecorder,
        contactDistance, meshContactMargin, toleranceLength, contactCacheAllocator))
    {
        for (PxU32 i = 0; i < lcontactBuffer.count; i++)
        {
            PxVec3 v = pose0.transformInv(lcontactBuffer.contacts[i].point);
            PxVec3 fv = v;
            PxReal separation = lcontactBuffer.contacts[i].separation;
            if(v.y < -halfHeight)
            {
                v.y = -halfHeight;
                separation += (v - fv).y;
            }
            if (v.y > halfHeight)
            {
                v.y = halfHeight;
                separation -= (v - fv).y;
            }
            auto fp = pose0.transform(v);
            contactBuffer.contact(pose0.transform(v), lcontactBuffer.contacts[i].normal, separation);
        }



        for (PxU32 i = 0; i < contactBuffer.count; i++)
        {
            
            DEBUG_DRAW_SPHERE(BoundingSphere(P2C(contactBuffer.contacts[i].point), Real(1)), Color::Black,1.0f / 30.0f,true);
            DEBUG_DRAW_RAY(Ray(P2C(contactBuffer.contacts[i].point), P2C(contactBuffer.contacts[i].normal)), Color::Blue,10.f, 1.0f / 30.0f, true);
        }
        return true;
        //contactRecorder.contactBuffer->contacts
    }

    //contactRecorder.contactBuffer->contact(p0.p, PxVec3(0, 1, 0), PxReal(10));
    return false;
}

PxU32 CylinderCallbacks::raycast(const PxVec3& origin, const PxVec3& unitDir, const PxGeometry& geom, const PxTransform& pose, PxReal maxDist, PxHitFlags hitFlags, PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride, PxRaycastThreadContext* threadContext) const
{
    return PxU32();
}

bool CylinderCallbacks::overlap(const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxOverlapThreadContext* threadContext) const
{
    return false;
}

bool CylinderCallbacks::sweep(const PxVec3& unitDir, const PxReal maxDist, const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxGeomSweepHit& sweepHit, PxHitFlags hitFlags, const PxReal inflation, PxSweepThreadContext* threadContext) const
{
    return false;
}

void CylinderCallbacks::visualize(const PxGeometry& geometry, PxRenderOutput& out, const PxTransform& absPose, const PxBounds3& cullbox) const
{
}

void CylinderCallbacks::computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const
{
    float H = (halfHeight * 2.0), R = radius;
    float Rsqere = R * R;
    massProperties.mass = PxPi * Rsqere * H;
    massProperties.inertiaTensor = PxMat33(PxZero);
    massProperties.centerOfMass = PxVec3(PxZero);
    massProperties.inertiaTensor[0][0] = massProperties.mass * Rsqere / 2.0f;
    massProperties.inertiaTensor[1][1] = massProperties.inertiaTensor[2][2] = massProperties.mass * (3 * Rsqere + H * H) / 12.0f;
}

bool CylinderCallbacks::usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const
{
    return false;
}
#endif
