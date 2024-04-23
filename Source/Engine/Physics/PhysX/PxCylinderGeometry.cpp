#if COMPILE_WITH_PHYSX
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Ray.h"
#include <Engine/Physics/CollisionCooking.h>
#include <Engine/Core/Log.h>

#include "PxCylinderGeometry.h"
#include <PhysX\PxPhysics.h>
#include <extensions/PxMassProperties.h>
#include <extensions/PxCustomGeometryExt.h>
#include <geomutils/PxContactBuffer.h>
#include <collision/PxCollisionDefs.h>
#include <PxImmediateMode.h>
#include <PhysX\extensions\PxDefaultStreams.h>

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

#define pointsLenght 130
static Float3 Cylinder32Points[pointsLenght] = 
{
   Float3(0,-0.5000003,1.0000002),
   Float3(0,0.50000006,1.0000005),
   Float3(0.19509041,0.50000006,0.98078567),
   Float3(0.19509041,-0.5000003,0.98078555),
   Float3(0.38268363,0.50000006,0.9238799),
   Float3(0.38268363,-0.5000003,0.9238798),
   Float3(0.5555705,0.50000006,0.83146995),
   Float3(0.5555705,-0.5000003,0.83146983),
   Float3(0.7071071,0.5000001,0.70710707),
   Float3(0.7071071,-0.50000024,0.70710695),
   Float3(0.83147,0.5000001,0.5555705),
   Float3(0.83147,-0.50000024,0.55557036),
   Float3(0.9238799,0.5000001,0.38268366),
   Float3(0.9238799,-0.50000024,0.38268355),
   Float3(0.9807857,0.5000002,0.19509046),
   Float3(0.9807857,-0.5000002,0.19509034),
   Float3(1.0000005,0.5000002,5.960467e-08),
   Float3(1.0000005,-0.5000002,-5.960467e-08),
   Float3(0.9807857,0.5000002,-0.19509034),
   Float3(0.9807857,-0.5000002,-0.19509046),
   Float3(0.9238799,0.50000024,-0.38268355),
   Float3(0.9238799,-0.5000001,-0.38268366),
   Float3(0.83147,0.50000024,-0.55557036),
   Float3(0.83147,-0.5000001,-0.5555705),
   Float3(0.7071071,0.50000024,-0.70710695),
   Float3(0.7071071,-0.5000001,-0.70710707),
   Float3(0.5555705,0.5000003,-0.83146983),
   Float3(0.5555705,-0.50000006,-0.83146995),
   Float3(0.38268363,0.5000003,-0.9238798),
   Float3(0.38268363,-0.50000006,-0.9238799),
   Float3(0.19509041,0.5000003,-0.98078555),
   Float3(0.19509041,-0.50000006,-0.98078567),
   Float3(0,0.5000003,-1.0000002),
   Float3(0,-0.50000006,-1.0000005),
   Float3(-0.19509041,0.5000003,-0.98078555),
   Float3(-0.19509041,-0.50000006,-0.98078567),
   Float3(-0.38268363,0.5000003,-0.9238798),
   Float3(-0.38268363,-0.50000006,-0.9238799),
   Float3(-0.5555705,0.5000003,-0.83146983),
   Float3(-0.5555705,-0.50000006,-0.83146995),
   Float3(-0.7071071,0.50000024,-0.70710695),
   Float3(-0.7071071,-0.5000001,-0.70710707),
   Float3(-0.83147,0.50000024,-0.55557036),
   Float3(-0.83147,-0.5000001,-0.5555705),
   Float3(-0.9238799,0.50000024,-0.38268355),
   Float3(-0.9238799,-0.5000001,-0.38268366),
   Float3(-0.9807857,0.5000002,-0.19509034),
   Float3(-0.9807857,-0.5000002,-0.19509046),
   Float3(-1.0000005,0.5000002,5.960467e-08),
   Float3(-1.0000005,-0.5000002,-5.960467e-08),
   Float3(-0.9807857,0.5000002,0.19509046),
   Float3(-0.9807857,-0.5000002,0.19509034),
   Float3(-0.9238799,0.5000001,0.38268366),
   Float3(-0.9238799,-0.50000024,0.38268355),
   Float3(-0.83147,0.5000001,0.5555705),
   Float3(-0.83147,-0.50000024,0.55557036),
   Float3(-0.7071071,0.5000001,0.70710707),
   Float3(-0.7071071,-0.50000024,0.70710695),
   Float3(-0.5555705,0.50000006,0.83146995),
   Float3(-0.5555705,-0.5000003,0.83146983),
   Float3(-0.38268363,0.50000006,0.9238799),
   Float3(-0.38268363,-0.5000003,0.9238798),
   Float3(0.19509041,0.50000006,0.98078567),
   Float3(0,0.50000006,1.0000005),
   Float3(-0.19509041,0.50000006,0.98078567),
   Float3(-0.38268363,0.50000006,0.9238799),
   Float3(-0.5555705,0.50000006,0.83146995),
   Float3(-0.7071071,0.5000001,0.70710707),
   Float3(-0.83147,0.5000001,0.5555705),
   Float3(-0.9238799,0.5000001,0.38268366),
   Float3(-0.9807857,0.5000002,0.19509046),
   Float3(-1.0000005,0.5000002,5.960467e-08),
   Float3(-0.9807857,0.5000002,-0.19509034),
   Float3(-0.9238799,0.50000024,-0.38268355),
   Float3(-0.83147,0.50000024,-0.55557036),
   Float3(-0.7071071,0.50000024,-0.70710695),
   Float3(-0.5555705,0.5000003,-0.83146983),
   Float3(-0.38268363,0.5000003,-0.9238798),
   Float3(-0.19509041,0.5000003,-0.98078555),
   Float3(0,0.5000003,-1.0000002),
   Float3(0.19509041,0.5000003,-0.98078555),
   Float3(0.38268363,0.5000003,-0.9238798),
   Float3(0.5555705,0.5000003,-0.83146983),
   Float3(0.7071071,0.50000024,-0.70710695),
   Float3(0.83147,0.50000024,-0.55557036),
   Float3(0.9238799,0.50000024,-0.38268355),
   Float3(0.9807857,0.5000002,-0.19509034),
   Float3(1.0000005,0.5000002,5.960467e-08),
   Float3(0.9807857,0.5000002,0.19509046),
   Float3(0.9238799,0.5000001,0.38268366),
   Float3(0.83147,0.5000001,0.5555705),
   Float3(0.7071071,0.5000001,0.70710707),
   Float3(0.5555705,0.50000006,0.83146995),
   Float3(0.38268363,0.50000006,0.9238799),
   Float3(-0.19509041,0.50000006,0.98078567),
   Float3(-0.19509041,-0.5000003,0.98078555),
   Float3(0,0.50000006,1.0000005),
   Float3(0,-0.5000003,1.0000002),
   Float3(0,-0.5000003,1.0000002),
   Float3(0.19509041,-0.5000003,0.98078555),
   Float3(0.38268363,-0.5000003,0.9238798),
   Float3(0.5555705,-0.5000003,0.83146983),
   Float3(0.7071071,-0.50000024,0.70710695),
   Float3(0.83147,-0.50000024,0.55557036),
   Float3(0.9238799,-0.50000024,0.38268355),
   Float3(0.9807857,-0.5000002,0.19509034),
   Float3(1.0000005,-0.5000002,-5.960467e-08),
   Float3(0.9807857,-0.5000002,-0.19509046),
   Float3(0.9238799,-0.5000001,-0.38268366),
   Float3(0.83147,-0.5000001,-0.5555705),
   Float3(0.7071071,-0.5000001,-0.70710707),
   Float3(0.5555705,-0.50000006,-0.83146995),
   Float3(0.38268363,-0.50000006,-0.9238799),
   Float3(0.19509041,-0.50000006,-0.98078567),
   Float3(0,-0.50000006,-1.0000005),
   Float3(-0.19509041,-0.50000006,-0.98078567),
   Float3(-0.38268363,-0.50000006,-0.9238799),
   Float3(-0.5555705,-0.50000006,-0.83146995),
   Float3(-0.7071071,-0.5000001,-0.70710707),
   Float3(-0.83147,-0.5000001,-0.5555705),
   Float3(-0.9238799,-0.5000001,-0.38268366),
   Float3(-0.9807857,-0.5000002,-0.19509046),
   Float3(-1.0000005,-0.5000002,-5.960467e-08),
   Float3(-0.9807857,-0.5000002,0.19509034),
   Float3(-0.9238799,-0.50000024,0.38268355),
   Float3(-0.83147,-0.50000024,0.55557036),
   Float3(-0.7071071,-0.50000024,0.70710695),
   Float3(-0.5555705,-0.5000003,0.83146983),
   Float3(-0.38268363,-0.5000003,0.9238798),
   Float3(-0.19509041,-0.5000003,0.98078555),
};

PxConvexMesh* CylinderCallbacks::LowPolyCylinder = nullptr;

CylinderCallbacks::CylinderCallbacks(PxPhysics* PhysX, PxReal radius_, PxReal halfHeight_) : radius(radius_), halfHeight(halfHeight_)
{
    if (LowPolyCylinder != nullptr)
        return;
    BytesContainer bcout = {};
    CollisionCooking::CookingInput data = {};
    data.ConvexFlags = ConvexMeshGenerationFlags::None;
    //set the data
    data.VertexData = &Cylinder32Points[0];//&[0] grabs the ptr at begining of the array
    data.VertexCount = pointsLenght - 1;
    if (!CollisionCooking::CookConvexMesh(data, bcout))
    {
        auto& stream = PxDefaultMemoryInputData(bcout.Get(), bcout.Length());
        LowPolyCylinder = PhysX->createConvexMesh(stream);
    }
#if 0 // data extractor

        auto arr = Array<byte>((byte*)bcout.Get(), bcout.Length());
        String out = String::Format(L"\n static PxU8* data points [{0}] = ", arr.Count());
        out.Append('{');
        size_t j = 0;

        for (size_t i = 0; i < arr.Count(); i++)
        {
            auto d = arr[i];
            out.Append(String::Format(L"{0},", d));
            if (j == 100) {
                j = 0;
                out.Append(String::Format(L"\n", d));
            }
            else
            {
                j++;
            }
        }
        LOG_STR(Info, out.ToString());
#endif
}

PxBounds3 CylinderCallbacks::getLocalBounds(const PxGeometry& geometry) const
{
    auto r = radius + 1;
    auto h = halfHeight + 1;
    PxVec3 min{ -r,-h,-r };
    PxVec3 max{ r,h,r };
    return PxBounds3(min, max);
}

bool CylinderCallbacks::generateContacts(const PxGeometry& geom0, const PxGeometry& geom1, const PxTransform& pose0, const PxTransform& pose1, const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength, PxContactBuffer& contactBuffer) const
{
    if (!LowPolyCylinder)
        return false;

    PxConvexMeshGeometry Geom (LowPolyCylinder);
    Geom.scale.scale = PxVec3(radius, halfHeight, radius);

    PxGeometry* pGeom0 = &Geom;

    const PxGeometry* pGeom1 = &geom1;

    PxContactBuffer lcontactBuffer{};
    ContactRecorder contactRecorder(lcontactBuffer);
    PxCache contactCache;
    ContactCacheAllocator contactCacheAllocator;
    
    bool out = immediate::PxGenerateContacts(&pGeom0, &pGeom1, &pose0, &pose1, &contactCache, 1, contactRecorder,
        contactDistance, meshContactMargin, toleranceLength, contactCacheAllocator);
    if (out)
    {
        for (PxU32 i = 0; i < lcontactBuffer.count; i++)
        {
            PxVec3 v = pose0.transformInv(lcontactBuffer.contacts[i].point);
            PxVec3 fv = v;
            PxReal separation = lcontactBuffer.contacts[i].separation;
            if (v.y > -halfHeight && v.y < halfHeight)
            {
            PxVec3 XZ = PxVec3(v.x, 0, v.z);
            v = XZ.getNormalized() * radius;
            separation -= (v - XZ).magnitude();
            v.y = fv.y;
            }
            contactBuffer.contact(pose0.transform(v), lcontactBuffer.contacts[i].normal, separation);
        }
    }
    //for (PxU32 i = 0; i < contactBuffer.count; i++)
    //{
    //    DEBUG_DRAW_SPHERE(BoundingSphere(P2C(contactBuffer.contacts[i].point), Real(1)), Color::Black, 1.0f / 30.0f, true);
    //    DEBUG_DRAW_RAY(Ray(P2C(contactBuffer.contacts[i].point), P2C(contactBuffer.contacts[i].normal)), Color::Blue, contactBuffer.contacts[i].separation * 10.0f, 1.0f / 30.0f, true);
    //}
    return out;
}

PxU32 CylinderCallbacks::raycast(const PxVec3& origin, const PxVec3& unitDir, const PxGeometry& geom, const PxTransform& pose, PxReal maxDist, PxHitFlags hitFlags, PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride, PxRaycastThreadContext* threadContext) const
{
    //[todo]
    //refrence
    //https://github.com/NVIDIA-Omniverse/PhysX/blob/main/physx/source/physxextensions/src/ExtCustomGeometryExt.cpp


    // When FLT_MAX is used as maxDist, it works bad with GJK algorithm.
    // Here I compute the maximum needed distance (wiseDist) as the diagonal
    // of the bounding box of both the geometry and the ray origin.

    //PxBounds3 bounds = PxBounds3::transformFast(pose, getLocalBounds(geom));
    //bounds.include(origin);
    //PxReal wiseDist = PxMin(maxDist, bounds.getDimensions().magnitude());
    //PxReal t;
    //PxVec3 n, p;
    //if (PxGjkQuery::raycast(*this, pose, origin, unitDir, wiseDist, t, n, p))
    //{
    //    PxGeomRaycastHit& hit = *rayHits;
    //    hit.distance = t;
    //    hit.position = p;
    //    hit.normal = n;
    //    return 1;
    //}



    return 0;
}

bool CylinderCallbacks::overlap(const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxOverlapThreadContext* threadContext) const
{
    //[todo]
    //refrence
    //https://github.com/NVIDIA-Omniverse/PhysX/blob/main/physx/source/physxextensions/src/ExtCustomGeometryExt.cpp

    //PxGjkQueryExt::ConvexGeomSupport geomSupport(geom1);
    //return PxGjkQuery::overlap(*this, geomSupport, pose0, pose1);

    return false;
}

bool CylinderCallbacks::sweep(const PxVec3& unitDir, const PxReal maxDist, const PxGeometry& geom0, const PxTransform& pose0, const PxGeometry& geom1, const PxTransform& pose1, PxGeomSweepHit& sweepHit, PxHitFlags hitFlags, const PxReal inflation, PxSweepThreadContext* threadContext) const
{
    //[todo]
    //refrence
    //https://github.com/NVIDIA-Omniverse/PhysX/blob/main/physx/source/physxextensions/src/ExtCustomGeometryExt.cpp

    return false;
}

void CylinderCallbacks::visualize(const PxGeometry& geometry, PxRenderOutput& out, const PxTransform& absPose, const PxBounds3& cullbox) const
{
}

void CylinderCallbacks::computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const
{
    float H = (halfHeight * 2.0f), R = radius;
    float Rsqere = R * R;
    massProperties.mass = PxPi * Rsqere * H;
    massProperties.inertiaTensor = PxMat33(PxZero);
    massProperties.centerOfMass = PxVec3(PxZero);
    massProperties.inertiaTensor[0][0] = massProperties.mass * Rsqere / 2.0f;
    massProperties.inertiaTensor[1][1] = massProperties.inertiaTensor[2][2] = massProperties.mass * (3 * Rsqere + H * H) / 12.0f;
}

bool CylinderCallbacks::usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const
{
    breakingThreshold = FLT_EPSILON;
    return false;
}
#endif
