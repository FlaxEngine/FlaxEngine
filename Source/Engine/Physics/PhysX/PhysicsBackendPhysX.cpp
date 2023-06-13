// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PHYSX

#include "PhysicsBackendPhysX.h"
#include "PhysicsStepperPhysX.h"
#include "SimulationEventCallbackPhysX.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/PhysicsScene.h"
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
#include "Engine/Physics/Actors/WheeledVehicle.h"
#include <ThirdParty/PhysX/vehicle/PxVehicleSDK.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUpdate.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleNoDrive.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDrive4W.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleDriveNW.h>
#include <ThirdParty/PhysX/vehicle/PxVehicleUtilSetup.h>
#include <ThirdParty/PhysX/PxFiltering.h>
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
#define PHYSX_HIT_BUFFER_SIZE	128

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
    PxBatchQuery* WheelRaycastBatchQuery = nullptr;
#endif
};

class AllocatorPhysX : public PxAllocatorCallback
{
    void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
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
        {
            return PxQueryHitType::eNONE;
        }

        // Check if skip triggers
        const bool hitTriggers = filterData.word2 != 0;
        if (!hitTriggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
            return PxQueryHitType::eNONE;

        const bool blockSingle = filterData.word1 != 0;
        return blockSingle ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
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

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
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

#define SCENE_QUERY_SETUP(blockSingle) auto scenePhysX = (ScenePhysX*)scene; if (scene == nullptr) return false; \
		const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eUV; \
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

#define SCENE_QUERY_COLLECT_OVERLAP_COLLIDER() results.Clear();  \
		results.Resize(buffer.getNbTouches(), false); \
		for (int32 i = 0; i < results.Count(); i++) \
		{ \
			auto& hitInfo = results[i]; \
			const auto& hit = buffer.getTouch(i); \
			hitInfo = hit.shape ? static_cast<Collider*>(hit.shape->userData) : nullptr; \
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
    PxTolerancesScale ToleranceScale;
    QueryFilterPhysX QueryFilter;
    CharacterQueryFilterPhysX CharacterQueryFilter;
    CharacterControllerFilterPhysX CharacterControllerFilter;
    Dictionary<PxScene*, Vector3, InlinedAllocation<32>> SceneOrigins;

    CriticalSection FlushLocker;
    Array<PxBase*> DeleteObjects;

    bool _queriesHitTriggers = true;
    PhysicsCombineMode _frictionCombineMode = PhysicsCombineMode::Average;
    PhysicsCombineMode _restitutionCombineMode = PhysicsCombineMode::Average;

#if WITH_VEHICLE
    bool VehicleSDKInitialized = false;
    Array<PxVehicleWheels*> WheelVehiclesCache;
    Array<PxRaycastQueryResult> WheelQueryResults;
    Array<PxRaycastHit> WheelHitResults;
    Array<PxWheelQueryResult> WheelVehiclesResultsPerWheel;
    Array<PxVehicleWheelQueryResult> WheelVehiclesResultsPerVehicle;
    PxVehicleDrivableSurfaceToTireFrictionPairs* WheelTireFrictions = nullptr;
    bool WheelTireFrictionsDirty = false;
    Array<float> WheelTireTypes;
#endif
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
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
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
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::ePOST_SOLVER_VELOCITY;
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
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

PxQueryHitType::Enum WheelRaycastPreFilter(PxFilterData filterData0, PxFilterData filterData1, const void* constantBlock, PxU32 constantBlockSize, PxHitFlags& queryFlags)
{
    // Hardcoded id for vehicle shapes masking
    if (filterData0.word3 == filterData1.word3)
    {
        return PxQueryHitType::eNONE;
    }

    // Collide for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    {
        return PxQueryHitType::eBLOCK;
    }

    return PxQueryHitType::eNONE;
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
        LOG(Warning, "Convex Mesh cooking failed. Error code: {0}, Input vertices count: {1}", result, input.VertexCount);
        return true;
    }

    // Copy result
    output.Copy(outputStream.getData(), outputStream.getSize());

    return false;
}

bool CollisionCooking::CookTriangleMesh(CookingInput& input, BytesContainer& output)
{
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
        LOG(Warning, "Triangle Mesh cooking failed. Error code: {0}, Input vertices count: {1}, indices count: {2}", result, input.VertexCount, input.IndexCount);
        return true;
    }

    // Copy result
    output.Copy(outputStream.getData(), outputStream.getSize());

    return false;
}

bool CollisionCooking::CookHeightField(int32 cols, int32 rows, const PhysicsBackend::HeightFieldSample* data, WriteStream& stream)
{
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
    WheelQueryResults.Resize(0);
    WheelHitResults.Resize(0);
    WheelVehiclesResultsPerWheel.Resize(0);
    WheelVehiclesResultsPerVehicle.Resize(0);
#endif
    RELEASE_PHYSX(DefaultMaterial);

    // Shutdown PhysX
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
    _frictionCombineMode = settings.FrictionCombineMode;
    _restitutionCombineMode = settings.RestitutionCombineMode;

    // TODO: setting eADAPTIVE_FORCE requires PxScene setup (physx docs: This flag is not mutable, and must be set in PxSceneDesc at scene creation.)
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
    if (!settings.DisableCCD)
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    if (settings.EnableAdaptiveForce)
        sceneDesc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    sceneDesc.simulationEventCallback = &scenePhysX->EventsCallback;
    sceneDesc.filterShader = FilterShader;
    sceneDesc.bounceThresholdVelocity = settings.BounceThresholdVelocity;
    if (sceneDesc.cpuDispatcher == nullptr)
    {
        scenePhysX->CpuDispatcher = PxDefaultCpuDispatcherCreate(Math::Clamp<uint32>(Platform::GetCPUInfo().ProcessorCoreCount - 1, 1, 4));
        CHECK_INIT(scenePhysX->CpuDispatcher, "PxDefaultCpuDispatcherCreate failed!");
        sceneDesc.cpuDispatcher = scenePhysX->CpuDispatcher;
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

#if WITH_VEHICLE
    if (scenePhysX->WheelVehicles.HasItems())
    {
        PROFILE_CPU_NAMED("Physics.Vehicles");

        // Update vehicles steering
        WheelVehiclesCache.Clear();
        WheelVehiclesCache.EnsureCapacity(scenePhysX->WheelVehicles.Count());
        int32 wheelsCount = 0;
        for (auto wheelVehicle : scenePhysX->WheelVehicles)
        {
            if (!wheelVehicle->IsActiveInHierarchy())
                continue;
            auto drive = (PxVehicleWheels*)wheelVehicle->_vehicle;
            ASSERT(drive);
            WheelVehiclesCache.Add(drive);
            wheelsCount += drive->mWheelsSimData.getNbWheels();

            float throttle = wheelVehicle->_throttle;
            float brake = wheelVehicle->_brake;
            if (wheelVehicle->UseReverseAsBrake)
            {
                const float invalidDirectionThreshold = 80.0f;
                const float breakThreshold = 8.0f;
                const float forwardSpeed = wheelVehicle->GetForwardSpeed();

                // Automatic gear change when changing driving direction
                if (Math::Abs(forwardSpeed) < invalidDirectionThreshold)
                {
                    if (throttle < -ZeroTolerance && wheelVehicle->GetCurrentGear() >= 0 && wheelVehicle->GetTargetGear() >= 0)
                    {
                        wheelVehicle->SetCurrentGear(-1);
                    }
                    else if (throttle > ZeroTolerance && wheelVehicle->GetCurrentGear() <= 0 && wheelVehicle->GetTargetGear() <= 0)
                    {
                        wheelVehicle->SetCurrentGear(1);
                    }
                }

                // Automatic break when changing driving direction
                if (throttle > 0.0f)
                {
                    if (forwardSpeed < -invalidDirectionThreshold)
                    {
                        brake = 1.0f;
                    }
                }
                else if (throttle < 0.0f)
                {
                    if (forwardSpeed > invalidDirectionThreshold)
                    {
                        brake = 1.0f;
                    }
                }
                else
                {
                    if (forwardSpeed < breakThreshold && forwardSpeed > -breakThreshold)
                    {
                        brake = 1.0f;
                    }
                }

                // Block throttle if user is changing driving direction
                if ((throttle > 0.0f && wheelVehicle->GetTargetGear() < 0) || (throttle < 0.0f && wheelVehicle->GetTargetGear() > 0))
                {
                    throttle = 0.0f;
                }

                throttle = Math::Abs(throttle);
            }
            else
            {
                throttle = Math::Max(throttle, 0.0f);
            }
            // @formatter:off
            // Reference: PhysX SDK docs
            // TODO: expose input control smoothing data
            static constexpr PxVehiclePadSmoothingData padSmoothing =
            {
	            {
		            6.0f,  // rise rate eANALOG_INPUT_ACCEL
		            6.0f,  // rise rate eANALOG_INPUT_BRAKE
		            12.0f, // rise rate eANALOG_INPUT_HANDBRAKE
		            2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
		            2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
	            },
	            {
		            10.0f, // fall rate eANALOG_INPUT_ACCEL
		            10.0f, // fall rate eANALOG_INPUT_BRAKE
		            12.0f, // fall rate eANALOG_INPUT_HANDBRAKE
		            5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
		            5.0f,  // fall rate eANALOG_INPUT_STEER_RIGHT
	            }
            };
            static constexpr PxVehicleKeySmoothingData keySmoothing =
            {
                {
                    3.0f,  // rise rate eANALOG_INPUT_ACCEL
                    3.0f,  // rise rate eANALOG_INPUT_BRAKE
                    10.0f, // rise rate eANALOG_INPUT_HANDBRAKE
                    2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
                    2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
                },
                {
                    5.0f,  // fall rate eANALOG_INPUT__ACCEL
                    5.0f,  // fall rate eANALOG_INPUT__BRAKE
                    10.0f, // fall rate eANALOG_INPUT__HANDBRAKE
                    5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
                    5.0f   // fall rate eANALOG_INPUT_STEER_RIGHT
                }
            };
            // Reference: PhysX SDK docs
            // TODO: expose steer vs forward curve into per-vehicle (up to 8 points, values clamped into 0/1 range)
            static constexpr PxF32 steerVsForwardSpeedData[] =
            {
	            0.0f,		1.0f,
	            20.0f,		0.9f,
	            65.0f,		0.8f,
	            120.0f,		0.7f,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
	            PX_MAX_F32, PX_MAX_F32,
            };
            const PxFixedSizeLookupTable<8> steerVsForwardSpeed(steerVsForwardSpeedData, 4);
            // @formatter:on
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
                    PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, scenePhysX->LastDeltaTime, false, *(PxVehicleDrive4W*)drive);
                    break;
                }
                case WheeledVehicle::DriveTypes::DriveNW:
                {
                    PxVehicleDriveNWRawInputData rawInputData;
                    rawInputData.setAnalogAccel(throttle);
                    rawInputData.setAnalogBrake(brake);
                    rawInputData.setAnalogSteer(wheelVehicle->_steering);
                    rawInputData.setAnalogHandbrake(wheelVehicle->_handBrake);
                    PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(padSmoothing, steerVsForwardSpeed, rawInputData, scenePhysX->LastDeltaTime, false, *(PxVehicleDriveNW*)drive);
                    break;
                }
                }
            }
            else
            {
                const float deadZone = 0.1f;
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
                    PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, scenePhysX->LastDeltaTime, false, *(PxVehicleDrive4W*)drive);
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
                    PxVehicleDriveNWSmoothDigitalRawInputsAndSetAnalogInputs(keySmoothing, steerVsForwardSpeed, rawInputData, scenePhysX->LastDeltaTime, false, *(PxVehicleDriveNW*)drive);
                    break;
                }
                }
            }
        }

        // Update batches queries cache
        if (wheelsCount > WheelQueryResults.Count())
        {
            if (scenePhysX->WheelRaycastBatchQuery)
                scenePhysX->WheelRaycastBatchQuery->release();
            WheelQueryResults.Resize(wheelsCount, false);
            WheelHitResults.Resize(wheelsCount, false);
            PxBatchQueryDesc desc(wheelsCount, 0, 0);
            desc.queryMemory.userRaycastResultBuffer = WheelQueryResults.Get();
            desc.queryMemory.userRaycastTouchBuffer = WheelHitResults.Get();
            desc.queryMemory.raycastTouchBufferSize = wheelsCount;
            desc.preFilterShader = WheelRaycastPreFilter;
            scenePhysX->WheelRaycastBatchQuery = scenePhysX->Scene->createBatchQuery(desc);
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
        for (int32 i = 0, ii = 0; i < scenePhysX->WheelVehicles.Count(); i++)
        {
            auto wheelVehicle = scenePhysX->WheelVehicles[i];
            if (!wheelVehicle->IsActiveInHierarchy())
                continue;
            auto drive = (PxVehicleWheels*)scenePhysX->WheelVehicles[ii]->_vehicle;
            auto& perVehicle = WheelVehiclesResultsPerVehicle[ii];
            ii++;
            perVehicle.nbWheelQueryResults = drive->mWheelsSimData.getNbWheels();
            perVehicle.wheelQueryResults = WheelVehiclesResultsPerWheel.Get() + wheelsCount;
            wheelsCount += perVehicle.nbWheelQueryResults;
        }

        // Update vehicles
        if (WheelVehiclesCache.Count() != 0)
        {
            PxVehicleSuspensionRaycasts(scenePhysX->WheelRaycastBatchQuery, WheelVehiclesCache.Count(), WheelVehiclesCache.Get(), WheelQueryResults.Count(), WheelQueryResults.Get());
            PxVehicleUpdates(scenePhysX->LastDeltaTime, scenePhysX->Scene->getGravity(), *WheelTireFrictions, WheelVehiclesCache.Count(), WheelVehiclesCache.Get(), WheelVehiclesResultsPerVehicle.Get());
        }

        // Synchronize state
        for (int32 i = 0, ii = 0; i < scenePhysX->WheelVehicles.Count(); i++)
        {
            auto wheelVehicle = scenePhysX->WheelVehicles[i];
            if (!wheelVehicle->IsActiveInHierarchy())
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
                state.TireContactPoint = P2C(perWheel.tireContactPoint) + scenePhysX->Origin;
                state.TireContactNormal = P2C(perWheel.tireContactNormal);
                state.TireFriction = perWheel.tireFriction;
                state.SteerAngle = RadiansToDegrees * perWheel.steerAngle;
                state.RotationAngle = -RadiansToDegrees * drive->mWheelsDynData.getWheelRotationAngle(j);
                state.SuspensionOffset = perWheel.suspJounce;
#if USE_EDITOR
                state.SuspensionTraceStart = P2C(perWheel.suspLineStart) + scenePhysX->Origin;
                state.SuspensionTraceEnd = P2C(perWheel.suspLineStart + perWheel.suspLineDir * perWheel.suspLineLength) + scenePhysX->Origin;
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

    {
        PROFILE_CPU_NAMED("Physics.SendEvents");

        scenePhysX->EventsCallback.CollectResults();
        scenePhysX->EventsCallback.SendTriggerEvents();
        scenePhysX->EventsCallback.SendCollisionEvents();
        scenePhysX->EventsCallback.SendJointEvents();
    }
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
    SceneOrigins[scenePhysX->Scene] = newOrigin;
}

void PhysicsBackend::AddSceneActor(void* scene, void* actor)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();
    scenePhysX->Scene->addActor(*(PxActor*)actor);
    FlushLocker.Unlock();
}

void PhysicsBackend::RemoveSceneActor(void* scene, void* actor)
{
    auto scenePhysX = (ScenePhysX*)scene;
    FlushLocker.Lock();
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

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;
    return scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter);
}

bool PhysicsBackend::RayCast(void* scene, const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(true);
    PxRaycastBuffer buffer;
    if (!scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::RayCastAll(void* scene, const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP(false);
    DynamicHitBuffer<PxRaycastHit> buffer;
    if (!scenePhysX->Scene->raycast(C2P(origin - scenePhysX->Origin), C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter);
}

bool PhysicsBackend::BoxCast(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::BoxCastAll(void* scene, const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter);
}

bool PhysicsBackend::SphereCast(void* scene, const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::SphereCastAll(void* scene, const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_ALL();
    return true;
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter);
}

bool PhysicsBackend::CapsuleCast(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_SINGLE();
    return true;
}

bool PhysicsBackend::CapsuleCastAll(void* scene, const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_SWEEP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
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
    return scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter);
}

bool PhysicsBackend::ConvexCast(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_SWEEP_1();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
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
    if (!scenePhysX->Scene->sweep(geometry, pose, C2P(direction), maxDistance, buffer, hitFlags, filterData, &QueryFilter))
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

bool PhysicsBackend::OverlapBox(void* scene, const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxBoxGeometry geometry(C2P(halfExtents));
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();
    return true;
}

bool PhysicsBackend::OverlapSphere(void* scene, const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin));
    const PxSphereGeometry geometry(radius);
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();
    return true;
}

bool PhysicsBackend::OverlapCapsule(void* scene, const Vector3& center, const float radius, const float height, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxCapsuleGeometry geometry(radius, height * 0.5f);
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();
    return true;
}

bool PhysicsBackend::OverlapConvex(void* scene, const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN(convexMesh && convexMesh->GetOptions().Type == CollisionDataType::ConvexMesh, false)
    SCENE_QUERY_SETUP_OVERLAP();
    const PxTransform pose(C2P(center - scenePhysX->Origin), C2P(rotation));
    const PxConvexMeshGeometry geometry((PxConvexMesh*)convexMesh->GetConvex(), PxMeshScale(C2P(scale)));
    if (!scenePhysX->Scene->overlap(geometry, pose, buffer, filterData, &QueryFilter))
        return false;
    SCENE_QUERY_COLLECT_OVERLAP_COLLIDER();
    return true;
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

void* PhysicsBackend::CreateShape(PhysicsColliderActor* collider, const CollisionShape& geometry, JsonAsset* material, bool enabled, bool trigger)
{
    const PxShapeFlags shapeFlags = GetShapeFlags(trigger, enabled);
    PxMaterial* materialPhysX = DefaultMaterial;
    if (material && !material->WaitForLoaded() && material->Instance)
    {
        materialPhysX = (PxMaterial*)((PhysicalMaterial*)material->Instance)->GetPhysicsMaterial();
    }
    PxGeometryHolder geometryPhysX;
    GetShapeGeometry(geometry, geometryPhysX);
    PxShape* shapePhysX = PhysX->createShape(geometryPhysX.any(), *materialPhysX, true, shapeFlags);
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

void PhysicsBackend::SetShapeMaterial(void* shape, JsonAsset* material)
{
    auto shapePhysX = (PxShape*)shape;
    PxMaterial* materialPhysX = DefaultMaterial;
    if (material && !material->WaitForLoaded() && material->Instance)
    {
        materialPhysX = (PxMaterial*)((PhysicalMaterial*)material->Instance)->GetPhysicsMaterial();
    }
    shapePhysX->setMaterials(&materialPhysX, 1);
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
    const bool result = PxGeometryQuery::computePenetration(dir, distance, shapeAPhysX->getGeometry().any(), poseA, shapeBPhysX->getGeometry().any(), poseB);
    direction = P2C(dir);
    return result;
}

float PhysicsBackend::ComputeShapeSqrDistanceToPoint(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& point, Vector3* closestPoint)
{
    auto shapePhysX = (PxShape*)shape;
    const PxTransform trans(C2P(position), C2P(orientation));
    return PxGeometryQuery::pointDistance(C2P(point), shapePhysX->getGeometry().any(), trans, (PxVec3*)closestPoint);
}

bool PhysicsBackend::RayCastShape(void* shape, const Vector3& position, const Quaternion& orientation, const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance)
{
    auto shapePhysX = (PxShape*)shape;
    const Vector3 sceneOrigin = SceneOrigins[shapePhysX->getActor() ? shapePhysX->getActor()->getScene() : nullptr];
    const PxTransform trans(C2P(position - sceneOrigin), C2P(orientation));
    const PxHitFlags hitFlags = (PxHitFlags)0;
    PxRaycastHit hit;
    if (PxGeometryQuery::raycast(C2P(origin - sceneOrigin), C2P(direction), shapePhysX->getGeometry().any(), trans, maxDistance, hitFlags, 1, &hit) != 0)
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
    const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eFACE_INDEX | PxHitFlag::eUV;
    PxRaycastHit hit;
    if (PxGeometryQuery::raycast(C2P(origin - sceneOrigin), C2P(direction), shapePhysX->getGeometry().any(), trans, maxDistance, hitFlags, 1, &hit) == 0)
        return false;
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
    jointPhysX->setDriveVelocity(Math::Max(value.Velocity, 0.0f));
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
    desc.radius = Math::Max(radius - desc.contactOffset, minSize);
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
    actor->_wheelsData.Resize(wheels.Count());
    auto actorPhysX = (PxRigidDynamic*)actor->GetPhysicsActor();

    InitVehicleSDK();

    // Get linked shapes for the wheels mapping
    Array<PxShape*, InlinedAllocation<8>> shapes;
    shapes.Resize(actorPhysX->getNbShapes());
    actorPhysX->getShapes(shapes.Get(), shapes.Count(), 0);
    const PxTransform centerOfMassOffset = actorPhysX->getCMassLocalPose();

    // Initialize wheels simulation data
    PxVec3 offsets[PX_MAX_NB_WHEELS];
    for (int32 i = 0; i < wheels.Count(); i++)
    {
        auto& wheel = *wheels[i];
        offsets[i] = C2P(wheel.Collider->GetLocalPosition());
    }
    PxF32 sprungMasses[PX_MAX_NB_WHEELS];
    const float mass = actorPhysX->getMass();
    // TODO: get gravityDirection from scenePhysX->Scene->getGravity()
    PxVehicleComputeSprungMasses(wheels.Count(), offsets, centerOfMassOffset.p, mass, 1, sprungMasses);
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(wheels.Count());
    for (int32 i = 0; i < wheels.Count(); i++)
    {
        auto& wheel = *wheels[i];

        auto& data = actor->_wheelsData[i];
        data.Collider = wheel.Collider;
        data.LocalOrientation = wheel.Collider->GetLocalOrientation();

        PxVehicleSuspensionData suspensionData;
        const float suspensionFrequency = 7.0f;
        suspensionData.mMaxCompression = wheel.SuspensionMaxRaise;
        suspensionData.mMaxDroop = wheel.SuspensionMaxDrop;
        suspensionData.mSprungMass = sprungMasses[i];
        suspensionData.mSpringStrength = Math::Square(suspensionFrequency) * suspensionData.mSprungMass;
        suspensionData.mSpringDamperRate = wheel.SuspensionDampingRate * 2.0f * Math::Sqrt(suspensionData.mSpringStrength * suspensionData.mSprungMass);

        PxVehicleTireData tire;
        int32 tireIndex = WheelTireTypes.Find(wheel.TireFrictionScale);
        if (tireIndex == -1)
        {
            // New tire type
            tireIndex = WheelTireTypes.Count();
            WheelTireTypes.Add(wheel.TireFrictionScale);
            WheelTireFrictionsDirty = true;
        }
        tire.mType = tireIndex;
        tire.mLatStiffX = wheel.TireLateralMax;
        tire.mLatStiffY = wheel.TireLateralStiffness;
        tire.mLongitudinalStiffnessPerUnitGravity = wheel.TireLongitudinalStiffness;

        PxVehicleWheelData wheelData;
        wheelData.mMass = wheel.Mass;
        wheelData.mRadius = wheel.Radius;
        wheelData.mWidth = wheel.Width;
        wheelData.mMOI = 0.5f * wheelData.mMass * Math::Square(wheelData.mRadius);
        wheelData.mDampingRate = M2ToCm2(wheel.DampingRate);
        wheelData.mMaxSteer = wheel.MaxSteerAngle * DegreesToRadians;
        wheelData.mMaxBrakeTorque = M2ToCm2(wheel.MaxBrakeTorque);
        wheelData.mMaxHandBrakeTorque = M2ToCm2(wheel.MaxHandBrakeTorque);

        PxVec3 centreOffset = centerOfMassOffset.transformInv(offsets[i]);
        PxVec3 forceAppPointOffset(centreOffset.x, wheel.SuspensionForceOffset, centreOffset.z);

        wheelsSimData->setTireData(i, tire);
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

        // Differential
        PxVehicleDifferential4WData differential4WData;
        differential4WData.mType = (PxVehicleDifferential4WData::Enum)differential.Type;
        differential4WData.mFrontRearSplit = differential.FrontRearSplit;
        differential4WData.mFrontLeftRightSplit = differential.FrontLeftRightSplit;
        differential4WData.mRearLeftRightSplit = differential.RearLeftRightSplit;
        differential4WData.mCentreBias = differential.CentreBias;
        differential4WData.mFrontBias = differential.FrontBias;
        differential4WData.mRearBias = differential.RearBias;
        driveSimData.setDiffData(differential4WData);

        // Engine
        PxVehicleEngineData engineData;
        engineData.mMOI = M2ToCm2(engine.MOI);
        engineData.mPeakTorque = M2ToCm2(engine.MaxTorque);
        engineData.mMaxOmega = RpmToRadPerS(engine.MaxRotationSpeed);
        engineData.mDampingRateFullThrottle = M2ToCm2(0.15f);
        engineData.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(2.0f);
        engineData.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(0.35f);
        driveSimData.setEngineData(engineData);

        // Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = Math::Max(gearbox.SwitchTime, 0.0f);
        driveSimData.setGearsData(gears);

        // Auto Box
        PxVehicleAutoBoxData autoBox;
        driveSimData.setAutoBoxData(autoBox);

        // Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = M2ToCm2(gearbox.ClutchStrength);
        driveSimData.setClutchData(clutch);

        // Ackermann steer accuracy
        PxVehicleAckermannGeometryData ackermann;
        ackermann.mAxleSeparation = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z);
        ackermann.mFrontWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x);
        ackermann.mRearWidth = Math::Abs(wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x - wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x);
        driveSimData.setAckermannGeometryData(ackermann);

        // Create vehicle drive
        auto drive4W = PxVehicleDrive4W::allocate(wheels.Count());
        drive4W->setup(PhysX, actorPhysX, *wheelsSimData, driveSimData, Math::Max(wheels.Count() - 4, 0));
        drive4W->setToRestState();
        drive4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        drive4W->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
        vehicle = drive4W;
        break;
    }
    case WheeledVehicle::DriveTypes::DriveNW:
    {
        PxVehicleDriveSimDataNW driveSimData;

        // Differential
        PxVehicleDifferentialNWData differentialNwData;
        for (int32 i = 0; i < wheels.Count(); i++)
            differentialNwData.setDrivenWheel(i, true);
        driveSimData.setDiffData(differentialNwData);

        // Engine
        PxVehicleEngineData engineData;
        engineData.mMOI = M2ToCm2(engine.MOI);
        engineData.mPeakTorque = M2ToCm2(engine.MaxTorque);
        engineData.mMaxOmega = RpmToRadPerS(engine.MaxRotationSpeed);
        engineData.mDampingRateFullThrottle = M2ToCm2(0.15f);
        engineData.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(2.0f);
        engineData.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(0.35f);
        driveSimData.setEngineData(engineData);

        // Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = Math::Max(gearbox.SwitchTime, 0.0f);
        driveSimData.setGearsData(gears);

        // Auto Box
        PxVehicleAutoBoxData autoBox;
        driveSimData.setAutoBoxData(autoBox);

        // Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = M2ToCm2(gearbox.ClutchStrength);
        driveSimData.setClutchData(clutch);

        // Create vehicle drive
        auto driveNW = PxVehicleDriveNW::allocate(wheels.Count());
        driveNW->setup(PhysX, actorPhysX, *wheelsSimData, driveSimData, wheels.Count());
        driveNW->setToRestState();
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
        driveNo->setToRestState();
        vehicle = driveNo;
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
    }
}

void PhysicsBackend::SetVehicleGearbox(void* vehicle, const void* value)
{
    auto drive = (PxVehicleDrive*)vehicle;
    auto& gearbox = *(const WheeledVehicle::GearboxSettings*)value;
    drive->mDriveDynData.setUseAutoGears(gearbox.AutoGear);
    drive->mDriveDynData.setAutoBoxSwitchTime(Math::Max(gearbox.SwitchTime, 0.0f));
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

void* PhysicsBackend::CreateConvexMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    PxDefaultMemoryInputData input(data, dataSize);
    PxConvexMesh* convexMesh = PhysX->createConvexMesh(input);
    localBounds = P2C(convexMesh->getLocalBounds());
    return convexMesh;
}

void* PhysicsBackend::CreateTriangleMesh(byte* data, int32 dataSize, BoundingBox& localBounds)
{
    PxDefaultMemoryInputData input(data, dataSize);
    PxTriangleMesh* triangleMesh = PhysX->createTriangleMesh(input);
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

float PhysicsBackend::GetHeightFieldHeight(void* heightField, float x, float z)
{
    auto heightFieldPhysX = (PxHeightField*)heightField;
    return heightFieldPhysX->getHeight(x, z);
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

void PhysicsBackend::DestroyObject(void* object)
{
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
