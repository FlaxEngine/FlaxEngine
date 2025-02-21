// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Physics.h"
#include "PhysicsScene.h"
#include "PhysicsBackend.h"
#include "PhysicalMaterial.h"
#include "PhysicsSettings.h"
#include "PhysicsStatistics.h"
#include "Colliders/Collider.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Threading/Threading.h"

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

void PhysicsSettings::Apply()
{
    Time::_physicsMaxDeltaTime = MaxDeltaTime;
    Platform::MemoryCopy(Physics::LayerMasks, LayerMasks, sizeof(LayerMasks));
    Physics::SetGravity(DefaultGravity);
    Physics::SetBounceThresholdVelocity(BounceThresholdVelocity);
    Physics::SetEnableCCD(!DisableCCD);
    PhysicsBackend::ApplySettings(*this);
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
    DESERIALIZE(BroadPhaseType);
    DESERIALIZE(SolverType);
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

PhysicalMaterial::~PhysicalMaterial()
{
    if (_material)
        PhysicsBackend::DestroyMaterial(_material);
}

bool PhysicsService::Init()
{
    // Initialize backend
    if (PhysicsBackend::Init())
        return true;

    // Create default scene
    Physics::DefaultScene = Physics::FindOrCreateScene(TEXT("Default"));
    return Physics::DefaultScene == nullptr;
}

void PhysicsService::LateUpdate()
{
    Physics::FlushRequests();
}

void PhysicsService::Dispose()
{
    // Ensure to finish (wait for simulation end)
    for (PhysicsScene* scene : Physics::Scenes)
    {
        scene->CollectResults();
    }

    // Dispose scenes
    for (PhysicsScene* scene : Physics::Scenes)
    {
        Delete(scene);
    }
    Physics::Scenes.Resize(0);
    Physics::DefaultScene = nullptr;

    // Dispose backend
    PhysicsBackend::Shutdown();
}

PhysicsScene* Physics::FindOrCreateScene(const StringView& name)
{
    auto scene = FindScene(name);
    if (scene == nullptr)
    {
        const auto& settings = *PhysicsSettings::Get();
        scene = New<PhysicsScene>();
        if (scene->Init(name, settings))
        {
            Delete(scene);
            scene = nullptr;
        }
        else
            Scenes.Add(scene);
        return scene;
    }
    return scene;
}

PhysicsScene* Physics::FindScene(const StringView& name)
{
    for (PhysicsScene* scene : Scenes)
    {
        if (scene->GetName() == name)
            return scene;
    }
    return nullptr;
}

bool Physics::GetAutoSimulation()
{
    return !DefaultScene || DefaultScene->GetAutoSimulation();
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
    for (PhysicsScene* scene : Scenes)
    {
        if (scene->GetAutoSimulation())
            scene->Simulate(dt);
    }
}

void Physics::CollectResults()
{
    for (PhysicsScene* scene : Scenes)
    {
        if (scene->GetAutoSimulation())
            scene->CollectResults();
    }
}

bool Physics::IsDuringSimulation()
{
    return DefaultScene && DefaultScene->IsDuringSimulation();
}

void Physics::FlushRequests()
{
    PROFILE_CPU_NAMED("Physics.FlushRequests");
    for (PhysicsScene* scene : Scenes)
        PhysicsBackend::FlushRequests(scene->GetPhysicsScene());
    PhysicsBackend::FlushRequests();
}

bool Physics::LineCast(const Vector3& start, const Vector3& end, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->LineCast(start, end, layerMask, hitTriggers);
}

bool Physics::LineCast(const Vector3& start, const Vector3& end, RayCastHit& hitInfo, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->LineCast(start, end, hitInfo, layerMask, hitTriggers);
}

bool Physics::LineCastAll(const Vector3& start, const Vector3& end, Array<RayCastHit>& results, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->LineCastAll(start, end, results, layerMask, hitTriggers);
}

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->RayCast(origin, direction, maxDistance, layerMask, hitTriggers);
}

bool Physics::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->RayCast(origin, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool Physics::RayCastAll(const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->RayCastAll(origin, direction, results, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->BoxCast(center, halfExtents, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->BoxCast(center, halfExtents, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->BoxCastAll(center, halfExtents, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->SphereCast(center, radius, direction, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCast(const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->SphereCast(center, radius, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool Physics::SphereCastAll(const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->SphereCastAll(center, radius, direction, results, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->CapsuleCast(center, radius, height, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->CapsuleCast(center, radius, height, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CapsuleCastAll(const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->CapsuleCastAll(center, radius, height, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->ConvexCast(center, convexMesh, scale, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->ConvexCast(center, convexMesh, scale, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::ConvexCastAll(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return DefaultScene->ConvexCastAll(center, convexMesh, scale, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool Physics::CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckBox(center, halfExtents, rotation, layerMask, hitTriggers);
}

bool Physics::CheckSphere(const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckSphere(center, radius, layerMask, hitTriggers);
}

bool Physics::CheckCapsule(const Vector3& center, const float radius, const float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckCapsule(center, radius, height, rotation, layerMask, hitTriggers);
}

bool Physics::CheckConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->CheckConvex(center, convexMesh, scale, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapBox(center, halfExtents, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapSphere(center, radius, results, layerMask, hitTriggers);
}

bool Physics::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapCapsule(center, radius, height, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapConvex(center, convexMesh, scale, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapBox(center, halfExtents, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapSphere(const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapSphere(center, radius, results, layerMask, hitTriggers);
}

bool Physics::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapCapsule(center, radius, height, results, rotation, layerMask, hitTriggers);
}

bool Physics::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return DefaultScene->OverlapConvex(center, convexMesh, scale, results, rotation, layerMask, hitTriggers);
}

PhysicsScene::PhysicsScene(const SpawnParams& params)
    : ScriptingObject(params)
{
}

PhysicsScene::~PhysicsScene()
{
    if (_scene)
        PhysicsBackend::DestroyScene(_scene);
}

String PhysicsScene::GetName() const
{
    return _name;
}

bool PhysicsScene::GetAutoSimulation() const
{
    return _autoSimulation;
}

void PhysicsScene::SetAutoSimulation(bool value)
{
    _autoSimulation = value;
}

void PhysicsScene::SetGravity(const Vector3& value)
{
    PhysicsBackend::SetSceneGravity(_scene, value);
}

Vector3 PhysicsScene::GetGravity() const
{
    return PhysicsBackend::GetSceneGravity(_scene);
}

bool PhysicsScene::GetEnableCCD() const
{
    return PhysicsBackend::GetSceneEnableCCD(_scene);
}

void PhysicsScene::SetEnableCCD(bool value)
{
    PhysicsBackend::SetSceneEnableCCD(_scene, value);
}

float PhysicsScene::GetBounceThresholdVelocity()
{
    return PhysicsBackend::GetSceneBounceThresholdVelocity(_scene);
}

void PhysicsScene::SetBounceThresholdVelocity(float value)
{
    PhysicsBackend::SetSceneBounceThresholdVelocity(_scene, value);
}

void PhysicsScene::SetOrigin(const Vector3& value)
{
    if (_origin != value)
    {
        PhysicsBackend::SetSceneOrigin(_scene, _origin, value);
        _origin = value;
    }
}

#if COMPILE_WITH_PROFILER

PhysicsStatistics PhysicsScene::GetStatistics() const
{
    PhysicsStatistics result;
    PhysicsBackend::GetSceneStatistics(_scene, result);
    return result;
}

#endif

bool PhysicsScene::Init(const StringView& name, const PhysicsSettings& settings)
{
    if (_scene)
    {
        PhysicsBackend::DestroyScene(_scene);
    }
    _name = name;
    _scene = PhysicsBackend::CreateScene(settings);
    return _scene == nullptr;
}

void PhysicsScene::Simulate(float dt)
{
    ASSERT(IsInMainThread() && !_isDuringSimulation);
    _isDuringSimulation = true;
    PhysicsBackend::StartSimulateScene(_scene, dt);
}

bool PhysicsScene::IsDuringSimulation() const
{
    return _isDuringSimulation;
}

void PhysicsScene::CollectResults()
{
    if (!_isDuringSimulation)
        return;
    ASSERT(IsInMainThread());
    PhysicsBackend::EndSimulateScene(_scene);
    _isDuringSimulation = false;
}

bool PhysicsScene::LineCast(const Vector3& start, const Vector3& end, uint32 layerMask, bool hitTriggers)
{
    Vector3 directionToEnd = end - start;
    const Real distanceToEnd = directionToEnd.Length();
    if (distanceToEnd >= ZeroTolerance)
        directionToEnd /= distanceToEnd;
    return PhysicsBackend::RayCast(_scene, start, directionToEnd, (float)distanceToEnd, layerMask, hitTriggers);
}

bool PhysicsScene::LineCast(const Vector3& start, const Vector3& end, RayCastHit& hitInfo, uint32 layerMask, bool hitTriggers)
{
    Vector3 directionToEnd = end - start;
    const Real distanceToEnd = directionToEnd.Length();
    if (distanceToEnd >= ZeroTolerance)
        directionToEnd /= distanceToEnd;
    return PhysicsBackend::RayCast(_scene, start, directionToEnd, hitInfo, (float)distanceToEnd, layerMask, hitTriggers);
}

bool PhysicsScene::LineCastAll(const Vector3& start, const Vector3& end, Array<RayCastHit>& results, uint32 layerMask, bool hitTriggers)
{
    Vector3 directionToEnd = end - start;
    const Real distanceToEnd = directionToEnd.Length();
    if (distanceToEnd >= ZeroTolerance)
        directionToEnd /= distanceToEnd;
    return PhysicsBackend::RayCastAll(_scene, start, directionToEnd, results, (float)distanceToEnd, layerMask, hitTriggers);
}

bool PhysicsScene::RayCast(const Vector3& origin, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::RayCast(_scene, origin, direction, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::RayCast(_scene, origin, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::RayCastAll(const Vector3& origin, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::RayCastAll(_scene, origin, direction, results, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::BoxCast(_scene, center, halfExtents, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::BoxCast(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::BoxCast(_scene, center, halfExtents, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::BoxCastAll(const Vector3& center, const Vector3& halfExtents, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::BoxCastAll(_scene, center, halfExtents, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::SphereCast(const Vector3& center, const float radius, const Vector3& direction, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::SphereCast(_scene, center, radius, direction, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::SphereCast(const Vector3& center, const float radius, const Vector3& direction, RayCastHit& hitInfo, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::SphereCast(_scene, center, radius, direction, hitInfo, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::SphereCastAll(const Vector3& center, const float radius, const Vector3& direction, Array<RayCastHit>& results, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::SphereCastAll(_scene, center, radius, direction, results, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::CapsuleCast(_scene, center, radius, height, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::CapsuleCast(const Vector3& center, const float radius, const float height, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::CapsuleCast(_scene, center, radius, height, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::CapsuleCastAll(const Vector3& center, const float radius, const float height, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::CapsuleCastAll(_scene, center, radius, height, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::ConvexCast(_scene, center, convexMesh, scale, direction, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::ConvexCast(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, RayCastHit& hitInfo, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::ConvexCast(_scene, center, convexMesh, scale, direction, hitInfo, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::ConvexCastAll(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Vector3& direction, Array<RayCastHit>& results, const Quaternion& rotation, const float maxDistance, uint32 layerMask, bool hitTriggers)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), false);
    return PhysicsBackend::ConvexCastAll(_scene, center, convexMesh, scale, direction, results, rotation, maxDistance, layerMask, hitTriggers);
}

bool PhysicsScene::CheckBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::CheckBox(_scene, center, halfExtents, rotation, layerMask, hitTriggers);
}

bool PhysicsScene::CheckSphere(const Vector3& center, const float radius, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::CheckSphere(_scene, center, radius, layerMask, hitTriggers);
}

bool PhysicsScene::CheckCapsule(const Vector3& center, const float radius, const float height, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::CheckCapsule(_scene, center, radius, height, rotation, layerMask, hitTriggers);
}

bool PhysicsScene::CheckConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::CheckConvex(_scene, center, convexMesh, scale, rotation, layerMask, hitTriggers);
}

bool PhysicsScene::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    Array<PhysicsColliderActor*> tmp;
    if (PhysicsBackend::OverlapBox(_scene, center, halfExtents, tmp, rotation, layerMask, hitTriggers))
    {
        results.EnsureCapacity(tmp.Count());
        for (PhysicsColliderActor* e : tmp)
        {
            if (e && e->Is<Collider>())
                results.Add((Collider*)e);
        }
        return true;
    }
    return false;
}

bool PhysicsScene::OverlapSphere(const Vector3& center, const float radius, Array<Collider*>& results, uint32 layerMask, bool hitTriggers)
{
    Array<PhysicsColliderActor*> tmp;
    if (PhysicsBackend::OverlapSphere(_scene, center, radius, tmp, layerMask, hitTriggers))
    {
        results.EnsureCapacity(tmp.Count());
        for (PhysicsColliderActor* e : tmp)
        {
            if (e && e->Is<Collider>())
                results.Add((Collider*)e);
        }
        return true;
    }
    return false;
}

bool PhysicsScene::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    Array<PhysicsColliderActor*> tmp;
    if (PhysicsBackend::OverlapCapsule(_scene, center, radius, height, tmp, rotation, layerMask, hitTriggers))
    {
        results.EnsureCapacity(tmp.Count());
        for (PhysicsColliderActor* e : tmp)
        {
            if (e && e->Is<Collider>())
                results.Add((Collider*)e);
        }
        return true;
    }
    return false;
}

bool PhysicsScene::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<Collider*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    Array<PhysicsColliderActor*> tmp;
    if (PhysicsBackend::OverlapConvex(_scene, center, convexMesh, scale, tmp, rotation, layerMask, hitTriggers))
    {
        results.EnsureCapacity(tmp.Count());
        for (PhysicsColliderActor* e : tmp)
        {
            if (e && e->Is<Collider>())
                results.Add((Collider*)e);
        }
        return true;
    }
    return false;
}

bool PhysicsScene::OverlapBox(const Vector3& center, const Vector3& halfExtents, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::OverlapBox(_scene, center, halfExtents, results, rotation, layerMask, hitTriggers);
}

bool PhysicsScene::OverlapSphere(const Vector3& center, const float radius, Array<PhysicsColliderActor*>& results, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::OverlapSphere(_scene, center, radius, results, layerMask, hitTriggers);
}

bool PhysicsScene::OverlapCapsule(const Vector3& center, const float radius, const float height, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::OverlapCapsule(_scene, center, radius, height, results, rotation, layerMask, hitTriggers);
}

bool PhysicsScene::OverlapConvex(const Vector3& center, const CollisionData* convexMesh, const Vector3& scale, Array<PhysicsColliderActor*>& results, const Quaternion& rotation, uint32 layerMask, bool hitTriggers)
{
    return PhysicsBackend::OverlapConvex(_scene, center, convexMesh, scale, results, rotation, layerMask, hitTriggers);
}
