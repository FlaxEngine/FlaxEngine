// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Physics.h"
#include "PhysicsScene.h"
#include "PhysicalMaterial.h"

#include "Engine/Core/Log.h"
#include "Engine/Platform/CPUInfo.h"
#include "PhysicsSettings.h"
#include "Utilities.h"
#include "PhysicsStepper.h"

#include "Engine/Level/Level.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Engine/Time.h"
#include <ThirdParty/PhysX/PxPhysicsAPI.h>
#if WITH_PVD
#include <ThirdParty/PhysX/pvd/PxPvd.h>
#endif

#define PHYSX_VEHICLE_DEBUG_TELEMETRY 0

#if PHYSX_VEHICLE_DEBUG_TELEMETRY
#include "Engine/Core/Utilities.h"
#endif

class PhysXAllocator : public PxAllocatorCallback
{
public:

    void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
        return Allocator::Allocate(size, 16);
    }

    void deallocate(void* ptr) override
    {
        Allocator::Free(ptr);
    }
};

class PhysXError : public PxErrorCallback
{
public:

    PhysXError()
    {
    }

    ~PhysXError()
    {
    }

public:

    // [PxErrorCallback]
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        LOG(Error, "PhysX Error! Code: {0}.\n{1}\nSource: {2} : {3}.", static_cast<int32>(code), String(message), String(file), line);
    }
};

PxPhysics* CPhysX = nullptr;
#if WITH_PVD
PxPvd* CPVD = nullptr;
#endif

namespace
{
    PhysXAllocator PhysXAllocatorCallback;
    PhysXError PhysXErrorCallback;
    PxTolerancesScale ToleranceScale;
    bool _queriesHitTriggers = true;
    PhysicsCombineMode _frictionCombineMode = PhysicsCombineMode::Average;
    PhysicsCombineMode _restitutionCombineMode = PhysicsCombineMode::Average;
    PxFoundation* _foundation = nullptr;
#if COMPILE_WITH_PHYSICS_COOKING
    PxCooking* Cooking = nullptr;
#endif
    PxMaterial* DefaultMaterial = nullptr;
#if WITH_VEHICLE
    bool VehicleSDKInitialized = false;
#endif

    void reset()
    {
        Physics::Scenes.Resize(1);
    }
}

PhysicsScene* Physics::DefaultScene = nullptr;
Array<PhysicsScene*> Physics::Scenes;
uint32 Physics::LayerMasks[32];

class PhysicsService : public EngineService
{
public:

    PhysicsService()
        : EngineService(TEXT("Physics"), 0)
    {
        for (int32 i = 0; i < 32; i++)
            Physics::LayerMasks[i] = MAX_uint32;
    }

    bool Init() override;
    void LateUpdate() override;
    void Dispose() override;
};

PhysicsService PhysicsServiceInstance;

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

#if WITH_VEHICLE

void InitVehicleSDK()
{
    if (!VehicleSDKInitialized)
    {
        VehicleSDKInitialized = true;
        PxInitVehicleSDK(*CPhysX);
        PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(1, 0, 0));
        PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
    }
}

#endif

void PhysicsSettings::Apply()
{
    Time::_physicsMaxDeltaTime = MaxDeltaTime;
    _queriesHitTriggers = QueriesHitTriggers;
    _frictionCombineMode = FrictionCombineMode;
    _restitutionCombineMode = RestitutionCombineMode;
    Platform::MemoryCopy(Physics::LayerMasks, LayerMasks, sizeof(LayerMasks));
    Physics::SetGravity(DefaultGravity);
    Physics::SetBounceThresholdVelocity(BounceThresholdVelocity);
    Physics::SetEnableCCD(!DisableCCD);

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

PhysicsSettings::PhysicsSettings()
{
    for (int32 i = 0; i < 32; i++)
        LayerMasks[i] = MAX_uint32;
}

void PhysicsSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(DefaultGravity);
    DESERIALIZE(TriangleMeshTriangleMinAreaThreshold);
    DESERIALIZE(BounceThresholdVelocity);
    DESERIALIZE(FrictionCombineMode);
    DESERIALIZE(RestitutionCombineMode);
    DESERIALIZE(DisableCCD);
    DESERIALIZE(EnableAdaptiveForce);
    DESERIALIZE(MaxDeltaTime);
    DESERIALIZE(EnableSubstepping);
    DESERIALIZE(SubstepDeltaTime);
    DESERIALIZE(MaxSubsteps);
    DESERIALIZE(QueriesHitTriggers);
    DESERIALIZE(SupportCookingAtRuntime);

    const auto layers = stream.FindMember("LayerMasks");
    if (layers != stream.MemberEnd())
    {
        auto& layersArray = layers->value;
        ASSERT(layersArray.IsArray());
        for (uint32 i = 0; i < layersArray.Size() && i < 32; i++)
        {
            LayerMasks[i] = layersArray[i].GetUint();
        }
    }
}

PhysicalMaterial::PhysicalMaterial()
    : _material(nullptr)
{
}

PhysicalMaterial::~PhysicalMaterial()
{
    if (_material)
    {
        for (auto scene : Physics::Scenes) 
            scene->RemoveMaterial(_material);
    }
}

PxMaterial* PhysicalMaterial::GetPhysXMaterial()
{
    if (_material == nullptr && CPhysX)
    {
        _material = CPhysX->createMaterial(Friction, Friction, Restitution);
        _material->userData = this;

        const PhysicsCombineMode useFrictionCombineMode = OverrideFrictionCombineMode ? FrictionCombineMode : _frictionCombineMode;
        _material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        const PhysicsCombineMode useRestitutionCombineMode = OverrideRestitutionCombineMode ? RestitutionCombineMode : _restitutionCombineMode;
        _material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));
    }

    return _material;
}

void PhysicalMaterial::UpdatePhysXMaterial()
{
    if (_material != nullptr)
    {
        _material->setStaticFriction(Friction);
        _material->setDynamicFriction(Friction);

        const PhysicsCombineMode useFrictionCombineMode = OverrideFrictionCombineMode ? FrictionCombineMode : _frictionCombineMode;
        _material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        _material->setRestitution(Restitution);
        const PhysicsCombineMode useRestitutionCombineMode = OverrideRestitutionCombineMode ? RestitutionCombineMode : _restitutionCombineMode;
        _material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));
    }
}

bool PhysicsService::Init()
{
#define CHECK_INIT(value, msg) if(!value) { LOG(Error, msg); return true; }

    auto& settings = *PhysicsSettings::Get();

    // Send info
    LOG(Info, "Setup NVIDIA PhysX {0}.{1}.{2}", PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX);

    // Init PhysX foundation object
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION, PhysXAllocatorCallback, PhysXErrorCallback);
    CHECK_INIT(_foundation, "PxCreateFoundation failed!");

    // Config
    ToleranceScale.length = 100;
    ToleranceScale.speed = 981;

    PxPvd* pvd = nullptr;
#if WITH_PVD
    {
        // Init PVD
        pvd = PxCreatePvd(*_foundation);
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
        CPVD = pvd;
    }

#endif

    // Init PhysX
    CPhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, ToleranceScale, false, pvd);
    CHECK_INIT(CPhysX, "PxCreatePhysics failed!");

    // Init extensions
    const bool extensionsInit = PxInitExtensions(*CPhysX, pvd);
    CHECK_INIT(extensionsInit, "PxInitExtensions failed!");

    // Init collision cooking
#if COMPILE_WITH_PHYSICS_COOKING
#if !USE_EDITOR
    if (settings.SupportCookingAtRuntime)
#endif
    {
        PxCookingParams cookingParams(ToleranceScale);
        cookingParams.meshWeldTolerance = 0.1f; // Weld to 1mm precision
        cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
        Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, cookingParams);
        CHECK_INIT(Cooking, "PxCreateCooking failed!");
    }
#endif

    Physics::DefaultScene = Physics::FindOrCreateScene(TEXT("Default"));

    // Create default resources
    DefaultMaterial = CPhysX->createMaterial(0.7f, 0.7f, 0.3f);

    return false;

#undef CHECK_INIT
}

void PhysicsService::LateUpdate()
{
    for (auto scene : Physics::Scenes)
        scene->FlushRequests();
}

void PhysicsService::Dispose()
{
    // Ensure to finish (wait for simulation end)

    for (auto scene : Physics::Scenes)
    {
        scene->CollectResults();

        if (CPhysX)
            scene->FlushRequests();
    }

#if WITH_VEHICLE
    if (VehicleSDKInitialized)
    {
        VehicleSDKInitialized = false;
        PxCloseVehicleSDK();
    }
#endif

    // Cleanup
    RELEASE_PHYSX(DefaultMaterial);

    Physics::Scenes.Resize(0);
    Physics::DefaultScene = nullptr;

    // Remove all scenes still registered
    const int32 numScenes = CPhysX ? CPhysX->getNbScenes() : 0;
    if (numScenes)
    {
        Array<PxScene*> PScenes;
        PScenes.Resize(numScenes);
        PScenes.SetAll(nullptr);
        CPhysX->getScenes(PScenes.Get(), sizeof(PxScene*) * numScenes);

        for (int32 i = 0; i < numScenes; i++)
        {
            if (PScenes[i])
            {
                PScenes[i]->release();
            }
        }
    }

#if COMPILE_WITH_PHYSICS_COOKING
    RELEASE_PHYSX(Cooking);
#endif

    if (CPhysX)
    {
        PxCloseExtensions();
    }

    RELEASE_PHYSX(CPhysX);
#if WITH_PVD
    RELEASE_PHYSX(CPVD);
#endif
    RELEASE_PHYSX(_foundation);
}

PxPhysics* Physics::GetPhysics()
{
    return CPhysX;
}

PxCooking* Physics::GetCooking()
{
    return Cooking;
}

PxTolerancesScale* Physics::GetTolerancesScale()
{
    return &ToleranceScale;
}

Vector3 Physics::GetGravity()
{
    return DefaultScene ? DefaultScene->GetGravity() : Vector3::Zero;
}

void Physics::SetGravity(const Vector3& value)
{
    if (DefaultScene)
        DefaultScene->SetGravity(value);
}

bool Physics::GetEnableCCD()
{
    return DefaultScene ? DefaultScene->GetEnableCCD() : !PhysicsSettings::Get()->DisableCCD;
}

void Physics::SetEnableCCD(const bool value)
{
    if (DefaultScene)
        DefaultScene->SetEnableCCD(value);
}

float Physics::GetBounceThresholdVelocity()
{
    return DefaultScene ? DefaultScene->GetBounceThresholdVelocity() : PhysicsSettings::Get()->BounceThresholdVelocity;
}

void Physics::SetBounceThresholdVelocity(const float value)
{
    if (DefaultScene)
        DefaultScene->SetBounceThresholdVelocity(value);
}

void Physics::Simulate(float dt)
{
    if (DefaultScene)
        DefaultScene->Simulate(dt);
}

void Physics::SimulateAll(float dt)
{
    for (auto scene : Scenes)
    {
        if (scene->GetAutoSimulation())
            scene->Simulate(dt);
    }
}

void Physics::CollectResults()
{
    if (DefaultScene)
        DefaultScene->CollectResults();
}

void Physics::CollectResultsAll()
{
    for (auto scene : Scenes)
        scene->CollectResults();
}

bool Physics::IsDuringSimulation()
{
    if (DefaultScene)
        return DefaultScene->IsDuringSimulation();

    return false;
}

PxMaterial* Physics::GetDefaultMaterial()
{
    return DefaultMaterial;
}

bool Physics::GetAutoSimulation()
{
    if (DefaultScene)
        return DefaultScene->GetAutoSimulation();

    return false;
}

void Physics::FlushRequestsAll()
{
    for (auto scene : Scenes) 
        scene->FlushRequests();
}

void Physics::RemoveJoint(Joint* joint)
{
    for (auto scene : Scenes) 
        scene->RemoveJoint(joint);
}

PhysicsScene* Physics::FindOrCreateScene(const String& name)
{
    auto scene = FindScene(name);

    if (scene == nullptr)
    {
        auto& settings = *PhysicsSettings::Get();

        scene = New<PhysicsScene>(name, settings);
        Scenes.Add(scene);
    }

    return scene;
}

PhysicsScene* Physics::FindScene(const String& name)
{
    for (auto scene : Scenes)
    {
        if (scene->GetName() == name)
            return scene;
    } 

    return nullptr;
}
