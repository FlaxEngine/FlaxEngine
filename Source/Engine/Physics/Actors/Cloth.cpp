// Copyright (c) Wojciech Figat. All rights reserved.

#include "Cloth.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Models/MeshAccessor.h"
#include "Engine/Graphics/Models/MeshBase.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#include "Engine/Level/Scene/SceneRendering.h"
#if USE_EDITOR
#include "Engine/Debug/DebugDraw.h"
#endif

Cloth::Cloth(const SpawnParams& params)
    : Actor(params)
{
    // Use the first mesh by default
    _mesh.LODIndex = _mesh.MeshIndex = 0;

    // Register for drawing to handle culling and distance LOD
    _drawCategory = SceneRendering::SceneDrawAsync;
}

void* Cloth::GetPhysicsCloth() const
{
    return _cloth;
}

ModelInstanceActor::MeshReference Cloth::GetMesh() const
{
    auto value = _mesh;
    value.Actor = Cast<ModelInstanceActor>(GetParent()); // Force to use cloth's parent only
    return value;
}

void Cloth::SetMesh(const ModelInstanceActor::MeshReference& value)
{
    if (_mesh.LODIndex == value.LODIndex && _mesh.MeshIndex == value.MeshIndex)
        return;

    // Remove mesh deformer (mesh index/lod changes)
    if (_meshDeformation)
    {
        Function<void(const MeshBase*, MeshDeformationData&)> deformer;
        deformer.Bind<Cloth, &Cloth::RunClothDeformer>(this);
        _meshDeformation->RemoveDeformer(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0, deformer);
        _meshDeformation->RemoveDeformer(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex1, deformer);
        _meshDeformation = nullptr;
    }

    _mesh = value;
    _mesh.Actor = nullptr; // Don't store this reference
#if WITH_CLOTH
    if (_cloth)
        Rebuild();
#endif
}

void Cloth::SetForce(const ForceSettings& value)
{
    _forceSettings = value;
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::SetClothForceSettings(_cloth, &value);
#endif
}

void Cloth::SetCollision(const CollisionSettings& value)
{
    _collisionSettings = value;
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::SetClothCollisionSettings(_cloth, &value);
#endif
}

void Cloth::SetSimulation(const SimulationSettings& value)
{
    _simulationSettings = value;
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::SetClothSimulationSettings(_cloth, &value);
#endif
}

void Cloth::SetFabric(const FabricSettings& value)
{
    _fabricSettings = value;
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::SetClothFabricSettings(_cloth, &value);
#endif
}

void Cloth::Rebuild()
{
#if WITH_CLOTH
    if (_cloth)
    {
        // Remove old
        if (IsDuringPlay())
            PhysicsBackend::RemoveCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
        DestroyCloth();
    }

    // Create new
    CreateCloth();
    if (IsDuringPlay() && _cloth)
        PhysicsBackend::AddCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
#endif
}

void Cloth::ClearInteria()
{
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::ClearClothInertia(_cloth);
#endif
}

Array<Float3> Cloth::GetParticles() const
{
    Array<Float3> result;
#if WITH_CLOTH
    if (_cloth)
    {
        PROFILE_CPU();
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
        result.Resize(particles.Length());
        const Float4* src = particles.Get();
        Float3* dst = result.Get();
        for (int32 i = 0; i < particles.Length(); i++)
            dst[i] = Float3(src[i]);
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
    return result;
}

void Cloth::SetParticles(Span<const Float3> value)
{
    PROFILE_CPU();
#if USE_CLOTH_SANITY_CHECKS
    {
        // Sanity check
        const Float3* src = value.Get();
        bool allValid = true;
        for (int32 i = 0; i < value.Length(); i++)
            allValid &= !src[i].IsNanOrInfinity();
        ASSERT(allValid);
    }
#endif
#if WITH_CLOTH
    if (_cloth)
    {
        // Update cloth particles
        PhysicsBackend::LockClothParticles(_cloth);
        PhysicsBackend::SetClothParticles(_cloth, Span<const Float4>(), value, Span<const float>());
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
}

Span<float> Cloth::GetPaint() const
{
    return ToSpan(_paint);
}

void Cloth::SetPaint(Span<const float> value)
{
    PROFILE_CPU();
#if USE_CLOTH_SANITY_CHECKS
    {
        // Sanity check
        const float* src = value.Get();
        bool allValid = true;
        for (int32 i = 0; i < value.Length(); i++)
            allValid &= !isnan(src[i]) && !isinf(src[i]);
        ASSERT(allValid);
    }
#endif
    if (value.IsInvalid())
    {
        // Remove paint when set to empty
        _paint.SetCapacity(0);
#if WITH_CLOTH
        if (_cloth)
        {
            PhysicsBackend::SetClothPaint(_cloth, value);
        }
#endif
        return;
    }
    _paint.Set(value.Get(), value.Length());
#if WITH_CLOTH
    if (_cloth)
    {
        // Update cloth particles
        Array<float> invMasses;
        CalculateInvMasses(invMasses);
        PhysicsBackend::LockClothParticles(_cloth);
        PhysicsBackend::SetClothParticles(_cloth, Span<const Float4>(), Span<const Float3>(), ToSpan<float, const float>(invMasses));
        PhysicsBackend::SetClothPaint(_cloth, value);
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
}

bool Cloth::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    if (!Actor::IntersectsItself(ray, distance, normal))
        return false;
#if WITH_CLOTH
    if (_cloth)
    {
        // Precise per-triangle intersection
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return false;
        BytesContainer indicesData;
        int32 indicesCount;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
            return false;
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
        const Transform transform = GetTransform();
        const bool indices16bit = indicesData.Length() / indicesCount == sizeof(uint16);
        const int32 trianglesCount = indicesCount / 3;
        bool result = false;
        distance = MAX_Real;
        for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
        {
            const int32 index = triangleIndex * 3;
            int32 i0, i1, i2;
            if (indices16bit)
            {
                i0 = indicesData.Get<uint16>()[index];
                i1 = indicesData.Get<uint16>()[index + 1];
                i2 = indicesData.Get<uint16>()[index + 2];
            }
            else
            {
                i0 = indicesData.Get<uint32>()[index];
                i1 = indicesData.Get<uint32>()[index + 1];
                i2 = indicesData.Get<uint32>()[index + 2];
            }
            const Vector3 v0 = transform.LocalToWorld(Vector3(particles[i0]));
            const Vector3 v1 = transform.LocalToWorld(Vector3(particles[i1]));
            const Vector3 v2 = transform.LocalToWorld(Vector3(particles[i2]));
            Real d;
            if (CollisionsHelper::RayIntersectsTriangle(ray, v0, v1, v2, d) && d < distance)
            {
                result = true;
                normal = Vector3::Normalize((v1 - v0) ^ (v2 - v0));
                distance = d;

                // Flip normal if needed as cloth is two-sided
                const Vector3 hitPos = ray.GetPoint(d);
                if (Vector3::DistanceSquared(hitPos + normal, ray.Position) > Math::Square(d))
                    normal = -normal;
            }
        }
        PhysicsBackend::UnlockClothParticles(_cloth);
        return result;
    }
#endif
    return false;
#else
    return Actor::IntersectsItself(ray, distance, normal);
#endif
}

void Cloth::Serialize(SerializeStream& stream, const void* otherObj)
{
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Cloth);

    SERIALIZE_MEMBER(Mesh, _mesh);
    SERIALIZE_MEMBER(Force, _forceSettings);
    SERIALIZE_MEMBER(Collision, _collisionSettings);
    SERIALIZE_MEMBER(Simulation, _simulationSettings);
    SERIALIZE_MEMBER(Fabric, _fabricSettings);
    if (Serialization::ShouldSerialize(_paint, other ? &other->_paint : nullptr))
    {
        // Serialize as Base64
        stream.JKEY("Paint");
        stream.Blob(_paint.Get(), _paint.Count() * sizeof(float));
    }
}

void Cloth::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Mesh, _mesh);
    _mesh.Actor = nullptr; // Don't store this reference
    DESERIALIZE_MEMBER(Force, _forceSettings);
    DESERIALIZE_MEMBER(Collision, _collisionSettings);
    DESERIALIZE_MEMBER(Simulation, _simulationSettings);
    DESERIALIZE_MEMBER(Fabric, _fabricSettings);
    DESERIALIZE_MEMBER(Paint, _paint);

#if USE_CLOTH_SANITY_CHECKS
    {
        // Sanity check
        const float* data = _paint.Get();
        bool allValid = true;
        for (int32 i = 0; i < _paint.Count(); i++)
            allValid &= !isnan(data[i]) && !isinf(data[i]);
        ASSERT(allValid);
    }
#endif

    // Refresh cloth when settings were changed
#if WITH_CLOTH
    if (_cloth)
        Rebuild();
#endif
}

#if USE_EDITOR

void Cloth::DrawPhysicsDebug(RenderView& view)
{
#if WITH_CLOTH && COMPILE_WITH_DEBUG_DRAW
    if (_cloth)
    {
        PROFILE_CPU();
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return;
        BytesContainer indicesData;
        int32 indicesCount;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
            return;
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
        const Transform transform = GetTransform();
        const bool indices16bit = indicesData.Length() / indicesCount == sizeof(uint16);
        const int32 trianglesCount = indicesCount / 3;
        for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
        {
            const int32 index = triangleIndex * 3;
            int32 i0, i1, i2;
            if (indices16bit)
            {
                i0 = indicesData.Get<uint16>()[index];
                i1 = indicesData.Get<uint16>()[index + 1];
                i2 = indicesData.Get<uint16>()[index + 2];
            }
            else
            {
                i0 = indicesData.Get<uint32>()[index];
                i1 = indicesData.Get<uint32>()[index + 1];
                i2 = indicesData.Get<uint32>()[index + 2];
            }
            if (_paint.Count() == particles.Length())
            {
                if (Math::Max(_paint[i0], _paint[i1], _paint[i2]) < ZeroTolerance)
                    continue;
            }
            const Vector3 v0 = transform.LocalToWorld(Vector3(particles[i0]));
            const Vector3 v1 = transform.LocalToWorld(Vector3(particles[i1]));
            const Vector3 v2 = transform.LocalToWorld(Vector3(particles[i2]));
            DEBUG_DRAW_TRIANGLE(v0, v1, v2, Color::Pink, 0, true);
        }
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
}

void Cloth::OnDebugDrawSelected()
{
#if WITH_CLOTH && COMPILE_WITH_DEBUG_DRAW
    if (_cloth)
    {
        DEBUG_DRAW_WIRE_BOX(_box, Color::Violet.RGBMultiplied(0.8f), 0, true);
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return;
        BytesContainer indicesData;
        int32 indicesCount;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
            return;
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
        const Transform transform = GetTransform();
        const bool indices16bit = indicesData.Length() / indicesCount == sizeof(uint16);
        const int32 trianglesCount = indicesCount / 3;
        for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
        {
            const int32 index = triangleIndex * 3;
            int32 i0, i1, i2;
            if (indices16bit)
            {
                i0 = indicesData.Get<uint16>()[index];
                i1 = indicesData.Get<uint16>()[index + 1];
                i2 = indicesData.Get<uint16>()[index + 2];
            }
            else
            {
                i0 = indicesData.Get<uint32>()[index];
                i1 = indicesData.Get<uint32>()[index + 1];
                i2 = indicesData.Get<uint32>()[index + 2];
            }
            const Vector3 v0 = transform.LocalToWorld(Vector3(particles[i0]));
            const Vector3 v1 = transform.LocalToWorld(Vector3(particles[i1]));
            const Vector3 v2 = transform.LocalToWorld(Vector3(particles[i2]));
            Color c0 = Color::White, c1 = Color::White, c2 = Color::White;
            if (_paint.Count() == particles.Length())
            {
                c0 = Color::Lerp(Color::Red, Color::White, _paint[i0]);
                c1 = Color::Lerp(Color::Red, Color::White, _paint[i1]);
                c2 = Color::Lerp(Color::Red, Color::White, _paint[i2]);
            }
            DebugDraw::DrawLine(v0, v1, c0, c1, 0, DebugDrawDepthTest);
            DebugDraw::DrawLine(v1, v2, c1, c2, 0, DebugDrawDepthTest);
            DebugDraw::DrawLine(v2, v0, c2, c0, 0, DebugDrawDepthTest);
        }
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif

    Actor::OnDebugDrawSelected();
}

#endif

void Cloth::BeginPlay(SceneBeginData* data)
{
#if WITH_CLOTH
    if (CreateCloth())
        LOG(Error, "Failed to create cloth '{0}'", GetNamePath());
#endif

    Actor::BeginPlay(data);
}

void Cloth::EndPlay()
{
    Actor::EndPlay();

    DestroyCloth();
}

void Cloth::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddPhysicsDebug<Cloth, &Cloth::DrawPhysicsDebug>(this);
#endif
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::AddCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
#endif

    Actor::OnEnable();
}

void Cloth::OnDisable()
{
    Actor::OnDisable();

#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::RemoveCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
#endif
#if USE_EDITOR
    GetSceneRendering()->RemovePhysicsDebug<Cloth, &Cloth::DrawPhysicsDebug>(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
}

void Cloth::OnDeleteObject()
{
    DestroyCloth();

    Actor::OnDeleteObject();
}

void Cloth::OnParentChanged()
{
    Actor::OnParentChanged();

#if WITH_CLOTH
    if (_cloth)
        Rebuild();
#endif
}

void Cloth::OnTransformChanged()
{
    Actor::OnTransformChanged();

#if WITH_CLOTH
    if (_cloth)
    {
        // Move cloth but consider this as teleport if the position delta is significant
        const float minTeleportDistanceSq = Math::Square(1000.0f);
        const bool teleport = Vector3::DistanceSquared(_cachedPosition, _transform.Translation) >= minTeleportDistanceSq;
        _cachedPosition = _transform.Translation;
        PhysicsBackend::SetClothTransform(_cloth, _transform, teleport);
    }
    else
#endif
    {
        _box = BoundingBox(_transform.Translation);
        _sphere = BoundingSphere(_transform.Translation, 0.0f);
    }
}

void Cloth::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    Actor::OnPhysicsSceneChanged(previous);

#if WITH_CLOTH
    if (_cloth)
    {
        PhysicsBackend::RemoveCloth(previous->GetPhysicsScene(), _cloth);
        void* scene = GetPhysicsScene()->GetPhysicsScene();
        PhysicsBackend::AddCloth(scene, _cloth);
    }
#endif
}

bool Cloth::CreateCloth()
{
#if WITH_CLOTH
    PROFILE_CPU();

    // Skip if all vertices are fixed so cloth sim doesn't make sense
    if (_paint.HasItems())
    {
        bool allZero = true;
        for (int32 i = 0; i < _paint.Count() && allZero; i++)
            allZero = _paint[i] <= ZeroTolerance;
        if (allZero)
            return false;
    }

    // Get mesh data
    // TODO: consider making it via async task so physics can wait on the cloth setup from mesh data just before next fixed update which gives more time when loading scene
    const ModelInstanceActor::MeshReference mesh = GetMesh();
    if (mesh.Actor == nullptr)
        return false;
    PhysicsClothDesc desc;
    desc.Actor = this;
    BytesContainer data;
    int32 count;
    if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Vertex0, data, count))
        return true;
    // TODO: use MeshAccessor vertex data layout descriptor instead hardcoded position data at the beginning of VB0
    desc.VerticesData = data.Get();
    desc.VerticesCount = count;
    desc.VerticesStride = data.Length() / count;
    if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, data, count))
        return true;
    desc.IndicesData = data.Get();
    desc.IndicesCount = count;
    desc.IndicesStride = data.Length() / count;
    Array<float> invMasses;
    CalculateInvMasses(invMasses);
    desc.InvMassesData = invMasses.Count() == desc.VerticesCount ? invMasses.Get() : nullptr;
    desc.InvMassesStride = sizeof(float);
    desc.MaxDistancesData = _paint.Count() == desc.VerticesCount ? _paint.Get() : nullptr;
    desc.MaxDistancesStride = sizeof(float);

    // Create cloth
    ASSERT(_cloth == nullptr);
    _cloth = PhysicsBackend::CreateCloth(desc);
    if (_cloth == nullptr)
        return true;
    _cachedPosition = _transform.Translation;
    PhysicsBackend::SetClothForceSettings(_cloth, &_forceSettings);
    PhysicsBackend::SetClothCollisionSettings(_cloth, &_collisionSettings);
    PhysicsBackend::SetClothSimulationSettings(_cloth, &_simulationSettings);
    PhysicsBackend::SetClothFabricSettings(_cloth, &_fabricSettings);
    PhysicsBackend::SetClothTransform(_cloth, _transform, true);
    PhysicsBackend::ClearClothInertia(_cloth);

    // Add cloth mesh deformer
    if (auto* deformation = mesh.Actor->GetMeshDeformation())
    {
        Function<void(const MeshBase*, MeshDeformationData&)> deformer;
        deformer.Bind<Cloth, &Cloth::RunClothDeformer>(this);
        deformation->AddDeformer(mesh.LODIndex, mesh.MeshIndex, MeshBufferType::Vertex0, deformer);
        if (_simulationSettings.ComputeNormals)
            deformation->AddDeformer(mesh.LODIndex, mesh.MeshIndex, MeshBufferType::Vertex1, deformer);
        _meshDeformation = deformation;
    }

    _lastMinDstSqr = MAX_Real;
#endif

    return false;
}

void Cloth::DestroyCloth()
{
#if WITH_CLOTH
    _lastMinDstSqr = MAX_Real;
    if (_meshDeformation)
    {
        Function<void(const MeshBase*, MeshDeformationData&)> deformer;
        deformer.Bind<Cloth, &Cloth::RunClothDeformer>(this);
        _meshDeformation->RemoveDeformer(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0, deformer);
        _meshDeformation->RemoveDeformer(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex1, deformer);
        _meshDeformation = nullptr;
    }
    if (_cloth)
    {
        PhysicsBackend::DestroyCloth(_cloth);
        _cloth = nullptr;
    }
#endif
}

void Cloth::CalculateInvMasses(Array<float>& invMasses)
{
    // Use per-particle max distance to evaluate which particles are immovable
#if WITH_CLOTH
    if (_paint.IsEmpty())
        return;
    PROFILE_CPU();

    // Get mesh data
    const ModelInstanceActor::MeshReference mesh = GetMesh();
    if (mesh.Actor == nullptr)
        return;
    BytesContainer verticesData;
    int32 verticesCount;
    if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Vertex0, verticesData, verticesCount))
        return;
    BytesContainer indicesData;
    int32 indicesCount;
    if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
        return;
    if (_paint.Count() != verticesCount)
    {
        // Fix incorrect paint data
        LOG(Warning, "Incorrect cloth '{}' paint size {} for mesh '{}' that has {} vertices", GetNamePath(), _paint.Count(), mesh.ToString(), verticesCount);
        int32 countBefore = _paint.Count();
        _paint.Resize(verticesCount);
        for (int32 i = countBefore; i < verticesCount; i++)
            _paint.Get()[i] = 0.0f;
    }
    const int32 verticesStride = verticesData.Length() / verticesCount;
    const bool indices16bit = indicesData.Length() / indicesCount == sizeof(uint16);
    const int32 trianglesCount = indicesCount / 3;

    // Sum triangle area for each influenced particle
    invMasses.Resize(verticesCount);
    Platform::MemoryClear(invMasses.Get(), verticesCount * sizeof(float));
    if (indices16bit)
    {
        for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
        {
            const int32 index = triangleIndex * 3;
            const int32 i0 = indicesData.Get<uint16>()[index];
            const int32 i1 = indicesData.Get<uint16>()[index + 1];
            const int32 i2 = indicesData.Get<uint16>()[index + 2];
            // TODO: use MeshAccessor vertex data layout descriptor instead hardcoded position data at the beginning of VB0
#define GET_POS(i) *(Float3*)((byte*)verticesData.Get() + i * verticesStride)
            const Float3 v0(GET_POS(i0));
            const Float3 v1(GET_POS(i1));
            const Float3 v2(GET_POS(i2));
#undef GET_POS
            const float area = Float3::TriangleArea(v0, v1, v2);
            invMasses.Get()[i0] += area;
            invMasses.Get()[i1] += area;
            invMasses.Get()[i2] += area;
        }
    }
    else
    {
        for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
        {
            const int32 index = triangleIndex * 3;
            const int32 i0 = indicesData.Get<uint32>()[index];
            const int32 i1 = indicesData.Get<uint32>()[index + 1];
            const int32 i2 = indicesData.Get<uint32>()[index + 2];
            // TODO: use MeshAccessor vertex data layout descriptor instead hardcoded position data at the beginning of VB0
#define GET_POS(i) *(Float3*)((byte*)verticesData.Get() + i * verticesStride)
            const Float3 v0(GET_POS(i0));
            const Float3 v1(GET_POS(i1));
            const Float3 v2(GET_POS(i2));
#undef GET_POS
            const float area = Float3::TriangleArea(v0, v1, v2);
            invMasses.Get()[i0] += area;
            invMasses.Get()[i1] += area;
            invMasses.Get()[i2] += area;
        }
    }

    // Count fixed vertices which max movement distance is zero
    int32 fixedCount = 0;
    float massSum = 0;
    for (int32 i = 0; i < verticesCount; i++)
    {
        float& mass = invMasses.Get()[i];
#if USE_CLOTH_SANITY_CHECKS
        // Sanity check
        ASSERT(!isnan(mass) && !isinf(mass) && mass >= 0.0f);
#endif
        const float maxDistance = _paint.Get()[i];
        if (maxDistance < 0.01f)
        {
            // Fixed
            fixedCount++;
            mass = 0.0f;
        }
        else
        {
            // Kinetic so include it's mass contribution
            massSum += mass;
        }
    }

    if (massSum > ZeroTolerance)
    {
        // Normalize and inverse particles mass
        const float massScale = (float)(verticesCount - fixedCount) / massSum;
        for (int32 i = 0; i < verticesCount; i++)
        {
            float& mass = invMasses.Get()[i];
            if (mass > 0.0f)
            {
                mass *= massScale;
                mass = 1.0f / mass;
            }
        }
    }

#if USE_CLOTH_SANITY_CHECKS
    {
        // Sanity check
        const float* data = invMasses.Get();
        bool allValid = true;
        for (int32 i = 0; i < invMasses.Count(); i++)
            allValid &= !isnan(data[i]) && !isinf(data[i]);
        ASSERT(allValid);
    }
#endif
#endif
}

bool Cloth::OnPreUpdate()
{
#if WITH_CLOTH
    if (!IsActiveInHierarchy())
        return true;
    if (!_simulationSettings.UpdateWhenOffscreen && _simulationSettings.CullDistance > 0)
    {
        // Cull based on distance
        bool cull = false;
        if (_lastMinDstSqr >= Math::Square(_simulationSettings.CullDistance))
            cull = true; // Cull
        else if (_lastMinDstSqr >= Math::Square(_simulationSettings.CullDistance * 0.8f))
            cull = _frameCounter % 4 == 0; // Update once every 4 frames
        else if (_lastMinDstSqr >= Math::Square(_simulationSettings.CullDistance * 0.5f))
            cull = _frameCounter % 2 == 0; // Update once every 2 frames
        _lastMinDstSqr = MAX_Real;
        _frameCounter++;
        if (cull)
            return true;
    }

    // Get current skinned mesh pose for the simulation of the non-kinematic vertices
    if (auto* animatedModel = Cast<AnimatedModel>(GetParent()))
    {
        if (animatedModel->GraphInstance.NodesPose.IsEmpty() || _paint.IsEmpty())
            return false;
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return false;
        BytesContainer verticesData;
        int32 verticesCount;
        GPUVertexLayout* layout;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Vertex0, verticesData, verticesCount, &layout))
            return false;
        MeshAccessor accessor;
        if (accessor.LoadBuffer(MeshBufferType::Vertex0, verticesData, layout))
            return false;
        auto positionStream = accessor.Position();
        auto blendIndicesStream = accessor.BlendIndices();
        auto blendWeightsStream = accessor.BlendWeights();
        if (!positionStream.IsValid() || !blendIndicesStream.IsValid() || !blendWeightsStream.IsValid())
            return false;
        if (verticesCount != _paint.Count())
        {
            LOG(Warning, "Incorrect cloth '{}' paint size {} for mesh '{}' that has {} vertices", GetNamePath(), _paint.Count(), mesh.ToString(), verticesCount);
            return false;
        }
        PROFILE_CPU_NAMED("Skinned Pose");
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
        // TODO: optimize memory allocs (eg. write directly to nvCloth mapped range or use shared allocator)
        Array<Float4> particlesSkinned;
        particlesSkinned.Set(particles.Get(), particles.Length());

        // TODO: optimize memory allocs (eg. get pose as Span<Matrix> for readonly)
        Array<Matrix> pose;
        animatedModel->GetCurrentPose(pose);
        const SkeletonData& skeleton = animatedModel->SkinnedModel->Skeleton;
        const SkeletonBone* bones = skeleton.Bones.Get();

        // Animated model uses skinning thus requires to set vertex position inverse to skeleton bones
        const float* paint = _paint.Get();
        bool anyFixed = false;
        for (int32 i = 0; i < verticesCount; i++)
        {
            if (paint[i] > ZeroTolerance)
                continue;

            // Load vertex
            Float3 position = positionStream.GetFloat3(i);
            const Int4 blendIndices = blendIndicesStream.GetFloat4(i);
            const Float4 blendWeights = blendWeightsStream.GetFloat4(i);

            // Calculate skinned vertex matrix from bones blending
            // TODO: optimize this or use _skinningData from AnimatedModel to access current mesh bones data directly
            Matrix matrix;
            const SkeletonBone& bone0 = bones[blendIndices.X];
            Matrix::Multiply(bone0.OffsetMatrix, pose[bone0.NodeIndex], matrix);
            Matrix boneMatrix = matrix * blendWeights.X;
            if (blendWeights.Y > 0.0f)
            {
                const SkeletonBone& bone1 = bones[blendIndices.Y];
                Matrix::Multiply(bone1.OffsetMatrix, pose[bone1.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Y;
            }
            if (blendWeights.Z > 0.0f)
            {
                const SkeletonBone& bone2 = bones[blendIndices.Z];
                Matrix::Multiply(bone2.OffsetMatrix, pose[bone2.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Z;
            }
            if (blendWeights.W > 0.0f)
            {
                const SkeletonBone& bone3 = bones[blendIndices.W];
                Matrix::Multiply(bone3.OffsetMatrix, pose[bone3.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.W;
            }

            // Skin vertex position (similar to GPU vertex shader)
            position = Float3::Transform(position, boneMatrix);

            // Transform back to the cloth space
            // TODO: skip when using identity?
            position = _localTransform.WorldToLocal(position);

            // Override fixed particle position
            particlesSkinned[i] = Float4(position, 0.0f);
            anyFixed = true;
        }

        if (anyFixed)
        {
            // Update particles
            PhysicsBackend::SetClothParticles(_cloth, ToSpan<Float4, const Float4>(particlesSkinned), Span<const Float3>(), Span<const float>());
            PhysicsBackend::SetClothPaint(_cloth, ToSpan<float, const float>(_paint));
        }

        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
    return false;
}

void Cloth::OnPostUpdate()
{
    if (_meshDeformation)
    {
        // Mark mesh as dirty
        Matrix invWorld;
        GetWorldToLocalMatrix(invWorld);
        BoundingBox localBounds;
        BoundingBox::Transform(_box, invWorld, localBounds);
        _meshDeformation->Dirty(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0, localBounds);
        if (_simulationSettings.ComputeNormals)
            _meshDeformation->Dirty(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex1, localBounds);

        // Update bounds (for mesh culling)
        auto* actor = (ModelInstanceActor*)GetParent();
        actor->UpdateBounds();
        if (_sceneRenderingKey != -1)
            GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
    }
}

void Cloth::Draw(RenderContext& renderContext)
{
    // Update min draw distance for the next simulation tick
    _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(_transform.Translation, renderContext.View.WorldPosition));
}

void Cloth::Draw(RenderContextBatch& renderContextBatch)
{
    // Update min draw distance for the next simulation tick
    const RenderContext& renderContext = renderContextBatch.GetMainContext();
    _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(_transform.Translation, renderContext.View.WorldPosition));
}

void Cloth::RunClothDeformer(const MeshBase* mesh, MeshDeformationData& deformation)
{
    if (!IsActiveInHierarchy())
        return;
    if (!_simulationSettings.ComputeNormals && deformation.Type != MeshBufferType::Vertex0)
        return;
#if WITH_CLOTH
    PROFILE_CPU_NAMED("Cloth");
    PhysicsBackend::LockClothParticles(_cloth);
    const Span<const Float4> particles = PhysicsBackend::GetClothParticles(_cloth);
    auto vbCount = (uint32)mesh->GetVertexCount();
    ASSERT((uint32)particles.Length() >= vbCount);

    // Calculate normals
    Array<Float3> normals;
    const ModelInstanceActor::MeshReference meshRef = GetMesh();
    BytesContainer indicesData;
    int32 indicesCount;
    if ((_simulationSettings.ComputeNormals || deformation.Type == MeshBufferType::Vertex1) &&
        meshRef.Actor && !meshRef.Actor->GetMeshData(meshRef, MeshBufferType::Index, indicesData, indicesCount))
    {
        PROFILE_CPU_NAMED("Normals");
        // TODO: optimize memory allocs (eg. use shared allocator)
        normals.Resize(vbCount);
        Platform::MemoryClear(normals.Get(), vbCount * sizeof(Float3));
        const bool indices16bit = indicesData.Length() / indicesCount == sizeof(uint16);
        const int32 trianglesCount = indicesCount / 3;
        if (indices16bit)
        {
            for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
            {
                const int32 index = triangleIndex * 3;
                const int32 i0 = indicesData.Get<uint16>()[index];
                const int32 i1 = indicesData.Get<uint16>()[index + 1];
                const int32 i2 = indicesData.Get<uint16>()[index + 2];
                const Float3 v0(particles.Get()[i0]);
                const Float3 v1(particles.Get()[i1]);
                const Float3 v2(particles.Get()[i2]);
                const Float3 normal = Float3::Cross(v1 - v0, v2 - v0);
                normals.Get()[i0] += normal;
                normals.Get()[i1] += normal;
                normals.Get()[i2] += normal;
            }
        }
        else
        {
            for (int32 triangleIndex = 0; triangleIndex < trianglesCount; triangleIndex++)
            {
                const int32 index = triangleIndex * 3;
                const int32 i0 = indicesData.Get<uint32>()[index];
                const int32 i1 = indicesData.Get<uint32>()[index + 1];
                const int32 i2 = indicesData.Get<uint32>()[index + 2];
                const Float3 v0(particles.Get()[i0]);
                const Float3 v1(particles.Get()[i1]);
                const Float3 v2(particles.Get()[i2]);
                const Float3 normal = Float3::Cross(v1 - v0, v2 - v0);
                normals.Get()[i0] += normal;
                normals.Get()[i1] += normal;
                normals.Get()[i2] += normal;
            }
        }
    }

    // Update mesh vertices based on the cloth particles positions
    MeshAccessor accessor;
    if (deformation.LoadMeshAccessor(accessor))
    {
        PhysicsBackend::UnlockClothParticles(_cloth);
        return;
    }
    if (auto* animatedModel = Cast<AnimatedModel>(GetParent()))
    {
        if (animatedModel->GraphInstance.NodesPose.IsEmpty())
        {
            // Delay until skinning data is ready
            PhysicsBackend::UnlockClothParticles(_cloth);
            _meshDeformation->Dirty(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0);
            return;
        }

        // TODO: optimize memory allocs (eg. get pose as Span<Matrix> for readonly)
        Array<Matrix> pose;
        animatedModel->GetCurrentPose(pose);
        const SkeletonData& skeleton = animatedModel->SkinnedModel->Skeleton;

        // Animated model uses skinning thus requires to set vertex position inverse to skeleton bones
        auto positionStream = accessor.Position();
        auto blendIndicesStream = accessor.BlendIndices();
        auto blendWeightsStream = accessor.BlendWeights();
        if (!positionStream.IsValid() || !blendIndicesStream.IsValid() || !blendWeightsStream.IsValid())
        {
            PhysicsBackend::UnlockClothParticles(_cloth);
            return;
        }
        const float* paint = _paint.Count() >= particles.Length() ? _paint.Get() : nullptr;
        for (uint32 i = 0; i < vbCount; i++)
        {
            // Skip fixed vertices
            if (paint && paint[i] < ZeroTolerance)
                continue;

            // Calculate skinned vertex matrix from bones blending
            const Int4 blendIndices = blendIndicesStream.GetFloat4(i);
            const Float4 blendWeights = blendWeightsStream.GetFloat4(i);
            // TODO: optimize this or use _skinningData from AnimatedModel to access current mesh bones data directly
            Matrix matrix;
            const SkeletonBone& bone0 = skeleton.Bones[blendIndices.X];
            Matrix::Multiply(bone0.OffsetMatrix, pose[bone0.NodeIndex], matrix);
            Matrix boneMatrix = matrix * blendWeights.X;
            if (blendWeights.Y > 0.0f)
            {
                const SkeletonBone& bone1 = skeleton.Bones[blendIndices.Y];
                Matrix::Multiply(bone1.OffsetMatrix, pose[bone1.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Y;
            }
            if (blendWeights.Z > 0.0f)
            {
                const SkeletonBone& bone2 = skeleton.Bones[blendIndices.Z];
                Matrix::Multiply(bone2.OffsetMatrix, pose[bone2.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Z;
            }
            if (blendWeights.W > 0.0f)
            {
                const SkeletonBone& bone3 = skeleton.Bones[blendIndices.W];
                Matrix::Multiply(bone3.OffsetMatrix, pose[bone3.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.W;
            }

            // Set vertex position so it will match cloth particle pos after skinning with bone matrix
            Matrix boneMatrixInv;
            Matrix::Invert(boneMatrix, boneMatrixInv);
            Float3 pos = *(Float3*)&particles.Get()[i];
            pos = Float3::Transform(pos, boneMatrixInv);
            positionStream.SetFloat3(i, pos);
        }

        if (_simulationSettings.ComputeNormals)
        {
            // Write normals
            auto normalStream = accessor.Normal();
            auto tangentStream = accessor.Tangent();
            if (normalStream.IsValid() && tangentStream.IsValid())
            {
                for (uint32 i = 0; i < vbCount; i++)
                {
                    Float3 normal = normals.Get()[i];
                    normal.Normalize();
                    Float3 n;
                    Float4 t;
                    RenderTools::CalculateTangentFrame(n, t, normal);
                    normalStream.SetFloat3(i, n);
                    tangentStream.SetFloat4(i, t);
                }
            }
        }
    }
    else if (deformation.Type == MeshBufferType::Vertex0)
    {
        // Copy particle positions to the mesh data
        auto positionStream = accessor.Position();
        if (positionStream.IsValid())
        {
            for (uint32 i = 0; i < vbCount; i++)
            {
                positionStream.SetFloat3(i, *(const Float3*)&particles.Get()[i]);
            }
        }
    }
    else
    {
        // Write normals for the modified vertices by the cloth
        auto normalStream = accessor.Normal();
        auto tangentStream = accessor.Tangent();
        if (normalStream.IsValid() && tangentStream.IsValid())
        {
            for (uint32 i = 0; i < vbCount; i++)
            {
                Float3 normal = normals.Get()[i];
                normal.Normalize();
                Float3 n;
                Float4 t;
                RenderTools::CalculateTangentFrame(n, t, normal);
                normalStream.SetFloat3(i, n);
                tangentStream.SetFloat4(i, t);
            }
        }
    }

    // Mark whole mesh as modified
    deformation.DirtyMinIndex = 0;
    deformation.DirtyMaxIndex = vbCount;

    PhysicsBackend::UnlockClothParticles(_cloth);
#endif
}
