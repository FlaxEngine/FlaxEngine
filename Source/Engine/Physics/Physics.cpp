// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Physics.h"
#include "PhysicalMaterial.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Platform/CPUInfo.h"
#include "PhysicsSettings.h"
#include "Utilities.h"
#include "PhysicsStepper.h"
#include "SimulationEventCallback.h"
#include "Engine/Level/Level.h"
#include "Actors/PhysicsActor.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Engine/Time.h"
#include <ThirdParty/PhysX/PxPhysicsAPI.h>
#include <ThirdParty/PhysX/PxActor.h>
#if WITH_PVD
#include <ThirdParty/PhysX/pvd/PxPvd.h>
#endif

// Temporary memory size used by the PhysX during the simulation. Must be multiply of 4kB and 16bit aligned.
#define SCRATCH_BLOCK_SIZE (1024 * 128)

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

PxFilterFlags PhysiXFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
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
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
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

enum class ActionType
{
    Sleep,
};

struct ActionData
{
    ActionType Type;
    PxActor* Actor;
};

PxPhysics* CPhysX = nullptr;
#if WITH_PVD
PxPvd* CPVD = nullptr;
#endif

namespace
{
    PhysXAllocator PhysXAllocatorCallback;
    PhysXError PhysXErrorCallback;
    PxSimulationFilterShader PhysXDefaultFilterShader = PxDefaultSimulationFilterShader;
    PxTolerancesScale ToleranceScale;
    SimulationEventCallback EventsCallback;
    void* ScratchMemory = nullptr;
    FixedStepper* Stepper = nullptr;
    CriticalSection FlushLocker;
    Array<PxActor*> NewActors;
    Array<ActionData> Actions;
    Array<PxActor*> DeadActors;
    Array<PxMaterial*> DeadMaterials;
    Array<PxBase*> _deadObjects;
    Array<PhysicsColliderActor*> DeadColliders;
    Array<Joint*> DeadJoints;
    bool _queriesHitTriggers = true;
    bool _isDuringSimulation = false;
    PhysicsCombineMode _frictionCombineMode = PhysicsCombineMode::Average;
    PhysicsCombineMode _restitutionCombineMode = PhysicsCombineMode::Average;
    PxFoundation* _foundation = nullptr;
#if COMPILE_WITH_PHYSICS_COOKING
    PxCooking* Cooking = nullptr;
#endif
    PxScene* PhysicsScene = nullptr;
    PxMaterial* DefaultMaterial = nullptr;
    PxControllerManager* ControllerManager = nullptr;
    PxCpuDispatcher* CpuDispatcher = nullptr;
}

bool Physics::AutoSimulation = true;
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
        Physics::RemoveMaterial(_material);
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

    auto cpuInfo = Platform::GetCPUInfo();
    auto& settings = *PhysicsSettings::Get();

    // Send info
    LOG(Info, "Setup NVIDIA PhysX {0}.{1}.{2}", PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX);

    // Init PhysX foundation object
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION, PhysXAllocatorCallback, PhysXErrorCallback);
    CHECK_INIT(_foundation, "PxCreateFoundation failed!");

    // Recording memory allocations is necessary if you want to 
    // use the memory facilities in PVD effectively.  Since PVD isn't necessarily connected
    // right away, we add a mechanism that records all outstanding memory allocations and
    // forwards them to PVD when it does connect.
    // This certainly has a performance and memory profile effect and thus should be used
    // only in non-production builds.

#if PHYSX_MEMORY_STATS
	_foundation->setReportAllocationNames(true);
#endif

    // Config
    ToleranceScale.length = 100;
    ToleranceScale.speed = 981;

    PxPvd* pvd = nullptr;
#if WITH_PVD
	{
		// Connection parameters
		const char*  pvd_host_ip = "127.0.0.1";  // IP of the PC which is running PVD
		int          port = 5425;                // TCP port to connect to, where PVD is listening
		unsigned int timeout = 100;              // Timeout in milliseconds to wait for PVD to respond, consoles and remote PCs need a higher timeout.

		// Init PVD
		pvd = PxCreatePvd(*_foundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(pvd_host_ip, port, timeout);
		const bool isConnected = pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
		if (isConnected)
		{
			LOG(Info, "Connected to PhysX Visual Debugger (PVD)"));
		}

		CPVD = pvd;
	}

#endif

    // Init top-level PhysX objects
    CPhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, ToleranceScale, false, pvd);
    CHECK_INIT(CPhysX, "PxCreatePhysics failed!");

    // Init Extensions
    const bool extensionsInit = PxInitExtensions(*CPhysX, pvd);
    CHECK_INIT(extensionsInit, "PxInitExtensions failed!");
#if WITH_VEHICLE
	PxInitVehicleSDK(*Physics);
#endif

#if COMPILE_WITH_PHYSICS_COOKING

#if !USE_EDITOR
    if (settings.SupportCookingAtRuntime)
#endif
    {
        // Init cooking
        PxCookingParams cookingParams(ToleranceScale);
        cookingParams.meshWeldTolerance = 0.1f; // Weld to 1mm precision
        cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
        Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, cookingParams);
        CHECK_INIT(Cooking, "PxCreateCooking failed!");
    }

#endif

    // Create scene description
    PxSceneDesc sceneDesc(CPhysX->getTolerancesScale());
    sceneDesc.gravity = C2P(settings.DefaultGravity);
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    //sceneDesc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS; // TODO: set it?
    if (!settings.DisableCCD)
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    if (settings.EnableAdaptiveForce)
        sceneDesc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    sceneDesc.simulationEventCallback = &EventsCallback;
    sceneDesc.filterShader = PhysiXFilterShader;
    sceneDesc.bounceThresholdVelocity = settings.BounceThresholdVelocity;
    if (sceneDesc.cpuDispatcher == nullptr)
    {
        CpuDispatcher = PxDefaultCpuDispatcherCreate(Math::Clamp<uint32>(cpuInfo.ProcessorCoreCount - 1, 1, 4));
        CHECK_INIT(CpuDispatcher, "PxDefaultCpuDispatcherCreate failed!");
        sceneDesc.cpuDispatcher = CpuDispatcher;
    }
    if (sceneDesc.filterShader == nullptr)
    {
        sceneDesc.filterShader = PhysXDefaultFilterShader;
    }

    // Create scene
    PhysicsScene = CPhysX->createScene(sceneDesc);
    CHECK_INIT(PhysicsScene, "createScene failed!");

    // Init characters controller
    ControllerManager = PxCreateControllerManager(*PhysicsScene);

    // Create default resources
    DefaultMaterial = CPhysX->createMaterial(0.7f, 0.7f, 0.3f);

    return false;

#undef CHECK_INIT
}

void PhysicsService::LateUpdate()
{
    Physics::FlushRequests();
}

void PhysicsService::Dispose()
{
    // Ensure to finish (wait for simulation end)
    Physics::CollectResults();

    // Cleanup
    if (CPhysX)
        Physics::FlushRequests();
    RELEASE_PHYSX(DefaultMaterial);
    SAFE_DELETE(Stepper);
    Allocator::Free(ScratchMemory);
    ScratchMemory = nullptr;
    RELEASE_PHYSX(ControllerManager);
    SAFE_DELETE(CpuDispatcher);

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
#if WITH_VEHICLE
		PxCloseVehicleSDK();
#endif
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

PxScene* Physics::GetScene()
{
    return PhysicsScene;
}

PxControllerManager* Physics::GetControllerManager()
{
    return ControllerManager;
}

PxTolerancesScale* Physics::GetTolerancesScale()
{
    return &ToleranceScale;
}

Vector3 Physics::GetGravity()
{
    return PhysicsScene ? P2C(PhysicsScene->getGravity()) : Vector3::Zero;
}

void Physics::SetGravity(const Vector3& value)
{
    if (PhysicsScene)
        PhysicsScene->setGravity(C2P(value));
}

bool Physics::GetEnableCCD()
{
    return PhysicsScene ? (PhysicsScene->getFlags() & PxSceneFlag::eENABLE_CCD) == PxSceneFlag::eENABLE_CCD : !PhysicsSettings::Get()->DisableCCD;
}

void Physics::SetEnableCCD(const bool value)
{
    if (PhysicsScene)
        PhysicsScene->setFlag(PxSceneFlag::eENABLE_CCD, value);
}

float Physics::GetBounceThresholdVelocity()
{
    return PhysicsScene ? PhysicsScene->getBounceThresholdVelocity() : PhysicsSettings::Get()->BounceThresholdVelocity;
}

void Physics::SetBounceThresholdVelocity(const float value)
{
    if (PhysicsScene)
        PhysicsScene->setBounceThresholdVelocity(value);
}

void Physics::Simulate(float dt)
{
    ASSERT(IsInMainThread() && !_isDuringSimulation);
    ASSERT(CPhysX);
    const auto& settings = *PhysicsSettings::Get();

    // Flush the old/new objects and the other requests before the simulation
    FlushRequests();

    // Clamp delta
    dt = Math::Clamp(dt, 0.0f, settings.MaxDeltaTime);

    // Prepare util objects
    if (ScratchMemory == nullptr)
    {
        ScratchMemory = Allocator::Allocate(SCRATCH_BLOCK_SIZE, 16);
    }
    if (Stepper == nullptr)
    {
        Stepper = New<FixedStepper>();
    }
    if (settings.EnableSubstepping)
    {
        // Use substeps
        Stepper->Setup(settings.SubstepDeltaTime, settings.MaxSubsteps);
    }
    else
    {
        // Use single step
        Stepper->Setup(dt);
    }

    // Start simulation (may not be fired due to too small delta time)
    _isDuringSimulation = true;
    if (Stepper->advance(PhysicsScene, dt, ScratchMemory, SCRATCH_BLOCK_SIZE) == false)
        return;
    EventsCallback.Clear();

    // TODO: move this call after rendering done
    Stepper->renderDone();
}

void Physics::CollectResults()
{
    ASSERT(IsInMainThread());
    if (!_isDuringSimulation)
        return;
    ASSERT(CPhysX && Stepper);

    {
        PROFILE_CPU_NAMED("Physics.Fetch");

        // Gather results (with waiting for the end)
        Stepper->wait(PhysicsScene);
    }

    {
        PROFILE_CPU_NAMED("Physics.FlushActiveTransforms");

        // Gather change info
        PxU32 activeActorsCount;
        PxActor** activeActors = PhysicsScene->getActiveActors(activeActorsCount);
        if (activeActorsCount > 0)
        {
            // Update changed transformations
            // TODO: use jobs system if amount if huge
            for (uint32 i = 0; i < activeActorsCount; i++)
            {
                const auto pxActor = (PxRigidActor*)*activeActors++;
                auto actor = dynamic_cast<IPhysicsActor*>((Actor*)pxActor->userData);
                ASSERT(actor);
                actor->OnActiveTransformChanged(pxActor->getGlobalPose());
            }
        }
    }

    {
        PROFILE_CPU_NAMED("Physics.SendEvents");

        EventsCallback.CollectResults();
        EventsCallback.SendTriggerEvents();
        EventsCallback.SendCollisionEvents();
        EventsCallback.SendJointEvents();
    }

    // End
    _isDuringSimulation = false;
}

bool Physics::IsDuringSimulation()
{
    return _isDuringSimulation;
}

PxMaterial* Physics::GetDefaultMaterial()
{
    return DefaultMaterial;
}

void Physics::FlushRequests()
{
    ASSERT(!IsDuringSimulation());
    ASSERT(CPhysX);

    PROFILE_CPU();

    FlushLocker.Lock();

    // Note: this does not handle case when actor is removed and added to the scene at the same time

    if (NewActors.HasItems())
    {
        GetScene()->addActors(NewActors.Get(), NewActors.Count());
        NewActors.Clear();
    }

    for (int32 i = 0; i < Actions.Count(); i++)
    {
        const auto action = Actions[i];
        switch (action.Type)
        {
        case ActionType::Sleep:
            static_cast<PxRigidDynamic*>(action.Actor)->putToSleep();
            break;
        }
    }
    Actions.Clear();

    if (DeadActors.HasItems())
    {
        GetScene()->removeActors(DeadActors.Get(), DeadActors.Count(), true);
        for (int32 i = 0; i < DeadActors.Count(); i++)
        {
            DeadActors[i]->release();
        }
        DeadActors.Clear();
    }

    if (DeadColliders.HasItems())
    {
        for (int32 i = 0; i < DeadColliders.Count(); i++)
        {
            EventsCallback.OnColliderRemoved(DeadColliders[i]);
        }
        DeadColliders.Clear();
    }

    if (DeadJoints.HasItems())
    {
        for (int32 i = 0; i < DeadJoints.Count(); i++)
        {
            EventsCallback.OnJointRemoved(DeadJoints[i]);
        }
        DeadJoints.Clear();
    }

    for (int32 i = 0; i < DeadMaterials.Count(); i++)
    {
        auto material = DeadMaterials[i];

        // Unlink ref to flax object
        material->userData = nullptr;

        material->release();
    }
    DeadMaterials.Clear();

    for (int32 i = 0; i < _deadObjects.Count(); i++)
    {
        _deadObjects[i]->release();
    }
    _deadObjects.Clear();

    FlushLocker.Unlock();
}

void Physics::RemoveMaterial(PxMaterial* material)
{
    ASSERT(material);

    FlushLocker.Lock();
    DeadMaterials.Add(material);
    FlushLocker.Unlock();
}

void Physics::RemoveObject(PxBase* obj)
{
    ASSERT(obj);

    FlushLocker.Lock();
    _deadObjects.Add(obj);
    FlushLocker.Unlock();
}

void Physics::AddActor(PxActor* actor)
{
    ASSERT(actor);

    FlushLocker.Lock();
    if (IsInMainThread())
    {
        GetScene()->addActor(*actor);
    }
    else
    {
        NewActors.Add(actor);
    }
    FlushLocker.Unlock();
}

void Physics::AddActor(PxRigidDynamic* actor, bool putToSleep)
{
    ASSERT(actor);

    FlushLocker.Lock();
    if (IsInMainThread())
    {
        GetScene()->addActor(*actor);
        if (putToSleep)
            actor->putToSleep();
    }
    else
    {
        NewActors.Add(actor);
        if (putToSleep)
            Actions.Add({ ActionType::Sleep, actor });
    }
    FlushLocker.Unlock();
}

void Physics::RemoveActor(PxActor* actor)
{
    ASSERT(actor);

    // Unlink ref to flax object
    actor->userData = nullptr;

    FlushLocker.Lock();
    DeadActors.Add(actor);
    FlushLocker.Unlock();
}

void Physics::RemoveCollider(PhysicsColliderActor* collider)
{
    ASSERT(collider);

    FlushLocker.Lock();
    DeadColliders.Add(collider);
    FlushLocker.Unlock();
}

void Physics::RemoveJoint(Joint* joint)
{
    ASSERT(joint);

    FlushLocker.Lock();
    DeadJoints.Add(joint);
    FlushLocker.Unlock();
}
