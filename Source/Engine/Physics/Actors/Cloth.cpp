// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Cloth.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Models/MeshBase.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Actors/AnimatedModel.h"
#if USE_EDITOR
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Debug/DebugDraw.h"
#endif

Cloth::Cloth(const SpawnParams& params)
    : Actor(params)
{
    // Use the first mesh by default
    _mesh.LODIndex = _mesh.MeshIndex = 0;
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
        _meshDeformation = nullptr;
    }

    _mesh = value;
    _mesh.Actor = nullptr; // Don't store this reference
    Rebuild();
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

        // Create new
        CreateCloth();
        if (IsDuringPlay())
            PhysicsBackend::AddCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
    }
#endif
}

void Cloth::ClearInteria()
{
#if WITH_CLOTH
    if (_cloth)
        PhysicsBackend::ClearClothInertia(_cloth);
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
}

#if USE_EDITOR

void Cloth::DrawPhysicsDebug(RenderView& view)
{
#if WITH_CLOTH
    if (_cloth)
    {
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return;
        BytesContainer indicesData;
        int32 indicesCount = 0;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
            return;
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothCurrentParticles(_cloth);
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
            // TODO: highlight immovable cloth particles with a different color
            DEBUG_DRAW_TRIANGLE(v0, v1, v2, Color::Pink, 0, true);
        }
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif
}

void Cloth::OnDebugDrawSelected()
{
#if WITH_CLOTH
    if (_cloth)
    {
        DEBUG_DRAW_WIRE_BOX(_box, Color::Violet.RGBMultiplied(0.8f), 0, true);
        const ModelInstanceActor::MeshReference mesh = GetMesh();
        if (mesh.Actor == nullptr)
            return;
        BytesContainer indicesData;
        int32 indicesCount = 0;
        if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, indicesData, indicesCount))
            return;
        PhysicsBackend::LockClothParticles(_cloth);
        const Span<const Float4> particles = PhysicsBackend::GetClothCurrentParticles(_cloth);
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
            // TODO: highlight immovable cloth particles with a different color
            DEBUG_DRAW_LINE(v0, v1, Color::White, 0, false);
            DEBUG_DRAW_LINE(v1, v2, Color::White, 0, false);
            DEBUG_DRAW_LINE(v2, v0, Color::White, 0, false);
        }
        PhysicsBackend::UnlockClothParticles(_cloth);
    }
#endif

    Actor::OnDebugDrawSelected();
}

#endif

void Cloth::BeginPlay(SceneBeginData* data)
{
    if (CreateCloth())
    {
        LOG(Error, "Failed to create cloth '{0}'", GetNamePath());
    }

    Actor::BeginPlay(data);
}

void Cloth::EndPlay()
{
    Actor::EndPlay();

    if (_cloth)
    {
        DestroyCloth();
    }
}

void Cloth::OnEnable()
{
#if USE_EDITOR
    GetSceneRendering()->AddPhysicsDebug<Cloth, &Cloth::DrawPhysicsDebug>(this);
#endif
#if WITH_CLOTH
    if (_cloth)
    {
        PhysicsBackend::AddCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
    }
#endif

    Actor::OnEnable();
}

void Cloth::OnDisable()
{
    Actor::OnDisable();

#if WITH_CLOTH
    if (_cloth)
    {
        PhysicsBackend::RemoveCloth(GetPhysicsScene()->GetPhysicsScene(), _cloth);
    }
#endif
#if USE_EDITOR
    GetSceneRendering()->RemovePhysicsDebug<Cloth, &Cloth::DrawPhysicsDebug>(this);
#endif
}

void Cloth::OnParentChanged()
{
    Actor::OnParentChanged();

    Rebuild();
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
    desc.VerticesData = data.Get();
    desc.VerticesCount = count;
    desc.VerticesStride = data.Length() / count;
    if (mesh.Actor->GetMeshData(mesh, MeshBufferType::Index, data, count))
        return true;
    desc.IndicesData = data.Get();
    desc.IndicesCount = count;
    desc.IndicesStride = data.Length() / count;

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
        _meshDeformation = deformation;
    }
#endif

    return false;
}

void Cloth::DestroyCloth()
{
#if WITH_CLOTH
    if (_meshDeformation)
    {
        Function<void(const MeshBase*, MeshDeformationData&)> deformer;
        deformer.Bind<Cloth, &Cloth::RunClothDeformer>(this);
        _meshDeformation->RemoveDeformer(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0, deformer);
        _meshDeformation = nullptr;
    }
    PhysicsBackend::DestroyCloth(_cloth);
    _cloth = nullptr;
#endif
}

void Cloth::OnUpdated()
{
    if (_meshDeformation)
    {
        // Mark mesh as dirty
        const Matrix invWorld = Matrix::Invert(_transform.GetWorld());
        BoundingBox localBounds;
        BoundingBox::Transform(_box, invWorld, localBounds);
        _meshDeformation->Dirty(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0, localBounds);

        // Update bounds (for mesh culling)
        auto* actor = (ModelInstanceActor*)GetParent();
        actor->UpdateBounds();
    }
}

void Cloth::RunClothDeformer(const MeshBase* mesh, MeshDeformationData& deformation)
{
#if WITH_CLOTH
    PROFILE_CPU_NAMED("Cloth");
    PhysicsBackend::LockClothParticles(_cloth);
    const Span<const Float4> particles = PhysicsBackend::GetClothCurrentParticles(_cloth);

    // Update mesh vertices based on the cloth particles positions
    auto vbData = deformation.VertexBuffer.Data.Get();
    auto vbCount = (uint32)mesh->GetVertexCount();
    auto vbStride = (uint32)deformation.VertexBuffer.Data.Count() / vbCount;
    // TODO: add support for mesh vertex data layout descriptor instead hardcoded position data at the beginning of VB0
    ASSERT((uint32)particles.Length() >= vbCount);
    if (auto* animatedModel = Cast<AnimatedModel>(GetParent()))
    {
        if (animatedModel->GraphInstance.NodesPose.IsEmpty())
        {
            // Delay unit skinning data is ready
            PhysicsBackend::UnlockClothParticles(_cloth);
            _meshDeformation->Dirty(_mesh.LODIndex, _mesh.MeshIndex, MeshBufferType::Vertex0);
            return;
        }

        // TODO: optimize memory allocs (eg. get pose as Span<Matrix> for readonly)
        Array<Matrix> pose;
        animatedModel->GetCurrentPose(pose);
        const SkeletonData& skeleton = animatedModel->SkinnedModel->Skeleton;

        // Animated model uses skinning thus requires to set vertex position inverse to skeleton bones
        ASSERT(vbStride == sizeof(VB0SkinnedElementType));
        for (uint32 i = 0; i < vbCount; i++)
        {
            VB0SkinnedElementType& vb0 = *(VB0SkinnedElementType*)vbData;

            // Calculate skinned vertex matrix from bones blending
            const Float4 blendWeights = vb0.BlendWeights.ToFloat4();
            // TODO: optimize this or use _skinningData from AnimatedModel to access current mesh bones data directly
            Matrix matrix;
            const SkeletonBone& bone0 = skeleton.Bones[vb0.BlendIndices.R];
            Matrix::Multiply(bone0.OffsetMatrix, pose[bone0.NodeIndex], matrix);
            Matrix boneMatrix = matrix * blendWeights.X;
            if (blendWeights.Y > 0.0f)
            {
                const SkeletonBone& bone1 = skeleton.Bones[vb0.BlendIndices.G];
                Matrix::Multiply(bone1.OffsetMatrix, pose[bone1.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Y;
            }
            if (blendWeights.Z > 0.0f)
            {
                const SkeletonBone& bone2 = skeleton.Bones[vb0.BlendIndices.B];
                Matrix::Multiply(bone2.OffsetMatrix, pose[bone2.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.Z;
            }
            if (blendWeights.W > 0.0f)
            {
                const SkeletonBone& bone3 = skeleton.Bones[vb0.BlendIndices.A];
                Matrix::Multiply(bone3.OffsetMatrix, pose[bone3.NodeIndex], matrix);
                boneMatrix += matrix * blendWeights.W;
            }

            // Set vertex position so it will match cloth particle pos after skinning with bone matrix
            Matrix boneMatrixInv;
            Matrix::Invert(boneMatrix, boneMatrixInv);
            Float3 pos = *(Float3*)&particles.Get()[i];
            vb0.Position = Float3::Transform(pos, boneMatrixInv);

            vbData += vbStride;
        }
    }
    else
    {
        for (uint32 i = 0; i < vbCount; i++)
        {
            *((Float3*)vbData) = *(Float3*)&particles.Get()[i];
            vbData += vbStride;
        }
    }

    // Mark whole mesh as modified
    deformation.DirtyMinIndex = 0;
    deformation.DirtyMaxIndex = vbCount;

    PhysicsBackend::UnlockClothParticles(_cloth);
#endif
}
