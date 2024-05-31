// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GlobalSignDistanceFieldPass.h"
#include "RenderList.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix3x4.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTools.h"
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
#define GLOBAL_SDF_DEBUG_FORCE_REDRAW 0 // Forces to redraw all SDF cascades every frame
#define GLOBAL_SDF_ACTOR_IS_STATIC(actor) EnumHasAllFlags(actor->GetStaticFlags(), StaticFlags::Lightmap | StaticFlags::Transform)

static_assert(GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT % 4 == 0, "Must be multiple of 4 due to data packing for GPU constant buffer.");
#if GLOBAL_SDF_DEBUG_CHUNKS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct alignas(GPU_SHADER_DATA_ALIGNMENT) ObjectRasterizeData
    {
    Matrix3x4 WorldToVolume;
    Matrix3x4 VolumeToWorld;
    Float3 VolumeToUVWMul;
    float MipOffset;
    Float3 VolumeToUVWAdd;
    float DecodeMul;
    Float3 VolumeLocalBoundsExtent;
    float DecodeAdd;
    });

PACK_STRUCT(struct alignas(GPU_SHADER_DATA_ALIGNMENT) Data
    {
    Float3 ViewWorldPos;
    float ViewNearPlane;
    Float3 Padding00;
    float ViewFarPlane;
    Float4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
    });

PACK_STRUCT(struct alignas(GPU_SHADER_DATA_ALIGNMENT) ModelsRasterizeData
    {
    Int3 ChunkCoord;
    float MaxDistance;
    Float3 CascadeCoordToPosMul;
    uint32 ObjectsCount;
    Float3 CascadeCoordToPosAdd;
    int32 CascadeResolution;
    int32 CascadeIndex;
    float CascadeVoxelSize;
    int32 CascadeMipResolution;
    int32 CascadeMipFactor;
    uint32 Objects[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
    uint32 GenerateMipTexResolution;
    uint32 GenerateMipCoordScale;
    uint32 GenerateMipTexOffsetX;
    uint32 GenerateMipMipOffsetX;
    });

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

struct RasterizeObject
{
    Actor* Actor;
    const ModelBase::SDFData* SDF;
    GPUTexture* Heightfield;
    Transform LocalToWorld;
    BoundingBox ObjectBounds;
    Float4 LocalToUV;
};

constexpr int32 RasterizeChunkKeyHashResolution = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;

struct RasterizeChunkKey
{
    uint32 Hash;
    uint32 Layer;
    Int3 Coord;

    FORCE_INLINE void NextLayer()
    {
        Layer++;
        Hash += RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution;
    }

    bool operator==(const RasterizeChunkKey& other) const
    {
        return Hash == other.Hash && Coord == other.Coord && Layer == other.Layer;
    }
};

uint32 GetHash(const RasterizeChunkKey& key)
{
    return key.Hash;
}

struct CascadeData
{
    Float3 Position;
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
};

class GlobalSignDistanceFieldCustomBuffer : public RenderBuffers::CustomBuffer, public ISceneRenderingListener
{
public:
    int32 FrameIndex = 0;
    int32 Resolution = 0;
    GPUTexture* Texture = nullptr;
    GPUTexture* TextureMip = nullptr;
    Vector3 Origin = Vector3::Zero;
    Array<CascadeData, FixedAllocation<4>> Cascades;
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
        RenderTargetPool::Release(Texture);
        RenderTargetPool::Release(TextureMip);
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
    Array<RasterizeObject> RasterizeObjectsCache;
    Dictionary<uint16, uint16> ObjectIndexToDataIndexCache;
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
            && EnumHasAllFlags(device->GetFormatFeatures(GLOBAL_SDF_FORMAT).Support, FormatSupport::ShaderSample | FormatSupport::Texture3D);
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
    _csGenerateMip = shader->GetCS("CS_GenerateMip");

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
    _csGenerateMip = nullptr;
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
    ChunksCache.SetCapacity(0);
    RasterizeObjectsCache.SetCapacity(0);
    ObjectIndexToDataIndexCache.SetCapacity(0);
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

    // Setup options
    int32 resolution, cascadesCount;
    switch (Graphics::GlobalSDFQuality)
    {
    case Quality::Low:
        resolution = 128;
        cascadesCount = 2;
        break;
    case Quality::Medium:
        resolution = 128;
        cascadesCount = 3;
        break;
    case Quality::High:
        resolution = 192;
        cascadesCount = 4;
        break;
    case Quality::Ultra:
    default:
        resolution = 256;
        cascadesCount = 4;
        break;
    }
    const int32 resolutionMip = Math::DivideAndRoundUp(resolution, GLOBAL_SDF_RASTERIZE_MIP_FACTOR);
    auto& giSettings = renderContext.List->Settings.GlobalIllumination;
    float distance = GraphicsSettings::Get()->GlobalSDFDistance;
    if (giSettings.Mode == GlobalIlluminationMode::DDGI)
        distance = Math::Max(distance, giSettings.Distance);
    distance = Math::Min(distance, renderContext.View.Far);
    const float cascadesDistanceScales[] = { 1.0f, 2.5f, 5.0f, 10.0f };
    const float distanceExtent = distance / cascadesDistanceScales[cascadesCount - 1];

    // Initialize buffers
    bool updated = false;
    if (sdfData.Cascades.Count() != cascadesCount || sdfData.Resolution != resolution)
    {
        sdfData.Cascades.Resize(cascadesCount);
        sdfData.Resolution = resolution;
        sdfData.FrameIndex = 0;
        updated = true;
        auto desc = GPUTextureDescription::New3D(resolution * cascadesCount, resolution, resolution, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
        {
            GPUTexture*& texture = sdfData.Texture;
            if (texture && texture->Width() != desc.Width)
            {
                RenderTargetPool::Release(texture);
                sdfData.Texture = nullptr;
            }
            if (!texture)
            {
                texture = RenderTargetPool::Get(desc);
                if (!texture)
                    return true;
                RENDER_TARGET_POOL_SET_NAME(texture, "GlobalSDF.Cascade");
            }
        }
        desc.Width = resolutionMip * cascadesCount;
        desc.Height = desc.Depth = resolutionMip;
        {
            GPUTexture*& texture = sdfData.TextureMip;
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
                RENDER_TARGET_POOL_SET_NAME(texture, "GlobalSDF.Cascade");
            }
        }
        uint64 memoryUsage = sdfData.Texture->GetMemoryUsage() + sdfData.TextureMip->GetMemoryUsage();
        LOG(Info, "Global SDF memory usage: {0} MB", memoryUsage / 1024 / 1024);
    }
    if (sdfData.Origin != renderContext.View.Origin)
    {
        sdfData.Origin = renderContext.View.Origin;
        updated = true;
    }
    GPUTexture* tmpMip = nullptr;
    if (updated)
    {
        PROFILE_GPU_CPU_NAMED("Init");
        for (auto& cascade : sdfData.Cascades)
        {
            cascade.NonEmptyChunks.Clear();
            cascade.StaticChunks.Clear();
        }
        context->ClearUA(sdfData.Texture, Float4::One);
        context->ClearUA(sdfData.TextureMip, Float4::One);
    }
    for (SceneRendering* scene : renderContext.List->Scenes)
        sdfData.ListenSceneRendering(scene);

    // Calculate origin for Global SDF by shifting it towards the view direction to account for better view frustum coverage
    Float3 viewPosition = renderContext.View.Position;
    {
        Float3 viewDirection = renderContext.View.Direction;
        const float cascade0Distance = distanceExtent * cascadesDistanceScales[0];
        const Vector2 viewRayHit = CollisionsHelper::LineHitsBox(viewPosition, viewPosition + viewDirection * (cascade0Distance * 2.0f), viewPosition - cascade0Distance, viewPosition + cascade0Distance);
        const float viewOriginOffset = (float)viewRayHit.Y * cascade0Distance * 0.6f;
        viewPosition += viewDirection * viewOriginOffset;
    }

    // Rasterize world geometry into Global SDF
    renderContext.View.Pass = DrawPass::GlobalSDF;
    uint32 viewMask = renderContext.View.RenderLayersMask;
    const bool useCache = !updated && !GLOBAL_SDF_DEBUG_FORCE_REDRAW;
    static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_GROUP_SIZE == 0, "Invalid chunk size for Global SDF rasterization group size.");
    const int32 rasterizeChunks = Math::CeilToInt((float)resolution / (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE);
    auto& chunks = ChunksCache;
    chunks.EnsureCapacity(rasterizeChunks * rasterizeChunks, false);
    bool anyDraw = false;
    const bool updateEveryFrame = false; // true if update all cascades every frame
    const int32 maxCascadeUpdatesPerFrame = 1; // maximum cascades to update at a single frame
    GPUTextureView* textureView = sdfData.Texture->ViewVolume();
    GPUTextureView* textureMipView = sdfData.TextureMip->ViewVolume();
    if (sdfData.FrameIndex++ > 128)
        sdfData.FrameIndex = 0;
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        // Reduce frequency of the updates
        if (useCache && !RenderTools::ShouldUpdateCascade(sdfData.FrameIndex, cascadeIndex, cascadesCount, maxCascadeUpdatesPerFrame, updateEveryFrame))
            continue;
        auto& cascade = sdfData.Cascades[cascadeIndex];
        const float cascadeDistance = distanceExtent * cascadesDistanceScales[cascadeIndex];
        const float cascadeMaxDistance = cascadeDistance * 2;
        const float cascadeVoxelSize = cascadeMaxDistance / (float)resolution;
        const float cascadeChunkSize = cascadeVoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_MIP_FACTOR == 0, "Adjust chunk size to match the mip factor scale.");
        const Float3 center = Float3::Floor(viewPosition / cascadeChunkSize) * cascadeChunkSize;
        //const Float3 center = Float3::Zero;
        BoundingBox cascadeBounds(center - cascadeDistance, center + cascadeDistance);
        // TODO: add scene detail scale factor to PostFx settings (eg. to increase or decrease scene details and quality)
        const float minObjectRadius = Math::Max(20.0f, cascadeVoxelSize * 2.0f); // Skip too small objects for this cascade

        // Clear cascade before rasterization
        {
            PROFILE_CPU_NAMED("Clear");
            chunks.Clear();
            RasterizeObjectsCache.Clear();
            _objectsBuffer->Clear();
            _objectsTextures.Clear();
        }

        // Check if cascade center has been moved
        if (!(useCache && Float3::NearEqual(cascade.Position, center, cascadeVoxelSize)))
        {
            // TODO: optimize for moving camera (copy sdf for cached chunks)
            cascade.StaticChunks.Clear();
        }
        cascade.Position = center;
        cascade.VoxelSize = cascadeVoxelSize;
        cascade.Bounds = cascadeBounds;

        // Draw all objects from all scenes into the cascade
        _objectsBufferCount = 0;
        _voxelSize = cascadeVoxelSize;
        _chunkSize = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        _cascadeBounds = cascadeBounds;
        _cascadeBounds.Minimum += 0.1f; // Adjust to prevent overflowing chunk keys (cascade bounds are used for clamping object bounds)
        _cascadeBounds.Maximum -= 0.1f; // Adjust to prevent overflowing chunk keys (cascade bounds are used for clamping object bounds)
        _cascadeIndex = cascadeIndex;
        _sdfData = &sdfData;
        const float objectMargin = _voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
        _sdfDataOriginMin = -sdfData.Origin - objectMargin;
        _sdfDataOriginMax = -sdfData.Origin + objectMargin;
        {
            PROFILE_CPU_NAMED("Draw");
            BoundingBox cascadeBoundsWorld = cascadeBounds.MakeOffsetted(sdfData.Origin);
            _cascadeCullingBounds = cascadeBoundsWorld;
            int32 actorsDrawn = 0;
            SceneRendering::DrawCategory drawCategories[] = { SceneRendering::SceneDraw, SceneRendering::SceneDrawAsync };
            for (auto* scene : renderContext.List->Scenes)
            {
                for (SceneRendering::DrawCategory drawCategory : drawCategories)
                {
                    auto& list = scene->Actors[drawCategory];
                    for (const auto& e : list)
                    {
                        if (e.Bounds.Radius >= minObjectRadius && viewMask & e.LayerMask && CollisionsHelper::BoxIntersectsSphere(cascadeBoundsWorld, e.Bounds))
                        {
                            //PROFILE_CPU_ACTOR(e.Actor);
                            e.Actor->Draw(renderContext);
                            actorsDrawn++;
                        }
                    }
                }
            }
            ZoneValue(actorsDrawn);
        }

        // Perform batched chunks rasterization
        anyDraw = true;
        context->ResetSR();
        ModelsRasterizeData data;
        data.CascadeCoordToPosMul = (Float3)cascadeBounds.GetSize() / (float)resolution;
        data.CascadeCoordToPosAdd = (Float3)cascadeBounds.Minimum + cascadeVoxelSize * 0.5f;
        data.MaxDistance = cascadeMaxDistance;
        data.CascadeResolution = resolution;
        data.CascadeMipResolution = resolutionMip;
        data.CascadeIndex = cascadeIndex;
        data.CascadeMipFactor = GLOBAL_SDF_RASTERIZE_MIP_FACTOR;
        data.CascadeVoxelSize = cascadeVoxelSize;
        context->BindUA(0, textureView);
        context->BindCB(1, _cb1);
        const int32 chunkDispatchGroups = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / GLOBAL_SDF_RASTERIZE_GROUP_SIZE;
        bool anyChunkDispatch = false;
        {
            PROFILE_GPU_CPU_NAMED("Clear Chunks");
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
        {
            PROFILE_GPU_CPU_NAMED("Rasterize Chunks");

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
            const auto& objectIndexToDataIndex = ObjectIndexToDataIndexCache;
            if (chunks.Count() != 0)
            {
                PROFILE_GPU_CPU_NAMED("Update Objects");
                auto& objectIndexToDataIndexCache = ObjectIndexToDataIndexCache;
                objectIndexToDataIndexCache.Clear();

                // Write used objects to the buffer
                const auto& rasterizeObjectsCache = RasterizeObjectsCache;
                for (const auto& e : chunks)
                {
                    auto& chunk = e.Value;
                    for (int32 i = 0; i < chunk.ModelsCount; i++)
                    {
                        auto objectIndex = chunk.Models[i];
                        if (objectIndexToDataIndexCache.ContainsKey(objectIndex))
                            continue;
                        const auto& object = rasterizeObjectsCache.Get()[objectIndex];

                        // Pick the SDF mip for the cascade
                        int32 mipLevelIndex = 1;
                        float worldUnitsPerVoxel = object.SDF->WorldUnitsPerVoxel * object.LocalToWorld.Scale.MaxValue() * 4;
                        const int32 mipLevels = object.SDF->Texture->MipLevels();
                        while (_voxelSize > worldUnitsPerVoxel && mipLevelIndex < mipLevels)
                        {
                            mipLevelIndex++;
                            worldUnitsPerVoxel *= 2.0f;
                        }
                        mipLevelIndex--;

                        // Add object data for the GPU buffer
                        uint16 dataIndex = _objectsBufferCount++;
                        ObjectRasterizeData objectData;
                        Matrix localToWorld, worldToLocal, volumeToWorld;
                        Matrix::Transformation(object.LocalToWorld.Scale, object.LocalToWorld.Orientation, object.LocalToWorld.Translation - _sdfData->Origin, localToWorld);
                        Matrix::Invert(localToWorld, worldToLocal);
                        BoundingBox localVolumeBounds(object.SDF->LocalBoundsMin, object.SDF->LocalBoundsMax);
                        Float3 volumeLocalBoundsExtent = localVolumeBounds.GetSize() * 0.5f;
                        Matrix worldToVolume = worldToLocal * Matrix::Translation(-(localVolumeBounds.Minimum + volumeLocalBoundsExtent));
                        Matrix::Invert(worldToVolume, volumeToWorld);
                        objectData.WorldToVolume.SetMatrixTranspose(worldToVolume);
                        objectData.VolumeToWorld.SetMatrixTranspose(volumeToWorld);
                        objectData.VolumeLocalBoundsExtent = volumeLocalBoundsExtent;
                        objectData.VolumeToUVWMul = object.SDF->LocalToUVWMul;
                        objectData.VolumeToUVWAdd = object.SDF->LocalToUVWAdd + (localVolumeBounds.Minimum + volumeLocalBoundsExtent) * object.SDF->LocalToUVWMul;
                        objectData.MipOffset = (float)mipLevelIndex;
                        objectData.DecodeMul = 2.0f * object.SDF->MaxDistance;
                        objectData.DecodeAdd = -object.SDF->MaxDistance;
                        _objectsBuffer->Write(objectData);
                        _objectsTextures.Add(object.SDF->Texture->ViewVolume());
                        _sdfData->ObjectTypes.Add(object.Actor->GetTypeHandle());

                        // Cache the mapping
                        objectIndexToDataIndexCache.Add(objectIndex, dataIndex);
                    }
                    for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
                    {
                        auto objectIndex = chunk.Heightfields[i];
                        if (objectIndexToDataIndexCache.ContainsKey(objectIndex))
                            continue;
                        const auto& object = rasterizeObjectsCache.Get()[objectIndex];

                        // Add object data for the GPU buffer
                        uint16 dataIndex = _objectsBufferCount++;
                        ObjectRasterizeData objectData;
                        Matrix localToWorld, worldToLocal;
                        Matrix::Transformation(object.LocalToWorld.Scale, object.LocalToWorld.Orientation, object.LocalToWorld.Translation - _sdfData->Origin, localToWorld);
                        Matrix::Invert(localToWorld, worldToLocal);
                        objectData.WorldToVolume.SetMatrixTranspose(worldToLocal);
                        objectData.VolumeToWorld.SetMatrixTranspose(localToWorld);
                        objectData.VolumeToUVWMul = Float3(object.LocalToUV.X, 1.0f, object.LocalToUV.Y);
                        objectData.VolumeToUVWAdd = Float3(object.LocalToUV.Z, 0.0f, object.LocalToUV.W);
                        objectData.MipOffset = (float)_cascadeIndex * 0.5f; // Use lower-quality mip for far cascades
                        _objectsBuffer->Write(objectData);
                        _objectsTextures.Add(object.Heightfield->View());
                        _sdfData->ObjectTypes.Add(object.Actor->GetTypeHandle());

                        // Cache the mapping
                        objectIndexToDataIndexCache.Add(objectIndex, dataIndex);
                    }
                }

                // Flush buffer
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
                    auto objectIndex = objectIndexToDataIndex.At(chunk.Models[i]);
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
                        auto objectIndex = objectIndexToDataIndex.At(chunk.Heightfields[i]);
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
                    Float3 chunkMin = cascadeBounds.Minimum + Float3(e.Key.Coord) * cascadeChunkSize;
                    BoundingBox chunkBounds(chunkMin, chunkMin + cascadeChunkSize);
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
                        auto objectIndex = objectIndexToDataIndex.At(chunk.Models[i]);
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
                        auto objectIndex = objectIndexToDataIndex.At(chunk.Heightfields[i]);
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
            PROFILE_GPU_CPU_NAMED("Generate Mip");
            context->ResetUA();
            const int32 mipDispatchGroups = Math::DivideAndRoundUp(resolutionMip, GLOBAL_SDF_MIP_GROUP_SIZE);
            static_assert((GLOBAL_SDF_MIP_FLOODS % 2) == 1, "Invalid Global SDF mip flood iterations count.");
            int32 floodFillIterations = chunks.Count() == 0 ? 1 : GLOBAL_SDF_MIP_FLOODS;
            if (!tmpMip)
            {
                // Use temporary texture to flood fill mip
                auto desc = GPUTextureDescription::New3D(resolutionMip, resolutionMip, resolutionMip, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
                tmpMip = RenderTargetPool::Get(desc);
                if (!tmpMip)
                    return true;
                RENDER_TARGET_POOL_SET_NAME(tmpMip, "GlobalSDF.Mip");
            }
            GPUTextureView* tmpMipView = tmpMip->ViewVolume();

            // Tex -> Mip
            // TODO: use push constants on DX12/Vulkan to provide those 4 uints to the shader
            data.GenerateMipTexResolution = data.CascadeResolution;
            data.GenerateMipCoordScale = data.CascadeMipFactor;
            data.GenerateMipTexOffsetX = data.CascadeIndex * data.CascadeResolution;
            data.GenerateMipMipOffsetX = data.CascadeIndex * data.CascadeMipResolution;
            context->UpdateCB(_cb1, &data);
            context->BindSR(0, textureView);
            context->BindUA(0, textureMipView);
            context->Dispatch(_csGenerateMip, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups);

            data.GenerateMipTexResolution = data.CascadeMipResolution;
            data.GenerateMipCoordScale = 1;
            for (int32 i = 1; i < floodFillIterations; i++)
            {
                context->ResetUA();
                if ((i & 1) == 1)
                {
                    // Mip -> Tmp
                    context->BindSR(0, textureMipView);
                    context->BindUA(0, tmpMipView);
                    data.GenerateMipTexOffsetX = data.CascadeIndex * data.CascadeMipResolution;
                    data.GenerateMipMipOffsetX = 0;
                }
                else
                {
                    // Tmp -> Mip
                    context->BindSR(0, tmpMipView);
                    context->BindUA(0, textureMipView);
                    data.GenerateMipTexOffsetX = 0;
                    data.GenerateMipMipOffsetX = data.CascadeIndex * data.CascadeMipResolution;
                }
                context->UpdateCB(_cb1, &data);
                context->Dispatch(_csGenerateMip, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups);
            }
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
    result.Texture = sdfData.Texture;
    result.TextureMip = sdfData.TextureMip;
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        auto& cascade = sdfData.Cascades[cascadeIndex];
        const float cascadeDistance = distanceExtent * cascadesDistanceScales[cascadeIndex];
        const float cascadeMaxDistance = cascadeDistance * 2;
        const float cascadeVoxelSize = cascadeMaxDistance / (float)resolution;
        const Float3 center = cascade.Position;
        result.Constants.CascadePosDistance[cascadeIndex] = Vector4(center, cascadeDistance);
        result.Constants.CascadeVoxelSize.Raw[cascadeIndex] = cascadeVoxelSize;
    }
    for (int32 cascadeIndex = cascadesCount; cascadeIndex < 4; cascadeIndex++)
    {
        result.Constants.CascadePosDistance[cascadeIndex] = result.Constants.CascadePosDistance[cascadesCount - 1];
        result.Constants.CascadeVoxelSize.Raw[cascadeIndex] = result.Constants.CascadeVoxelSize.Raw[cascadesCount - 1];
    }
    result.Constants.Resolution = (float)resolution;
    result.Constants.CascadesCount = cascadesCount;
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
    const Float2 outputSize(output->Size());
    {
        Data data;
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        for (int32 i = 0; i < 4; i++)
            data.ViewFrustumWorldRays[i] = Float4(renderContext.List->FrustumCornersWs[i + 4], 0);
        data.GlobalSDF = bindingData.Constants;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }
    context->BindSR(0, bindingData.Texture ? bindingData.Texture->ViewVolume() : nullptr);
    context->BindSR(1, bindingData.TextureMip ? bindingData.TextureMip->ViewVolume() : nullptr);
    context->SetState(_psDebug);
    context->SetRenderTarget(output->View());
    context->SetViewportAndScissors(outputSize.X, outputSize.Y);
    context->DrawFullscreenTriangle();
}

void GlobalSignDistanceFieldPass::RasterizeModelSDF(Actor* actor, const ModelBase::SDFData& sdf, const Transform& localToWorld, const BoundingBox& objectBounds)
{
    if (!sdf.Texture)
        return;
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    const int32 residentMipLevels = sdf.Texture->ResidentMipLevels();
    if (residentMipLevels != 0)
    {
        // Setup object data
        BoundingBox objectBoundsCascade;
        Vector3::Clamp(objectBounds.Minimum + _sdfDataOriginMin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, _cascadeBounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + _sdfDataOriginMax, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, _cascadeBounds.Minimum, objectBoundsCascade.Maximum);
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / _chunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / _chunkSize);

        // Add object data
        const uint16 dataIndex = RasterizeObjectsCache.Count();
        auto& data = RasterizeObjectsCache.AddOne();
        data.Actor = actor;
        data.SDF = &sdf;
        data.LocalToWorld = localToWorld;
        data.ObjectBounds = objectBounds;

        // Inject object into the intersecting cascade chunks
        RasterizeChunkKey key;
        auto& chunks = ChunksCache;
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

                    chunk->Models[chunk->ModelsCount++] = dataIndex;
                }
            }
        }
    }

    // Track streaming for textures used in static chunks to invalidate cache
    if (!dynamic && residentMipLevels != sdf.Texture->MipLevels() && !_sdfData->SDFTextures.Contains(sdf.Texture))
    {
        sdf.Texture->Deleted.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(_sdfData);
        sdf.Texture->ResidentMipsChanged.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(_sdfData);
        _sdfData->SDFTextures.Add(sdf.Texture);
    }
}

void GlobalSignDistanceFieldPass::RasterizeHeightfield(Actor* actor, GPUTexture* heightfield, const Transform& localToWorld, const BoundingBox& objectBounds, const Float4& localToUV)
{
    if (!heightfield)
        return;
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    const int32 residentMipLevels = heightfield->ResidentMipLevels();
    if (residentMipLevels != 0)
    {
        // Setup object data
        BoundingBox objectBoundsCascade;
        Vector3::Clamp(objectBounds.Minimum + _sdfDataOriginMin, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, _cascadeBounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + _sdfDataOriginMax, _cascadeBounds.Minimum, _cascadeBounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, _cascadeBounds.Minimum, objectBoundsCascade.Maximum);
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / _chunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / _chunkSize);

        // Add object data
        const uint16 dataIndex = RasterizeObjectsCache.Count();
        auto& data = RasterizeObjectsCache.AddOne();
        data.Actor = actor;
        data.Heightfield = heightfield;
        data.LocalToWorld = localToWorld;
        data.ObjectBounds = objectBounds;
        data.LocalToUV = localToUV;

        // Inject object into the intersecting cascade chunks
        RasterizeChunkKey key;
        auto& chunks = ChunksCache;
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

                    chunk->Heightfields[chunk->HeightfieldsCount++] = dataIndex;
                }
            }
        }
    }

    // Track streaming for textures used in static chunks to invalidate cache
    if (!dynamic && residentMipLevels != heightfield->MipLevels() && !_sdfData->SDFTextures.Contains(heightfield))
    {
        heightfield->Deleted.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(_sdfData);
        heightfield->ResidentMipsChanged.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(_sdfData);
        _sdfData->SDFTextures.Add(heightfield);
    }
}
