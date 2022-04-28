// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GlobalSignDistanceFieldPass.h"
#include "RenderList.h"
#include "Engine/Core/Math/Int3.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Level/Actors/StaticModel.h"

// Some of those constants must match in shader
// TODO: try using R8 format for Global SDF
#define GLOBAL_SDF_FORMAT PixelFormat::R16_Float
#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28 // The maximum amount of models to rasterize at once as a batch into Global SDF.
#define GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT 2 // The maximum amount of heightfields to store in a single chunk.
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32 // Global SDF chunk size in voxels.
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4 // The margin in voxels around objects for culling. Reduces artifacts but reduces performance.
#define GLOBAL_SDF_RASTERIZE_MIP_FACTOR 4 // Global SDF mip resolution downscale factor.
#define GLOBAL_SDF_MIP_GROUP_SIZE 4
#define GLOBAL_SDF_MIP_FLOODS 5 // Amount of flood fill passes for mip.
#define GLOBAL_SDF_DEBUG_CHUNKS 0
#define GLOBAL_SDF_ACTOR_IS_STATIC(actor) ((actor->GetStaticFlags() & (StaticFlags::Lightmap | StaticFlags::Transform)) == (int32)(StaticFlags::Lightmap | StaticFlags::Transform))

static_assert(GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT % 4 == 0, "Must be multiple of 4 due to data packing for GPU constant buffer.");
#if GLOBAL_SDF_DEBUG_CHUNKS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct ObjectRasterizeData
    {
    Matrix WorldToVolume; // TODO: use 3x4 matrix
    Matrix VolumeToWorld; // TODO: use 3x4 matrix
    Vector3 VolumeToUVWMul;
    float MipOffset;
    Vector3 VolumeToUVWAdd;
    float DecodeMul;
    Vector3 VolumeLocalBoundsExtent;
    float DecodeAdd;
    });

PACK_STRUCT(struct Data
    {
    Vector3 ViewWorldPos;
    float ViewNearPlane;
    Vector3 Padding00;
    float ViewFarPlane;
    Vector4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::GlobalSDFData GlobalSDF;
    });

PACK_STRUCT(struct ModelsRasterizeData
    {
    Int3 ChunkCoord;
    float MaxDistance;
    Vector3 CascadeCoordToPosMul;
    int ObjectsCount;
    Vector3 CascadeCoordToPosAdd;
    int32 CascadeResolution;
    float Padding0;
    float CascadeVoxelSize;
    int32 CascadeMipResolution;
    int32 CascadeMipFactor;
    uint32 Objects[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
    });

struct RasterizeModel
{
    Matrix WorldToVolume;
    Matrix VolumeToWorld;
    Vector3 VolumeToUVWMul;
    Vector3 VolumeToUVWAdd;
    Vector3 VolumeLocalBoundsExtent;
    float MipOffset;
    const ModelBase::SDFData* SDF;
};

struct RasterizeChunk
{
    uint16 ModelsCount;
    uint16 HeightfieldsCount : 15;
    uint16 Dynamic : 1;
    uint16 Models[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
    uint16 Heightfields[GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT];

    RasterizeChunk()
    {
        ModelsCount = 0;
        HeightfieldsCount = 0;
        Dynamic = false;
    }
};

constexpr int32 RasterizeChunkKeyHashResolution = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;

struct RasterizeChunkKey
{
    uint32 Hash;
    int32 Layer;
    Int3 Coord;

    FORCE_INLINE void NextLayer()
    {
        Layer++;
        Hash += RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution;
    }

    friend bool operator==(const RasterizeChunkKey& a, const RasterizeChunkKey& b)
    {
        return a.Hash == b.Hash && a.Coord == b.Coord && a.Layer == b.Layer;
    }
};

uint32 GetHash(const RasterizeChunkKey& key)
{
    return key.Hash;
}

struct CascadeData
{
    GPUTexture* Texture = nullptr;
    GPUTexture* Mip = nullptr;
    Vector3 Position;
    float VoxelSize;
    BoundingBox Bounds;
    HashSet<RasterizeChunkKey> NonEmptyChunks;
    HashSet<RasterizeChunkKey> StaticChunks;

    FORCE_INLINE void OnSceneRenderingDirty(const BoundingBox& objectBounds)
    {
        if (StaticChunks.IsEmpty() || !Bounds.Intersects(objectBounds))
            return;

        BoundingBox objectBoundsCascade;
        const float objectMargin = VoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
        Vector3::Clamp(objectBounds.Minimum - objectMargin, Bounds.Minimum, Bounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, Bounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + objectMargin, Bounds.Minimum, Bounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, Bounds.Minimum, objectBoundsCascade.Maximum);
        const float chunkSize = VoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / chunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / chunkSize);

        // Invalidate static chunks intersecting with dirty bounds
        RasterizeChunkKey key;
        key.Layer = 0;
        for (key.Coord.Z = objectChunkMin.Z; key.Coord.Z <= objectChunkMax.Z; key.Coord.Z++)
        {
            for (key.Coord.Y = objectChunkMin.Y; key.Coord.Y <= objectChunkMax.Y; key.Coord.Y++)
            {
                for (key.Coord.X = objectChunkMin.X; key.Coord.X <= objectChunkMax.X; key.Coord.X++)
                {
                    key.Hash = key.Coord.Z * (RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution) + key.Coord.Y * RasterizeChunkKeyHashResolution + key.Coord.X;
                    StaticChunks.Remove(key);
                }
            }
        }
    }

    ~CascadeData()
    {
        RenderTargetPool::Release(Texture);
        RenderTargetPool::Release(Mip);
    }
};

class GlobalSignDistanceFieldCustomBuffer : public RenderBuffers::CustomBuffer, public ISceneRenderingListener
{
public:
    CascadeData Cascades[4];
    HashSet<ScriptingTypeHandle> ObjectTypes;
    HashSet<GPUTexture*> SDFTextures;
    GlobalSignDistanceFieldPass::BindingData Result;

    ~GlobalSignDistanceFieldCustomBuffer()
    {
        for (const auto& e : SDFTextures)
        {
            e.Item->Deleted.Unbind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(this);
            e.Item->ResidentMipsChanged.Unbind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(this);
        }
    }

    void OnSDFTextureDeleted(ScriptingObject* object)
    {
        auto* texture = (GPUTexture*)object;
        if (SDFTextures.Remove(texture))
        {
            texture->Deleted.Unbind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(this);
            texture->ResidentMipsChanged.Unbind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(this);
        }
    }

    void OnSDFTextureResidentMipsChanged(GPUTexture* texture)
    {
        // Stop tracking texture streaming once it gets fully loaded
        if (texture->ResidentMipLevels() == texture->MipLevels())
        {
            OnSDFTextureDeleted(texture);

            // Clear static chunks cache
            for (auto& cascade : Cascades)
                cascade.StaticChunks.Clear();
        }
    }

    FORCE_INLINE void OnSceneRenderingDirty(const BoundingBox& objectBounds)
    {
        for (auto& cascade : Cascades)
            cascade.OnSceneRenderingDirty(objectBounds);
    }

    // [ISceneRenderingListener]
    void OnSceneRenderingAddActor(Actor* a) override
    {
        if (GLOBAL_SDF_ACTOR_IS_STATIC(a) && ObjectTypes.Contains(a->GetTypeHandle()))
        {
            OnSceneRenderingDirty(a->GetBox());
        }
    }

    void OnSceneRenderingUpdateActor(Actor* a, const BoundingSphere& prevBounds) override
    {
        if (GLOBAL_SDF_ACTOR_IS_STATIC(a) && ObjectTypes.Contains(a->GetTypeHandle()))
        {
            OnSceneRenderingDirty(BoundingBox::FromSphere(prevBounds));
            OnSceneRenderingDirty(a->GetBox());
        }
    }

    void OnSceneRenderingRemoveActor(Actor* a) override
    {
        if (GLOBAL_SDF_ACTOR_IS_STATIC(a) && ObjectTypes.Contains(a->GetTypeHandle()))
        {
            OnSceneRenderingDirty(a->GetBox());
        }
    }

    void OnSceneRenderingClear(SceneRendering* scene) override
    {
        for (auto& cascade : Cascades)
            cascade.StaticChunks.Clear();
    }
};

namespace
{
    Dictionary<RasterizeChunkKey, RasterizeChunk> ChunksCache;
}

String GlobalSignDistanceFieldPass::ToString() const
{
    return TEXT("GlobalSignDistanceFieldPass");
}

bool GlobalSignDistanceFieldPass::Init()
{
    // Check platform support
    const auto device = GPUDevice::Instance;
    _supported = device->GetFeatureLevel() >= FeatureLevel::SM5 && device->Limits.HasCompute && device->Limits.HasTypedUAVLoad
            && FORMAT_FEATURES_ARE_SUPPORTED(device->GetFormatFeatures(GLOBAL_SDF_FORMAT).Support, FormatSupport::ShaderSample | FormatSupport::Texture3D);
    return false;
}

bool GlobalSignDistanceFieldPass::setupResources()
{
    if (!_supported)
        return true;

    // Load shader
    if (!_shader)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GlobalSignDistanceField"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<GlobalSignDistanceFieldPass, &GlobalSignDistanceFieldPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;

    const auto device = GPUDevice::Instance;
    const auto shader = _shader->GetShader();

    // Check shader
    _cb0 = shader->GetCB(0);
    _cb1 = shader->GetCB(1);
    if (!_cb0 || !_cb1)
        return true;
    _csRasterizeModel0 = shader->GetCS("CS_RasterizeModel", 0);
    _csRasterizeModel1 = shader->GetCS("CS_RasterizeModel", 1);
    _csRasterizeHeightfield = shader->GetCS("CS_RasterizeHeightfield");
    _csClearChunk = shader->GetCS("CS_ClearChunk");
    _csGenerateMip0 = shader->GetCS("CS_GenerateMip", 0);
    _csGenerateMip1 = shader->GetCS("CS_GenerateMip", 1);

    // Init buffer
    if (!_objectsBuffer)
        _objectsBuffer = New<DynamicStructuredBuffer>(64u * (uint32)sizeof(ObjectRasterizeData), (uint32)sizeof(ObjectRasterizeData), false, TEXT("GlobalSDF.ObjectsBuffer"));

    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug)
    {
        _psDebug = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Debug");
        if (_psDebug->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void GlobalSignDistanceFieldPass::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _csRasterizeModel0 = nullptr;
    _csRasterizeModel1 = nullptr;
    _csRasterizeHeightfield = nullptr;
    _csClearChunk = nullptr;
    _csGenerateMip0 = nullptr;
    _csGenerateMip1 = nullptr;
    _cb0 = nullptr;
    _cb1 = nullptr;
    invalidateResources();
}

#endif

void GlobalSignDistanceFieldPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE(_objectsBuffer);
    _objectsTextures.Resize(0);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _shader = nullptr;
    ChunksCache.Clear();
    ChunksCache.SetCapacity(0);
}

bool GlobalSignDistanceFieldPass::Get(const RenderBuffers* buffers, BindingData& result)
{
    auto* sdfData = buffers ? buffers->FindCustomBuffer<GlobalSignDistanceFieldCustomBuffer>(TEXT("GlobalSignDistanceField")) : nullptr;
    if (sdfData && sdfData->LastFrameUsed + 1 >= Engine::FrameCount) // Allow to use SDF from the previous frame (eg. particles in Editor using the Editor viewport in Game viewport - Game render task runs first)
    {
        result = sdfData->Result;
        return false;
    }
    return true;
}

bool GlobalSignDistanceFieldPass::Render(RenderContext& renderContext, GPUContext* context, BindingData& result)
{
    // Skip if not supported
    if (checkIfSkipPass())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& sdfData = *renderContext.Buffers->GetCustomBuffer<GlobalSignDistanceFieldCustomBuffer>(TEXT("GlobalSignDistanceField"));

    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (sdfData.LastFrameUsed == currentFrame)
    {
        result = sdfData.Result;
        return false;
    }
    sdfData.LastFrameUsed = currentFrame;
    PROFILE_GPU_CPU("Global SDF");

    // TODO: configurable via graphics settings
    const int32 resolution = 256;
    const int32 resolutionMip = Math::DivideAndRoundUp(resolution, GLOBAL_SDF_RASTERIZE_MIP_FACTOR);
    // TODO: configurable via postFx settings
    const float distanceExtent = 2000.0f;
    const float cascadesDistances[] = { distanceExtent, distanceExtent * 2.0f, distanceExtent * 4.0f, distanceExtent * 8.0f };

    // Initialize buffers
    auto desc = GPUTextureDescription::New3D(resolution, resolution, resolution, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
    bool updated = false;
    for (auto& cascade : sdfData.Cascades)
    {
        GPUTexture*& texture = cascade.Texture;
        if (texture && texture->Width() != desc.Width)
        {
            RenderTargetPool::Release(texture);
            texture = nullptr;
        }
        if (!texture)
        {
            texture = RenderTargetPool::Get(desc);
            if (!texture)
                return true;
            updated = true;
        }
    }
    desc = GPUTextureDescription::New3D(resolutionMip, resolutionMip, resolutionMip, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
    for (auto& cascade : sdfData.Cascades)
    {
        GPUTexture*& texture = cascade.Mip;
        if (texture && texture->Width() != desc.Width)
        {
            RenderTargetPool::Release(texture);
            texture = nullptr;
        }
        if (!texture)
        {
            texture = RenderTargetPool::Get(desc);
            if (!texture)
                return true;
            updated = true;
        }
    }
    GPUTexture* tmpMip = nullptr;
    if (updated)
    {
        PROFILE_GPU_CPU("Init");
        for (auto& cascade : sdfData.Cascades)
        {
            cascade.NonEmptyChunks.Clear();
            cascade.StaticChunks.Clear();
            context->ClearUA(cascade.Texture, Vector4::One);
            context->ClearUA(cascade.Mip, Vector4::One);
        }
        LOG(Info, "Global SDF memory usage: {0} MB", (sdfData.Cascades[0].Texture->GetMemoryUsage() + sdfData.Cascades[0].Mip->GetMemoryUsage()) * ARRAY_COUNT(sdfData.Cascades) / 1024 / 1024);
    }
    for (SceneRendering* scene : renderContext.List->Scenes)
        sdfData.ListenSceneRendering(scene);

    // Rasterize world geometry into Global SDF
    renderContext.View.Pass = DrawPass::GlobalSDF;
    uint32 viewMask = renderContext.View.RenderLayersMask;
    const bool useCache = !updated;
    static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_GROUP_SIZE == 0, "Invalid chunk size for Global SDF rasterization group size.");
    const int32 rasterizeChunks = Math::CeilToInt((float)resolution / (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE);
    auto& chunks = ChunksCache;
    chunks.EnsureCapacity(rasterizeChunks * rasterizeChunks, false);
    bool anyDraw = false;
    const uint64 cascadeFrequencies[] = { 2, 3, 5, 11 };
    //const uint64 cascadeFrequencies[] = { 1, 1, 1, 1 };
    for (int32 cascade = 0; cascade < 4; cascade++)
    for (int32 cascadeIndex = 0; cascadeIndex < 4; cascadeIndex++)
    {
        // Reduce frequency of the updates
        if (useCache && (Engine::FrameCount % cascadeFrequencies[cascadeIndex]) != 0)
            continue;
        auto& cascade = sdfData.Cascades[cascadeIndex];
        const float distance = cascadesDistances[cascadeIndex];
        const float maxDistance = distance * 2;
        const float voxelSize = maxDistance / resolution;
        const float chunkSize = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_MIP_FACTOR == 0, "Adjust chunk size to match the mip factor scale.");
        const Vector3 center = Vector3::Floor(renderContext.View.Position / chunkSize) * chunkSize;
        //const Vector3 center = Vector3::Zero;
        BoundingBox cascadeBounds(center - distance, center + distance);
        // TODO: add scene detail scale factor to PostFx settings (eg. to increase or decrease scene details and quality)
        const float minObjectRadius = Math::Max(20.0f, voxelSize * 0.5f); // Skip too small objects for this cascade
        GPUTextureView* cascadeView = cascade.Texture->ViewVolume();
        GPUTextureView* cascadeMipView = cascade.Mip->ViewVolume();

        // Clear cascade before rasterization
        {
            PROFILE_CPU_NAMED("Clear");
            chunks.Clear();
            _objectsBuffer->Clear();
            _objectsTextures.Clear();
        }

        // Check if cascade center has been moved
        if (!(useCache && Vector3::NearEqual(cascade.Position, center, voxelSize)))
        {
            // TODO: optimize for moving camera (copy sdf for cached chunks)
            cascade.StaticChunks.Clear();
        }
        cascade.Position = center;
        cascade.VoxelSize = voxelSize;
        cascade.Bounds = cascadeBounds;

        // Draw all objects from all scenes into the cascade
        _objectsBufferCount = 0;
        _voxelSize = voxelSize;
        _cascadeBounds = cascadeBounds;
        _cascadeIndex = cascadeIndex;
        _sdfData = &sdfData;
        {
            PROFILE_CPU_NAMED("Draw");
            for (SceneRendering* scene : renderContext.List->Scenes)
            {
                for (const auto& e : scene->Actors)
                {
                    if (viewMask & e.LayerMask && e.Bounds.Radius >= minObjectRadius && CollisionsHelper::BoxIntersectsSphere(cascadeBounds, e.Bounds))
                    {
                        e.Actor->Draw(renderContext);
                    }
                }
            }
        }

        // Perform batched chunks rasterization
        if (!anyDraw)
        {
            anyDraw = true;
            context->ResetSR();
            tmpMip = RenderTargetPool::Get(desc);
            if (!tmpMip)
                return true;
        }
        ModelsRasterizeData data;
        data.CascadeCoordToPosMul = cascadeBounds.GetSize() / resolution;
        data.CascadeCoordToPosAdd = cascadeBounds.Minimum + voxelSize * 0.5f;
        data.MaxDistance = maxDistance;
        data.CascadeResolution = resolution;
        data.CascadeMipResolution = resolutionMip;
        data.CascadeMipFactor = GLOBAL_SDF_RASTERIZE_MIP_FACTOR;
        data.CascadeVoxelSize = voxelSize;
        context->BindUA(0, cascadeView);
        context->BindCB(1, _cb1);
        const int32 chunkDispatchGroups = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / GLOBAL_SDF_RASTERIZE_GROUP_SIZE;
        bool anyChunkDispatch = false;
        {
            PROFILE_GPU_CPU("Clear Chunks");
            for (auto it = cascade.NonEmptyChunks.Begin(); it.IsNotEnd(); ++it)
            {
                auto& key = it->Item;
                if (chunks.ContainsKey(key))
                    continue;

                // Clear empty chunk
                cascade.NonEmptyChunks.Remove(it);
                data.ChunkCoord = key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                context->UpdateCB(_cb1, &data);
                context->Dispatch(_csClearChunk, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                anyChunkDispatch = true;
                // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches
            }
        }
        // TODO: rasterize models into global sdf relative to the cascade origin to prevent fp issues on large worlds
        {
            PROFILE_GPU_CPU("Rasterize Chunks");

            // Update static chunks
            for (auto it = chunks.Begin(); it.IsNotEnd(); ++it)
            {
                auto& e = *it;
                if (e.Key.Layer != 0)
                    continue;
                if (e.Value.Dynamic)
                {
                    // Remove static chunk with dynamic objects
                    cascade.StaticChunks.Remove(e.Key);
                }
                else if (cascade.StaticChunks.Contains(e.Key))
                {
                    // Skip updating static chunk
                    auto key = e.Key;
                    while (chunks.Remove(key))
                        key.NextLayer();
                }
                else
                {
                    // Add to cache (render now but skip next frame)
                    cascade.StaticChunks.Add(e.Key);
                }
            }

            // Send models data to the GPU
            if (chunks.Count() != 0)
            {
                PROFILE_GPU_CPU("Update Objects");
                _objectsBuffer->Flush(context);
            }
            context->BindSR(0, _objectsBuffer->GetBuffer() ? _objectsBuffer->GetBuffer()->View() : nullptr);

            // Rasterize non-empty chunks (first layer so can override existing chunk data)
            for (const auto& e : chunks)
            {
                if (e.Key.Layer != 0)
                    continue;
                auto& chunk = e.Value;
                cascade.NonEmptyChunks.Add(e.Key);

                for (int32 i = 0; i < chunk.ModelsCount; i++)
                {
                    auto objectIndex = chunk.Models[i];
                    data.Objects[i] = objectIndex;
                    context->BindSR(i + 1, _objectsTextures[objectIndex]);
                }
                for (int32 i = chunk.ModelsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                    context->UnBindSR(i + 1);
                data.ChunkCoord = e.Key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                data.ObjectsCount = chunk.ModelsCount;
                context->UpdateCB(_cb1, &data);
                auto cs = data.ObjectsCount != 0 ? _csRasterizeModel0 : _csClearChunk; // Terrain-only chunk can be quickly cleared
                context->Dispatch(cs, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                anyChunkDispatch = true;
                // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches (maybe cache per-shader write/read flags for all UAVs?)

                if (chunk.HeightfieldsCount != 0)
                {
                    // Inject heightfield (additive)
                    for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
                    {
                        auto objectIndex = chunk.Heightfields[i];
                        data.Objects[i] = objectIndex;
                        context->BindSR(i + 1, _objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.HeightfieldsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.HeightfieldsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeHeightfield, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                }

#if GLOBAL_SDF_DEBUG_CHUNKS
                // Debug draw chunk bounds in world space with number of models in it
                if (cascadeIndex + 1 == GLOBAL_SDF_DEBUG_CHUNKS)
                {
                    int32 count = chunk.ModelsCount + chunk.HeightfieldsCount;
                    RasterizeChunkKey tmp = e.Key;
                    tmp.NextLayer();
                    while (chunks.ContainsKey(tmp))
                    {
                        count += chunks[tmp].ModelsCount + chunks[tmp].HeightfieldsCount;
                        tmp.NextLayer();
                    }
                    Vector3 chunkMin = cascadeBounds.Minimum + Vector3(e.Key.Coord) * chunkSize;
                    BoundingBox chunkBounds(chunkMin, chunkMin + chunkSize);
                    DebugDraw::DrawWireBox(chunkBounds, Color::Red, 0, false);
                    DebugDraw::DrawText(StringUtils::ToString(count), chunkBounds.GetCenter(), Color::Red);
                }
#endif
            }

            // Rasterize non-empty chunks (additive layers so so need combine with existing chunk data)
            for (const auto& e : chunks)
            {
                if (e.Key.Layer == 0)
                    continue;
                auto& chunk = e.Value;
                data.ChunkCoord = e.Key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;

                if (chunk.ModelsCount != 0)
                {
                    // Inject models (additive)
                    for (int32 i = 0; i < chunk.ModelsCount; i++)
                    {
                        auto objectIndex = chunk.Models[i];
                        data.Objects[i] = objectIndex;
                        context->BindSR(i + 1, _objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.ModelsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.ModelsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeModel1, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                }

                if (chunk.HeightfieldsCount != 0)
                {
                    // Inject heightfields (additive)
                    for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
                    {
                        auto objectIndex = chunk.Heightfields[i];
                        data.Objects[i] = objectIndex;
                        context->BindSR(i + 1, _objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.HeightfieldsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.HeightfieldsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeHeightfield, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                }
                anyChunkDispatch = true;
            }
        }

        // Generate mip out of cascade (empty chunks have distance value 1 which is incorrect so mip will be used as a fallback - lower res)
        if (updated || anyChunkDispatch)
        {
            PROFILE_GPU_CPU("Generate Mip");
            context->UpdateCB(_cb1, &data);
            context->ResetUA();
            context->BindSR(0, cascadeView);
            context->BindUA(0, cascadeMipView);
            const int32 mipDispatchGroups = Math::DivideAndRoundUp(resolutionMip, GLOBAL_SDF_MIP_GROUP_SIZE);
            int32 floodFillIterations = chunks.Count() == 0 ? 1 : GLOBAL_SDF_MIP_FLOODS;
            context->Dispatch(_csGenerateMip0, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups);
            context->UnBindSR(0);
            GPUTextureView* tmpMipView = tmpMip->ViewVolume();
            for (int32 i = 1; i < floodFillIterations; i++)
            {
                context->ResetUA();
                context->BindSR(0, cascadeMipView);
                context->BindUA(0, tmpMipView);
                context->Dispatch(_csGenerateMip1, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups);
                Swap(tmpMipView, cascadeMipView);
            }
            if (floodFillIterations % 2 == 0)
                Swap(tmpMipView, cascadeMipView);
        }
    }

    RenderTargetPool::Release(tmpMip);
    if (anyDraw)
    {
        context->UnBindCB(1);
        context->ResetUA();
        context->FlushState();
        context->ResetSR();
        context->FlushState();
    }

    // Copy results
    static_assert(ARRAY_COUNT(result.Cascades) == ARRAY_COUNT(sdfData.Cascades), "Invalid cascades count.");
    static_assert(ARRAY_COUNT(result.CascadeMips) == ARRAY_COUNT(sdfData.Cascades), "Invalid cascades count.");
    static_assert(ARRAY_COUNT(sdfData.Cascades) == 4, "Invalid cascades count.");
    for (int32 cascadeIndex = 0; cascadeIndex < 4; cascadeIndex++)
    {
        auto& cascade = sdfData.Cascades[cascadeIndex];
        const float distance = cascadesDistances[cascadeIndex];
        const float maxDistance = distance * 2;
        const float voxelSize = maxDistance / resolution;
        const Vector3 center = cascade.Position;
        result.GlobalSDF.CascadePosDistance[cascadeIndex] = Vector4(center, distance);
        result.GlobalSDF.CascadeVoxelSize.Raw[cascadeIndex] = voxelSize;
        result.Cascades[cascadeIndex] = cascade.Texture;
        result.CascadeMips[cascadeIndex] = cascade.Mip;
    }
    result.GlobalSDF.Resolution = (float)resolution;
    sdfData.Result = result;
    return false;
}

void GlobalSignDistanceFieldPass::RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output)
{
    BindingData bindingData;
    if (Render(renderContext, context, bindingData))
    {
        context->Draw(output, renderContext.Buffers->GBuffer0);
        return;
    }

    PROFILE_GPU_CPU("Global SDF Debug");
    const Vector2 outputSize(output->Size());
    {
        Data data;
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        for (int32 i = 0; i < 4; i++)
            data.ViewFrustumWorldRays[i] = Vector4(renderContext.List->FrustumCornersWs[i + 4], 0);
        data.GlobalSDF = bindingData.GlobalSDF;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }
    for (int32 i = 0; i < 4; i++)
    {
        context->BindSR(i, bindingData.Cascades[i]->ViewVolume());
        context->BindSR(i + 4, bindingData.CascadeMips[i]->ViewVolume());
    }
    context->SetState(_psDebug);
    context->SetRenderTarget(output->View());
    context->SetViewportAndScissors(outputSize.X, outputSize.Y);
    context->DrawFullscreenTriangle();
}

void GlobalSignDistanceFieldPass::RasterizeModelSDF(Actor* actor, const ModelBase::SDFData& sdf, const Matrix& localToWorld, const BoundingBox& objectBounds)
{
    if (!sdf.Texture || sdf.Texture->ResidentMipLevels() == 0)
        return;

    // Setup object data
    BoundingBox objectBoundsCascade;
    const float objectMargin = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
    Vector3::Clamp(objectBounds.Minimum - objectMargin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Minimum);
    Vector3::Subtract(objectBoundsCascade.Minimum, _cascadeBounds.Minimum, objectBoundsCascade.Minimum);
    Vector3::Clamp(objectBounds.Maximum + objectMargin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Maximum);
    Vector3::Subtract(objectBoundsCascade.Maximum, _cascadeBounds.Minimum, objectBoundsCascade.Maximum);
    const float chunkSize = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
    Int3 objectChunkMin(objectBoundsCascade.Minimum / chunkSize);
    Int3 objectChunkMax(objectBoundsCascade.Maximum / chunkSize);
    Matrix worldToLocal, volumeToWorld;
    Matrix::Invert(localToWorld, worldToLocal);
    BoundingBox localVolumeBounds(sdf.LocalBoundsMin, sdf.LocalBoundsMax);
    Vector3 volumeLocalBoundsExtent = localVolumeBounds.GetSize() * 0.5f;
    Matrix worldToVolume = worldToLocal * Matrix::Translation(-(localVolumeBounds.Minimum + volumeLocalBoundsExtent));
    Matrix::Invert(worldToVolume, volumeToWorld);

    // Pick the SDF mip for the cascade
    int32 mipLevelIndex = 1;
    float worldUnitsPerVoxel = sdf.WorldUnitsPerVoxel * localToWorld.GetScaleVector().MaxValue() * 2;
    while (_voxelSize > worldUnitsPerVoxel && mipLevelIndex < sdf.Texture->MipLevels())
    {
        mipLevelIndex++;
        worldUnitsPerVoxel *= 2.0f;
    }
    mipLevelIndex--;

    // Volume -> Local -> UVW
    Vector3 volumeToUVWMul = sdf.LocalToUVWMul;
    Vector3 volumeToUVWAdd = sdf.LocalToUVWAdd + (localVolumeBounds.Minimum + volumeLocalBoundsExtent) * sdf.LocalToUVWMul;

    // Add object data for the GPU buffer
    uint16 objectIndex = _objectsBufferCount++;
    ObjectRasterizeData objectData;
    Matrix::Transpose(worldToVolume, objectData.WorldToVolume);
    Matrix::Transpose(volumeToWorld, objectData.VolumeToWorld);
    objectData.VolumeLocalBoundsExtent = volumeLocalBoundsExtent;
    objectData.VolumeToUVWMul = volumeToUVWMul;
    objectData.VolumeToUVWAdd = volumeToUVWAdd;
    objectData.MipOffset = (float)mipLevelIndex;
    objectData.DecodeMul = 2.0f * sdf.MaxDistance;
    objectData.DecodeAdd = -sdf.MaxDistance;
    _objectsBuffer->Write(objectData);
    _objectsTextures.Add(sdf.Texture->ViewVolume());

    // Inject object into the intersecting cascade chunks
    _sdfData->ObjectTypes.Add(actor->GetTypeHandle());
    RasterizeChunkKey key;
    auto& chunks = ChunksCache;
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    for (key.Coord.Z = objectChunkMin.Z; key.Coord.Z <= objectChunkMax.Z; key.Coord.Z++)
    {
        for (key.Coord.Y = objectChunkMin.Y; key.Coord.Y <= objectChunkMax.Y; key.Coord.Y++)
        {
            for (key.Coord.X = objectChunkMin.X; key.Coord.X <= objectChunkMax.X; key.Coord.X++)
            {
                key.Layer = 0;
                key.Hash = key.Coord.Z * (RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution) + key.Coord.Y * RasterizeChunkKeyHashResolution + key.Coord.X;
                RasterizeChunk* chunk = &chunks[key];
                chunk->Dynamic |= dynamic;

                // Move to the next layer if chunk has overflown
                while (chunk->ModelsCount == GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT)
                {
                    key.NextLayer();
                    chunk = &chunks[key];
                }

                chunk->Models[chunk->ModelsCount++] = objectIndex;
            }
        }
    }

    // Track streaming for textures used in static chunks to invalidate cache
    if (!dynamic && sdf.Texture->ResidentMipLevels() != sdf.Texture->MipLevels() && !_sdfData->SDFTextures.Contains(sdf.Texture))
    {
        sdf.Texture->Deleted.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(_sdfData);
        sdf.Texture->ResidentMipsChanged.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(_sdfData);
        _sdfData->SDFTextures.Add(sdf.Texture);
    }
}

void GlobalSignDistanceFieldPass::RasterizeHeightfield(Actor* actor, GPUTexture* heightfield, const Matrix& localToWorld, const BoundingBox& objectBounds, const Vector4& localToUV)
{
    if (!heightfield || heightfield->ResidentMipLevels() == 0)
        return;

    // Setup object data
    BoundingBox objectBoundsCascade;
    const float objectMargin = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
    Vector3::Clamp(objectBounds.Minimum - objectMargin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Minimum);
    Vector3::Subtract(objectBoundsCascade.Minimum, _cascadeBounds.Minimum, objectBoundsCascade.Minimum);
    Vector3::Clamp(objectBounds.Maximum + objectMargin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Maximum);
    Vector3::Subtract(objectBoundsCascade.Maximum, _cascadeBounds.Minimum, objectBoundsCascade.Maximum);
    const float chunkSize = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
    const Int3 objectChunkMin(objectBoundsCascade.Minimum / chunkSize);
    const Int3 objectChunkMax(objectBoundsCascade.Maximum / chunkSize);

    // Add object data for the GPU buffer
    uint16 objectIndex = _objectsBufferCount++;
    ObjectRasterizeData objectData;
    Matrix worldToLocal;
    Matrix::Invert(localToWorld, worldToLocal);
    Matrix::Transpose(worldToLocal, objectData.WorldToVolume);
    Matrix::Transpose(localToWorld, objectData.VolumeToWorld);
    objectData.VolumeToUVWMul = Vector3(localToUV.X, 1.0f, localToUV.Y);
    objectData.VolumeToUVWAdd = Vector3(localToUV.Z, 0.0f, localToUV.W);
    objectData.MipOffset = (float)_cascadeIndex * 0.5f; // Use lower-quality mip for far cascades
    _objectsBuffer->Write(objectData);
    _objectsTextures.Add(heightfield->View());

    // Inject object into the intersecting cascade chunks
    _sdfData->ObjectTypes.Add(actor->GetTypeHandle());
    RasterizeChunkKey key;
    auto& chunks = ChunksCache;
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    for (key.Coord.Z = objectChunkMin.Z; key.Coord.Z <= objectChunkMax.Z; key.Coord.Z++)
    {
        for (key.Coord.Y = objectChunkMin.Y; key.Coord.Y <= objectChunkMax.Y; key.Coord.Y++)
        {
            for (key.Coord.X = objectChunkMin.X; key.Coord.X <= objectChunkMax.X; key.Coord.X++)
            {
                key.Layer = 0;
                key.Hash = key.Coord.Z * (RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution) + key.Coord.Y * RasterizeChunkKeyHashResolution + key.Coord.X;
                RasterizeChunk* chunk = &chunks[key];
                chunk->Dynamic |= dynamic;

                // Move to the next layer if chunk has overflown
                while (chunk->HeightfieldsCount == GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT)
                {
                    key.NextLayer();
                    chunk = &chunks[key];
                }

                chunk->Heightfields[chunk->HeightfieldsCount++] = objectIndex;
            }
        }
    }

    // Track streaming for textures used in static chunks to invalidate cache
    if (!dynamic && heightfield->ResidentMipLevels() != heightfield->MipLevels() && !_sdfData->SDFTextures.Contains(heightfield))
    {
        heightfield->Deleted.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(_sdfData);
        heightfield->ResidentMipsChanged.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(_sdfData);
        _sdfData->SDFTextures.Add(heightfield);
    }
}
