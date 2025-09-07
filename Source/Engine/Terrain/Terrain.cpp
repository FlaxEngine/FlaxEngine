// Copyright (c) Wojciech Figat. All rights reserved.

#include "Terrain.h"
#include "TerrainPatch.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicalMaterial.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"
#include "Engine/Renderer/GI/GlobalSurfaceAtlasPass.h"

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
    _drawCategory = SceneRendering::SceneDrawAsync;
    _physicalMaterials.Resize(8);
}

Terrain::~Terrain()
{
    // Cleanup
    _patches.ClearDelete();
}

void Terrain::UpdateBounds()
{
    PROFILE_CPU();
    _box = BoundingBox(_transform.Translation);
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        auto patch = _patches[i];
        patch->UpdateBounds();
        BoundingBox::Merge(_box, patch->_bounds, _box);
    }
    BoundingSphere::FromBox(_box, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
}

void Terrain::CacheNeighbors()
{
    PROFILE_CPU();
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
        {
            patch->Chunks[chunkIndex].CacheNeighbors();
        }
    }
}

void Terrain::UpdateLayerBits()
{
    if (_patches.IsEmpty())
        return;

    // Own layer ID
    const uint32 mask0 = GetLayerMask();

    // Own layer mask
    const uint32 mask1 = Physics::LayerMasks[GetLayer()];

    // Update the shapes layer bits
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            PhysicsBackend::SetShapeFilterMask(patch->_physicsShape, mask0, mask1);
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

void Terrain::ClosestPoint(const Vector3& point, Vector3& result) const
{
    Real minDistance = MAX_Real;
    Vector3 tmp;
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            patch->ClosestPoint(point, tmp);
            const auto distance = Vector3::DistanceSquared(point, tmp);
            if (distance < minDistance)
            {
                minDistance = distance;
                result = tmp;
            }
        }
    }
}

bool Terrain::ContainsPoint(const Vector3& point) const
{
    return false;
}

void Terrain::DrawPatch(const RenderContext& renderContext, const Int2& patchCoord, MaterialBase* material, int32 lodIndex) const
{
    auto patch = GetPatch(patchCoord);
    if (patch)
    {
        for (int32 i = 0; i < Terrain::ChunksCount; i++)
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

#if TERRAIN_USE_PHYSICS_DEBUG

void Terrain::DrawPhysicsDebug(RenderView& view)
{
    PROFILE_CPU();
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
    if (_boundsExtent == value)
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

void Terrain::SetPhysicalMaterials(const Array<JsonAssetReference<PhysicalMaterial>, FixedAllocation<8>>& value)
{
    _physicalMaterials = value;
    _physicalMaterials.Resize(8);
    JsonAsset* materials[8];
    for (int32 i = 0; i < 8; i++)
        materials[i] = _physicalMaterials[i];
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches.Get()[pathIndex];
        if (patch->HasCollision())
            PhysicsBackend::SetShapeMaterials(patch->_physicsShape, ToSpan(materials, 8));
    }
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
    if (_chunkSize == 0)
        Setup();
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
    if (_chunkSize == 0)
        Setup();

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

void Terrain::Draw(RenderContextBatch& renderContextBatch)
{
    PROFILE_CPU();
    if (DrawSetup(renderContextBatch.GetMainContext()))
        return;
    HashSet<TerrainChunk*, RendererAllocation> drawnChunks;
    for (RenderContext& renderContext : renderContextBatch.Contexts)
    {
        const DrawPass drawModes = DrawModes & renderContext.View.Pass;
        if (drawModes == DrawPass::None)
            continue;
        DrawImpl(renderContext, drawnChunks);
    }
}

void Terrain::Draw(RenderContext& renderContext)
{
    const DrawPass drawModes = DrawModes & renderContext.View.Pass;
    if (drawModes == DrawPass::None)
        return;
    PROFILE_CPU();
    if (DrawSetup(renderContext))
        return;
    HashSet<TerrainChunk*, RendererAllocation> drawnChunks;
    DrawImpl(renderContext, drawnChunks);
}

bool Terrain::DrawSetup(RenderContext& renderContext)
{
    // Special drawing modes
    const DrawPass drawModes = DrawModes & renderContext.View.Pass;
    if (drawModes == DrawPass::GlobalSDF)
    {
        const float chunkSize = TERRAIN_UNITS_PER_VERTEX * (float)_chunkSize;
        const float posToUV = 0.25f / chunkSize;
        Float4 localToUV(posToUV, posToUV, 0.0f, 0.0f);
        for (const TerrainPatch* patch : _patches)
        {
            if (!patch->Heightmap)
                continue;
            Transform patchTransform;
            patchTransform.Translation = patch->_offset + Vector3(0, patch->_yOffset, 0);
            patchTransform.Orientation = Quaternion::Identity;
            patchTransform.Scale = Float3(1.0f, patch->_yHeight, 1.0f);
            patchTransform = _transform.LocalToWorld(patchTransform);
            GlobalSignDistanceFieldPass::Instance()->RasterizeHeightfield(this, patch->Heightmap->GetTexture(), patchTransform, patch->_bounds, localToUV);
        }
        return true;
    }
    if (drawModes == DrawPass::GlobalSurfaceAtlas)
    {
        for (TerrainPatch* patch : _patches)
        {
            if (!patch->Heightmap)
                continue;
            Matrix localToWorld, worldToLocal;
            BoundingSphere chunkSphere;
            BoundingBox localBounds;
            for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
            {
                TerrainChunk* chunk = &patch->Chunks[chunkIndex];
                chunk->GetTransform().GetWorld(localToWorld); // TODO: large-worlds
                Matrix::Invert(localToWorld, worldToLocal);
                BoundingBox::Transform(chunk->GetBounds(), worldToLocal, localBounds);
                BoundingSphere::FromBox(chunk->GetBounds(), chunkSphere);
                GlobalSurfaceAtlasPass::Instance()->RasterizeActor(this, chunk, chunkSphere, chunk->GetTransform(), localBounds, 1 << 2, false);
            }
        }
        return true;
    }

    // Reset cached LOD for chunks (prevent LOD transition from invisible chunks)
    for (int32 patchIndex = 0; patchIndex < _patches.Count(); patchIndex++)
    {
        const auto patch = _patches[patchIndex];
        for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
        {
            auto chunk = &patch->Chunks[chunkIndex];
            chunk->_cachedDrawLOD = 0;
        }
    }

    return false;
}

void Terrain::DrawImpl(RenderContext& renderContext, HashSet<TerrainChunk*, RendererAllocation>& drawnChunks)
{
    // Collect chunks to render and calculate LOD/material for them (required to be done before to gather NeighborLOD)
    Array<TerrainChunk*, RendererAllocation> drawChunks;

    // Frustum vs Box culling for patches
    const BoundingFrustum frustum = renderContext.View.CullingFrustum;
    const Vector3 origin = renderContext.View.Origin;
    for (int32 patchIndex = 0; patchIndex < _patches.Count(); patchIndex++)
    {
        const auto patch = _patches[patchIndex];
        BoundingBox bounds(patch->_bounds.Minimum - origin, patch->_bounds.Maximum - origin);
        if (renderContext.View.IsCullingDisabled || frustum.Intersects(bounds))
        {
            // Skip if has no heightmap or it's not loaded
            if (patch->Heightmap == nullptr || patch->Heightmap->GetTexture()->ResidentMipLevels() == 0)
                continue;

            // Frustum vs Box culling for chunks
            for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
            {
                auto chunk = &patch->Chunks[chunkIndex];
                bounds = BoundingBox(chunk->_bounds.Minimum - origin, chunk->_bounds.Maximum - origin);
                if (renderContext.View.IsCullingDisabled || frustum.Intersects(bounds))
                {
                    if (!drawnChunks.Contains(chunk) && !chunk->PrepareDraw(renderContext))
                        continue;

                    // Add chunk for drawing
                    drawChunks.Add(chunk);
                    drawnChunks.Add(chunk);
                }
            }
        }
    }

    // Draw all visible chunks
    for (int32 i = 0; i < drawChunks.Count(); i++)
    {
        drawChunks.Get()[i]->Draw(renderContext);
    }
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
        for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
        {
            auto chunk = &patch->Chunks[chunkIndex];
            DebugDraw::DrawBox(chunk->_bounds, Color(chunk->_x / (float)Terrain::ChunksCountEdge, 1.0f, chunk->_z / (float)Terrain::ChunksCountEdge));
        }
    }
    */
}

#endif

bool Terrain::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
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
    SERIALIZE_MEMBER(PhysicalMaterials, _physicalMaterials);
    SERIALIZE(Material);
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
    DESERIALIZE_MEMBER(PhysicalMaterials, _physicalMaterials);
    DESERIALIZE(Material);
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

    // [Deprecated on 07.02.2022, expires on 07.02.2024]
    if (modifier->EngineBuild <= 6330)
    {
        MARK_CONTENT_DEPRECATED();
        DrawModes |= DrawPass::GlobalSDF;
    }
    // [Deprecated on 27.04.2022, expires on 27.04.2024]
    if (modifier->EngineBuild <= 6331)
    {
        MARK_CONTENT_DEPRECATED();
        DrawModes |= DrawPass::GlobalSurfaceAtlas;
    }

    // [Deprecated on 15.02.2024, expires on 15.02.2026]
    JsonAssetReference<PhysicalMaterial> PhysicalMaterial;
    DESERIALIZE(PhysicalMaterial);
    if (PhysicalMaterial)
    {
        MARK_CONTENT_DEPRECATED();
        for (auto& e : _physicalMaterials)
            e = PhysicalMaterial;
    }
}

RigidBody* Terrain::GetAttachedRigidBody() const
{
    // Terrains are always static things
    return nullptr;
}

void Terrain::OnEnable()
{
    GetScene()->Navigation.Actors.Add(this);
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if TERRAIN_USE_PHYSICS_DEBUG
    GetSceneRendering()->AddPhysicsDebug(this);
#endif
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        auto patch = _patches[i];
        if (patch->_physicsActor)
        {
            PhysicsBackend::AddSceneActor(scene, patch->_physicsActor);
        }
    }

    // Base
    Actor::OnEnable();
}

void Terrain::OnDisable()
{
    GetScene()->Navigation.Actors.Remove(this);
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
#if TERRAIN_USE_PHYSICS_DEBUG
    GetSceneRendering()->RemovePhysicsDebug(this);
#endif
    void* scene = GetPhysicsScene()->GetPhysicsScene();
    for (int32 i = 0; i < _patches.Count(); i++)
    {
        auto patch = _patches[i];
        if (patch->_physicsActor)
        {
            PhysicsBackend::RemoveSceneActor(scene, patch->_physicsActor);
        }
    }

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
    if (_cachedScale != _transform.Scale)
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
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void Terrain::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    // Update physics
    for (int32 pathIndex = 0; pathIndex < _patches.Count(); pathIndex++)
    {
        const auto patch = _patches[pathIndex];
        if (patch->HasCollision())
        {
            PhysicsBackend::SetShapeState(patch->_physicsShape, IsActiveInHierarchy(), false);
        }
    }
}

void Terrain::OnPhysicsSceneChanged(PhysicsScene* previous)
{
    PhysicsColliderActor::OnPhysicsSceneChanged(previous);

    for (auto patch : _patches)
        patch->OnPhysicsSceneChanged(previous);
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
