// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PHYSX

#include "PhysicsBackendPhysX.h"
#include "PhysicsStepperPhysX.h"
#include "SimulationEventCallbackPhysX.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsStatistics.h"
#include "Engine/Physics/CollisionCooking.h"
#include "Engine/Physics/Actors/IPhysicsActor.h"
#include "Engine/Physics/Joints/Limits.h"
#include "Engine/Physics/Joints/DistanceJoint.h"
#include "Engine/Physics/Joints/HingeJoint.h"
#include "Engine/Physics/Joints/SliderJoint.h"
#include "Engine/Physics/Joints/SphericalJoint.h"
#include "Engine/Physics/Joints/D6Joint.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/WriteStream.h"
#include <ThirdParty/PhysX/PxPhysicsAPI.h>
#include <ThirdParty/PhysX/PxQueryFiltering.h>
#include <ThirdParty/PhysX/extensions/PxFixedJoint.h>
#include <ThirdParty/PhysX/extensions/PxSphericalJoint.h>
#if WITH_VEHICLE
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Physics/Actors/WheeledVehicle.h"
#include <ThirdParty/PhysX/vehicle/PxVehicleSDK.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUpdate.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleNoDrive.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDrive4W.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDriveNW.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDriveTank.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUtilSetup.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleComponents.h>
#include <ThirdParty/PhysX/PxFiltering.h>
#endif
#if WITH_CLOTH
#include "Engine/Physics/Actors/Cloth.h"
#include "Engine/Threading/JobSystem.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/NvCloth/Callbacks.h>
#include <ThirdParty/NvCloth/Factory.h>
#include <ThirdParty/NvCloth/Cloth.h>
#include <ThirdParty/NvCloth/Fabric.h>
#include <ThirdParty/NvCloth/Solver.h>
#include <ThirdParty/NvCloth/NvClothExt/ClothFabricCooker.h>
#define MAX_CLOTH_SPHERE_COUNT 32
#define MAX_CLOTH_PLANE_COUNT 32
#define CLOTH_COLLISIONS_UPDATE_RATE 10 // Frames between cloth collisions updates
#endif
#if WITH_PVD
#include <ThirdParty/PhysX/pvd/PxPvd.h>
#endif

// Temporary memory size used by the PhysX during the simulation. Must be multiply of 4kB and 16bit aligned.
#define PHYSX_SCRATCH_BLOCK_SIZE (1024 * 128)

// Enables vehicles simulation debugging
#define PHYSX_VEHICLE_DEBUG_TELEMETRY 0

// Enables using debug naming for PhysX objects (eg. for debugging)
#define PHYSX_DEBUG_NAMING 0

// Temporary result buffer size
#define PHYSX_HIT_BUFFER_SIZE 128

struct ActionDataPhysX
{
    PhysicsBackend::ActionType Type;
    PxActor* Actor;
};

struct ScenePhysX
{
    PxScene* Scene = nullptr;
    PxCpuDispatcher* CpuDispatcher = nullptr;
    PxControllerManager* ControllerManager = nullptr;
    void* ScratchMemory = nullptr;
    Vector3 Origin = Vector3::Zero;
    float LastDeltaTime = 0.0f;
    FixedStepper Stepper;
    SimulationEventCallback EventsCallback;
    Array<PxActor*> RemoveActors;
    Array<PhysicsColliderActor*> RemoveColliders;
    Array<Joint*> RemoveJoints;
    Array<ActionDataPhysX> Actions;
#if WITH_VEHICLE
    Array<WheeledVehicle*> WheelVehicles;
    PxBatchQueryExt* WheelRaycastBatchQuery = nullptr;
    int32 WheelRaycastBatchQuerySize = 0;
#endif
#if WITH_CLOTH
    nv::cloth::Solver* ClothSolver = nullptr;
    Array<nv::cloth::Cloth*> ClothsList;
#endif

#if WITH_VEHICLE
    void UpdateVehicles(float dt);
#endif
#if WITH_CLOTH
    void PreSimulateCloth(int32 i);
    void SimulateCloth(int32 i);
    void UpdateCloths(float dt);
#endif
};

class AllocatorPhysX : public PxAllocatorCallback
{
    void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
        ASSERT(size < 1024 * 1024 * 1024); // Prevent invalid allocation size
        return Allocator::Allocate(size, 16);
    }

    void deallocate(void* ptr) override
    {
        Allocator::Free(ptr);
    }
};

class ErrorPhysX : public PxErrorCallback
{
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        LOG(Error, "PhysX Error! Code: {0}.\n{1}\nSource: {2} : {3}.", static_cast<int32>(code), String(message), String(file), line);
    }
};

#if WITH_CLOTH

class AssertPhysX : public nv::cloth::PxAssertHandler
{
public:
    void operator()(const char* exp, const char* file, int line, bool& ignore) override
    {
        Platform::Error(String(exp));
        Platform::Crash(line, file);
    }
};

class ProfilerPhysX : public PxProfilerCallback
{
public:
    void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override
    {
        return nullptr;
    }

    void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override
    {
    }
};

struct FabricSettings
{
    int32 Refs;
    nv::cloth::Vector<int32_t>::Type PhraseTypes;
    PhysicsClothDesc Desc;
    Array<byte> InvMasses;

    bool MatchesDesc(const PhysicsClothDesc& r) const
    {
        if (Desc.VerticesData == r.VerticesData &&
            Desc.VerticesStride == r.VerticesStride &&
            Desc.VerticesCount == r.VerticesCount &&
            Desc.IndicesData == r.IndicesData &&
            Desc.IndicesStride == r.IndicesStride &&
            Desc.IndicesCount == r.IndicesCount &&
            Desc.InvMassesStride == r.InvMassesStride)
        {
            if (Desc.InvMassesData && r.InvMassesData)
                return Platform::MemoryCompare(InvMasses.Get(), r.InvMassesData, r.VerticesCount * r.InvMassesStride) == 0;
            return !Desc.InvMassesData && !r.InvMassesData;
        }
        return false;
    }
};

struct ClothSettings
{
    bool Culled = false;
    bool SceneCollisions = false;
    bool CollisionsUpdateFramesRandomize = true;
    byte CollisionsUpdateFramesLeft = 0;
    float GravityScale = 1.0f;
    float CollisionThickness = 0.0f;
    Cloth* Actor;

    bool UpdateBounds(const nv::cloth::Cloth* clothPhysX) const
    {
        // Get cloth particles bounds (in local-space)
        const PxVec3& clothBoundsPos = clothPhysX->getBoundingBoxCenter();
        const PxVec3& clothBoundsSize = clothPhysX->getBoundingBoxScale();
        BoundingBox localBounds;
        BoundingBox::FromPoints(P2C(clothBoundsPos - clothBoundsSize), P2C(clothBoundsPos + clothBoundsSize), localBounds);

        // Automatic cloth reset when simulation fails (eg. ends with NaN)
        if (localBounds.Minimum.IsNanOrInfinity() || localBounds.Maximum.IsNanOrInfinity())
            return true;

        // Transform local-space bounds into world-space
        const PxTransform clothPose(clothPhysX->getTranslation(), clothPhysX->getRotation());
        const Transform clothTrans(P2C(clothPose.p), P2C(clothPose.q));
        Vector3 boundsCorners[8];
        localBounds.GetCorners(boundsCorners);
        for (Vector3& c : boundsCorners)
            clothTrans.LocalToWorld(c, c);

        // Setup bounds
        BoundingBox::FromPoints(boundsCorners, 8, const_cast<BoundingBox&>(Actor->GetBox()));
        BoundingSphere::FromBox(Actor->GetBox(), const_cast<BoundingSphere&>(Actor->GetSphere()));
        return false;
    }
};

#endif

class QueryFilterPhysX : public PxQueryFilterCallback
{
    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // Early out to avoid crashing
        if (!shape)
            return PxQueryHitType::eNONE;

        // Check mask
        const PxFilterData shapeFilter = shape->getQueryFilterData();
        if ((filterData.word0 & shapeFilter.word0) == 0)
            return PxQueryHitType::eNONE;

        // Check if skip triggers
        const bool hitTriggers = filterData.word2 != 0;
        if (!hitTriggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
            return PxQueryHitType::eNONE;

        const bool blockSingle = filterData.word1 != 0;
        return blockSingle ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
    {
        // Not used
        return PxQueryHitType::eNONE;
    }
};

class CharacterQueryFilterPhysX : public PxQueryFilterCallback
{
    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // Early out to avoid crashing
        if (!shape)
            return PxQueryHitType::eNONE;

        // Let triggers through
        if (PxFilterObjectIsTrigger(shape->getFlags()))
            return PxQueryHitType::eNONE;

        // Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
        const PxFilterData shapeFilter = shape->getQueryFilterData();
        if (filterData.word0 & shapeFilter.word1)
            return PxQueryHitType::eBLOCK;

        return PxQueryHitType::eNONE;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
    {
        // Not used
        return PxQueryHitType::eNONE;
    }
};

class CharacterControllerFilterPhysX : public PxControllerFilterCallback
{
    static PxShape* getShape(const PxController& controller)
    {
        PxRigidDynamic* actor = controller.getActor();

        // Early out if no actor or no shapes
        if (!actor || actor->getNbShapes() < 1)
            return nullptr;

        // Get first shape only
        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);

        return shape;
    }

    bool filter(const PxController& a, const PxController& b) override
    {
        // Early out to avoid crashing
        PxShape* shapeA = getShape(a);
        if (!shapeA)
            return false;

        PxShape* shapeB = getShape(b);
        if (!shapeB)
            return false;

        // Let triggers through
        if (PxFilterObjectIsTrigger(shapeB->getFlags()))
            return false;

        // Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
        const PxFilterData shapeFilterA = shapeA->getQueryFilterData();
        const PxFilterData shapeFilterB = shapeB->getQueryFilterData();
        if (shapeFilterA.word0 & shapeFilterB.word1)
            return true;

        return false;
    }
};

class CharacterControllerHitReportPhysX : public PxUserControllerHitReport
{
    void onHit(const PxControllerHit& hit, Collision& c)
    {
        if (c.ThisActor == nullptr || c.OtherActor == nullptr)
        {
            // One of the actors was deleted (eg. via RigidBody destroyed by gameplay) then skip processing this collision
            return;
        }

        c.Impulse = Vector3::Zero;
        c.ThisVelocity = P2C(hit.dir) * hit.length;
        c.OtherVelocity = Vector3::Zero;
        c.ContactsCount = 1;
        ContactPoint& contact = c.Contacts[0];
        contact.Point = P2C(hit.worldPos);
        contact.Normal = P2C(hit.worldNormal);
        contact.Separation = 0.0f;

        //auto simulationEventCallback = static_cast<SimulationEventCallback*>(hit.controller->getScene()->getSimulationEventCallback());
        //simulationEventCallback->Collisions[SimulationEventCallback::CollidersPair(c.ThisActor, c.OtherActor)] = c;
        // TODO: build additional list for hit-only events to properly send enter/exit pairs instead of spamming every frame whether controller executes move

        // Single-hit collision
        c.ThisActor->OnCollisionEnter(c);
        c.SwapObjects();
        c.ThisActor->OnCollisionEnter(c);
        c.SwapObjects();
        c.ThisActor->OnCollisionExit(c);
        c.SwapObjects();
        c.ThisActor->OnCollisionExit(c);
    }

    void onShapeHit(const PxControllerShapeHit& hit) override
    {
        Collision c;
        PxShape* controllerShape;
        hit.controller->getActor()->getShapes(&controllerShape, 1);
        c.ThisActor = static_cast<PhysicsColliderActor*>(controllerShape->userData);
        c.OtherActor = static_cast<PhysicsColliderActor*>(hit.shape->userData);
        onHit(hit, c);
    }

    void onControllerHit(const PxControllersHit& hit) override
    {
        Collision c;
        PxShape* controllerShape;
        hit.controller->getActor()->getShapes(&controllerShape, 1);
        c.ThisActor = static_cast<PhysicsColliderActor*>(controllerShape->userData);
        hit.other->getActor()->getShapes(&controllerShape, 1);
        c.OtherActor = static_cast<PhysicsColliderActor*>(controllerShape->userData);
        onHit(hit, c);
    }

    void onObstacleHit(const PxControllerObstacleHit& hit) override
    {
    }
};

#if WITH_VEHICLE

class WheelFilterPhysX : public PxQueryFilterCallback
{
public:
    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        if (!shape)
            return PxQueryHitType::eNONE;
        const PxFilterData shapeFilter = shape->getQueryFilterData();

        // Let triggers through
        if (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
            return PxQueryHitType::eNONE;

        // Hardcoded id for vehicle shapes masking
        if (filterData.word3 == shapeFilter.word3)
            return PxQueryHitType::eNONE;

        // Collide for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
        if ((filterData.word0 & shapeFilter.word1) && (shapeFilter.word0 & filterData.word1))
            return PxQueryHitType::eBLOCK;

        return PxQueryHitType::eNONE;
    }
};

#endif

class WriteStreamPhysX : public PxOutputStream
{
public:
    WriteStream* Stream;

    uint32_t write(const void* src, uint32_t count) override
    {
        Stream->WriteBytes(src, (int32)count);
        return count;
    }
};

template<typename HitType>
class DynamicHitBuffer : public PxHitCallback<HitType>
{
private:
    uint32 _count;
    HitType _buffer[PHYSX_HIT_BUFFER_SIZE];

public:
    DynamicHitBuffer()
        : PxHitCallback<HitType>(_buffer, PHYSX_HIT_BUFFER_SIZE)
        , _count(0)
    {
    }

public:
    // Computes the number of any hits in this result, blocking or touching.
    PX_INLINE PxU32 getNbAnyHits() const
    {
        return getNbTouches();
    }

    // Convenience iterator used to access any hits in this result, blocking or touching.
    PX_INLINE const HitType& getAnyHit(const PxU32 index) const
    {
        PX_ASSERT(index < getNbTouches() + PxU32(this->hasBlock));
        return index < getNbTouches() ? getTouches()[index] : this->block;
    }

    PX_INLINE PxU32 getNbTouches() const
    {
        return _count;
    }

    PX_INLINE const HitType* getTouches() const
    {
        return _buffer;
    }

    PX_INLINE const HitType& getTouch(const PxU32 index) const
    {
        PX_ASSERT(index < getNbTouches());
        return _buffer[index];
    }

    PX_INLINE PxU32 getMaxNbTouches() const
    {
        return PHYSX_HIT_BUFFER_SIZE;
    }

protected:
    PxAgain processTouches(const HitType* buffer, PxU32 nbHits) override
    {
        nbHits = Math::Min(nbHits, PHYSX_HIT_BUFFER_SIZE - _count);
        for (PxU32 i = 0; i < nbHits; i++)
        {
            _buffer[_count + i] = buffer[i];
        }
        _count += nbHits;
        return true;
    }

    void finalizeQuery() override
    {
        if (this->hasBlock)
        {
            // Blocking hits go to hits
            processTouches(&this->block, 1);
        }
    }
};

#define PxHitFlagEmpty (PxHitFlags)0
#define SCENE_QUERY_FLAGS (PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eFACE_INDEX | PxHitFlag::eUV)

#define SCENE_QUERY_SETUP(blockSingle) auto scenePhysX = (ScenePhysX*)scene; if (scene == nullptr) return false; \
		PxQueryFilterData filterData; \
		filterData.flags |=  PxQueryFlag::ePREFILTER; \
		filterData.data.word0 = layerMask; \
		filterData.data.word1 = blockSingle ? 1 : 0; \
		filterData.data.word2 = hitTriggers ? 1 : 0

#define SCENE_QUERY_SETUP_SWEEP_1() SCENE_QUERY_SETUP(true); \
		PxSweepBufferN<1> buffer

#define SCENE_QUERY_SETUP_SWEEP() SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxSweepHit> buffer

#define SCENE_QUERY_SETUP_OVERLAP_1() SCENE_QUERY_SETUP(false); \
		PxOverlapBufferN<1> buffer

#define SCENE_QUERY_SETUP_OVERLAP() SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxOverlapHit> buffer

#define SCENE_QUERY_COLLECT_SINGLE() const auto& hit = buffer.getAnyHit(0); \
        P2C(hit, hitInfo); \
		hitInfo.Point += scenePhysX->Origin

#define SCENE_QUERY_COLLECT_ALL() results.Clear(); \
		results.Resize(buffer.getNbAnyHits(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			const auto& hit = buffer.getAnyHit(i); \
			P2C(hit, results[i]); \
			results[i].Point += scenePhysX->Origin; \
		}

#define SCENE_QUERY_COLLECT_OVERLAP() results.Clear();  \
		results.Resize(buffer.getNbTouches(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			auto& hitInfo = results[i]; \
			const auto& hit = buffer.getTouch(i); \
			hitInfo = hit.shape ? static_cast<PhysicsColliderActor*>(hit.shape->userData) : nullptr; \
		}

namespace
{
    PxFoundation* Foundation = nullptr;
    PxPhysics* PhysX = nullptr;
#if WITH_PVD
    PxPvd* PVD = nullptr;
#endif
#if COMPILE_WITH_PHYSICS_COOKING
    PxCooking* Cooking = nullptr;
#endif
    PxMaterial* DefaultMaterial = nullptr;
    AllocatorPhysX AllocatorCallback;
    ErrorPhysX ErrorCallback;
#if WITH_CLOTH
    AssertPhysX AssertCallback;
    ProfilerPhysX ProfilerCallback;
#endif
    PxTolerancesScale ToleranceScale;
    QueryFilterPhysX QueryFilter;
    CharacterQueryFilterPhysX CharacterQueryFilter;
    CharacterControllerFilterPhysX CharacterControllerFilter;
    CharacterControllerHitReportPhysX CharacterControllerHitReport;
    Dictionary<PxScene*, Vector3, InlinedAllocation<32>> SceneOrigins;

    CriticalSection FlushLocker;
    Array<PxBase*> DeleteObjects;

    bool _queriesHitTriggers = true;
    bool _enableCCD = true;
    PhysicsCombineMode _frictionCombineMode = PhysicsCombineMode::Average;
    PhysicsCombineMode _restitutionCombineMode = PhysicsCombineMode::Average;

#if WITH_VEHICLE
    bool VehicleSDKInitialized = false;
    Array<PxVehicleWheels*> WheelVehiclesCache;
    Array<PxWheelQueryResult> WheelVehiclesResultsPerWheel;
    Array<PxVehicleWheelQueryResult> WheelVehiclesResultsPerVehicle;
    PxVehicleDrivableSurfaceToTireFrictionPairs* WheelTireFrictions = nullptr;
    bool WheelTireFrictionsDirty = false;
    Array<float> WheelTireTypes;
    WheelFilterPhysX WheelRaycastFilter;
#endif

#if WITH_CLOTH
    CriticalSection ClothLocker;
    nv::cloth::Factory* ClothFactory = nullptr;
    Dictionary<nv::cloth::Fabric*, FabricSettings> Fabrics;
    Dictionary<nv::cloth::Cloth*, ClothSettings> Cloths;
#endif
}

FORCE_INLINE PxPlane transform(const PxPlane& plane, const PxMat33& inverse)
{
    PxPlane result;
    result.n.x = plane.n.x * inverse.column0.x + plane.n.y * inverse.column0.y + plane.n.z * inverse.column0.z;
    result.n.y = plane.n.x * inverse.column1.x + plane.n.y * inverse.column1.y + plane.n.z * inverse.column1.z;
    result.n.z = plane.n.x * inverse.column2.x + plane.n.y * inverse.column2.y + plane.n.z * inverse.column2.z;
    result.d = plane.d;
    return result;
}

PxShapeFlags GetShapeFlags(bool isTrigger, bool isEnabled)
{
#if WITH_PVD
    PxShapeFlags flags = PxShapeFlag::eVISUALIZATION;
#else
    PxShapeFlags flags = static_cast<PxShapeFlags>(0);
#endif

    if (isEnabled)
    {
        if (isTrigger)
        {
            flags |= PxShapeFlag::eTRIGGER_SHAPE;
            if (_queriesHitTriggers)
                flags |= PxShapeFlag::eSCENE_QUERY_SHAPE;
        }
        else
        {
            flags = PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE;
        }
    }

    return flags;
}

void GetShapeGeometry(const CollisionShape& shape, PxGeometryHolder& geometry)
{
    switch (shape.Type)
    {
    case CollisionShape::Types::Sphere:
        geometry.storeAny(PxSphereGeometry(shape.Sphere.Radius));
        break;
    case CollisionShape::Types::Box:
        geometry.storeAny(PxBoxGeometry(shape.Box.HalfExtents[0], shape.Box.HalfExtents[1], shape.Box.HalfExtents[2]));
        break;
    case CollisionShape::Types::Capsule:
        geometry.storeAny(PxCapsuleGeometry(shape.Capsule.Radius, shape.Capsule.HalfHeight));
        break;
    case CollisionShape::Types::ConvexMesh:
        geometry.storeAny(PxConvexMeshGeometry((PxConvexMesh*)shape.ConvexMesh.ConvexMesh, PxMeshScale(PxVec3(shape.ConvexMesh.Scale[0], shape.ConvexMesh.Scale[1], shape.ConvexMesh.Scale[2]))));
        break;
    case CollisionShape::Types::TriangleMesh:
        geometry.storeAny(PxTriangleMeshGeometry((PxTriangleMesh*)shape.TriangleMesh.TriangleMesh, PxMeshScale(PxVec3(shape.TriangleMesh.Scale[0], shape.TriangleMesh.Scale[1], shape.TriangleMesh.Scale[2]))));
        break;
    case CollisionShape::Types::HeightField:
        geometry.storeAny(PxHeightFieldGeometry((PxHeightField*)shape.HeightField.HeightField, PxMeshGeometryFlags(0), Math::Max(shape.HeightField.HeightScale, PX_MIN_HEIGHTFIELD_Y_SCALE), Math::Max(shape.HeightField.RowScale, PX_MIN_HEIGHTFIELD_XZ_SCALE), Math::Max(shape.HeightField.ColumnScale, PX_MIN_HEIGHTFIELD_XZ_SCALE)));
        break;
    }
}

void GetShapeMaterials(Array<PxMaterial*, InlinedAllocation<1>>& materialsPhysX, Span<JsonAsset*> materials)
{
    materialsPhysX.Resize(materials.Length());
    for (int32 i = 0; i < materials.Length(); i++)
    {
        PxMaterial* materialPhysX = DefaultMaterial;
        const JsonAsset* material = materials.Get()[i];
        if (material && !material->WaitForLoaded() && material->Instance)
            materialPhysX = (PxMaterial*)((PhysicalMaterial*)material->Instance)->GetPhysicsMaterial();
        materialsPhysX.Get()[i] = materialPhysX;
    }
}

PxFilterFlags FilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    const bool maskTest = (filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1);

    // Let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        if (maskTest)
        {
            // Notify trigger if masks specify it
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        }
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        return PxFilterFlag::eDEFAULT;
    }

    // Send events for the kinematic actors but don't solve the contact
    if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        return PxFilterFlag::eSUPPRESS;
    }

    // Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
    if (maskTest)
    {
        pairFlags |= PxPairFlag::eSOLVE_CONTACT;
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        pairFlags |= PxPairFlag::ePOST_SOLVER_VELOCITY;
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
        if (_enableCCD)
            pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
        return PxFilterFlag::eDEFAULT;
    }

    // Ignore pair (no collisions nor events)
    return PxFilterFlag::eKILL;
}

#if WITH_VEHICLE

void InitVehicleSDK()
{
    if (!VehicleSDKInitialized)
    {
        VehicleSDKInitialized = true;
        PxInitVehicleSDK(*PhysX);
        PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
        PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
    }
}

void ScenePhysX::UpdateVehicles(float dt)
{
    if (WheelVehicles.IsEmpty())
        return;
    PROFILE_CPU_NAMED("Physics.Vehicles");

    // Update vehicles steering
    WheelVehiclesCache.Clear();
    WheelVehiclesCache.EnsureCapacity(WheelVehicles.Count());
    int32 wheelsCount = 0;
    for (auto wheelVehicle : WheelVehicles)
    {
        if (!wheelVehicle->IsActiveInHierarchy() || !wheelVehicle->GetEnableSimulation())
            continue;
        auto drive = (PxVehicleWheels*)wheelVehicle->_vehicle;
        ASSERT(drive);
        WheelVehiclesCache.Add(drive);
        wheelsCount += drive->mWheelsSimData.getNbWheels();

        const float deadZone = 0.1f;
        bool isTank = wheelVehicle->_driveType == WheeledVehicle::DriveTypes::Tank;
        float throttle = wheelVehicle->_throttle;
        float steering = wheelVehicle->_steering;
        float brake = wheelVehicle->_brake;
        float leftThrottle = wheelVehicle->_tankLeftThrottle;
        float rightThrottle = wheelVehicle->_tankRightThrottle;
        float leftBrake = Math::Max(wheelVehicle->_tankLeftBrake, wheelVehicle->_handBrake);
        float rightBrake = Math::Max(wheelVehicle->_tankRightBrake, wheelVehicle->_handBrake);
        WheeledVehicle::DriveModes vehicleDriveMode = wheelVehicle->_driveControl.DriveMode;

        if (isTank)
        {
            // Converting default vehicle controls to tank controls.
            if (throttle != 0 || steering != 0)
            {
                leftThrottle = Math::Clamp(throttle + steering, -1.0f, 1.0f);
                rightThrottle = Math::Clamp(throttle - steering, -1.0f, 1.0f);
            }
        }

        // Converting special tank drive mode to standard tank mode when is turning.
        if (isTank && vehicleDriveMode == WheeledVehicle::DriveModes::Standard)
        {
            // Special inputs when turning vehicle -1 1 to left or 1 -1 to turn right
            // to:
            // Standard inputs when turning vehicle 0 1 to left or 1 0 to turn right

            if (leftThrottle < -deadZone && rightThrottle > deadZone)
            {
                leftThrottle = 0;
                leftBrake = 1;
            }
            else if (leftThrottle > deadZone && rightThrottle < -deadZone)
            {
                rightThrottle = 0;
                rightBrake = 1;
            }
        }

        if (wheelVehicle->UseReverseAsBrake)
        {
            const float invalidDirectionThreshold = 80.0f;
            const float breakThreshold = 8.0f;
            const float forwardSpeed = wheelVehicle->GetForwardSpeed();
            int currentGear = wheelVehicle->GetCurrentGear();
            // Tank tracks direction: 1 forward -1 backward 0 neutral
            bool toForward = false;
            toForward |= throttle > deadZone;
            toForward |= (leftThrottle > deadZone) && (rightThrottle > deadZone); // 1  1

            bool toBackward = false;
            toBackward |= throttle < -deadZone;
            toBackward |= (leftThrottle < -deadZone) && (rightThrottle < -deadZone); // -1 -1
            toBackward |= (leftThrottle < -deadZone) && (rightThrottle < deadZone); // -1  0
            toBackward |= (leftThrottle < deadZone) && (rightThrottle < -deadZone); //  0 -1

            bool isTankTurning = false;

            if (isTank)
            {
                isTankTurning |= leftThrottle > deadZone && rightThrottle < -deadZone; //  1 -1
                isTankTurning |= leftThrottle < -deadZone && rightThrottle > deadZone; // -1  1
                isTankTurning |= leftThrottle < deadZone && rightThrottle > deadZone; //  0  1
                isTankTurning |= leftThrottle > deadZone && rightThrottle < deadZone; //  1  0
                isTankTurning |= leftThrottle < -deadZone && rightThrottle < deadZone; // -1  0
                isTankTurning |= leftThrottle < deadZone && rightThrottle < -deadZone; //  0 -1

                if (toForward || toBackward)
                {
                    isTankTurning = false;
                }
            }

            // Automatic gear change when changing driving direction
            if (Math::Abs(forwardSpeed) < invalidDirectionThreshold)
            {
                int targetGear = wheelVehicle->GetTargetGear();
                if (toBackward && currentGear > 0 && targetGear >= 0)
                {
                    currentGear = -1;
                }
                else if (!toBackward && currentGear <= 0 && targetGear <= 0)
                {
                    currentGear = 1;
                }
                else if (isTankTurning && currentGear <= 0)
                {
                    currentGear = 1;
                }

                if (wheelVehicle->GetCurrentGear() != currentGear)
                {
                    wheelVehicle->SetCurrentGear(currentGear);
                }
            }

            // Automatic break when changing driving direction
            if (toForward)
            {
                if (forwardSpeed < -invalidDirectionThreshold)
                {
                    brake = 1.0f;
                    leftBrake = 1.0f;
                    rightBrake = 1.0f;
                }
            }
            else if (toBackward)
            {
                if (forwardSpeed > invalidDirectionThreshold)
                {
                    brake = 1.0f;
                    leftBrake = 1.0f;
                    rightBrake = 1.0f;
                }
            }
            else
            {
                if (forwardSpeed < breakThreshold && forwardSpeed > -breakThreshold && !isTankTurning) // not accelerating, very slow speed -> stop
                {
                    brake = 1.0f;
                    leftBrake = 1.0f;
                    rightBrake = 1.0f;
                }
            }

            // Block throttle if user is changing driving direction
            if ((toForward && currentGear < 0) || (toBackward && currentGear > 0))
            {
                throttle = 0.0f;
                leftThrottle = 0;
                rightThrottle = 0;
            }

            throttle = Math::Abs(throttle);

            if (isTank)
            {
                // invert acceleration when moving to backward because tank inputs can be < 0
                if (currentGear < 0)
                {
                    float lt = -leftThrottle;
                    float rt = -rightThrottle;
                    float lb = leftBrake;
                    float rb = rightBrake;
                    leftThrottle = rt;
                    rightThrottle = lt;
                    leftBrake = rb;
                    rightBrake = lb;
                }
            }
        }
        else
        {
            throttle = Math::Max(throttle, 0.0f);
        }

        // Force brake the another side track to turn faster
        if (Math::Abs(leftThrottle) > deadZone && Math::Abs(rightThrottle) < deadZone)
        {
            rightBrake = 1.0f;
        }
        if (Math::Abs(rightThrottle) > deadZone && Math::Abs(leftThrottle) < deadZone)
        {
            leftBrake = 1.0f;
        }

        // Smooth input controls
        // @formatter:off
        PxVehiclePadSmoothingData padSmoothing =
        {
            {
                wheelVehicle->_driveControl.RiseRateAcceleration, // rise rate eANALOG_INPUT_ACCEL
                wheelVehicle->_driveControl.RiseRateBrake,        // rise rate eANALOG_INPUT_BRAKE
                wheelVehicle->_driveControl.RiseRateHandBrake,    // rise rate eANALOG_INPUT_HANDBRAKE
                wheelVehicle->_driveControl.RiseRateSteer,        // rise rate eANALOG_INPUT_STEER_LEFT
                wheelVehicle->_driveControl.RiseRateSteer,        // rise rate eANALOG_INPUT_STEER_RIGHT
            },
            {
                wheelVehicle->_driveControl.FallRateAcceleration, // fall rate eANALOG_INPUT_ACCEL
                wheelVehicle->_driveControl.FallRateBrake,        // fall rate eANALOG_INPUT_BRAKE
                wheelVehicle->_driveControl.FallRateHandBrake,    // fall rate eANALOG_INPUT_HANDBRAKE
                wheelVehicle->_driveControl.FallRateSteer,        // fall rate eANALOG_INPUT_STEER_LEFT
                wheelVehicle->_driveControl.FallRateSteer,        // fall rate eANALOG_INPUT_STEER_RIGHT
            }
        };
        PxVehicleKeySmoothingData keySmoothing =
        {
            {
                wheelVehicle->_driveControl.RiseRateAcceleration, // rise rate eANALOG_INPUT_ACCEL
                wheelVehicle->_driveControl.RiseRateBrake,        // rise rate eANALOG_INPUT_BRAKE
                wheelVehicle->_driveControl.RiseRateHandBrake,    // rise rate eANALOG_INPUT_HANDBRAKE
                wheelVehicle->_driveControl.RiseRateSteer,        // rise rate eANALOG_INPUT_STEER_LEFT
                wheelVehicle->_driveControl.RiseRateSteer,        // rise rate eANALOG_INPUT_STEER_RIGHT
            },
            {
                wheelVehicle->_driveControl.FallRateAcceleration, // fall rate eANALOG_INPUT_ACCEL
                wheelVehicle->_driveControl.FallRateBrake,        // fall rate eANALOG_INPUT_BRAKE
                wheelVehicle->_driveControl.FallRateHandBrake,    // fall rate eANALOG_INPUT_HANDBRAKE
                wheelVehicle->_driveControl.FallRateSteer,        // fall rate eANALOG_INPUT_STEER_LEFT
                wheelVehicle->_driveControl.FallRateSteer,        // fall rate eANALOG_INPUT_STEER_RIGHT
            }
        };
        // @formatter:on

        // Reduce steer by speed to make vehicle easier to maneuver 
        constexpr int steerVsSpeedN = 8;
        PxF32 steerVsForwardSpeedData[steerVsSpeedN];
        const int lastSteerVsSpeedIndex = wheelVehicle->_driveControl.SteerVsSpeed.Count() - 1;
        int steerVsSpeedIndex = 0;

        // Steer vs speed data structure example:
        // array:
        // speed,   steer
        // 1000,    1.0,
        // 2000,    0.7,
        // 5000,    0.5,
        // ..

        // fill the steerVsForwardSpeedData with the speed and steer
        for (int32 i = 0; i < 8; i += 2)
        {
            steerVsForwardSpeedData[i] = wheelVehicle->_driveControl.SteerVsSpeed[steerVsSpeedIndex].Speed;
            steerVsForwardSpeedData[i + 1] = wheelVehicle->_driveControl.SteerVsSpeed[steerVsSpeedIndex].Steer;
            steerVsSpeedIndex = Math::Min(steerVsSpeedIndex + 1, lastSteerVsSpeedIndex);
        }
        const PxFixedSizeLookupTable<steerVsSpeedN> steerVsForwardSpeed(steerVsForwardSpeedData, 4);

        if (wheelVehicle->UseAnalogSteering)
        {
            switch (wheelVehicle->_driveTypeCurrent)
            {
            case WheeledVehicle::DriveTypes::Drive4W:
            {
                PxVehicleDrive4WRawInputData rawInputData;
                rawInputData.setAnalogAccel(throttle);
                rawInputData.setAnalogBrake(brake);
                rawInputData.setAnalogSteer(wheelVehicle->_steering);
                rawInputData.setAnalogHandbrake(wheelVehicle->_handBrake);
                PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, dt, false, *(PxVehicleDrive4W*)drive);
                break;
            }
            case WheeledVehicle::DriveTypes::DriveNW:
            {
                PxVehicleDriveNWRawInputData rawInputData;
                rawInputData.setAnalogAccel(throttle);
                rawInputData.setAnalogBrake(brake);
                rawInputData.setAnalogSteer(wheelVehicle->_steering);
                rawInputData.setAnalogHandbrake(wheelVehicle->_handBrake);
                PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, dt, false, *(PxVehicleDriveNW*)drive);
                break;
            }
            case WheeledVehicle::DriveTypes::Tank:
            {
                PxVehicleDriveTankRawInputData driveMode = vehicleDriveMode == WheeledVehicle::DriveModes::Standard ? PxVehicleDriveTankControlModel::eSTANDARD : PxVehicleDriveTankControlModel::eSPECIAL;
                PxVehicleDriveTankRawInputData rawInputData = PxVehicleDriveTankRawInputData(driveMode);
                rawInputData.setAnalogAccel(Math::Max(Math::Abs(leftThrottle), Math::Abs(rightThrottle)));
                rawInputData.setAnalogLeftBrake(leftBrake);
                rawInputData.setAnalogRightBrake(rightBrake);
                rawInputData.setAnalogLeftThrust(leftThrottle);
                rawInputData.setAnalogRightThrust(rightThrottle);
                PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, rawInputData, dt, *(PxVehicleDriveTank*)drive);
                break;
            }
            }
        }
        else
        {
            switch (wheelVehicle->_driveTypeCurrent)
            {
            case WheeledVehicle::DriveTypes::Drive4W:
            {
                PxVehicleDrive4WRawInputData rawInputData;
                rawInputData.setDigitalAccel(throttle > deadZone);
                rawInputData.setDigitalBrake(brake > deadZone);
                rawInputData.setDigitalSteerLeft(wheelVehicle->_steering < -deadZone);
                rawInputData.setDigitalSteerRight(wheelVehicle->_steering > deadZone);
                rawInputData.setDigitalHandbrake(wheelVehicle->_handBrake > deadZone);
                PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, dt, false, *(PxVehicleDrive4W*)drive);
                break;
            }
            case WheeledVehicle::DriveTypes::DriveNW:
            {
                PxVehicleDriveNWRawInputData rawInputData;
                rawInputData.setDigitalAccel(throttle > deadZone);
                rawInputData.setDigitalBrake(brake > deadZone);
                rawInputData.setDigitalSteerLeft(wheelVehicle->_steering < -deadZone);
                rawInputData.setDigitalSteerRight(wheelVehicle->_steering > deadZone);
                rawInputData.setDigitalHandbrake(wheelVehicle->_handBrake > deadZone);
                PxVehicleDriveNWSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, dt, false, *(PxVehicleDriveNW*)drive);
                break;
            }
            case WheeledVehicle::DriveTypes::Tank:
            {
                // Convert analog inputs to digital inputs
                leftThrottle = Math::Round(leftThrottle);
                rightThrottle = Math::Round(rightThrottle);
                leftBrake = Math::Round(leftBrake);
                rightBrake = Math::Round(rightBrake);

                PxVehicleDriveTankRawInputData driveMode = vehicleDriveMode == WheeledVehicle::DriveModes::Standard ? PxVehicleDriveTankControlModel::eSTANDARD : PxVehicleDriveTankControlModel::eSPECIAL;
                PxVehicleDriveTankRawInputData rawInputData = PxVehicleDriveTankRawInputData(driveMode);
                rawInputData.setAnalogAccel(Math::Max(Math::Abs(leftThrottle), Math::Abs(rightThrottle)));
                rawInputData.setAnalogLeftBrake(leftBrake);
                rawInputData.setAnalogRightBrake(rightBrake);
                rawInputData.setAnalogLeftThrust(leftThrottle);
                rawInputData.setAnalogRightThrust(rightThrottle);

                // Needs to pass analog values to vehicle to maintain current movement direction because digital inputs accept only true/false values to tracks thrust instead of -1 to 1 
                PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, rawInputData, dt, *(PxVehicleDriveTank*)drive);
                break;
            }
            }
        }
    }

    // Update batches queries cache
    if (wheelsCount > WheelRaycastBatchQuerySize)
    {
        if (WheelRaycastBatchQuery)
            WheelRaycastBatchQuery->release();
        WheelRaycastBatchQuerySize = wheelsCount;
        WheelRaycastBatchQuery = PxCreateBatchQueryExt(*Scene, &WheelRaycastFilter, wheelsCount, wheelsCount, 0, 0, 0, 0);
    }

    // Update lookup table that maps wheel type into the surface friction
    if (!WheelTireFrictions || WheelTireFrictionsDirty)
    {
        WheelTireFrictionsDirty = false;
        RELEASE_PHYSX(WheelTireFrictions);
        Array<PxMaterial*, InlinedAllocation<8>> materials;
        materials.Resize(Math::Min<int32>((int32)PhysX->getNbMaterials(), PxVehicleDrivableSurfaceToTireFrictionPairs::eMAX_NB_SURFACE_TYPES));
        PxMaterial** materialsPtr = materials.Get();
        PhysX->getMaterials(materialsPtr, materials.Count(), 0);
        Array<PxVehicleDrivableSurfaceType, InlinedAllocation<8>> tireTypes;
        tireTypes.Resize(materials.Count());
        PxVehicleDrivableSurfaceType* tireTypesPtr = tireTypes.Get();
        for (int32 i = 0; i < tireTypes.Count(); i++)
            tireTypesPtr[i].mType = i;
        WheelTireFrictions = PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(WheelTireTypes.Count(), materials.Count());
        WheelTireFrictions->setup(WheelTireTypes.Count(), materials.Count(), (const PxMaterial**)materialsPtr, tireTypesPtr);
        for (int32 material = 0; material < materials.Count(); material++)
        {
            float friction = materialsPtr[material]->getStaticFriction();
            for (int32 tireType = 0; tireType < WheelTireTypes.Count(); tireType++)
            {
                float scale = WheelTireTypes[tireType];
                WheelTireFrictions->setTypePairFriction(material, tireType, friction * scale);
            }
        }
    }

    // Setup cache for wheel states
    WheelVehiclesResultsPerVehicle.Resize(WheelVehiclesCache.Count(), false);
    WheelVehiclesResultsPerWheel.Resize(wheelsCount, false);
    wheelsCount = 0;
    for (int32 i = 0, ii = 0; i < WheelVehicles.Count(); i++)
    {
        auto wheelVehicle = WheelVehicles[i];
        if (!wheelVehicle->IsActiveInHierarchy() || !wheelVehicle->GetEnableSimulation())
            continue;
        auto drive = (PxVehicleWheels*)WheelVehicles[ii]->_vehicle;
        auto& perVehicle = WheelVehiclesResultsPerVehicle[ii];
        ii++;
        perVehicle.nbWheelQueryResults = drive->mWheelsSimData.getNbWheels();
        perVehicle.wheelQueryResults = WheelVehiclesResultsPerWheel.Get() + wheelsCount;
        wheelsCount += perVehicle.nbWheelQueryResults;
    }

    // Update vehicles
    if (WheelVehiclesCache.Count() != 0)
    {
        PxVehicleSuspensionRaycasts(WheelRaycastBatchQuery, WheelVehiclesCache.Count(), WheelVehiclesCache.Get());
        PxVehicleUpdates(dt, Scene->getGravity(), *WheelTireFrictions, WheelVehiclesCache.Count(), WheelVehiclesCache.Get(), WheelVehiclesResultsPerVehicle.Get());
    }

    // Synchronize state
    for (int32 i = 0, ii = 0; i < WheelVehicles.Count(); i++)
    {
        auto wheelVehicle = WheelVehicles[i];
        if (!wheelVehicle->IsActiveInHierarchy() || !wheelVehicle->GetEnableSimulation())
            continue;
        auto drive = WheelVehiclesCache[ii];
        auto& perVehicle = WheelVehiclesResultsPerVehicle[ii];
        ii++;
#if PHYSX_VEHICLE_DEBUG_TELEMETRY
        LOG(Info, "Vehicle[{}] Gear={}, RPM={}", ii, wheelVehicle->GetCurrentGear(), (int32)wheelVehicle->GetEngineRotationSpeed());
#endif

        // Update wheels
        for (int32 j = 0; j < wheelVehicle->_wheelsData.Count(); j++)
        {
            auto& wheelData = wheelVehicle->_wheelsData[j];
            auto& perWheel = perVehicle.wheelQueryResults[j];
#if PHYSX_VEHICLE_DEBUG_TELEMETRY
            LOG(Info, "Vehicle[{}] Wheel[{}] longitudinalSlip={}, lateralSlip={}, suspSpringForce={}", ii, j, Utilities::RoundTo2DecimalPlaces(perWheel.longitudinalSlip), Utilities::RoundTo2DecimalPlaces(perWheel.lateralSlip), (int32)perWheel.suspSpringForce);
#endif

            auto& state = wheelData.State;
            state.IsInAir = perWheel.isInAir;
            state.TireContactCollider = perWheel.tireContactShape ? static_cast<PhysicsColliderActor*>(perWheel.tireContactShape->userData) : nullptr;
            state.TireContactPoint = P2C(perWheel.tireContactPoint) + Origin;
            state.TireContactNormal = P2C(perWheel.tireContactNormal);
            state.TireFriction = perWheel.tireFriction;
            state.SteerAngle = RadiansToDegrees * perWheel.steerAngle;
            state.RotationAngle = -RadiansToDegrees * drive->mWheelsDynData.getWheelRotationAngle(j);
            state.SuspensionOffset = perWheel.suspJounce;
#if USE_EDITOR
            state.SuspensionTraceStart = P2C(perWheel.suspLineStart) + Origin;
            state.SuspensionTraceEnd = P2C(perWheel.suspLineStart + perWheel.suspLineDir * perWheel.suspLineLength) + Origin;
#endif

            if (!wheelData.Collider)
                continue;
            auto shape = (PxShape*)wheelData.Collider->GetPhysicsShape();

            // Update wheel collider transformation
            auto localPose = shape->getLocalPose();
            Transform t = wheelData.Collider->GetLocalTransform();
            t.Orientation = Quaternion::Euler(-state.RotationAngle, state.SteerAngle, 0) * wheelData.LocalOrientation;
            t.Translation = P2C(localPose.p) / wheelVehicle->GetScale() - t.Orientation * wheelData.Collider->GetCenter();
            wheelData.Collider->SetLocalTransform(t);
        }
    }
}

#endif

#if WITH_CLOTH

void ScenePhysX::PreSimulateCloth(int32 i)
{
    PROFILE_CPU();
    auto clothPhysX = ClothsList[i];
    auto& clothSettings = Cloths[clothPhysX];

    if (clothSettings.Actor->OnPreUpdate())
    {
        // Cull simulation based on distance
        if (!clothSettings.Culled)
        {
            clothSettings.Culled = true;
            ClothLocker.Lock();
            ClothSolver->removeCloth(clothPhysX);
            ClothLocker.Unlock();
        }
        return;
    }
    if (clothSettings.Culled)
    {
        clothSettings.Culled = false;
        ClothLocker.Lock();
        ClothSolver->addCloth(clothPhysX);
        ClothLocker.Unlock();
    }

    // Setup automatic scene collisions with colliders around the cloth
    if (clothSettings.SceneCollisions && clothSettings.CollisionsUpdateFramesLeft == 0)
    {
        PROFILE_CPU_NAMED("Collisions");
        clothSettings.CollisionsUpdateFramesLeft = CLOTH_COLLISIONS_UPDATE_RATE;
        if (clothSettings.CollisionsUpdateFramesRandomize)
        {
            clothSettings.CollisionsUpdateFramesRandomize = false;
            clothSettings.CollisionsUpdateFramesLeft = i % CLOTH_COLLISIONS_UPDATE_RATE;
        }

        // Reset existing colliders
        clothPhysX->setSpheres(nv::cloth::Range<const PxVec4>(), 0, clothPhysX->getNumSpheres());
        clothPhysX->setPlanes(nv::cloth::Range<const PxVec4>(), 0, clothPhysX->getNumPlanes());
        clothPhysX->setTriangles(nv::cloth::Range<const PxVec3>(), 0, clothPhysX->getNumTriangles());

        // Setup environment query
        const bool hitTriggers = false;
        const bool blockSingle = false;
        PxQueryFilterData filterData;
        filterData.flags |= PxQueryFlag::ePREFILTER;
        filterData.data.word1 = blockSingle ? 1 : 0;
        filterData.data.word2 = hitTriggers ? 1 : 0;
        filterData.data.word0 = Physics::LayerMasks[clothSettings.Actor->GetLayer()];
        const PxTransform clothPose(clothPhysX->getTranslation(), clothPhysX->getRotation());
        const PxVec3 clothBoundsPos = clothPhysX->getBoundingBoxCenter();
        const PxVec3 clothBoundsSize = clothPhysX->getBoundingBoxScale();
        const PxTransform overlapPose(clothPose.transform(clothBoundsPos), clothPose.q);
        const float boundsMargin = 1.6f; // Pick nearby objects
        const PxSphereGeometry overlapGeo(clothBoundsSize.magnitude() * boundsMargin);

        // Find any colliders around the cloth
        DynamicHitBuffer<PxOverlapHit> buffer;
        if (Scene->overlap(overlapGeo, overlapPose, buffer, filterData, &QueryFilter))
        {
            const float collisionThickness = clothSettings.CollisionThickness;
            for (uint32 j = 0; j < buffer.getNbTouches(); j++)
            {
                const auto& hit = buffer.getTouch(j);
                if (hit.shape)
                {
                    const PxGeometry& geo = hit.shape->getGeometry();
                    const PxTransform shapeToCloth = clothPose.transformInv(hit.actor->getGlobalPose().transform(hit.shape->getLocalPose()));
                    // TODO: maybe use shared spheres/planes buffers for batched assigning?
                    switch (geo.getType())
                    {
                    case PxGeometryType::eSPHERE:
                    {
                        const PxSphereGeometry& geoSphere = (const PxSphereGeometry&)geo;
                        const PxVec4 packedSphere(shapeToCloth.p, geoSphere.radius + collisionThickness);
                        const nv::cloth::Range<const PxVec4> sphereRange(&packedSphere, &packedSphere + 1);
                        const uint32_t spheresCount = clothPhysX->getNumSpheres();
                        if (spheresCount + 1 > MAX_CLOTH_SPHERE_COUNT)
                            break;
                        clothPhysX->setSpheres(sphereRange, spheresCount, spheresCount);
                        break;
                    }
                    case PxGeometryType::eCAPSULE:
                    {
                        const PxCapsuleGeometry& geomCapsule = (const PxCapsuleGeometry&)geo;
                        const PxVec4 packedSpheres[2] = {
                            PxVec4(shapeToCloth.transform(PxVec3(+geomCapsule.halfHeight, 0, 0)), geomCapsule.radius + collisionThickness),
                            PxVec4(shapeToCloth.transform(PxVec3(-geomCapsule.halfHeight, 0, 0)), geomCapsule.radius + collisionThickness)
                        };
                        const nv::cloth::Range<const PxVec4> sphereRange(packedSpheres, packedSpheres + 2);
                        const uint32_t spheresCount = clothPhysX->getNumSpheres();
                        if (spheresCount + 2 > MAX_CLOTH_SPHERE_COUNT)
                            break;
                        clothPhysX->setSpheres(sphereRange, spheresCount, spheresCount);
                        const uint32_t packedCapsules[2] = { spheresCount, spheresCount + 1 };
                        const int32 capsulesCount = clothPhysX->getNumCapsules();
                        clothPhysX->setCapsules(nv::cloth::Range<const uint32_t>(packedCapsules, packedCapsules + 2), capsulesCount, capsulesCount);
                        break;
                    }
                    case PxGeometryType::eBOX:
                    {
                        const PxBoxGeometry& geomBox = (const PxBoxGeometry&)geo;
                        const uint32_t planesCount = clothPhysX->getNumPlanes();
                        if (planesCount + 6 > MAX_CLOTH_PLANE_COUNT)
                            break;
                        const PxPlane packedPlanes[6] = {
                            PxPlane(PxVec3(1, 0, 0), -geomBox.halfExtents.x - collisionThickness).transform(shapeToCloth),
                            PxPlane(PxVec3(-1, 0, 0), -geomBox.halfExtents.x - collisionThickness).transform(shapeToCloth),
                            PxPlane(PxVec3(0, 1, 0), -geomBox.halfExtents.y - collisionThickness).transform(shapeToCloth),
                            PxPlane(PxVec3(0, -1, 0), -geomBox.halfExtents.y - collisionThickness).transform(shapeToCloth),
                            PxPlane(PxVec3(0, 0, 1), -geomBox.halfExtents.z - collisionThickness).transform(shapeToCloth),
                            PxPlane(PxVec3(0, 0, -1), -geomBox.halfExtents.z - collisionThickness).transform(shapeToCloth)
                        };
                        clothPhysX->setPlanes(nv::cloth::Range<const PxVec4>((const PxVec4*)packedPlanes, (const PxVec4*)packedPlanes + 6), planesCount, planesCount);
                        const PxU32 convexMask = PxU32(0x3f << planesCount);
                        const uint32_t convexesCount = clothPhysX->getNumConvexes();
                        clothPhysX->setConvexes(nv::cloth::Range<const PxU32>(&convexMask, &convexMask + 1), convexesCount, convexesCount);
                        break;
                    }
                    case PxGeometryType::eCONVEXMESH:
                    {
                        const PxConvexMeshGeometry& geomConvexMesh = (const PxConvexMeshGeometry&)geo;
                        const PxU32 convexPlanesCount = geomConvexMesh.convexMesh->getNbPolygons();
                        const uint32_t planesCount = clothPhysX->getNumPlanes();
                        if (planesCount + convexPlanesCount > MAX_CLOTH_PLANE_COUNT)
                            break;
                        const PxMat33 convexToShapeInv = geomConvexMesh.scale.toMat33().getInverse();
                        // TODO: merge convexToShapeInv with shapeToCloth to have a single matrix multiplication
                        PxPlane planes[MAX_CLOTH_PLANE_COUNT];
                        for (PxU32 k = 0; k < convexPlanesCount; k++)
                        {
                            PxHullPolygon polygon;
                            geomConvexMesh.convexMesh->getPolygonData(k, polygon);
                            polygon.mPlane[3] -= collisionThickness;
                            planes[k] = transform(reinterpret_cast<const PxPlane&>(polygon.mPlane), convexToShapeInv).transform(shapeToCloth);
                        }
                        clothPhysX->setPlanes(nv::cloth::Range<const PxVec4>((const PxVec4*)planes, (const PxVec4*)planes + convexPlanesCount), planesCount, planesCount);
                        const PxU32 convexMask = PxU32(((1 << convexPlanesCount) - 1) << planesCount);
                        const uint32_t convexesCount = clothPhysX->getNumConvexes();
                        clothPhysX->setConvexes(nv::cloth::Range<const PxU32>(&convexMask, &convexMask + 1), convexesCount, convexesCount);
                        break;
                    }
                    // Cloth vs Triangle collisions are too slow for real-time use
#if 0
                    case PxGeometryType::eTRIANGLEMESH:
                    {
                        const PxTriangleMeshGeometry& geomTriangleMesh = (const PxTriangleMeshGeometry&)geo;
                        if (geomTriangleMesh.triangleMesh->getNbTriangles() >= 1024)
                            break; // Ignore too-tessellated meshes due to poor solver performance
                        // TODO: use shared memory allocators maybe? maybe per-frame stack allocator?
                        Array<PxVec3> vertices;
                        vertices.Add(geomTriangleMesh.triangleMesh->getVertices(), geomTriangleMesh.triangleMesh->getNbVertices());
                        const PxMat33 triangleMeshToShape = geomTriangleMesh.scale.toMat33();
                        // TODO: merge triangleMeshToShape with shapeToCloth to have a single matrix multiplication
                        for (int32 k = 0; k < vertices.Count(); k++)
                        {
                            PxVec3& v = vertices.Get()[k];
                            v = shapeToCloth.transform(triangleMeshToShape.transform(v));
                        }
                        Array<PxVec3> triangles;
                        triangles.Resize(geomTriangleMesh.triangleMesh->getNbTriangles() * 3);
                        if (geomTriangleMesh.triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES)
                        {
                            auto indices = (const uint16*)geomTriangleMesh.triangleMesh->getTriangles();
                            for (int32 k = 0; k < triangles.Count(); k++)
                                triangles.Get()[k] = vertices.Get()[indices[k]];
                        }
                        else
                        {
                            auto indices = (const uint32*)geomTriangleMesh.triangleMesh->getTriangles();
                            for (int32 k = 0; k < triangles.Count(); k++)
                                triangles.Get()[k] = vertices.Get()[indices[k]];
                        }
                        const uint32_t trianglesCount = clothPhysX->getNumTriangles();
	                    clothPhysX->setTriangles(nv::cloth::Range<const PxVec3>(triangles.begin(), triangles.end()), trianglesCount, trianglesCount);
                        break;
                    }
                    case PxGeometryType::eHEIGHTFIELD:
                    {
                        const PxHeightFieldGeometry& geomHeightField = (const PxHeightFieldGeometry&)geo;
                        // TODO: heightfield collisions (gather triangles only nearby cloth to not blow sim perf)
                        break;
                    }
#endif
                    }
                }
            }
        }
    }
    clothSettings.CollisionsUpdateFramesLeft--;
}

void ScenePhysX::SimulateCloth(int32 i)
{
    PROFILE_CPU();
    ClothSolver->simulateChunk(i);
}

void ScenePhysX::UpdateCloths(float dt)
{
    nv::cloth::Solver* clothSolver = ClothSolver;
    if (!clothSolver || ClothsList.IsEmpty())
        return;
    PROFILE_CPU_NAMED("Physics.Cloth");

    {
        PROFILE_CPU_NAMED("Pre");
        Function<void(int32)> job;
        job.Bind<ScenePhysX, &ScenePhysX::PreSimulateCloth>(this);
        JobSystem::Execute(job, ClothsList.Count());
    }

    {
        PROFILE_CPU_NAMED("Simulation");
        if (clothSolver->beginSimulation(dt))
        {
            Function<void(int32)> job;
            job.Bind<ScenePhysX, &ScenePhysX::SimulateCloth>(this);
            JobSystem::Execute(job, clothSolver->getSimulationChunkCount());
            clothSolver->endSimulation();
        }
    }

    {
        PROFILE_CPU_NAMED("Post");
        ScopeLock lock(ClothLocker);
        Array<Cloth*> brokenCloths;
        for (auto clothPhysX : ClothsList)
        {
            const auto& clothSettings = Cloths[clothPhysX];
            if (clothSettings.Culled)
                continue;
            if (clothSettings.UpdateBounds(clothPhysX))
                brokenCloths.Add(clothSettings.Actor);
            clothSettings.Actor->OnPostUpdate();
        }
        for (auto cloth : brokenCloths)
        {
            // Rebuild cloth object but keep fabric ref to prevent fabric recook
            auto fabric = &((nv::cloth::Cloth*)cloth->GetPhysicsCloth())->getFabric();
            Fabrics[fabric].Refs++;
            fabric->incRefCount();
            cloth->Rebuild();
            fabric->decRefCount();
            if (--Fabrics[fabric].Refs == 0)
                Fabrics.Remove(fabric);
        }
    }
}

#endif

void* PhysicalMaterial::GetPhysicsMaterial()
{
    if (_material == nullptr && PhysX)
    {
        auto material = PhysX->createMaterial(Friction, Friction, Restitution);
        material->userData = this;
        _material = material;

        const PhysicsCombineMode useFrictionCombineMode = OverrideFrictionCombineMode ? FrictionCombineMode : _frictionCombineMode;
        material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        const PhysicsCombineMode useRestitutionCombineMode = OverrideRestitutionCombineMode ? RestitutionCombineMode : _restitutionCombineMode;
        material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));

#if WITH_VEHICLE
        WheelTireFrictionsDirty = true;
#endif
    }
    return _material;
}

void PhysicalMaterial::UpdatePhysicsMaterial()
{
    auto material = (PxMaterial*)_material;
    if (material != nullptr)
    {
        material->setStaticFriction(Friction);
        material->setDynamicFriction(Friction);

        const PhysicsCombineMode useFrictionCombineMode = OverrideFrictionCombineMode ? FrictionCombineMode : _frictionCombineMode;
        material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        material->setRestitution(Restitution);
        const PhysicsCombineMode useRestitutionCombineMode = OverrideRestitutionCombineMode ? RestitutionCombineMode : _restitutionCombineMode;
        material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));

#if WITH_VEHICLE
        WheelTireFrictionsDirty = true;
#endif
    }
}

#if COMPILE_WITH_PHYSICS_COOKING

#define ENSURE_CAN_COOK \
    auto cooking = Cooking; \
    if (cooking == nullptr) \
	{ \
		LOG(Warning, "Physics collisions cooking is disabled at runtime. Enable Physics Settings option SupportCookingAtRuntime to use collision generation at runtime."); \
		return true; \
	}

bool CollisionCooking::CookConvexMesh(CookingInput& input, BytesContainer& output)
{
    PROFILE_CPU();
    ENSURE_CAN_COOK;
    if (input.VertexCount == 0)
        LOG(Warning, "Empty mesh data for collision cooking.");

    // Init options
    PxConvexMeshDesc desc;
    desc.points.count = input.VertexCount;
    desc.points.stride = sizeof(Float3);
    desc.points.data = input.VertexData;
    desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
    if (input.ConvexVertexLimit == 0)
        desc.vertexLimit = CONVEX_VERTEX_MAX;
    else
        desc.vertexLimit = (PxU16)Math::Clamp(input.ConvexVertexLimit, CONVEX_VERTEX_MIN, CONVEX_VERTEX_MAX);
    if (EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::SkipValidation))
        desc.flags |= PxConvexFlag::Enum::eDISABLE_MESH_VALIDATION;
    if (EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::UsePlaneShifting))
        desc.flags |= PxConvexFlag::Enum::ePLANE_SHIFTING;
    if (EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::UseFastInteriaComputation))
        desc.flags |= PxConvexFlag::Enum::eFAST_INERTIA_COMPUTATION;
    if (EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::ShiftVertices))
        desc.flags |= PxConvexFlag::Enum::eSHIFT_VERTICES;
    PxCookingParams cookingParams = cooking->getParams();
    cookingParams.suppressTriangleMeshRemapTable = EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::SuppressFaceRemapTable);
    cooking->setParams(cookingParams);

    // Perform cooking
    PxDefaultMemoryOutputStream outputStream;
    PxConvexMeshCookingResult::Enum result;
    if (!cooking->cookConvexMesh(desc, outputStream, &result))
    {
        LOG(Warning, "Convex Mesh cooking failed. Error code: {0}, Input vertices count: {1}", (int32)result, input.VertexCount);
        return true;
    }

    // Copy result
    output.Copy(outputStream.getData(), outputStream.getSize());

    return false;
}

bool CollisionCooking::CookTriangleMesh(CookingInput& input, BytesContainer& output)
{
    PROFILE_CPU();
    ENSURE_CAN_COOK;
    if (input.VertexCount == 0 || input.IndexCount == 0)
        LOG(Warning, "Empty mesh data for collision cooking.");

    // Init options
    PxTriangleMeshDesc desc;
    desc.points.count = input.VertexCount;
    desc.points.stride = sizeof(Float3);
    desc.points.data = input.VertexData;
    desc.triangles.count = input.IndexCount / 3;
    desc.triangles.stride = 3 * (input.Is16bitIndexData ? sizeof(uint16) : sizeof(uint32));
    desc.triangles.data = input.IndexData;
    desc.flags = input.Is16bitIndexData ? PxMeshFlag::e16_BIT_INDICES : (PxMeshFlag::Enum)0;
    PxCookingParams cookingParams = cooking->getParams();
    cookingParams.suppressTriangleMeshRemapTable = EnumHasAnyFlags(input.ConvexFlags, ConvexMeshGenerationFlags::SuppressFaceRemapTable);
    cooking->setParams(cookingParams);

    // Perform cooking
    PxDefaultMemoryOutputStream outputStream;
    PxTriangleMeshCookingResult::Enum result;
    if (!cooking->cookTriangleMesh(desc, outputStream, &result))
    {
        LOG(Warning, "Triangle Mesh cooking failed. Error code: {0}, Input vertices count: {1}, indices count: {2}", (int32)result, input.VertexCount, input.IndexCount);
        return true;
    }

    // Copy result
    output.Copy(outputStream.getData(), outputStream.getSize());

    return false;
}

bool CollisionCooking::CookHeightField(int32 cols, int32 rows, const PhysicsBackend::HeightFieldSample* data, WriteStream& stream)
{
    PROFILE_CPU();
    ENSURE_CAN_COOK;

    PxHeightFieldDesc heightFieldDesc;
    heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
    heightFieldDesc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES;
    heightFieldDesc.nbColumns = cols;
    heightFieldDesc.nbRows = rows;
    heightFieldDesc.samples.data = data;
    heightFieldDesc.samples.stride = sizeof(PhysicsBackend::HeightFieldSample);

    WriteStreamPhysX outputStream;
    outputStream.Stream = &stream;
    if (!Cooking->cookHeightField(heightFieldDesc, outputStream))
    {
        LOG(Warning, "Height Field collision cooking failed.");
        return true;
    }
    return false;
}

#endif

PxPhysics* PhysicsBackendPhysX::GetPhysics()
{
    return PhysX;
}

#if COMPILE_WITH_PHYSICS_COOKING

PxCooking* PhysicsBackendPhysX::GetCooking()
{
    return Cooking;
}

#endif

PxMaterial* PhysicsBackendPhysX::GetDefaultMaterial()
{
    return DefaultMaterial;
}

void PhysicsBackendPhysX::SimulationStepDone(PxScene* scene, float dt)
{
#if WITH_VEHICLE
    ScenePhysX* scenePhysX = nullptr;
    for (auto e : Physics::Scenes)
    {
        if (((ScenePhysX*)e->GetPhysicsScene())->Scene == scene)
        {
            scenePhysX = (ScenePhysX*)e->GetPhysicsScene();
            break;
        }
    }
    if (!scenePhysX)
        return;
    scenePhysX->UpdateVehicles(dt);
#endif
}

bool PhysicsBackend::Init()
{
#define CHECK_INIT(value, msg) if (!value) { LOG(Error, msg); return true; }
    auto& settings = *PhysicsSettings::Get();

    // Init PhysX foundation object
    LOG(Info, "Setup NVIDIA PhysX {0}.{1}.{2}", PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX);
    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, AllocatorCallback, ErrorCallback);
    CHECK_INIT(Foundation, "PxCreateFoundation failed!");

    // Init debugger
    PxPvd* pvd = nullptr;
#if WITH_PVD
    {
        // Init PVD
        pvd = PxCreatePvd(*Foundation);
        PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 100);
        //PxPvdTransport* transport = PxDefaultPvdFileTransportCreate("D:\\physx_sample.pxd2");
        if (transport)
        {
            const bool isConnected = pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
            if (isConnected)
            {
                LOG(Info, "Connected to PhysX Visual Debugger (PVD)");
            }
        }
        PVD = pvd;
    }

#endif

    // Init PhysX
    ToleranceScale.length = 100;
    ToleranceScale.speed = 981;
    PhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, ToleranceScale, false, pvd);
    CHECK_INIT(PhysX, "PxCreatePhysics failed!");

    // Init extensions
    const bool extensionsInit = PxInitExtensions(*PhysX, pvd);
    CHECK_INIT(extensionsInit, "PxInitExtensions failed!");

    // Init collision cooking
#if COMPILE_WITH_PHYSICS_COOKING
#if !USE_EDITOR
    if (settings.SupportCookingAtRuntime)
#endif
    {
        PxCookingParams cookingParams(ToleranceScale);
        cookingParams.meshWeldTolerance = 0.1f; // 1mm precision
        cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
        Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, cookingParams);
        CHECK_INIT(Cooking, "PxCreateCooking failed!");
    }
#endif

    // Create default material
    DefaultMaterial = PhysX->createMaterial(0.7f, 0.7f, 0.3f);

    // Return origin 0,0,0 for invalid/null scenes
    SceneOrigins.Add((PxScene*)nullptr, Vector3::Zero);

    return false;
}

void PhysicsBackend::Shutdown()
{
    // Remove all scenes still registered
    const int32 numScenes = PhysX ? PhysX->getNbScenes() : 0;
    if (numScenes)
    {
        Array<PxScene*> scenes;
        scenes.Resize(numScenes);
        scenes.SetAll(nullptr);
        PhysX->getScenes(scenes.Get(), sizeof(PxScene*) * numScenes);
        for (PxScene* scene : scenes)
        {
            if (scene)
                scene->release();
        }
    }

    // Cleanup any resources
#if WITH_VEHICLE
    RELEASE_PHYSX(WheelTireFrictions);
    WheelVehiclesResultsPerWheel.Resize(0);
    WheelVehiclesResultsPerVehicle.Resize(0);
#endif
    RELEASE_PHYSX(DefaultMaterial);

    // Shutdown PhysX
#if WITH_CLOTH
    if (ClothFactory)
    {
        NvClothDestroyFactory(ClothFactory);
        ClothFactory = nullptr;
    }
#endif
#if WITH_VEHICLE
    if (VehicleSDKInitialized)
    {
        VehicleSDKInitialized = false;
        PxCloseVehicleSDK();
    }
#endif
#if COMPILE_WITH_PHYSICS_COOKING
    RELEASE_PHYSX(Cooking);
#endif
    if (PhysX)
    {
        PxCloseExtensions();
        PhysX->release();
        PhysX = nullptr;
    }
#if WITH_PVD
    RELEASE_PHYSX(PVD);
#endif
    RELEASE_PHYSX(Foundation);
    SceneOrigins.Clear();
}

void PhysicsBackend::ApplySettings(const PhysicsSettings& settings)
{
    _queriesHitTriggers = settings.QueriesHitTriggers;
    _enableCCD = !settings.DisableCCD;
    _frictionCombineMode = settings.FrictionCombineMode;
    _restitutionCombineMode = settings.RestitutionCombineMode;

    // TODO: update all shapes filter data
    // TODO: update all shapes flags

    /*
    {
        get all actors and then:
        
        const PxU32 numShapes = actor->getNbShapes();
        PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
        actor->getShapes(shapes, numShapes);
        for (PxU32 i = 0; i < numShapes; i++)
        {
            ..
        }
        SAMPLE_FREE(shapes);
    }*/
}

void* PhysicsBackend::CreateScene(const PhysicsSettings& settings)
{
#define CHECK_INIT(value, msg) if (!value) { LOG(Error, msg); return nullptr; }
    auto scenePhysX = New<ScenePhysX>();

    // Create scene description
    PxSceneDesc sceneDesc(ToleranceScale);
    sceneDesc.gravity = C2P(settings.DefaultGravity);
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    if (!settings.DisableCCD)
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.simulationEventCallback = &scenePhysX->EventsCallback;
    sceneDesc.filterShader = FilterShader;
    sceneDesc.bounceThresholdVelocity = settings.BounceThresholdVelocity;
    if (settings.EnableEnhancedDeterminism)
        sceneDesc.flags |= PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
    switch (settings.SolverType)
    {
    case PhysicsSolverType::ProjectedGaussSeidelIterativeSolver:
        sceneDesc.solverType = PxSolverType::ePGS;
        break;
    case PhysicsSolverType::TemporalGaussSeidelSolver:
        sceneDesc.solverType = PxSolverType::eTGS;
        break;
    }
    if (sceneDesc.cpuDispatcher == nullptr)
    {
        scenePhysX->CpuDispatcher = PxDefaultCpuDispatcherCreate(Math::Clamp<uint32>(Platform::GetCPUInfo().ProcessorCoreCount - 1, 1, 4));
        CHECK_INIT(scenePhysX->CpuDispatcher, "PxDefaultCpuDispatcherCreate failed!");
        sceneDesc.cpuDispatcher = scenePhysX->CpuDispatcher;
    }
    switch (settings.BroadPhaseType)
    {
    case PhysicsBroadPhaseType::SweepAndPrune:
        sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;
        break;
    case PhysicsBroadPhaseType::MultiBoxPruning:
        sceneDesc.broadPhaseType = PxBroadPhaseType::eMBP;
        break;
    case PhysicsBroadPhaseType::AutomaticBoxPruning:
        sceneDesc.broadPhaseType = PxBroadPhaseType::eABP;
        break;
    case PhysicsBroadPhaseType::ParallelAutomaticBoxPruning:
        sceneDesc.broadPhaseType = PxBroadPhaseType::ePABP;
        break;
    }

    // Create scene
    scenePhysX->Scene = PhysX->createScene(sceneDesc);
    CHECK_INIT(scenePhysX->Scene, "createScene failed!");
    SceneOrigins[scenePhysX->Scene] = Vector3::Zero;
#if WITH_PVD
    auto pvdClient = scenePhysX->Scene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES | PxPvdSceneFlag::eTRANSMIT_CONTACTS);
    }
    else
    {
        LOG(Info, "Missing PVD client scene.");
    }
#endif

    // Init characters controller
    scenePhysX->ControllerManager = PxCreateControllerManager(*scenePhysX->Scene);

#undef CHECK_INIT
    return scenePhysX;
}

void PhysicsBackend::DestroyScene(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;

    // Flush any latent actions related to this scene
    FlushRequests(scene);

    // Release resources
    SceneOrigins.Remove(scenePhysX->Scene);
#if WITH_VEHICLE
    RELEASE_PHYSX(scenePhysX->WheelRaycastBatchQuery);
    scenePhysX->WheelRaycastBatchQuerySize = 0;
#endif
#if WITH_CLOTH
    if (scenePhysX->ClothSolver)
    {
        NV_CLOTH_DELETE(scenePhysX->ClothSolver);
        scenePhysX->ClothSolver = nullptr;
    }
#endif
    RELEASE_PHYSX(scenePhysX->ControllerManager);
    SAFE_DELETE(scenePhysX->CpuDispatcher);
    Allocator::Free(scenePhysX->ScratchMemory);
    scenePhysX->Scene->release();

    Delete(scenePhysX);
}

void PhysicsBackend::StartSimulateScene(void* scene, float dt)
{
    auto scenePhysX = (ScenePhysX*)scene;
    const auto& settings = *PhysicsSettings::Get();

    // Clamp delta
    dt = Math::Clamp(dt, 0.0f, settings.MaxDeltaTime);

    // Prepare util objects
    if (scenePhysX->ScratchMemory == nullptr)
    {
        scenePhysX->ScratchMemory = Allocator::Allocate(PHYSX_SCRATCH_BLOCK_SIZE, 16);
    }
    if (settings.EnableSubstepping)
    {
        // Use substeps
        scenePhysX->Stepper.Setup(settings.SubstepDeltaTime, settings.MaxSubsteps);
    }
    else
    {
        // Use single step
        scenePhysX->Stepper.Setup(dt);
    }

    // Start simulation (may not be fired due to too small delta time)
    if (scenePhysX->Stepper.advance(scenePhysX->Scene, dt, scenePhysX->ScratchMemory, PHYSX_SCRATCH_BLOCK_SIZE) == false)
        return;
    scenePhysX->EventsCallback.Clear();
    scenePhysX->LastDeltaTime = dt;

    // TODO: move this call after rendering done
    scenePhysX->Stepper.renderDone();
}

void PhysicsBackend::EndSimulateScene(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;

    {
        PROFILE_CPU_NAMED("Physics.Fetch");

        // Gather results (with waiting for the end)
        scenePhysX->Stepper.wait(scenePhysX->Scene);
    }

    {
        PROFILE_CPU_NAMED("Physics.FlushActiveTransforms");

        // Gather change info
        PxU32 activeActorsCount;
        PxActor** activeActors = scenePhysX->Scene->getActiveActors(activeActorsCount);
        if (activeActorsCount > 0)
        {
            // Update changed transformations
            // TODO: use jobs system if amount if huge
            for (uint32 i = 0; i < activeActorsCount; i++)
            {
                const auto pxActor = (PxRigidActor*)*activeActors++;
                auto actor = static_cast<IPhysicsActor*>(pxActor->userData);
                if (actor)
                    actor->OnActiveTransformChanged();
            }
        }
    }

#if WITH_CLOTH
    scenePhysX->UpdateCloths(scenePhysX->LastDeltaTime);
#endif

    {
        PROFILE_CPU_NAMED("Physics.SendEvents");
        scenePhysX->EventsCallback.SendTriggerEvents();
        scenePhysX->EventsCallback.SendCollisionEvents();
        scenePhysX->EventsCallback.SendJointEvents();
    }

    // Clear delta after simulation ended
    scenePhysX->LastDeltaTime = 0.0f;
}

Vector3 PhysicsBackend::GetSceneGravity(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;
    return P2C(scenePhysX->Scene->getGravity());
}

void PhysicsBackend::SetSceneGravity(void* scene, const Vector3& value)
{
    auto scenePhysX = (ScenePhysX*)scene;
    scenePhysX->Scene->setGravity(C2P(value));
#if WITH_CLOTH
    if (scenePhysX->ClothSolver)
    {
        const int32 clothsCount = scenePhysX->ClothSolver->getNumCloths();
        nv::cloth::Cloth* const* cloths = scenePhysX->ClothSolver->getClothList();
        for (int32 i = 0; i < clothsCount; i++)
        {
            auto clothPhysX = (nv::cloth::Cloth*)cloths[i];
            const auto& clothSettings = Cloths[clothPhysX];
            clothPhysX->setGravity(C2P(value * clothSettings.GravityScale));
        }
    }
#endif
}

bool PhysicsBackend::GetSceneEnableCCD(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;
    return (scenePhysX->Scene->getFlags() & PxSceneFlag::eENABLE_CCD) == PxSceneFlag::eENABLE_CCD;
}

void PhysicsBackend::SetSceneEnableCCD(void* scene, bool value)
{
    auto scenePhysX = (ScenePhysX*)scene;
    scenePhysX->Scene->setFlag(PxSceneFlag::eENABLE_CCD, value);
}

float PhysicsBackend::GetSceneBounceThresholdVelocity(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;
    return scenePhysX->Scene->getBounceThresholdVelocity();
}

void PhysicsBackend::SetSceneBounceThresholdVelocity(void* scene, float value)
{
    auto scenePhysX = (ScenePhysX*)scene;
    scenePhysX->Scene->setBounceThresholdVelocity(value);
}

void PhysicsBackend::SetSceneOrigin(void* scene, const Vector3& oldOrigin, const Vector3& newOrigin)
{
    auto scenePhysX = (ScenePhysX*)scene;
    const PxVec3 shift = C2P(newOrigin - oldOrigin);
    scenePhysX->Origin = newOrigin;
    scenePhysX->Scene->shiftOrigin(shift);
    scenePhysX->ControllerManager->shiftOrigin(shift);
#if WITH_VEHICLE
    WheelVehiclesCache.Clear();
    for (auto wheelVehicle : scenePhysX->WheelVehicles)
    {
        if (!wheelVehicle->IsActiveInHierarchy())
            continue;
        auto drive = (PxVehicleWheels*)wheelVehicle->_vehicle;
        ASSERT(drive);
        WheelVehiclesCache.Add(drive);
    }
    PxVehicleShiftOrigin(shift, WheelVehiclesCache.Count(), WheelVehiclesCache.Get());
#endif
#if WITH_CLOTH
    if (scenePhysX->ClothSolver)
    {
        const int32 clothsCount = scenePhysX->ClothSolver->getNumCloths();
        nv::cloth::Cloth* const* cloths = scenePhysX->ClothSolver->getClothList();
        for (int32 i = 0; i < clothsCount; i++)
        {
            cloths[i]->teleport(shift);
        }
    }
#endif
    SceneOrigins[scenePhysX->Scene] = newOrigin;
}

void PhysicsBackend::AddSceneActor(void* scene, void* actor)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();
    scenePhysX->Scene->addActor(*(PxActor*)actor);
    FlushLocker.Unlock();
}

void PhysicsBackend::RemoveSceneActor(void* scene, void* actor, bool immediately)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();
    if (immediately)
        scenePhysX->Scene->removeActor(*(PxActor*)actor);
    else
        scenePhysX->RemoveActors.Add((PxActor*)actor);
    FlushLocker.Unlock();
}

void PhysicsBackend::AddSceneActorAction(void* scene, void* actor, ActionType action)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();
    auto& a = scenePhysX->Actions.AddOne();
    a.Actor = (PxActor*)actor;
    a.Type = action;
    FlushLocker.Unlock();
}

#if COMPILE_WITH_PROFILER

void PhysicsBackend::GetSceneStatistics(void* scene, PhysicsStatistics& result)
{
    PROFILE_CPU_NAMED("Physics.Statistics");
    auto scenePhysX = (ScenePhysX*)scene;

    PxSimulationStatistics px;
    scenePhysX->Scene->getSimulationStatistics(px);

    result.ActiveDynamicBodies = px.nbActiveDynamicBodies;
    result.ActiveKinematicBodies = px.nbActiveKinematicBodies;
    result.ActiveJoints = px.nbActiveConstraints;
    result.StaticBodies = px.nbStaticBodies;
    result.DynamicBodies = px.nbDynamicBodies;
    result.KinematicBodies = px.nbKinematicBodies;
    result.NewPairs = px.nbNewPairs;
    result.LostPairs = px.nbLostPairs;
    result.NewTouches = px.nbNewTouches;
    result.LostTouches = px.nbLostTouches;
}

#endif

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;
    return scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, PxHitFlagEmpty, filterData, &QueryFilter);
}

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;
    if (!scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::RayCastAll(void* scene, const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(false);
    DynamicHitBuffer<PxRaycastHit> buffer;
    if (!scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, PxHitFlagEmpty, filterData, &QueryFilter);
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::BoxCastAll(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, PxHitFlagEmpty, filterData, &QueryFilter);
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::SphereCastAll(void* scene, const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, PxHitFlagEmpty, filterData, &QueryFilter);
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::CapsuleCastAll(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, PxHitFlagEmpty, filterData, &QueryFilter);
}

bool PhysicsBackend::ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::ConvexCastAll(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, SCENE_QUERY_FLAGS, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::CheckBox(void* scene, const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    return scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter);
}

bool PhysicsBackend::CheckSphere(void* scene, const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    return scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter);
}

bool PhysicsBackend::CheckCapsule(void* scene, const Vector3& center, const float radius, const float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    return scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter);
}

bool PhysicsBackend::CheckConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_OVERLAP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    return scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter);
}

bool PhysicsBackend::OverlapBox(void* scene, const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP();
    return true;
}

bool PhysicsBackend::OverlapSphere(void* scene, const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP();
    return true;
}

bool PhysicsBackend::OverlapCapsule(void* scene, const Vector3& center, const float radius, const float height, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP();
    return true;
}

bool PhysicsBackend::OverlapConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP();
    return true;
}

PhysicsBackend::ActorFlags PhysicsBackend::GetActorFlags(void* actor)
{
    auto actorPhysX = (PxActor*)actor;
    auto flags = actorPhysX->getActorFlags();
    auto result = ActorFlags::None;
    if (flags & PxActorFlag::eDISABLE_GRAVITY)
        result |= ActorFlags::NoGravity;
    if (flags & PxActorFlag::eDISABLE_SIMULATION)
        result |= ActorFlags::NoSimulation;
    return result;
}

void PhysicsBackend::SetActorFlags(void* actor, ActorFlags value)
{
    auto actorPhysX = (PxActor*)actor;
    auto flags = (PxActorFlags)0;
#if WITH_PVD
    flags |= PxActorFlag::eVISUALIZATION;
#endif
    if (EnumHasAnyFlags(value, ActorFlags::NoGravity))
        flags |= PxActorFlag::eDISABLE_GRAVITY;
    if (EnumHasAnyFlags(value, ActorFlags::NoSimulation))
        flags |= PxActorFlag::eDISABLE_SIMULATION;
    actorPhysX->setActorFlags(flags);
}

void PhysicsBackend::GetActorBounds(void* actor, BoundingBox& bounds)
{
    auto actorPhysX = (PxActor*)actor;
    const float boundsScale = 1.02f;
    bounds = P2C(actorPhysX->getWorldBounds(boundsScale));
    const Vector3 sceneOrigin = SceneOrigins[actorPhysX->getScene()];
    bounds.Minimum += sceneOrigin;
    bounds.Maximum += sceneOrigin;
}

int32 PhysicsBackend::GetRigidActorShapesCount(void* actor)
{
    auto actorPhysX = (PxRigidActor*)actor;
    return actorPhysX->getNbShapes();
}

void* PhysicsBackend::CreateRigidDynamicActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene)
{
    const Vector3 sceneOrigin = SceneOrigins[scene ? ((ScenePhysX*)scene)->Scene : nullptr];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    PxRigidDynamic* actorPhysX = PhysX->createRigidDynamic(trans);
    actorPhysX->userData = actor;
#if PHYSX_DEBUG_NAMING
    actorPhysX->setName("RigidDynamicActor");
#endif
#if WITH_PVD
    actorPhysX->setActorFlag(PxActorFlag::eVISUALIZATION, true);
#endif
    return actorPhysX;
}

void* PhysicsBackend::CreateRigidStaticActor(IPhysicsActor* actor, const Vector3& position, const Quaternion& orientation, void* scene)
{
    const Vector3 sceneOrigin = SceneOrigins[scene ? ((ScenePhysX*)scene)->Scene : nullptr];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    PxRigidStatic* actorPhysX = PhysX->createRigidStatic(trans);
    actorPhysX->userData = actor;
#if PHYSX_DEBUG_NAMING
    actorPhysX->setName("RigidStaticActor");
#endif
#if WITH_PVD
    actorPhysX->setActorFlag(PxActorFlag::eVISUALIZATION, true);
#endif
    return actorPhysX;
}

PhysicsBackend::RigidDynamicFlags PhysicsBackend::GetRigidDynamicActorFlags(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    auto flags = actorPhysX->getRigidBodyFlags();
    auto result = RigidDynamicFlags::None;
    if (flags & PxRigidBodyFlag::eKINEMATIC)
        result |= RigidDynamicFlags::Kinematic;
    if (flags & PxRigidBodyFlag::eENABLE_CCD)
        result |= RigidDynamicFlags::CCD;
    return result;
}

void PhysicsBackend::SetRigidDynamicActorFlags(void* actor, RigidDynamicFlags value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    auto flags = (PxRigidBodyFlags)0;
    if (EnumHasAnyFlags(value, RigidDynamicFlags::Kinematic))
        flags |= PxRigidBodyFlag::eKINEMATIC;
    if (EnumHasAnyFlags(value, RigidDynamicFlags::CCD))
        flags |= PxRigidBodyFlag::eENABLE_CCD;
    actorPhysX->setRigidBodyFlags(flags);
}

void PhysicsBackend::GetRigidActorPose(void* actor, Vector3& position, Quaternion& orientation)
{
    auto actorPhysX = (PxRigidActor*)actor;
    auto pose = actorPhysX->getGlobalPose();
    const Vector3 sceneOrigin = SceneOrigins[actorPhysX->getScene()];
    position = P2C(pose.p) + sceneOrigin;
    orientation = P2C(pose.q);
}

void PhysicsBackend::SetRigidActorPose(void* actor, const Vector3& position, const Quaternion& orientation, bool kinematic, bool wakeUp)
{
    const Vector3 sceneOrigin = SceneOrigins[((PxActor*)actor)->getScene()];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    if (kinematic)
    {
        auto actorPhysX = (PxRigidDynamic*)actor;
        if (actorPhysX->getActorFlags() & PxActorFlag::eDISABLE_SIMULATION)
        {
            // Ensures the disabled kinematic actor ends up in the correct pose after enabling simulation
            actorPhysX->setGlobalPose(trans, wakeUp);
        }
        else
            actorPhysX->setKinematicTarget(trans);
    }
    else
    {
        auto actorPhysX = (PxRigidActor*)actor;
        actorPhysX->setGlobalPose(trans, wakeUp);
    }
}

void PhysicsBackend::SetRigidDynamicActorLinearDamping(void* actor, float value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setLinearDamping(value);
}

void PhysicsBackend::SetRigidDynamicActorAngularDamping(void* actor, float value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setAngularDamping(value);
}

void PhysicsBackend::SetRigidDynamicActorMaxAngularVelocity(void* actor, float value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setMaxAngularVelocity(value);
}

void PhysicsBackend::SetRigidDynamicActorConstraints(void* actor, RigidbodyConstraints value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setRigidDynamicLockFlags(static_cast<PxRigidDynamicLockFlag::Enum>(value));
}

Vector3 PhysicsBackend::GetRigidDynamicActorLinearVelocity(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return P2C(actorPhysX->getLinearVelocity());
}

void PhysicsBackend::SetRigidDynamicActorLinearVelocity(void* actor, const Vector3& value, bool wakeUp)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setLinearVelocity(C2P(value), wakeUp);
}

Vector3 PhysicsBackend::GetRigidDynamicActorAngularVelocity(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return P2C(actorPhysX->getAngularVelocity());
}

void PhysicsBackend::SetRigidDynamicActorAngularVelocity(void* actor, const Vector3& value, bool wakeUp)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setAngularVelocity(C2P(value), wakeUp);
}

Vector3 PhysicsBackend::GetRigidDynamicActorCenterOfMass(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return P2C(actorPhysX->getCMassLocalPose().p);
}

void PhysicsBackend::SetRigidDynamicActorCenterOfMassOffset(void* actor, const Float3& value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    auto pose = actorPhysX->getCMassLocalPose();
    pose.p += C2P(value);
    actorPhysX->setCMassLocalPose(pose);
}

bool PhysicsBackend::GetRigidDynamicActorIsSleeping(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return actorPhysX->isSleeping();
}

void PhysicsBackend::RigidDynamicActorSleep(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->putToSleep();
}

void PhysicsBackend::RigidDynamicActorWakeUp(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->wakeUp();
}

float PhysicsBackend::GetRigidDynamicActorSleepThreshold(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return actorPhysX->getSleepThreshold();
}

void PhysicsBackend::SetRigidDynamicActorSleepThreshold(void* actor, float value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setSleepThreshold(value);
}

float PhysicsBackend::GetRigidDynamicActorMaxDepenetrationVelocity(void* actor)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    return actorPhysX->getMaxDepenetrationVelocity();
}

void PhysicsBackend::SetRigidDynamicActorMaxDepenetrationVelocity(void* actor, float value)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setMaxDepenetrationVelocity(value);
}

void PhysicsBackend::SetRigidDynamicActorSolverIterationCounts(void* actor, int32 minPositionIters, int32 minVelocityIters)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->setSolverIterationCounts(Math::Clamp(minPositionIters, 1, 255), Math::Clamp(minVelocityIters, 1, 255));
}

void PhysicsBackend::UpdateRigidDynamicActorMass(void* actor, float& mass, float massScale, bool autoCalculate)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    if (autoCalculate)
    {
        // Calculate per-shape densities (convert kg/m^3 into engine units)
        const float minDensity = 0.08375f; // Hydrogen density
        const float defaultDensity = 1000.0f; // Water density
        Array<float, InlinedAllocation<32>> densities;
        for (uint32 i = 0; i < actorPhysX->getNbShapes(); i++)
        {
            PxShape* shape;
            actorPhysX->getShapes(&shape, 1, i);
            if (shape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE)
            {
                float density = defaultDensity;
                PxMaterial* material;
                if (shape->getMaterials(&material, 1, 0) == 1)
                {
                    if (const auto mat = (PhysicalMaterial*)material->userData)
                    {
                        density = Math::Max(mat->Density, minDensity);
                    }
                }
                densities.Add(KgPerM3ToKgPerCm3(density));
            }
        }
        if (densities.IsEmpty())
            densities.Add(KgPerM3ToKgPerCm3(defaultDensity));

        // Auto calculated mass
        PxRigidBodyExt::updateMassAndInertia(*actorPhysX, densities.Get(), densities.Count());
        mass = actorPhysX->getMass();
        massScale = Math::Max(massScale, 0.001f);
        if (!Math::IsOne(massScale))
        {
            mass *= massScale;
            actorPhysX->setMass(mass);
        }
    }
    else
    {
        // Use fixed mass
        PxRigidBodyExt::setMassAndUpdateInertia(*actorPhysX, Math::Max(mass * massScale, 0.001f));
    }
}

void PhysicsBackend::AddRigidDynamicActorForce(void* actor, const Vector3& force, ForceMode mode)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->addForce(C2P(force), static_cast<PxForceMode::Enum>(mode));
}

void PhysicsBackend::AddRigidDynamicActorForceAtPosition(void* actor, const Vector3& force, const Vector3& position, ForceMode mode)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    const Vector3 sceneOrigin = SceneOrigins[actorPhysX->getScene()];
    PxRigidBodyExt::addForceAtPos(*actorPhysX, C2P(force), C2P(position - sceneOrigin), static_cast<PxForceMode::Enum>(mode));
}

void PhysicsBackend::AddRigidDynamicActorTorque(void* actor, const Vector3& torque, ForceMode mode)
{
    auto actorPhysX = (PxRigidDynamic*)actor;
    actorPhysX->addTorque(C2P(torque), static_cast<PxForceMode::Enum>(mode));
}

void* PhysicsBackend::CreateShape(PhysicsColliderActor* collider, const CollisionShape& geometry, Span<JsonAsset*> materials, bool enabled, bool trigger)
{
    const PxShapeFlags shapeFlags = GetShapeFlags(trigger, enabled);
    Array<PxMaterial*, InlinedAllocation<1>> materialsPhysX;
    GetShapeMaterials(materialsPhysX, materials);
    PxGeometryHolder geometryPhysX;
    GetShapeGeometry(geometry, geometryPhysX);
    PxShape* shapePhysX = PhysX->createShape(geometryPhysX.any(), materialsPhysX.Get(), materialsPhysX.Count(), true, shapeFlags);
    shapePhysX->userData = collider;
#if PHYSX_DEBUG_NAMING
    shapePhysX->setName("Shape");
#endif
    return shapePhysX;
}

void PhysicsBackend::SetShapeState(void* shape, bool enabled, bool trigger)
{
    auto shapePhysX = (PxShape*)shape;
    const PxShapeFlags shapeFlags = GetShapeFlags(trigger, enabled);
    shapePhysX->setFlags(shapeFlags);
}

void PhysicsBackend::SetShapeFilterMask(void* shape, uint32 mask0, uint32 mask1)
{
    auto shapePhysX = (PxShape*)shape;
    PxFilterData filterData;
    filterData.word0 = mask0;
    filterData.word1 = mask1;
    shapePhysX->setSimulationFilterData(filterData);
    shapePhysX->setQueryFilterData(filterData);
}

void* PhysicsBackend::GetShapeActor(void* shape)
{
    auto shapePhysX = (PxShape*)shape;
    return shapePhysX->getActor();
}

void PhysicsBackend::GetShapePose(void* shape, Vector3& position, Quaternion& orientation)
{
    auto shapePhysX = (PxShape*)shape;
    PxRigidActor* actorPhysX = shapePhysX->getActor();
    PxTransform pose = actorPhysX->getGlobalPose().transform(shapePhysX->getLocalPose());
    const Vector3 sceneOrigin = SceneOrigins[actorPhysX->getScene()];
    position = P2C(pose.p) + sceneOrigin;
    orientation = P2C(pose.q);
}

CollisionShape::Types PhysicsBackend::GetShapeType(void* shape)
{
    auto shapePhysX = (PxShape*)shape;
    CollisionShape::Types type;
    switch (shapePhysX->getGeometryType())
    {
    case PxGeometryType::eSPHERE:
        type = CollisionShape::Types::Sphere;
        break;
    case PxGeometryType::eCAPSULE:
        type = CollisionShape::Types::Capsule;
        break;
    case PxGeometryType::eBOX:
        type = CollisionShape::Types::Box;
        break;
    case PxGeometryType::eCONVEXMESH:
        type = CollisionShape::Types::ConvexMesh;
        break;
    case PxGeometryType::eTRIANGLEMESH:
        type = CollisionShape::Types::TriangleMesh;
        break;
    case PxGeometryType::eHEIGHTFIELD:
        type = CollisionShape::Types::HeightField;
        break;
    default: ;
    }
    return type;
}

void PhysicsBackend::GetShapeLocalPose(void* shape, Vector3& position, Quaternion& orientation)
{
    auto shapePhysX = (PxShape*)shape;
    auto pose = shapePhysX->getLocalPose();
    position = P2C(pose.p);
    orientation = P2C(pose.q);
}

void PhysicsBackend::SetShapeLocalPose(void* shape, const Vector3& position, const Quaternion& orientation)
{
    auto shapePhysX = (PxShape*)shape;
    shapePhysX->setLocalPose(PxTransform(C2P(position), C2P(orientation)));
}

void PhysicsBackend::SetShapeContactOffset(void* shape, float value)
{
    auto shapePhysX = (PxShape*)shape;
    shapePhysX->setContactOffset(Math::Max(shapePhysX->getRestOffset() + ZeroTolerance, value));
}

void PhysicsBackend::SetShapeMaterials(void* shape, Span<JsonAsset*> materials)
{
    auto shapePhysX = (PxShape*)shape;
    Array<PxMaterial*, InlinedAllocation<1>> materialsPhysX;
    GetShapeMaterials(materialsPhysX, materials);
    shapePhysX->setMaterials(materialsPhysX.Get(), materialsPhysX.Count());
}

void PhysicsBackend::SetShapeGeometry(void* shape, const CollisionShape& geometry)
{
    auto shapePhysX = (PxShape*)shape;
    PxGeometryHolder geometryPhysX;
    GetShapeGeometry(geometry, geometryPhysX);
    shapePhysX->setGeometry(geometryPhysX.any());
}

void PhysicsBackend::AttachShape(void* shape, void* actor)
{
    auto shapePhysX = (PxShape*)shape;
    auto actorPhysX = (PxRigidActor*)actor;
    actorPhysX->attachShape(*shapePhysX);
}

void PhysicsBackend::DetachShape(void* shape, void* actor)
{
    auto shapePhysX = (PxShape*)shape;
    auto actorPhysX = (PxRigidActor*)actor;
    actorPhysX->detachShape(*shapePhysX);
}

bool PhysicsBackend::ComputeShapesPenetration(void* shapeA, void* shapeB, const Vector3& positionA, const Quaternion& orientationA, const Vector3& positionB, const Quaternion& orientationB, Vector3& direction, float& distance)
{
    auto shapeAPhysX = (PxShape*)shapeA;
    auto shapeBPhysX = (PxShape*)shapeB;
    const PxTransform poseA(C2P(positionA), C2P(orientationA));
    const PxTransform poseB(C2P(positionB), C2P(orientationB));
    PxVec3 dir = C2P(direction);
    const bool result = PxGeometryQuery::computePenetration(dir, distance, shapeAPhysX->getGeometry(), poseA, shapeBPhysX->getGeometry(), poseB);
    direction = P2C(dir);
    return result;
}

float PhysicsBackend::ComputeShapeSqrDistanceToPoint(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& point, Vector3* closestPoint)
{
    auto shapePhysX = (PxShape*)shape;
    const PxTransform trans(C2P(position), C2P(orientation));
#if USE_LARGE_WORLDS
    PxVec3 closestPointPx;
    float result = PxGeometryQuery::pointDistance(C2P(point), shapePhysX->getGeometry(), trans, &closestPointPx);
    *closestPoint = P2C(closestPointPx);
    return result;
#else
    return PxGeometryQuery::pointDistance(C2P(point), shapePhysX->getGeometry(), trans, (PxVec3*)closestPoint);
#endif
}

bool PhysicsBackend::RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance)
{
    auto shapePhysX = (PxShape*)shape;
    const Vector3 sceneOrigin = SceneOrigins[shapePhysX->getActor() ? shapePhysX->getActor()->getScene() : nullptr];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    PxRaycastHit hit;
    if (PxGeometryQuery::raycast(C2P(origin - sceneOrigin), C2P(direction), shapePhysX->getGeometry(), trans, maxDistance, PxHitFlagEmpty, 1, &hit) != 0)
    {
        resultHitDistance = hit.distance;
        return true;
    }
    return false;
}

bool PhysicsBackend::RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance)
{
    auto shapePhysX = (PxShape*)shape;
    const Vector3 sceneOrigin = SceneOrigins[shapePhysX->getActor() ? shapePhysX->getActor()->getScene() : nullptr];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    PxRaycastHit hit;
    if (PxGeometryQuery::raycast(C2P(origin - sceneOrigin), C2P(direction), shapePhysX->getGeometry(), trans, maxDistance, SCENE_QUERY_FLAGS, 1, &hit) == 0)
        return false;
    hit.shape = shapePhysX;
    P2C(hit, hitInfo);
    hitInfo.Point += sceneOrigin;
    return true;
}

void PhysicsBackend::SetJointFlags(void* joint, JointFlags value)
{
    auto jointPhysX = (PxJoint*)joint;
    jointPhysX->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, EnumHasAnyFlags(value, JointFlags::Collision));
}

void PhysicsBackend::SetJointActors(void* joint, void* actors0, void* actor1)
{
    auto jointPhysX = (PxJoint*)joint;
    jointPhysX->setActors((PxRigidActor*)actors0, (PxRigidActor*)actor1);
}

void PhysicsBackend::SetJointActorPose(void* joint, const Vector3& position, const Quaternion& orientation, uint8 index)
{
    auto jointPhysX = (PxJoint*)joint;
    jointPhysX->setLocalPose((PxJointActorIndex::Enum)index, PxTransform(C2P(position), C2P(orientation)));
}

void PhysicsBackend::SetJointBreakForce(void* joint, float force, float torque)
{
    auto jointPhysX = (PxJoint*)joint;
    jointPhysX->setBreakForce(force, torque);
}

void PhysicsBackend::GetJointForce(void* joint, Vector3& linear, Vector3& angular)
{
    auto jointPhysX = (PxJoint*)joint;
    if (jointPhysX->getConstraint())
    {
        PxVec3 linearPhysX = C2P(linear), angularPhysX = C2P(angular);
        jointPhysX->getConstraint()->getForce(linearPhysX, angularPhysX);
        linear = P2C(linearPhysX);
        angular = P2C(angularPhysX);
    }
    else
    {
        linear = Vector3::Zero;
        angular = Vector3::Zero;
    }
}

void* PhysicsBackend::CreateFixedJoint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxFixedJoint* joint = PxFixedJointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("FixedJoint");
#endif
    return joint;
}

void* PhysicsBackend::CreateDistanceJoint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxDistanceJoint* joint = PxDistanceJointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("DistanceJoint");
#endif
    return joint;
}

void* PhysicsBackend::CreateHingeJoint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxRevoluteJoint* joint = PxRevoluteJointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("HingeJoint");
#endif
    return joint;
}

void* PhysicsBackend::CreateSliderJoint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxPrismaticJoint* joint = PxPrismaticJointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("SliderJoint");
#endif
    return joint;
}

void* PhysicsBackend::CreateSphericalJoint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxSphericalJoint* joint = PxSphericalJointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("SphericalJoint");
#endif
    return joint;
}

void* PhysicsBackend::CreateD6Joint(const PhysicsJointDesc& desc)
{
    const PxTransform trans0(C2P(desc.Pos0), C2P(desc.Rot0));
    const PxTransform trans1(C2P(desc.Pos1), C2P(desc.Rot1));
    PxD6Joint* joint = PxD6JointCreate(*PhysX, (PxRigidActor*)desc.Actor0, trans0, (PxRigidActor*)desc.Actor1, trans1);
    joint->userData = desc.Joint;
#if PHYSX_DEBUG_NAMING
    joint->setName("D6Joint");
#endif
    return joint;
}

void PhysicsBackend::SetDistanceJointFlags(void* joint, DistanceJointFlag flags)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    jointPhysX->setDistanceJointFlags((PxDistanceJointFlag::Enum)flags);
}

void PhysicsBackend::SetDistanceJointMinDistance(void* joint, float value)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    jointPhysX->setMinDistance(value);
}

void PhysicsBackend::SetDistanceJointMaxDistance(void* joint, float value)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    jointPhysX->setMaxDistance(value);
}

void PhysicsBackend::SetDistanceJointTolerance(void* joint, float value)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    jointPhysX->setTolerance(value);
}

void PhysicsBackend::SetDistanceJointSpring(void* joint, const SpringParameters& value)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    jointPhysX->setStiffness(value.Stiffness);
    jointPhysX->setDamping(value.Damping);
}

float PhysicsBackend::GetDistanceJointDistance(void* joint)
{
    auto jointPhysX = (PxDistanceJoint*)joint;
    return jointPhysX->getDistance();
}

void PhysicsBackend::SetHingeJointFlags(void* joint, HingeJointFlag value, bool driveFreeSpin)
{
    auto jointPhysX = (PxRevoluteJoint*)joint;
    PxRevoluteJointFlags flags = (PxRevoluteJointFlags)0;
    if (EnumHasAnyFlags(value, HingeJointFlag::Limit))
        flags |= PxRevoluteJointFlag::eLIMIT_ENABLED;
    if (EnumHasAnyFlags(value, HingeJointFlag::Drive))
        flags |= PxRevoluteJointFlag::eDRIVE_ENABLED;
    if (driveFreeSpin)
        flags |= PxRevoluteJointFlag::eDRIVE_FREESPIN;
    jointPhysX->setRevoluteJointFlags(flags);
}

void PhysicsBackend::SetHingeJointLimit(void* joint, const LimitAngularRange& value)
{
    auto jointPhysX = (PxRevoluteJoint*)joint;
    PxJointAngularLimitPair limit(value.Lower * DegreesToRadians, Math::Max(value.Upper, value.Lower) * DegreesToRadians, value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setLimit(limit);
}

void PhysicsBackend::SetHingeJointDrive(void* joint, const HingeJointDrive& value)
{
    auto jointPhysX = (PxRevoluteJoint*)joint;
    jointPhysX->setDriveVelocity(value.Velocity);
    jointPhysX->setDriveForceLimit(Math::Max(value.ForceLimit, 0.0f));
    jointPhysX->setDriveGearRatio(Math::Max(value.GearRatio, 0.0f));
    jointPhysX->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_FREESPIN, value.FreeSpin);
}

float PhysicsBackend::GetHingeJointAngle(void* joint)
{
    auto jointPhysX = (PxRevoluteJoint*)joint;
    return jointPhysX->getAngle();
}

float PhysicsBackend::GetHingeJointVelocity(void* joint)
{
    auto jointPhysX = (PxRevoluteJoint*)joint;
    return jointPhysX->getVelocity();
}

void PhysicsBackend::SetSliderJointFlags(void* joint, SliderJointFlag value)
{
    auto jointPhysX = (PxPrismaticJoint*)joint;
    jointPhysX->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, EnumHasAnyFlags(value, SliderJointFlag::Limit));
}

void PhysicsBackend::SetSliderJointLimit(void* joint, const LimitLinearRange& value)
{
    auto jointPhysX = (PxPrismaticJoint*)joint;
    PxJointLinearLimitPair limit(ToleranceScale, value.Lower, value.Upper, value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setLimit(limit);
}

float PhysicsBackend::GetSliderJointPosition(void* joint)
{
    auto jointPhysX = (PxPrismaticJoint*)joint;
    return jointPhysX->getPosition();
}

float PhysicsBackend::GetSliderJointVelocity(void* joint)
{
    auto jointPhysX = (PxPrismaticJoint*)joint;
    return jointPhysX->getVelocity();
}

void PhysicsBackend::SetSphericalJointFlags(void* joint, SphericalJointFlag value)
{
    auto jointPhysX = (PxSphericalJoint*)joint;
    jointPhysX->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, EnumHasAnyFlags(value, SphericalJointFlag::Limit));
}

void PhysicsBackend::SetSphericalJointLimit(void* joint, const LimitConeRange& value)
{
    auto jointPhysX = (PxSphericalJoint*)joint;
    PxJointLimitCone limit(Math::Clamp(value.YLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), Math::Clamp(value.ZLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setLimitCone(limit);
}

void PhysicsBackend::SetD6JointMotion(void* joint, D6JointAxis axis, D6JointMotion value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    jointPhysX->setMotion(static_cast<PxD6Axis::Enum>(axis), static_cast<PxD6Motion::Enum>(value));
}

void PhysicsBackend::SetD6JointDrive(void* joint, const D6JointDriveType index, const D6JointDrive& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxD6JointDrive drive;
    if (value.Acceleration)
        drive.flags = PxD6JointDriveFlag::eACCELERATION;
    drive.stiffness = value.Stiffness;
    drive.damping = value.Damping;
    drive.forceLimit = value.ForceLimit;
    ASSERT_LOW_LAYER(drive.isValid());
    jointPhysX->setDrive(static_cast<PxD6Drive::Enum>(index), drive);
}

void PhysicsBackend::SetD6JointLimitLinear(void* joint, const LimitLinear& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxJointLinearLimit limit(ToleranceScale, Math::Max(value.Extent, ZeroTolerance), value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setLinearLimit(limit);
}

void PhysicsBackend::SetD6JointLimitTwist(void* joint, const LimitAngularRange& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxJointAngularLimitPair limit(value.Lower * DegreesToRadians, Math::Max(value.Upper, value.Lower) * DegreesToRadians, value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setTwistLimit(limit);
}

void PhysicsBackend::SetD6JointLimitSwing(void* joint, const LimitConeRange& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxJointLimitCone limit(Math::Clamp(value.YLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), Math::Clamp(value.ZLimitAngle * DegreesToRadians, ZeroTolerance, PI - ZeroTolerance), value.ContactDist);
    limit.stiffness = value.Spring.Stiffness;
    limit.damping = value.Spring.Damping;
    limit.restitution = value.Restitution;
    ASSERT_LOW_LAYER(limit.isValid());
    jointPhysX->setSwingLimit(limit);
}

Vector3 PhysicsBackend::GetD6JointDrivePosition(void* joint)
{
    auto jointPhysX = (PxD6Joint*)joint;
    return P2C(jointPhysX->getDrivePosition().p);
}

void PhysicsBackend::SetD6JointDrivePosition(void* joint, const Vector3& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxTransform t = jointPhysX->getDrivePosition();
    t.p = C2P(value);
    jointPhysX->setDrivePosition(t);
}

Quaternion PhysicsBackend::GetD6JointDriveRotation(void* joint)
{
    auto jointPhysX = (PxD6Joint*)joint;
    return P2C(jointPhysX->getDrivePosition().q);
}

void PhysicsBackend::SetD6JointDriveRotation(void* joint, const Quaternion& value)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxTransform t = jointPhysX->getDrivePosition();
    t.q = C2P(value);
    jointPhysX->setDrivePosition(t);
}

void PhysicsBackend::GetD6JointDriveVelocity(void* joint, Vector3& linear, Vector3& angular)
{
    auto jointPhysX = (PxD6Joint*)joint;
    PxVec3 linearPhysX, angularPhysX;
    jointPhysX->getDriveVelocity(linearPhysX, angularPhysX);
    linear = P2C(linearPhysX);
    angular = P2C(angularPhysX);
}

void PhysicsBackend::SetD6JointDriveVelocity(void* joint, const Vector3& linear, const Vector3& angular)
{
    auto jointPhysX = (PxD6Joint*)joint;
    jointPhysX->setDriveVelocity(C2P(linear), C2P(angular));
}

float PhysicsBackend::GetD6JointTwist(void* joint)
{
    auto jointPhysX = (PxD6Joint*)joint;
    return jointPhysX->getTwistAngle();
}

float PhysicsBackend::GetD6JointSwingY(void* joint)
{
    auto jointPhysX = (PxD6Joint*)joint;
    return jointPhysX->getSwingYAngle();
}

float PhysicsBackend::GetD6JointSwingZ(void* joint)
{
    auto jointPhysX = (PxD6Joint*)joint;
    return jointPhysX->getSwingZAngle();
}

void* PhysicsBackend::CreateController(void* scene, IPhysicsActor* actor, PhysicsColliderActor* collider, float contactOffset, const Vector3& position, float slopeLimit, int32 nonWalkableMode, JsonAsset* material, float radius, float height, float stepOffset, void*& shape)
{
    auto scenePhysX = (ScenePhysX*)scene;
    const Vector3 sceneOrigin = SceneOrigins[scenePhysX->Scene];
    PxCapsuleControllerDesc desc;
    desc.userData = actor;
    desc.reportCallback = &CharacterControllerHitReport;
    desc.contactOffset = Math::Max(contactOffset, ZeroTolerance);
    desc.position = PxExtendedVec3(position.X - sceneOrigin.X, position.Y - sceneOrigin.Y, position.Z - sceneOrigin.Z);
    desc.slopeLimit = Math::Cos(slopeLimit * DegreesToRadians);
    desc.nonWalkableMode = static_cast<PxControllerNonWalkableMode::Enum>(nonWalkableMode);
    desc.climbingMode = PxCapsuleClimbingMode::eEASY;
    if (material && !material->WaitForLoaded())
        desc.material = (PxMaterial*)((PhysicalMaterial*)material->Instance)->GetPhysicsMaterial();
    else
        desc.material = DefaultMaterial;
    const float minSize = 0.001f;
    desc.height = Math::Max(height, minSize);
    desc.radius = Math::Max(radius - Math::Max(contactOffset, 0.0f), minSize);
    desc.stepOffset = Math::Min(stepOffset, desc.height + desc.radius * 2.0f - minSize);
    auto controllerPhysX = (PxCapsuleController*)scenePhysX->ControllerManager->createController(desc);
    PxRigidActor* actorPhysX = controllerPhysX->getActor();
    ASSERT(actorPhysX && actorPhysX->getNbShapes() == 1);
    actorPhysX->getShapes((PxShape**)&shape, 1);
    actorPhysX->userData = actor;
    auto shapePhysX = (PxShape*)shape;
    shapePhysX->userData = collider;
#if PHYSX_DEBUG_NAMING
    actorPhysX->setName("CCActor");
    shapePhysX->setName("CCShape");
#endif
    return controllerPhysX;
}

void* PhysicsBackend::GetControllerRigidDynamicActor(void* controller)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    return controllerPhysX->getActor();
}

void PhysicsBackend::SetControllerSize(void* controller, float radius, float height)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    controllerPhysX->setRadius(radius);
    controllerPhysX->resize(height);
}

void PhysicsBackend::SetControllerSlopeLimit(void* controller, float value)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    controllerPhysX->setSlopeLimit(Math::Cos(value * DegreesToRadians));
}

void PhysicsBackend::SetControllerNonWalkableMode(void* controller, int32 value)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    controllerPhysX->setNonWalkableMode(static_cast<PxControllerNonWalkableMode::Enum>(value));
}

void PhysicsBackend::SetControllerStepOffset(void* controller, float value)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    controllerPhysX->setStepOffset(value);
}

Vector3 PhysicsBackend::GetControllerUpDirection(void* controller)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    return P2C(controllerPhysX->getUpDirection());
}

void PhysicsBackend::SetControllerUpDirection(void* controller, const Vector3& value)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    controllerPhysX->setUpDirection(C2P(value));
}

Vector3 PhysicsBackend::GetControllerPosition(void* controller)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    const Vector3 origin = SceneOrigins[controllerPhysX->getScene()];
    return P2C(controllerPhysX->getPosition()) + origin;
}

void PhysicsBackend::SetControllerPosition(void* controller, const Vector3& value)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    const Vector3 sceneOrigin = SceneOrigins[controllerPhysX->getScene()];
    controllerPhysX->setPosition(PxExtendedVec3(value.X - sceneOrigin.X, value.Y - sceneOrigin.Y, value.Z - sceneOrigin.Z));
}

int32 PhysicsBackend::MoveController(void* controller, void* shape, const Vector3& displacement, float minMoveDistance, float deltaTime)
{
    auto controllerPhysX = (PxCapsuleController*)controller;
    auto shapePhysX = (PxShape*)shape;
    PxFilterData filterData = shapePhysX->getSimulationFilterData();
    PxControllerFilters filters;
    filters.mFilterData = &filterData;
    filters.mFilterCallback = &CharacterQueryFilter;
    filters.mFilterFlags = PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;
    filters.mCCTFilterCallback = &CharacterControllerFilter;
    return (byte)controllerPhysX->move(C2P(displacement), minMoveDistance, deltaTime, filters);
}

#if WITH_VEHICLE

PxVehicleDifferential4WData CreatePxVehicleDifferential4WData(const WheeledVehicle::DifferentialSettings& settings)
{
    PxVehicleDifferential4WData differential4WData;
    differential4WData.mType = (PxVehicleDifferential4WData::Enum)settings.Type;
    differential4WData.mFrontRearSplit = settings.FrontRearSplit;
    differential4WData.mFrontLeftRightSplit = settings.FrontLeftRightSplit;
    differential4WData.mRearLeftRightSplit = settings.RearLeftRightSplit;
    differential4WData.mCentreBias = settings.CentreBias;
    differential4WData.mFrontBias = settings.FrontBias;
    differential4WData.mRearBias = settings.RearBias;
    return differential4WData;
}

PxVehicleDifferentialNWData CreatePxVehicleDifferentialNWData(const WheeledVehicle::DifferentialSettings& settings, const Array<WheeledVehicle::Wheel*, FixedAllocation<PX_MAX_NB_WHEELS>>& wheels)
{
    PxVehicleDifferentialNWData differentialNwData;
    for (int32 i = 0; i < wheels.Count(); i++)
        differentialNwData.setDrivenWheel(i, true);
    return differentialNwData;
}

PxVehicleEngineData CreatePxVehicleEngineData(const WheeledVehicle::EngineSettings& settings)
{
    PxVehicleEngineData engineData;
    engineData.mMOI = M2ToCm2(settings.MOI);
    engineData.mPeakTorque = M2ToCm2(settings.MaxTorque);
    engineData.mMaxOmega = RpmToRadPerS(settings.MaxRotationSpeed);
    engineData.mDampingRateFullThrottle = M2ToCm2(0.15f);
    engineData.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(2.0f);
    engineData.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(0.35f);
    return engineData;
}

PxVehicleGearsData CreatePxVehicleGearsData(const WheeledVehicle::GearboxSettings& settings)
{
    PxVehicleGearsData gears;

    // Total gears is forward gears + neutral/rear gears
    const int32 gearsCount = Math::Clamp<int32>(settings.ForwardGearsRatios + 2, 2, PxVehicleGearsData::eGEARSRATIO_COUNT);

    // Setup gears torque/top speed relations
    // Higher torque = less speed
    // Low torque = high speed

    // Example:
    // ForwardGearsRatios = 4
    // GearRev = -5
    // Gear0 = 0  
    // Gear1 = 4.2
    // Gear2 = 3.4
    // Gear3 = 2.6
    // Gear4 = 1.8
    // Gear5 = 1

    gears.mRatios[0] = (float)-(gearsCount - 2); // Reverse
    gears.mRatios[1] = 0; // Neutral

    // Setup all gears except neutral and reverse
    for (int32 i = gearsCount; i > 2; i--)
    {
        float gearsRatios = (float)settings.ForwardGearsRatios;
        float currentGear = i - 2.0f;
        gears.mRatios[i] = Math::Lerp(gearsRatios, 1.0f, currentGear / gearsRatios);
    }

    // Reset unused gears
    for (int32 i = gearsCount; i < PxVehicleGearsData::eGEARSRATIO_COUNT; i++)
        gears.mRatios[i] = 0;

    gears.mSwitchTime = Math::Max(settings.SwitchTime, 0.0f);
    gears.mNbRatios = gearsCount;
    return gears;
}

PxVehicleAutoBoxData CreatePxVehicleAutoBoxData()
{
    return PxVehicleAutoBoxData();
}

PxVehicleClutchData CreatePxVehicleClutchData(const WheeledVehicle::GearboxSettings& settings)
{
    PxVehicleClutchData clutch;
    clutch.mStrength = M2ToCm2(settings.ClutchStrength);
    return clutch;
}

PxVehicleSuspensionData CreatePxVehicleSuspensionData(const WheeledVehicle::Wheel& settings, const PxReal wheelSprungMass)
{
    PxVehicleSuspensionData suspensionData;
    const float suspensionFrequency = 7.0f;
    suspensionData.mMaxCompression = settings.SuspensionMaxRaise;
    suspensionData.mMaxDroop = settings.SuspensionMaxDrop;
    suspensionData.mSprungMass = wheelSprungMass * settings.SprungMassMultiplier;
    suspensionData.mSpringStrength = Math::Square(suspensionFrequency) * suspensionData.mSprungMass;
    suspensionData.mSpringDamperRate = settings.SuspensionDampingRate * 2.0f * Math::Sqrt(suspensionData.mSpringStrength * suspensionData.mSprungMass);
    return suspensionData;
}

PxVehicleTireData CreatePxVehicleTireData(const WheeledVehicle::Wheel& settings)
{
    PxVehicleTireData tire;
    int32 tireIndex = WheelTireTypes.Find(settings.TireFrictionScale);
    if (tireIndex == -1)
    {
        // New tire type
        tireIndex = WheelTireTypes.Count();
        WheelTireTypes.Add(settings.TireFrictionScale);
        WheelTireFrictionsDirty = true;
    }
    tire.mType = tireIndex;
    tire.mLatStiffX = settings.TireLateralMax;
    tire.mLatStiffY = settings.TireLateralStiffness;
    tire.mLongitudinalStiffnessPerUnitGravity = settings.TireLongitudinalStiffness;
    return tire;
}

PxVehicleWheelData CreatePxVehicleWheelData(const WheeledVehicle::Wheel& settings)
{
    PxVehicleWheelData wheelData;
    wheelData.mMass = settings.Mass;
    wheelData.mRadius = settings.Radius;
    wheelData.mWidth = settings.Width;
    wheelData.mMOI = 0.5f * wheelData.mMass * Math::Square(wheelData.mRadius);
    wheelData.mDampingRate = M2ToCm2(settings.DampingRate);
    wheelData.mMaxSteer = settings.MaxSteerAngle * DegreesToRadians;
    wheelData.mMaxBrakeTorque = M2ToCm2(settings.MaxBrakeTorque);
    wheelData.mMaxHandBrakeTorque = M2ToCm2(settings.MaxHandBrakeTorque);
    return wheelData;
}

PxVehicleAckermannGeometryData CreatePxVehicleAckermannGeometryData(PxVehicleWheelsSimData* wheelsSimData)
{
    PxVehicleAckermannGeometryData ackermann;
    ackermann.mAxleSeparation = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z);
    ackermann.mFrontWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x);
    ackermann.mRearWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x);
    return ackermann;
}

PxVehicleAntiRollBarData CreatePxPxVehicleAntiRollBarData(const WheeledVehicle::AntiRollBar& settings, int32 leftWheelIndex, int32 rightWheelIndex)
{
    PxVehicleAntiRollBarData antiRollBar;
    antiRollBar.mWheel0 = leftWheelIndex;
    antiRollBar.mWheel1 = rightWheelIndex;
    antiRollBar.mStiffness = settings.Stiffness;
    return antiRollBar;
}

bool SortWheelsFrontToBack(WheeledVehicle::Wheel* const& a, WheeledVehicle::Wheel* const& b)
{
    return a->Collider && b->Collider && a->Collider->GetLocalPosition().Z > b->Collider->GetLocalPosition().Z;
}

void* PhysicsBackend::CreateVehicle(WheeledVehicle* actor)
{
    // Get wheels
    Array<WheeledVehicle::Wheel*, FixedAllocation<PX_MAX_NB_WHEELS>> wheels;
    for (auto& wheel : actor->_wheels)
    {
        if (!wheel.Collider)
        {
            LOG(Warning, "Missing wheel collider in vehicle {0}", actor->ToString());
            continue;
        }
        if (wheel.Collider->GetParent() != actor)
        {
            LOG(Warning, "Invalid wheel collider {1} in vehicle {0} attached to {2} (wheels needs to be added as children to vehicle)", actor->ToString(), wheel.Collider->ToString(), wheel.Collider->GetParent() ? wheel.Collider->GetParent()->ToString() : String::Empty);
            continue;
        }
        if (wheel.Collider->GetIsTrigger())
        {
            LOG(Warning, "Invalid wheel collider {1} in vehicle {0} cannot be a trigger", actor->ToString(), wheel.Collider->ToString());
            continue;
        }
        if (wheel.Collider->IsDuringPlay())
        {
            wheels.Add(&wheel);
        }
    }
    if (wheels.IsEmpty())
    {
        // No wheel, no car
        // No woman, no cry
        return nullptr;
    }

    // Physx vehicles needs to have all wheels sorted to apply controls correctly.
    // Its needs to know what wheels are on front to turn wheel to correctly side
    // and needs to know wheel side to apply throttle to correctly direction for each track when using tanks.
    // Anti roll bars needs to have all wheels sorted to get correctly wheel index too.
    if (actor->_driveType != WheeledVehicle::DriveTypes::NoDrive)
    {
        Sorting::QuickSort(wheels.Get(), wheels.Count(), SortWheelsFrontToBack);

        // Sort wheels by side
        if (actor->_driveType == WheeledVehicle::DriveTypes::Tank)
        {
            for (int32 i = 0; i < wheels.Count() - 1; i += 2)
            {
                auto a = wheels[i];
                auto b = wheels[i + 1];
                if (!a->Collider || !b->Collider)
                    continue;
                if (a->Collider->GetLocalPosition().X > b->Collider->GetLocalPosition().X)
                {
                    wheels[i] = b;
                    wheels[i + 1] = a;
                }
            }
        }
    }
    
    actor->_wheelsData.Resize(wheels.Count());
    auto actorPhysX = (PxRigidDynamic*)actor->GetPhysicsActor();

    InitVehicleSDK();

    // Get linked shapes for the wheels mapping
    Array<PxShape*, InlinedAllocation<8>> shapes;
    shapes.Resize(actorPhysX->getNbShapes());
    actorPhysX->getShapes(shapes.Get(), shapes.Count(), 0);
    PxTransform centerOfMassOffset = actorPhysX->getCMassLocalPose();
    centerOfMassOffset.q = PxQuat(PxIdentity);

    // Initialize wheels simulation data
    PxVec3 offsets[PX_MAX_NB_WHEELS];
    for (int32 i = 0; i < wheels.Count(); i++)
        offsets[i] = C2P(wheels[i]->Collider->GetLocalPosition());
    PxF32 sprungMasses[PX_MAX_NB_WHEELS];
    const float mass = actorPhysX->getMass();
    // TODO: get gravityDirection from scenePhysX->Scene->getGravity()
    PxVehicleComputeSprungMasses(wheels.Count(), offsets, centerOfMassOffset.p, mass, 1, sprungMasses);
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(wheels.Count());
    int32 wheelsCount = wheels.Count();
    for (int32 i = 0; i < wheelsCount; i++)
    {
        auto& wheel = *wheels[i];

        auto& data = actor->_wheelsData[i];
        data.Collider = wheel.Collider;
        data.LocalOrientation = wheel.Collider->GetLocalOrientation();

        PxVec3 centreOffset = centerOfMassOffset.transformInv(offsets[i]);
        PxVec3 forceAppPointOffset(centreOffset.x, wheel.SuspensionForceOffset, centreOffset.z);

        const PxVehicleTireData& tireData = CreatePxVehicleTireData(wheel);
        const PxVehicleWheelData& wheelData = CreatePxVehicleWheelData(wheel);
        const PxVehicleSuspensionData& suspensionData = CreatePxVehicleSuspensionData(wheel, sprungMasses[i]);

        wheelsSimData->setTireData(i, tireData);
        wheelsSimData->setWheelData(i, wheelData);
        wheelsSimData->setSuspensionData(i, suspensionData);
        wheelsSimData->setSuspTravelDirection(i, centerOfMassOffset.rotate(PxVec3(0.0f, -1.0f, 0.0f)));
        wheelsSimData->setWheelCentreOffset(i, centreOffset);
        wheelsSimData->setSuspForceAppPointOffset(i, forceAppPointOffset);
        wheelsSimData->setTireForceAppPointOffset(i, forceAppPointOffset);
        wheelsSimData->setSubStepCount(4.0f * 100.0f, 3, 1);
        wheelsSimData->setMinLongSlipDenominator(4.0f * 100.0f);

        PxShape* wheelShape = (PxShape*)wheel.Collider->GetPhysicsShape();
        if (wheel.Collider->IsActiveInHierarchy())
        {
            wheelsSimData->setWheelShapeMapping(i, shapes.Find(wheelShape));

            // Setup Vehicle ID inside word3 for suspension raycasts to ignore self
            PxFilterData filter = wheelShape->getQueryFilterData();
            filter.word3 = actor->GetID().D + 1;
            wheelShape->setQueryFilterData(filter);
            wheelShape->setSimulationFilterData(filter);
            wheelsSimData->setSceneQueryFilterData(i, filter);

            // Remove wheels from the simulation (suspension force hold the vehicle)
            wheelShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        }
        else
        {
            wheelsSimData->setWheelShapeMapping(i, -1);
            wheelsSimData->disableWheel(i);
        }
    }
    // Add Anti roll bars for wheels axles
    for (int32 i = 0; i < actor->GetAntiRollBars().Count(); i++)
    {
        int32 axleIndex = actor->GetAntiRollBars()[i].Axle;
        int32 leftWheelIndex = axleIndex * 2;
        int32 rightWheelIndex = leftWheelIndex + 1;
        if (leftWheelIndex >= wheelsCount || rightWheelIndex >= wheelsCount)
            continue;

        const PxVehicleAntiRollBarData& antiRollBar = CreatePxPxVehicleAntiRollBarData(actor->GetAntiRollBars()[i], leftWheelIndex, rightWheelIndex);
        wheelsSimData->addAntiRollBarData(antiRollBar);
    }
    for (auto child : actor->Children)
    {
        auto collider = ScriptingObject::Cast<Collider>(child);
        if (collider && collider->GetAttachedRigidBody() == actor)
        {
            bool isWheel = false;
            for (auto& w : wheels)
            {
                if (w->Collider == collider)
                {
                    isWheel = true;
                    break;
                }
            }
            if (!isWheel)
            {
                // Setup Vehicle ID inside word3 for suspension raycasts to ignore self
                PxShape* shape = (PxShape*)collider->GetPhysicsShape();
                PxFilterData filter = shape->getQueryFilterData();
                filter.word3 = actor->GetID().D + 1;
                shape->setQueryFilterData(filter);
                shape->setSimulationFilterData(filter);
            }
        }
    }

    // Initialize vehicle drive
    void* vehicle = nullptr;
    const auto& differential = actor->_differential;
    const auto& engine = actor->_engine;
    const auto& gearbox = actor->_gearbox;
    switch (actor->_driveType)
    {
    case WheeledVehicle::DriveTypes::Drive4W:
    {
        PxVehicleDriveSimData4W driveSimData;
        const PxVehicleDifferential4WData& differentialData = CreatePxVehicleDifferential4WData(differential);
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        const PxVehicleGearsData& gearsData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);
        const PxVehicleAckermannGeometryData& geometryData = CreatePxVehicleAckermannGeometryData(wheelsSimData);

        driveSimData.setDiffData(differentialData);
        driveSimData.setEngineData(engineData);
        driveSimData.setGearsData(gearsData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);
        driveSimData.setAckermannGeometryData(geometryData);

        // Create vehicle drive
        auto drive4W = PxVehicleDrive4W::allocate(wheels.Count());
        drive4W->setup(PhysX, actorPhysX, *wheelsSimData, driveSimData, Math::Max(wheels.Count() - 4, 0));
        drive4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        drive4W->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
        vehicle = drive4W;
        break;
    }
    case WheeledVehicle::DriveTypes::DriveNW:
    {
        PxVehicleDriveSimDataNW driveSimData;
        const PxVehicleDifferentialNWData& differentialData = CreatePxVehicleDifferentialNWData(differential, wheels);
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        const PxVehicleGearsData& gearsData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);

        driveSimData.setDiffData(differentialData);
        driveSimData.setEngineData(engineData);
        driveSimData.setGearsData(gearsData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);

        // Create vehicle drive
        auto driveNW = PxVehicleDriveNW::allocate(wheels.Count());
        driveNW->setup(PhysX, actorPhysX, *wheelsSimData, driveSimData, wheels.Count());
        driveNW->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        driveNW->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
        vehicle = driveNW;
        break;
    }
    case WheeledVehicle::DriveTypes::NoDrive:
    {
        // Create vehicle drive
        auto driveNo = PxVehicleNoDrive::allocate(wheels.Count());
        driveNo->setup(PhysX, actorPhysX, *wheelsSimData);
        vehicle = driveNo;
        break;
    }
    case WheeledVehicle::DriveTypes::Tank:
    {
        PxVehicleDriveSimData4W driveSimData;
        const PxVehicleDifferential4WData& differentialData = CreatePxVehicleDifferential4WData(differential);
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        const PxVehicleGearsData& gearsData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);
        const PxVehicleAckermannGeometryData& geometryData = CreatePxVehicleAckermannGeometryData(wheelsSimData);

        driveSimData.setDiffData(differentialData);
        driveSimData.setEngineData(engineData);
        driveSimData.setGearsData(gearsData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);
        driveSimData.setAckermannGeometryData(geometryData);

        // Create vehicle drive
        auto driveTank = PxVehicleDriveTank::allocate(wheels.Count());
        driveTank->setup(PhysX, actorPhysX, *wheelsSimData, driveSimData, wheels.Count());
        driveTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        driveTank->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
        vehicle = driveTank;
        break;
    }
    default:
        CRASH;
    }
    wheelsSimData->free();

    return vehicle;
}

void PhysicsBackend::DestroyVehicle(void* vehicle, int32 driveType)
{
    switch ((WheeledVehicle::DriveTypes)driveType)
    {
    case WheeledVehicle::DriveTypes::Drive4W:
        ((PxVehicleDrive4W*)vehicle)->free();
        break;
    case WheeledVehicle::DriveTypes::DriveNW:
        ((PxVehicleDriveNW*)vehicle)->free();
        break;
    case WheeledVehicle::DriveTypes::NoDrive:
        ((PxVehicleNoDrive*)vehicle)->free();
        break;
    case WheeledVehicle::DriveTypes::Tank:
        ((PxVehicleDriveTank*)vehicle)->free();
        break;
    }
}

void PhysicsBackend::UpdateVehicleWheels(WheeledVehicle* actor)
{
    auto drive = (PxVehicleWheels*)actor->_vehicle;
    PxVehicleWheelsSimData* wheelsSimData = &drive->mWheelsSimData;
    for (uint32 i = 0; i < wheelsSimData->getNbWheels(); i++)
    {
        auto& wheel = actor->_wheels[i];
        const PxVehicleSuspensionData& suspensionData = CreatePxVehicleSuspensionData(wheel, wheelsSimData->getSuspensionData(i).mSprungMass);
        const PxVehicleTireData& tireData = CreatePxVehicleTireData(wheel);
        const PxVehicleWheelData& wheelData = CreatePxVehicleWheelData(wheel);
        wheelsSimData->setSuspensionData(i, suspensionData);
        wheelsSimData->setTireData(i, tireData);
        wheelsSimData->setWheelData(i, wheelData);
    }
}

void PhysicsBackend::UpdateVehicleAntiRollBars(WheeledVehicle* actor)
{
    int wheelsCount = actor->_wheels.Count();
    auto drive = (PxVehicleWheels*)actor->_vehicle;
    PxVehicleWheelsSimData* wheelsSimData = &drive->mWheelsSimData;

    // Update anti roll bars for wheels axles
    const auto& antiRollBars = actor->GetAntiRollBars();
    for (int32 i = 0; i < antiRollBars.Count(); i++)
    {
        const int32 axleIndex = antiRollBars.Get()[i].Axle;
        const int32 leftWheelIndex = axleIndex * 2;
        const int32 rightWheelIndex = leftWheelIndex + 1;
        if (leftWheelIndex >= wheelsCount || rightWheelIndex >= wheelsCount)
            continue;

        const PxVehicleAntiRollBarData& antiRollBar = CreatePxPxVehicleAntiRollBarData(antiRollBars.Get()[i], leftWheelIndex, rightWheelIndex);
        if ((int32)wheelsSimData->getNbAntiRollBarData() - 1 < i)
        {
            wheelsSimData->addAntiRollBarData(antiRollBar);
        }
        else
        {
            wheelsSimData->setAntiRollBarData(axleIndex, antiRollBar);
        }
    }
}

void PhysicsBackend::SetVehicleEngine(void* vehicle, const void* value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    auto& engine = *(const WheeledVehicle::EngineSettings*)value;
    switch (drive->getVehicleType())
    {
    case PxVehicleTypes::eDRIVE4W:
    {
        auto drive4W = (PxVehicleDrive4W*)drive;
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        PxVehicleDriveSimData4W& driveSimData = drive4W->mDriveSimData;
        driveSimData.setEngineData(engineData);
        break;
    }
    case PxVehicleTypes::eDRIVENW:
    {
        auto drive4W = (PxVehicleDriveNW*)drive;
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        PxVehicleDriveSimDataNW& driveSimData = drive4W->mDriveSimData;
        driveSimData.setEngineData(engineData);
        break;
    }
    case PxVehicleTypes::eDRIVETANK:
    {
        auto driveTank = (PxVehicleDriveTank*)drive;
        const PxVehicleEngineData& engineData = CreatePxVehicleEngineData(engine);
        PxVehicleDriveSimData& driveSimData = driveTank->mDriveSimData;
        driveSimData.setEngineData(engineData);
        break;
    }
    }
}

void PhysicsBackend::SetVehicleDifferential(void* vehicle, const void* value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    auto& differential = *(const WheeledVehicle::DifferentialSettings*)value;
    switch (drive->getVehicleType())
    {
    case PxVehicleTypes::eDRIVE4W:
    {
        auto drive4W = (PxVehicleDrive4W*)drive;
        const PxVehicleDifferential4WData& differentialData = CreatePxVehicleDifferential4WData(differential);
        PxVehicleDriveSimData4W& driveSimData = drive4W->mDriveSimData;
        driveSimData.setDiffData(differentialData);
        break;
    }
    }
}

void PhysicsBackend::SetVehicleGearbox(void* vehicle, const void* value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    auto& gearbox = *(const WheeledVehicle::GearboxSettings*)value;
    drive->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
    drive->mDriveDynData.setAutoBoxSwitchTime(Math::Max(gearbox.SwitchTime, 0.0f));
    switch (drive->getVehicleType())
    {
    case PxVehicleTypes::eDRIVE4W:
    {
        auto drive4W = (PxVehicleDrive4W*)drive;
        const PxVehicleGearsData& gearData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        PxVehicleDriveSimData4W& driveSimData = drive4W->mDriveSimData;
        driveSimData.setGearsData(gearData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);
        break;
    }
    case PxVehicleTypes::eDRIVENW:
    {
        auto drive4W = (PxVehicleDriveNW*)drive;
        const PxVehicleGearsData& gearData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        PxVehicleDriveSimDataNW& driveSimData = drive4W->mDriveSimData;
        driveSimData.setGearsData(gearData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);
        break;
    }
    case PxVehicleTypes::eDRIVETANK:
    {
        auto driveTank = (PxVehicleDriveTank*)drive;
        const PxVehicleGearsData& gearData = CreatePxVehicleGearsData(gearbox);
        const PxVehicleClutchData& clutchData = CreatePxVehicleClutchData(gearbox);
        const PxVehicleAutoBoxData& autoBoxData = CreatePxVehicleAutoBoxData();
        PxVehicleDriveSimData& driveSimData = driveTank->mDriveSimData;
        driveSimData.setGearsData(gearData);
        driveSimData.setAutoBoxData(autoBoxData);
        driveSimData.setClutchData(clutchData);
        break;
    }
    }
}

int32 PhysicsBackend::GetVehicleTargetGear(void* vehicle)
{
    auto drive = (PxVehicleDrive*)vehicle;
    return (int32)drive->mDriveDynData.getTargetGear() - 1;
}

void PhysicsBackend::SetVehicleTargetGear(void* vehicle, int32 value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    drive->mDriveDynData.startGearChange((PxU32)(value + 1));
}

int32 PhysicsBackend::GetVehicleCurrentGear(void* vehicle)
{
    auto drive = (PxVehicleDrive*)vehicle;
    return (int32)drive->mDriveDynData.getCurrentGear() - 1;
}

void PhysicsBackend::SetVehicleCurrentGear(void* vehicle, int32 value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    drive->mDriveDynData.forceGearChange((PxU32)(value + 1));
}

float PhysicsBackend::GetVehicleForwardSpeed(void* vehicle)
{
    auto drive = (PxVehicleDrive*)vehicle;
    return drive->computeForwardSpeed();
}

float PhysicsBackend::GetVehicleSidewaysSpeed(void* vehicle)
{
    auto drive = (PxVehicleDrive*)vehicle;
    return drive->computeSidewaysSpeed();
}

float PhysicsBackend::GetVehicleEngineRotationSpeed(void* vehicle)
{
    auto drive = (PxVehicleDrive*)vehicle;
    return RadPerSToRpm(drive->mDriveDynData.getEngineRotationSpeed());
}

void PhysicsBackend::AddVehicle(void* scene, WheeledVehicle* actor)
{
    auto scenePhysX = (ScenePhysX*)scene;
    scenePhysX->WheelVehicles.Add(actor);
}

void PhysicsBackend::RemoveVehicle(void* scene, WheeledVehicle* actor)
{
    auto scenePhysX = (ScenePhysX*)scene;
    scenePhysX->WheelVehicles.Remove(actor);
}

#endif

#if WITH_CLOTH

void* PhysicsBackend::CreateCloth(const PhysicsClothDesc& desc)
{
    PROFILE_CPU();
#if USE_CLOTH_SANITY_CHECKS
    {
        // Sanity check
        bool allValid = true;
        for (uint32 i = 0; i < desc.VerticesCount; i++)
            allValid &= !(*(Float3*)((byte*)desc.VerticesData + i * desc.VerticesStride)).IsNanOrInfinity();
        if (desc.InvMassesData)
        {
            for (uint32 i = 0; i < desc.VerticesCount; i++)
            {
                const float v = *(float*)((byte*)desc.InvMassesData + i * desc.InvMassesStride);
                allValid &= !isnan(v) && !isinf(v);
            }
        }
        if (desc.MaxDistancesData)
        {
            for (uint32 i = 0; i < desc.VerticesCount; i++)
            {
                const float v = *(float*)((byte*)desc.MaxDistancesData + i * desc.MaxDistancesStride);
                allValid &= !isnan(v) && !isinf(v);
            }
        }
        ASSERT(allValid);
    }
#endif

    // Lazy-init NvCloth
    ScopeLock lock(ClothLocker);
    if (ClothFactory == nullptr)
    {
        nv::cloth::InitializeNvCloth(&AllocatorCallback, &ErrorCallback, &AssertCallback, &ProfilerCallback);
        ClothFactory = NvClothCreateFactoryCPU();
        ASSERT(ClothFactory);
    }

    // Cook fabric from the mesh data
    nv::cloth::Fabric* fabric = nullptr;
    {
        for (auto& e : Fabrics)
        {
            if (e.Value.MatchesDesc(desc))
            {
                fabric = e.Key;
                break;
            }
        }
        if (fabric)
        {
            Fabrics[fabric].Refs++;
            fabric->incRefCount();
        }
        else
        {
            PROFILE_CPU_NAMED("CookFabric");
            nv::cloth::ClothMeshDesc meshDesc;
            meshDesc.points.data = desc.VerticesData;
            meshDesc.points.stride = desc.VerticesStride;
            meshDesc.points.count = desc.VerticesCount;
            meshDesc.invMasses.data = desc.InvMassesData;
            meshDesc.invMasses.stride = desc.InvMassesStride;
            meshDesc.invMasses.count = desc.InvMassesData ? desc.VerticesCount : 0;
            meshDesc.triangles.data = desc.IndicesData;
            meshDesc.triangles.stride = desc.IndicesStride * 3;
            meshDesc.triangles.count = desc.IndicesCount / 3;
            if (desc.IndicesStride == sizeof(uint16))
                meshDesc.flags |= nv::cloth::MeshFlag::e16_BIT_INDICES;
            const Float3 gravity(PhysicsSettings::Get()->DefaultGravity);
            nv::cloth::Vector<int32_t>::Type phaseTypeInfo;
            fabric = NvClothCookFabricFromMesh(ClothFactory, meshDesc, gravity.Raw, &phaseTypeInfo);
            if (!fabric)
            {
                LOG(Error, "NvClothCookFabricFromMesh failed");
                return nullptr;
            }
            FabricSettings fabricSettings;
            fabricSettings.Refs = 1;
            fabricSettings.PhraseTypes.swap(phaseTypeInfo);
            fabricSettings.Desc = desc;
            if (desc.InvMassesData)
                fabricSettings.InvMasses.Set((byte*)desc.InvMassesData, desc.VerticesCount * desc.InvMassesStride);
            Fabrics.Add(fabric, MoveTemp(fabricSettings));
        }
    }

    // Create cloth object
    static_assert(sizeof(Float4) == sizeof(PxVec4), "Size mismatch");
    Array<Float4> initialState;
    initialState.Resize((int32)desc.VerticesCount);
    if (desc.InvMassesData)
    {
        for (uint32 i = 0; i < desc.VerticesCount; i++)
            initialState.Get()[i] = Float4(*(Float3*)((byte*)desc.VerticesData + i * desc.VerticesStride), *(float*)((byte*)desc.InvMassesData + i * desc.InvMassesStride));
    }
    else
    {
        for (uint32 i = 0; i < desc.VerticesCount; i++)
            initialState.Get()[i] = Float4(*(Float3*)((byte*)desc.VerticesData + i * desc.VerticesStride), 1.0f);
    }
    const nv::cloth::Range<PxVec4> initialParticlesRange((PxVec4*)initialState.Get(), (PxVec4*)initialState.Get() + initialState.Count());
    nv::cloth::Cloth* clothPhysX = ClothFactory->createCloth(initialParticlesRange, *fabric);
    fabric->decRefCount();
    if (!clothPhysX)
    {
        LOG(Error, "createCloth failed");
        return nullptr;
    }
    if (desc.MaxDistancesData)
    {
        nv::cloth::Range<PxVec4> motionConstraints = clothPhysX->getMotionConstraints();
        ASSERT(motionConstraints.size() == desc.VerticesCount);
        for (uint32 i = 0; i < desc.VerticesCount; i++)
            motionConstraints.begin()[i] = PxVec4(*(PxVec3*)((byte*)desc.VerticesData + i * desc.VerticesStride), *(float*)((byte*)desc.MaxDistancesData + i * desc.MaxDistancesStride));
    }
    clothPhysX->setUserData(desc.Actor);

    // Setup settings
    ClothSettings clothSettings;
    clothSettings.Actor = desc.Actor;
    clothSettings.UpdateBounds(clothPhysX);
    Cloths.Add(clothPhysX, clothSettings);

    return clothPhysX;
}

void PhysicsBackend::DestroyCloth(void* cloth)
{
    ScopeLock lock(ClothLocker);
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    if (--Fabrics[&clothPhysX->getFabric()].Refs == 0)
        Fabrics.Remove(&clothPhysX->getFabric());
    Cloths.Remove(clothPhysX);
    NV_CLOTH_DELETE(clothPhysX);
}

void PhysicsBackend::SetClothForceSettings(void* cloth, const void* settingsPtr)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    const auto& settings = *(const Cloth::ForceSettings*)settingsPtr;
    clothPhysX->setGravity(C2P(PhysicsSettings::Get()->DefaultGravity * settings.GravityScale));
    clothPhysX->setDamping(PxVec3(settings.Damping));
    clothPhysX->setLinearDrag(PxVec3(settings.LinearDrag));
    clothPhysX->setAngularDrag(PxVec3(settings.AngularDrag));
    clothPhysX->setLinearInertia(PxVec3(settings.LinearInertia));
    clothPhysX->setAngularInertia(PxVec3(settings.AngularInertia));
    clothPhysX->setCentrifugalInertia(PxVec3(settings.CentrifugalInertia));
    clothPhysX->setDragCoefficient(Math::Saturate(settings.AirDragCoefficient) * 0.01f);
    clothPhysX->setLiftCoefficient(Math::Saturate(settings.AirLiftCoefficient) * 0.01f);
    clothPhysX->setFluidDensity(Math::Max(settings.AirDensity, ZeroTolerance));
    auto& clothSettings = Cloths[clothPhysX];
    clothSettings.GravityScale = settings.GravityScale;
}

void PhysicsBackend::SetClothCollisionSettings(void* cloth, const void* settingsPtr)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    const auto& settings = *(const Cloth::CollisionSettings*)settingsPtr;
    clothPhysX->setFriction(settings.Friction);
    clothPhysX->setCollisionMassScale(settings.MassScale);
    clothPhysX->enableContinuousCollision(settings.ContinuousCollisionDetection);
    clothPhysX->setSelfCollisionDistance(settings.SelfCollisionDistance);
    clothPhysX->setSelfCollisionStiffness(settings.SelfCollisionStiffness);
    auto& clothSettings = Cloths[clothPhysX];
    if (clothSettings.SceneCollisions != settings.SceneCollisions && !settings.SceneCollisions)
    {
        // Remove colliders
        clothPhysX->setSpheres(nv::cloth::Range<const PxVec4>(), 0, clothPhysX->getNumSpheres());
        clothPhysX->setPlanes(nv::cloth::Range<const PxVec4>(), 0, clothPhysX->getNumPlanes());
        clothPhysX->setTriangles(nv::cloth::Range<const PxVec3>(), 0, clothPhysX->getNumTriangles());
    }
    clothSettings.CollisionsUpdateFramesLeft = 0;
    clothSettings.SceneCollisions = settings.SceneCollisions;
    clothSettings.CollisionThickness = settings.CollisionThickness;
}

void PhysicsBackend::SetClothSimulationSettings(void* cloth, const void* settingsPtr)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    const auto& settings = *(const Cloth::SimulationSettings*)settingsPtr;
    clothPhysX->setSolverFrequency(settings.SolverFrequency);
    clothPhysX->setMotionConstraintScaleBias(settings.MaxParticleDistance, 0.0f);
    clothPhysX->setWindVelocity(C2P(settings.WindVelocity));
}

void PhysicsBackend::SetClothFabricSettings(void* cloth, const void* settingsPtr)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    const auto& settings = *(const Cloth::FabricSettings*)settingsPtr;
    const auto& fabricSettings = Fabrics[&clothPhysX->getFabric()];
    Array<nv::cloth::PhaseConfig> configs;
    configs.Resize(fabricSettings.PhraseTypes.size());
    for (int32 i = 0; i < configs.Count(); i++)
    {
        auto& config = configs[i];
        config.mPhaseIndex = i;
        const Cloth::FabricAxisSettings* axisSettings;
        switch (fabricSettings.PhraseTypes[i])
        {
        case nv::cloth::ClothFabricPhaseType::eVERTICAL:
            axisSettings = &settings.Vertical;
            break;
        case nv::cloth::ClothFabricPhaseType::eHORIZONTAL:
            axisSettings = &settings.Horizontal;
            break;
        case nv::cloth::ClothFabricPhaseType::eBENDING:
            axisSettings = &settings.Bending;
            break;
        case nv::cloth::ClothFabricPhaseType::eSHEARING:
            axisSettings = &settings.Shearing;
            break;
        }
        config.mStiffness = axisSettings->Stiffness;
        config.mStiffnessMultiplier = axisSettings->StiffnessMultiplier;
        config.mCompressionLimit = axisSettings->CompressionLimit;
        config.mStretchLimit = axisSettings->StretchLimit;
    }
    clothPhysX->setPhaseConfig(nv::cloth::Range<const nv::cloth::PhaseConfig>(configs.begin(), configs.end()));
}

void PhysicsBackend::SetClothTransform(void* cloth, const Transform& transform, bool teleport)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    if (teleport)
    {
        clothPhysX->teleportToLocation(C2P(transform.Translation), C2P(transform.Orientation));
    }
    else
    {
        clothPhysX->setTranslation(C2P(transform.Translation));
        clothPhysX->setRotation(C2P(transform.Orientation));
    }
    auto& clothSettings = Cloths[clothPhysX];
    clothSettings.CollisionsUpdateFramesLeft = 0;
    clothSettings.UpdateBounds(clothPhysX);
}

void PhysicsBackend::ClearClothInertia(void* cloth)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    clothPhysX->clearInertia();
}

void PhysicsBackend::LockClothParticles(void* cloth)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    clothPhysX->lockParticles();
}

void PhysicsBackend::UnlockClothParticles(void* cloth)
{
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    clothPhysX->unlockParticles();
}

Span<const Float4> PhysicsBackend::GetClothParticles(void* cloth)
{
    auto clothPhysX = (const nv::cloth::Cloth*)cloth;
    const nv::cloth::MappedRange<const PxVec4> range = clothPhysX->getCurrentParticles();
    return Span<const Float4>((const Float4*)range.begin(), (int32)range.size());
}

void PhysicsBackend::SetClothParticles(void* cloth, Span<const Float4> value, Span<const Float3> positions, Span<const float> invMasses)
{
    PROFILE_CPU();
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    nv::cloth::MappedRange<PxVec4> range = clothPhysX->getCurrentParticles();
    const uint32_t size = range.size();
    PxVec4* dst = range.begin();
    if (value.IsValid())
    {
        // Set XYZW
        CHECK((uint32_t)value.Length() >= size);
        const Float4* src = value.Get();
        Platform::MemoryCopy(dst, src, size * sizeof(Float4));

        // Apply previous particles too for immovable particles
        nv::cloth::MappedRange<PxVec4> range2 = clothPhysX->getPreviousParticles();
        for (uint32 i = 0; i < size; i++)
        {
            if (src[i].W <= ZeroTolerance)
                range2.begin()[i] = (PxVec4&)src[i];
        }
    }
    if (positions.IsValid())
    {
        // Set XYZ
        CHECK((uint32_t)positions.Length() >= size);
        const Float3* src = positions.Get();
        for (uint32 i = 0; i < size; i++)
            dst[i] = PxVec4(C2P(src[i]), dst[i].w);
    }
    if (invMasses.IsValid())
    {
        // Set W
        CHECK((uint32_t)invMasses.Length() >= size);
        const float* src = invMasses.Get();
        for (uint32 i = 0; i < size; i++)
            dst[i].w = src[i];

        // Apply previous particles too
        nv::cloth::MappedRange<PxVec4> range2 = clothPhysX->getPreviousParticles();
        for (uint32 i = 0; i < size; i++)
            range2.begin()[i].w = src[i];
    }
}

void PhysicsBackend::SetClothPaint(void* cloth, Span<const float> value)
{
    PROFILE_CPU();
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    if (value.IsValid())
    {
        const nv::cloth::MappedRange<const PxVec4> range = ((const nv::cloth::Cloth*)clothPhysX)->getCurrentParticles();
        nv::cloth::Range<PxVec4> motionConstraints = clothPhysX->getMotionConstraints();
        ASSERT(motionConstraints.size() <= (uint32)value.Length());
        for (int32 i = 0; i < value.Length(); i++)
            motionConstraints.begin()[i] = PxVec4(range[i].getXYZ(), value[i]);
    }
    else
    {
        clothPhysX->clearMotionConstraints();
    }
}

void PhysicsBackend::AddCloth(void* scene, void* cloth)
{
    ScopeLock lock(ClothLocker);
    auto scenePhysX = (ScenePhysX*)scene;
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    if (scenePhysX->ClothSolver == nullptr)
    {
        scenePhysX->ClothSolver = ClothFactory->createSolver();
        ASSERT(scenePhysX->ClothSolver);
    }
    scenePhysX->ClothSolver->addCloth(clothPhysX);
    scenePhysX->ClothsList.Add(clothPhysX);
}

void PhysicsBackend::RemoveCloth(void* scene, void* cloth)
{
    ScopeLock lock(ClothLocker);
    auto scenePhysX = (ScenePhysX*)scene;
    auto clothPhysX = (nv::cloth::Cloth*)cloth;
    auto& clothSettings = Cloths[clothPhysX];
    if (clothSettings.Culled)
        clothSettings.Culled = false;
    else
        scenePhysX->ClothSolver->removeCloth(clothPhysX);
    scenePhysX->ClothsList.Remove(clothPhysX);
}

#endif

void* PhysicsBackend::CreateConvexMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    PxDefaultMemoryInputData input(data, dataSize);
    PxConvexMesh* convexMesh = PhysX->createConvexMesh(input);
    if (convexMesh)
        localBounds = P2C(convexMesh->getLocalBounds());
    return convexMesh;
}

void* PhysicsBackend::CreateTriangleMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    PxDefaultMemoryInputData input(data, dataSize);
    PxTriangleMesh* triangleMesh = PhysX->createTriangleMesh(input);
    if (triangleMesh)
        localBounds = P2C(triangleMesh->getLocalBounds());
    return triangleMesh;
}

void* PhysicsBackend::CreateHeightField(byte* data, int32 dataSize)
{
    PxDefaultMemoryInputData input(data, dataSize);
    PxHeightField* heightField = PhysX->createHeightField(input);
    return heightField;
}

void PhysicsBackend::GetConvexMeshTriangles(void* contextMesh, Array<Float3, HeapAllocation>& vertexBuffer, Array<int, HeapAllocation>& indexBuffer)
{
    PROFILE_CPU();
    auto contextMeshPhysX = (PxConvexMesh*)contextMesh;
    uint32 numIndices = 0;
    uint32 numVertices = contextMeshPhysX->getNbVertices();
    const uint32 numPolygons = contextMeshPhysX->getNbPolygons();
    for (uint32 i = 0; i < numPolygons; i++)
    {
        PxHullPolygon face;
        const bool status = contextMeshPhysX->getPolygonData(i, face);
        ASSERT(status);
        numIndices += (face.mNbVerts - 2) * 3;
    }

    vertexBuffer.Resize(numVertices);
    indexBuffer.Resize(numIndices);
    auto outVertices = vertexBuffer.Get();
    auto outIndices = indexBuffer.Get();
    const PxVec3* convexVertices = contextMeshPhysX->getVertices();
    const byte* convexIndices = contextMeshPhysX->getIndexBuffer();

    for (uint32 i = 0; i < numVertices; i++)
        *outVertices++ = P2C(convexVertices[i]);

    for (uint32 i = 0; i < numPolygons; i++)
    {
        PxHullPolygon face;
        contextMeshPhysX->getPolygonData(i, face);

        const PxU8* faceIndices = convexIndices + face.mIndexBase;
        for (uint32 j = 2; j < face.mNbVerts; j++)
        {
            *outIndices++ = faceIndices[0];
            *outIndices++ = faceIndices[j];
            *outIndices++ = faceIndices[j - 1];
        }
    }
}

void PhysicsBackend::GetTriangleMeshTriangles(void* triangleMesh, Array<Float3>& vertexBuffer, Array<int32, HeapAllocation>& indexBuffer)
{
    PROFILE_CPU();
    auto triangleMeshPhysX = (PxTriangleMesh*)triangleMesh;
    uint32 numVertices = triangleMeshPhysX->getNbVertices();
    uint32 numIndices = triangleMeshPhysX->getNbTriangles() * 3;

    vertexBuffer.Resize(numVertices);
    indexBuffer.Resize(numIndices);
    auto outVertices = vertexBuffer.Get();
    auto outIndices = indexBuffer.Get();
    const PxVec3* vertices = triangleMeshPhysX->getVertices();
    for (uint32 i = 0; i < numVertices; i++)
        *outVertices++ = P2C(vertices[i]);

    if (triangleMeshPhysX->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES)
    {
        const uint16* indices = (const uint16*)triangleMeshPhysX->getTriangles();

        uint32 numTriangles = numIndices / 3;
        for (uint32 i = 0; i < numTriangles; i++)
        {
            outIndices[i * 3 + 0] = (uint32)indices[i * 3 + 0];
            outIndices[i * 3 + 1] = (uint32)indices[i * 3 + 1];
            outIndices[i * 3 + 2] = (uint32)indices[i * 3 + 2];
        }
    }
    else
    {
        const uint32* indices = (const uint32*)triangleMeshPhysX->getTriangles();

        uint32 numTriangles = numIndices / 3;
        for (uint32 i = 0; i < numTriangles; i++)
        {
            outIndices[i * 3 + 0] = indices[i * 3 + 0];
            outIndices[i * 3 + 1] = indices[i * 3 + 1];
            outIndices[i * 3 + 2] = indices[i * 3 + 2];
        }
    }
}

const uint32* PhysicsBackend::GetTriangleMeshRemap(void* triangleMesh, uint32& count)
{
    auto triangleMeshPhysX = (PxTriangleMesh*)triangleMesh;
    count = triangleMeshPhysX->getNbTriangles();
    return triangleMeshPhysX->getTrianglesRemap();
}

void PhysicsBackend::GetHeightFieldSize(void* heightField, int32& rows, int32& columns)
{
    auto heightFieldPhysX = (PxHeightField*)heightField;
    rows = (int32)heightFieldPhysX->getNbRows();
    columns = (int32)heightFieldPhysX->getNbColumns();
}

float PhysicsBackend::GetHeightFieldHeight(void* heightField, int32 x, int32 z)
{
    auto heightFieldPhysX = (PxHeightField*)heightField;
    return heightFieldPhysX->getHeight((float)x, (float)z);
}

PhysicsBackend::HeightFieldSample PhysicsBackend::GetHeightFieldSample(void* heightField, int32 x, int32 z)
{
    auto heightFieldPhysX = (PxHeightField*)heightField;
    auto sample = heightFieldPhysX->getSample(x, z);
    return { sample.height, sample.materialIndex0, sample.materialIndex1 };
}

bool PhysicsBackend::ModifyHeightField(void* heightField, int32 startCol, int32 startRow, int32 cols, int32 rows, const HeightFieldSample* data)
{
    auto heightFieldPhysX = (PxHeightField*)heightField;
    PxHeightFieldDesc heightFieldDesc;
    heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
    heightFieldDesc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES;
    heightFieldDesc.nbColumns = cols;
    heightFieldDesc.nbRows = rows;
    heightFieldDesc.samples.data = data;
    heightFieldDesc.samples.stride = sizeof(HeightFieldSample);
    return !heightFieldPhysX->modifySamples(startCol, startRow, heightFieldDesc, true);
}

void PhysicsBackend::FlushRequests()
{
    FlushLocker.Lock();

    // Delete objects
    for (int32 i = 0; i < DeleteObjects.Count(); i++)
        DeleteObjects[i]->release();
    DeleteObjects.Clear();

    FlushLocker.Unlock();
}

void PhysicsBackend::FlushRequests(void* scene)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();

    // Perform latent actions
    for (int32 i = 0; i < scenePhysX->Actions.Count(); i++)
    {
        const auto action = scenePhysX->Actions[i];
        switch (action.Type)
        {
        case ActionType::Sleep:
            static_cast<PxRigidDynamic*>(action.Actor)->putToSleep();
            break;
        }
    }
    scenePhysX->Actions.Clear();

    // Remove objects
    if (scenePhysX->RemoveActors.HasItems())
    {
        scenePhysX->Scene->removeActors(scenePhysX->RemoveActors.Get(), scenePhysX->RemoveActors.Count(), true);
        scenePhysX->RemoveActors.Clear();
    }
    if (scenePhysX->RemoveColliders.HasItems())
    {
        for (int32 i = 0; i < scenePhysX->RemoveColliders.Count(); i++)
            scenePhysX->EventsCallback.OnColliderRemoved(scenePhysX->RemoveColliders[i]);
        scenePhysX->RemoveColliders.Clear();
    }
    if (scenePhysX->RemoveJoints.HasItems())
    {
        for (int32 i = 0; i < scenePhysX->RemoveJoints.Count(); i++)
            scenePhysX->EventsCallback.OnJointRemoved(scenePhysX->RemoveJoints[i]);
        scenePhysX->RemoveJoints.Clear();
    }

    FlushLocker.Unlock();
}

void PhysicsBackend::DestroyActor(void* actor)
{
    ASSERT_LOW_LAYER(actor);
    auto actorPhysX = (PxActor*)actor;
    actorPhysX->userData = nullptr;
    FlushLocker.Lock();
    DeleteObjects.Add(actorPhysX);
    FlushLocker.Unlock();
}

void PhysicsBackend::DestroyShape(void* shape)
{
    ASSERT_LOW_LAYER(shape);
    auto shapePhysX = (PxShape*)shape;
    shapePhysX->userData = nullptr;
    FlushLocker.Lock();
    DeleteObjects.Add(shapePhysX);
    FlushLocker.Unlock();
}

void PhysicsBackend::DestroyJoint(void* joint)
{
    ASSERT_LOW_LAYER(joint);
    auto jointPhysX = (PxJoint*)joint;
    jointPhysX->userData = nullptr;
    FlushLocker.Lock();
    DeleteObjects.Add(jointPhysX);
    FlushLocker.Unlock();
}

void PhysicsBackend::DestroyController(void* controller)
{
    ASSERT_LOW_LAYER(controller);
    auto controllerPhysX = (PxController*)controller;
    controllerPhysX->getActor()->userData = nullptr;
    controllerPhysX->release();
}

void PhysicsBackend::DestroyMaterial(void* material)
{
    if (!PhysX)
        return; // Skip when called by Content unload after Physics is disposed
    ASSERT_LOW_LAYER(material);
    auto materialPhysX = (PxMaterial*)material;
    materialPhysX->userData = nullptr;
    FlushLocker.Lock();
    DeleteObjects.Add(materialPhysX);
    FlushLocker.Unlock();
}

void PhysicsBackend::DestroyObject(void* object)
{
    if (!PhysX)
        return; // Skip when called by Content unload after Physics is disposed
    ASSERT_LOW_LAYER(object);
    auto objectPhysX = (PxBase*)object;
    FlushLocker.Lock();
    DeleteObjects.Add(objectPhysX);
    FlushLocker.Unlock();
}

void PhysicsBackend::RemoveCollider(PhysicsColliderActor* collider)
{
    for (auto scene : Physics::Scenes)
    {
        auto scenePhysX = (ScenePhysX*)scene->GetPhysicsScene();
        scenePhysX->RemoveColliders.Add(collider);
    }
}

void PhysicsBackend::RemoveJoint(Joint* joint)
{
    for (auto scene : Physics::Scenes)
    {
        auto scenePhysX = (ScenePhysX*)scene->GetPhysicsScene();
        scenePhysX->RemoveJoints.Add(joint);
    }
}

#endif
