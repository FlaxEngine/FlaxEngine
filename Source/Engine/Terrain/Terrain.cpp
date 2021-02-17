// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Terrain.h"
#include "TerrainPatch.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/PhysX/PxFiltering.h>

Terrain::Terrain(const SpawnParams& params)
    : PhysicsColliderActor(params)
    , _lodBias(0)
    , _forcedLod(-1)
    , _collisionLod(-1)
    , _lodCount(0)
    , _chunkSize(0)
    , _scaleInLightmap(0.1f)
    , _lodDistribution(0.6f)
    , _boundsExtent(Vector3::Zero)
    , _cachedScale(1.0f)
{
    PhysicalMaterial.Changed.Bind<Terrain, &Terrain::OnPhysicalMaterialChanged>(this);
}

Terrain::~Terrain()
{
    // Cleanup
    _patches.ClearDelete();
}

void Terrain::UpdateBounds()
{
    PROFILE_CPU();
    _box = BoundingBox(_transform.Translation, _transform.Translation);
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        auto patch = _patches[i];
        patch->UpdateBounds();
        BoundingBox::Merge(_box, patch->_bounds, _box);
    }
    BoundingSphere::FromBox(_box, _sphere);
}

void Terrain::CacheNeighbors()
{
    PROFILE_CPU();
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        for (int32 chunkIndex = 0; chunkIndex < TerrainPatch::CHUNKS_COUNT; chunkIndex++)
        {
            patch->Chunks[chunkIndex].CacheNeighbors();
        }
    }
}

void Terrain::UpdateLayerBits()
{
    if (_patches.IsEmpty())
        return;

    PxFilterData filterData;

    // Own layer ID
    filterData.word0 = GetLayerMask();

    // Own layer mask
    filterData.word1 = Physics::LayerMasks[GetLayer()];

    // Update the shapes layer bits
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->_physicsShape->setSimulationFilterData(filterData);
            patch->_physicsShape->setQueryFilterData(filterData);
        }
    }
}

void Terrain::RemoveLightmap()
{
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        patch->RemoveLightmap();
    }
}

bool Terrain::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, float maxDistance) const
{
    float minDistance = MAX_float;
    bool result = false;
    const Ray ray(origin, direction);

    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision() &&
            patch->_bounds.Intersects(ray) &&
            patch->RayCast(origin, direction, resultHitDistance, maxDistance) &&
            resultHitDistance < minDistance)
        {
            minDistance = resultHitDistance;
            result = true;
        }
    }

    resultHitDistance = minDistance;
    return result;
}

bool Terrain::RayCast(const Vector3& origin, const Vector3& direction, float& resultHitDistance, TerrainChunk*& resultChunk, float maxDistance) const
{
    float minDistance = MAX_float;
    TerrainChunk* minChunk = nullptr;
    bool result = false;
    const Ray ray(origin, direction);

    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision() &&
            patch->_bounds.Intersects(ray) &&
            patch->RayCast(origin, direction, resultHitDistance, resultChunk, maxDistance) &&
            resultHitDistance < minDistance)
        {
            minDistance = resultHitDistance;
            minChunk = resultChunk;
            result = true;
        }
    }

    resultHitDistance = minDistance;
    resultChunk = minChunk;
    return result;
}

bool Terrain::RayCast(const Ray& ray, float& resultHitDistance, Int2& resultPatchCoord, Int2& resultChunkCoord, float maxDistance) const
{
    TerrainChunk* resultChunk;
    if (RayCast(ray.Position, ray.Direction, resultHitDistance, resultChunk, maxDistance))
    {
        resultPatchCoord.X = resultChunk->GetPatch()->GetX();
        resultPatchCoord.Y = resultChunk->GetPatch()->GetZ();
        resultChunkCoord.X = resultChunk->GetX();
        resultChunkCoord.Y = resultChunk->GetZ();
        return true;
    }

    resultPatchCoord = Int2::Zero;
    resultChunkCoord = Int2::Zero;
    return false;
}

bool Terrain::RayCast(const Vector3& origin, const Vector3& direction, RayCastHit& hitInfo, float maxDistance) const
{
    float minDistance = MAX_float;
    bool result = false;
    RayCastHit tmpHit;
    const Ray ray(origin, direction);

    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision() &&
            patch->_bounds.Intersects(ray) &&
            patch->RayCast(origin, direction, tmpHit, maxDistance) &&
            tmpHit.Distance < minDistance)
        {
            minDistance = tmpHit.Distance;
            hitInfo = tmpHit;
            result = true;
        }
    }

    return result;
}

void Terrain::ClosestPoint(const Vector3& position, Vector3& result) const
{
    float minDistance = MAX_float;
    Vector3 tmp;

    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->ClosestPoint(position, &tmp);
            const auto distance = Vector3::DistanceSquared(position, tmp);
            if (distance < minDistance)
            {
                minDistance = distance;
                result = tmp;
            }
        }
    }
}

void Terrain::DrawPatch(const RenderContext& renderContext, const Int2& patchCoord, MaterialBase* material, int32 lodIndex) const
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        for (int32 i = 0; i < TerrainPatch::CHUNKS_COUNT; i++)
            patch->Chunks[i].Draw(renderContext, material, lodIndex);
    }
}

void Terrain::DrawChunk(const RenderContext& renderContext, const Int2& patchCoord, const Int2& chunkCoord, MaterialBase* material, int32 lodIndex) const
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        const auto chunk = patch->GetChunk(chunkCoord);
        if (chunk)
        {
            chunk->Draw(renderContext, material, lodIndex);
        }
    }
}

void Terrain::OnPhysicalMaterialChanged()
{
    if (_patches.IsEmpty())
        return;

    PxMaterial* material = Physics::GetDefaultMaterial();
    if (PhysicalMaterial)
    {
        if (!PhysicalMaterial->WaitForLoaded())
        {
            material = ((::PhysicalMaterial*)PhysicalMaterial->Instance)->GetPhysXMaterial();
        }
    }

    // Update the shapes material
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->_physicsShape->setMaterials(&material, 1);
        }
    }
}

#if TERRAIN_USE_PHYSICS_DEBUG

void Terrain::DrawPhysicsDebug(RenderView& view)
{
	for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
	{
		_patches[pathIndex]->DrawPhysicsDebug(view);
	}
}

#endif

void Terrain::SetLODDistribution(float value)
{
    _lodDistribution = value;
}

void Terrain::SetScaleInLightmap(float value)
{
    _scaleInLightmap = value;
}

void Terrain::SetBoundsExtent(const Vector3& value)
{
    if (Vector3::NearEqual(_boundsExtent, value))
        return;

    _boundsExtent = value;
    UpdateBounds();
}

void Terrain::SetCollisionLOD(int32 value)
{
    value = Math::Clamp(value, -1, TERRAIN_MAX_LODS);
    if (value == _collisionLod)
        return;

    _collisionLod = value;

#if !BUILD_RELEASE
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        const auto patch = _patches[i];
        if (patch->HasCollision())
        {
            LOG(Warning, "Changing Terrain CollisionLOD has no effect for patches that have already collision created. Patch {0}x{1} won't be updated.", patch->_x, patch->_z);
        }
    }
#endif
}

TerrainPatch* Terrain::GetPatch(const Int2& patchCoord) const
{
    return GetPatch(patchCoord.X, patchCoord.Y);
}

TerrainPatch* Terrain::GetPatch(int32 x, int32 z) const
{
    TerrainPatch* result = nullptr;
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        const auto patch = _patches[i];
        if (patch->_x == x && patch->_z == z)
        {
            result = patch;
            break;
        }
    }
    return result;
}

int32 Terrain::GetPatchIndex(const Int2& patchCoord) const
{
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        const auto patch = _patches[i];
        if (patch->_x == patchCoord.X && patch->_z == patchCoord.Y)
            return i;
    }
    return -1;
}

void Terrain::GetPatchCoord(int32 patchIndex, Int2& patchCoord) const
{
    const auto patch = GetPatch(patchIndex);
    if (patch)
    {
        patchCoord.X = patch->GetX();
        patchCoord.Y = patch->GetZ();
    }
}

void Terrain::GetPatchBounds(int32 patchIndex, BoundingBox& bounds) const
{
    const auto patch = GetPatch(patchIndex);
    if (patch)
    {
        bounds = patch->GetBounds();
    }
}

void Terrain::GetChunkBounds(int32 patchIndex, int32 chunkIndex, BoundingBox& bounds) const
{
    const auto patch = GetPatch(patchIndex);
    if (patch)
    {
        const auto chunk = patch->GetChunk(chunkIndex);
        if (chunk)
        {
            bounds = chunk->GetBounds();
        }
    }
}

MaterialBase* Terrain::GetChunkOverrideMaterial(const Int2& patchCoord, const Int2& chunkCoord) const
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        const auto chunk = patch->GetChunk(chunkCoord);
        if (chunk)
        {
            return chunk->OverrideMaterial;
        }
    }
    return nullptr;
}

void Terrain::SetChunkOverrideMaterial(const Int2& patchCoord, const Int2& chunkCoord, MaterialBase* value)
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        const auto chunk = patch->GetChunk(chunkCoord);
        if (chunk)
        {
            chunk->OverrideMaterial = value;
        }
    }
}

#if TERRAIN_EDITING

bool Terrain::SetupPatchHeightMap(const Int2& patchCoord, int32 heightMapLength, const float* heightMap, const byte* holesMask, bool forceUseVirtualStorage)
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        return patch->SetupHeightMap(heightMapLength, heightMap, holesMask, forceUseVirtualStorage);
    }
    return true;
}

bool Terrain::SetupPatchSplatMap(const Int2& patchCoord, int32 index, int32 splatMapLength, const Color32* splatMap, bool forceUseVirtualStorage)
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        return patch->SetupSplatMap(index, splatMapLength, splatMap, forceUseVirtualStorage);
    }
    return true;
}

#endif

#if TERRAIN_EDITING

void Terrain::Setup(int32 lodCount, int32 chunkSize)
{
    LOG(Info, "Terrain setup for {0} LODs ({1} chunk edge quads)", lodCount, chunkSize);

    _patches.ClearDelete();

    _lodCount = lodCount;
    _chunkSize = chunkSize;
}

void Terrain::AddPatches(const Int2& numberOfPatches)
{
    _patches.ClearDelete();
    _patches.EnsureCapacity(numberOfPatches.X * numberOfPatches.Y);

    for (int32 z = 0; z < numberOfPatches.Y; z++)
    {
        for (int32 x = 0; x < numberOfPatches.X; x++)
        {
            auto patch = ::New<TerrainPatch>();
            patch->Init(this, x, z);
            _patches.Add(patch);
        }
    }

    CacheNeighbors();

    if (IsDuringPlay())
    {
        for (int32 i = 0; i < _patches.Count(); i++)
        {
            auto patch = _patches[i];
            patch->UpdateTransform();
            patch->CreateCollision();
        }
        UpdateLayerBits();
    }

    UpdateBounds();
}

void Terrain::AddPatch(const Int2& patchCoord)
{
    auto patch = GetPatch(patchCoord);
    if (patch != nullptr)
    {
        LOG(Warning, "Cannot add patch at {0}x{1}. The patch at the given location already exists.", patchCoord.X, patchCoord.Y);
        return;
    }

    patch = ::New<TerrainPatch>();
    patch->Init(this, patchCoord.X, patchCoord.Y);
    _patches.Add(patch);

    CacheNeighbors();

    if (IsDuringPlay())
    {
        patch->UpdateTransform();
        patch->CreateCollision();
        UpdateLayerBits();
    }

    UpdateBounds();
}

void Terrain::RemovePatch(const Int2& patchCoord)
{
    const auto patch = GetPatch(patchCoord);
    if (patch == nullptr)
    {
        LOG(Warning, "Cannot remove patch at {0}x{1}. It does not exist.", patchCoord.X, patchCoord.Y);
        return;
    }

    ::Delete(patch);
    _patches.Remove(patch);

    CacheNeighbors();

    if (IsDuringPlay())
    {
        UpdateBounds();
    }
}

#endif

void Terrain::Draw(RenderContext& renderContext)
{
    DrawPass drawModes = (DrawPass)(DrawModes & renderContext.View.Pass);
    if (drawModes == DrawPass::None)
        return;

    PROFILE_CPU();

    // Collect chunks to render and calculate LOD/material for them (required to be done before to gather NeighborLOD)
    static Array<TerrainChunk*> chunks;
    chunks.Clear();
    chunks.EnsureCapacity(_patches.Count() * TerrainPatch::CHUNKS_COUNT);

    // Frustum vs Box culling for patches
    const BoundingFrustum frustum = renderContext.View.CullingFrustum;
    for (int32 patchIndex = 0; patchIndex < _patches.Count(); patchIndex++)
    {
        const auto patch = _patches[patchIndex];
        if (frustum.Intersects(patch->_bounds))
        {
            // Skip if has no heightmap or it's not loaded
            if (patch->Heightmap == nullptr || patch->Heightmap->GetTexture()->ResidentMipLevels() == 0)
                continue;

            // Frustum vs Box culling for chunks
            for (int32 chunkIndex = 0; chunkIndex < TerrainPatch::CHUNKS_COUNT; chunkIndex++)
            {
                auto chunk = &patch->Chunks[chunkIndex];
                chunk->_cachedDrawLOD = 0;
                if (frustum.Intersects(chunk->_bounds))
                {
                    if (chunk->PrepareDraw(renderContext))
                    {
                        // Add chunk for drawing
                        chunks.Add(chunk);
                    }
                }
            }
        }
        else
        {
            // Reset cached LOD for chunks (prevent LOD transition from invisible chunks)
            for (int32 chunkIndex = 0; chunkIndex < TerrainPatch::CHUNKS_COUNT; chunkIndex++)
            {
                auto chunk = &patch->Chunks[chunkIndex];
                chunk->_cachedDrawLOD = 0;
            }
        }
    }

    // Draw all visible chunks
    for (int32 i = 0; i < chunks.Count(); i++)
    {
        chunks[i]->Draw(renderContext);
    }
}

void Terrain::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

#if USE_EDITOR

//#include "Engine/Debug/DebugDraw.h"

void Terrain::OnDebugDrawSelected()
{
    Actor::OnDebugDrawSelected();

    /*
    // TODO: add editor option to draw selected terrain chunks bounds?
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        for (int32 chunkIndex = 0; chunkIndex < TerrainPatch::CHUNKS_COUNT; chunkIndex++)
        {
            auto chunk = &patch->Chunks[chunkIndex];
            DebugDraw::DrawBox(chunk->_bounds, Color(chunk->_x / (float)TerrainPatch::CHUNKS_COUNT_EDGE, 1.0f, chunk->_z / (float)TerrainPatch::CHUNKS_COUNT_EDGE));
        }
    }
    */
}

#endif

bool Terrain::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    float minDistance = MAX_float;
    Vector3 minDistanceNormal = Vector3::Up;
    float tmpDistance;
    Vector3 tmpNormal;
    bool result = false;

    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision() &&
            patch->_bounds.Intersects(ray) &&
            patch->RayCast(ray.Position, ray.Direction, tmpDistance, tmpNormal) &&
            tmpDistance < minDistance)
        {
            minDistance = tmpDistance;
            minDistanceNormal = tmpNormal;
            result = true;
        }
    }

    distance = minDistance;
    normal = minDistanceNormal;
    return result;
}

void Terrain::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Terrain);

    SERIALIZE_MEMBER(LODBias, _lodBias);
    SERIALIZE_MEMBER(ForcedLOD, _forcedLod);
    SERIALIZE_MEMBER(LODDistribution, _lodDistribution);
    SERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
    SERIALIZE_MEMBER(BoundsExtent, _boundsExtent);
    SERIALIZE_MEMBER(CollisionLOD, _collisionLod);
    SERIALIZE(Material);
    SERIALIZE(PhysicalMaterial);
    SERIALIZE(DrawModes);

    SERIALIZE_MEMBER(LODCount, _lodCount);
    SERIALIZE_MEMBER(ChunkSize, _chunkSize);

    if (_patches.HasItems())
    {
        stream.JKEY("Patches");
        stream.StartArray();
        for (int32 patchIndex = 0; patchIndex < _patches.Count(); patchIndex++)
        {
            stream.StartObject();
            _patches[patchIndex]->Serialize(stream, other && other->_patches.Count() == _patches.Count() ? other->_patches[patchIndex] : nullptr);
            stream.EndObject();
        }
        stream.EndArray();
    }
}

void Terrain::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    auto member = stream.FindMember("LODBias");
    if (member != stream.MemberEnd() && member->value.IsInt())
    {
        SetLODBias(member->value.GetInt());
    }

    member = stream.FindMember("ForcedLOD");
    if (member != stream.MemberEnd() && member->value.IsInt())
    {
        SetForcedLOD(member->value.GetInt());
    }

    member = stream.FindMember("CollisionLOD");
    if (member != stream.MemberEnd() && member->value.IsInt())
    {
        SetCollisionLOD(member->value.GetInt());
    }

    DESERIALIZE_MEMBER(LODDistribution, _lodDistribution);
    DESERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
    DESERIALIZE_MEMBER(BoundsExtent, _boundsExtent);
    DESERIALIZE(Material);
    DESERIALIZE(PhysicalMaterial);
    DESERIALIZE(DrawModes);

    member = stream.FindMember("LODCount");
    if (member != stream.MemberEnd() && member->value.IsInt())
    {
        _lodCount = member->value.GetInt();
    }

    member = stream.FindMember("ChunkSize");
    if (member != stream.MemberEnd() && member->value.IsInt())
    {
        _chunkSize = member->value.GetInt();
    }

    member = stream.FindMember("Patches");
    if (member != stream.MemberEnd() && member->value.IsArray())
    {
        auto& patchesData = member->value;
        const auto patchesCount = (int32)patchesData.Size();

        // Update patches if collection size is different
        if (patchesCount != _patches.Count())
        {
            _patches.ClearDelete();

            for (int32 i = 0; i < patchesCount; i++)
            {
                auto patch = ::New<TerrainPatch>();
                patch->Init(this, 0, 0);
                _patches.Add(patch);
            }
        }

        // Load patches
        for (int32 i = 0; i < patchesCount; i++)
        {
            auto patch = _patches[i];
            auto& patchData = patchesData[i];

            patch->Deserialize(patchData, modifier);
        }

#if !BUILD_RELEASE
        // Validate patches locations
        for (int32 i = 0; i < patchesCount; i++)
        {
            const auto patch = _patches[i];
            for (int32 j = i + 1; j < patchesCount; j++)
            {
                if (_patches[j]->_x == patch->_x && _patches[j]->_z == patch->_z)
                {
                    LOG(Warning, "Invalid terrain data! Overlapping terrain patches.");
                }
            }
        }
#endif
    }
}

RigidBody* Terrain::GetAttachedRigidBody() const
{
    // Terrains are always static things
    return nullptr;
}

void Terrain::OnEnable()
{
    GetSceneRendering()->AddGeometry(this);
#if TERRAIN_USE_PHYSICS_DEBUG
	GetSceneRendering()->AddPhysicsDebug<Terrain, &Terrain::DrawPhysicsDebug>(this);
#endif

    // Base
    Actor::OnEnable();
}

void Terrain::OnDisable()
{
    GetSceneRendering()->RemoveGeometry(this);
#if TERRAIN_USE_PHYSICS_DEBUG
	GetSceneRendering()->RemovePhysicsDebug<Terrain, &Terrain::DrawPhysicsDebug>(this);
#endif

    // Base
    Actor::OnDisable();
}

void Terrain::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    for (int32 i = 0; i < _patches.Count(); i++)
    {
        auto patch = _patches[i];
        patch->UpdateTransform();
    }
    if (!Vector3::NearEqual(_cachedScale, _transform.Scale))
    {
        _cachedScale = _transform.Scale;
        for (int32 i = 0; i < _patches.Count(); i++)
        {
            auto patch = _patches[i];
            if (patch->HasCollision())
            {
                patch->UpdateCollisionScale();
            }
        }
    }
    UpdateBounds();
}

void Terrain::OnLayerChanged()
{
    // Base
    Actor::OnLayerChanged();

    UpdateLayerBits();
}

void Terrain::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    // Update physics
    const PxShapeFlags shapeFlags = GetShapeFlags(false, IsActiveInHierarchy());
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->_physicsShape->setFlags(shapeFlags);
        }
    }
}

void Terrain::BeginPlay(SceneBeginData* data)
{
    CacheNeighbors();
    _cachedScale = _transform.Scale;
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (!patch->HasCollision())
        {
            patch->CreateCollision();
        }
    }
    UpdateLayerBits();

    // Base
    Actor::BeginPlay(data);
}

void Terrain::EndPlay()
{
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->DestroyCollision();
        }
    }

    // Base
    Actor::EndPlay();
}
