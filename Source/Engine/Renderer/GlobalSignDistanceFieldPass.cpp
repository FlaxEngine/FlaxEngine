// Copyright (c) Wojciech Figat. All rights reserved.

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
#include "Engine/Threading/JobSystem.h"

// Some of those constants must match in shader
#define GLOBAL_SDF_FORMAT PixelFormat::R8_SNorm
#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28 // The maximum amount of models to rasterize at once as a batch into Global SDF.
#define GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT 2 // The maximum amount of heightfields to store in a single chunk.
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32 // Global SDF chunk size in voxels.
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4 // The margin in voxels around objects for culling. Reduces artifacts but reduces performance.
#define GLOBAL_SDF_RASTERIZE_MIP_FACTOR 4 // Global SDF mip resolution downscale factor.
#define GLOBAL_SDF_MIP_GROUP_SIZE 4
#define GLOBAL_SDF_MIP_FLOODS 5 // Amount of flood fill passes for mip.
#define GLOBAL_SDF_DEBUG_CHUNKS 0 // Toggles debug drawing of Global SDF chunks bounds including objects count label (only for the first cascade)
#define GLOBAL_SDF_DEBUG_FORCE_REDRAW 0 // Forces to redraw all SDF cascades every frame
#define GLOBAL_SDF_ACTOR_IS_STATIC(actor) EnumHasAllFlags(actor->GetStaticFlags(), StaticFlags::Lightmap | StaticFlags::Transform)

static_assert(GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT % 4 == 0, "Must be multiple of 4 due to data packing for GPU constant buffer.");
#if GLOBAL_SDF_DEBUG_CHUNKS
#include "Engine/Debug/DebugDraw.h"
#endif

GPU_CB_STRUCT(ObjectRasterizeData {
    Matrix3x4 WorldToVolume;
    Matrix3x4 VolumeToWorld;
    Float3 VolumeToUVWMul;
    float MipOffset;
    Float3 VolumeToUVWAdd;
    float DecodeMul;
    Float3 VolumeLocalBoundsExtent;
    float DecodeAdd;
    });

GPU_CB_STRUCT(Data {
    Float3 ViewWorldPos;
    float ViewNearPlane;
    Float3 Padding00;
    float ViewFarPlane;
    Float4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
    });

GPU_CB_STRUCT(ModelsRasterizeData {
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
    Float2 Padding10;
    float MipMaxDistanceLoad;
    float MipMaxDistanceStore;
    uint32 MipTexResolution;
    uint32 MipCoordScale;
    uint32 MipTexOffsetX;
    uint32 MipMipOffsetX;
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
    bool Dirty;
    int32 Index;
    float ChunkSize;
    float MaxDistanceTex;
    float MaxDistanceMip;
    Float3 Position;
    float VoxelSize;
    float Extent;
    BoundingBox Bounds;
    BoundingBox CullingBounds;
    BoundingBox RasterizeBounds;
    Vector3 OriginMin;
    Vector3 OriginMax;
    HashSet<RasterizeChunkKey> NonEmptyChunks;
    HashSet<RasterizeChunkKey> StaticChunks;

    // Cache
    Dictionary<RasterizeChunkKey, RasterizeChunk> Chunks;
    Array<RasterizeObject> RasterizeObjects;
    Array<byte> ObjectsData;
    Array<GPUTextureView*> ObjectsTextures;
    Dictionary<uint16, uint16> ObjectIndexToDataIndex;
    HashSet<GPUTexture*> PendingSDFTextures;
    HashSet<ScriptingTypeHandle> PendingObjectTypes;

    void OnSceneRenderingDirty(const BoundingBox& objectBounds)
    {
        if (StaticChunks.IsEmpty() || !Bounds.Intersects(objectBounds))
            return;

        BoundingBox objectBoundsCascade;
        const float objectMargin = VoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
        Vector3::Clamp(objectBounds.Minimum - objectMargin, Bounds.Minimum, Bounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, Bounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + objectMargin, Bounds.Minimum, Bounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, Bounds.Minimum, objectBoundsCascade.Maximum);
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / ChunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / ChunkSize);

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

    // Async objects drawing cache
    Array<int64, FixedAllocation<1>> AsyncDrawWaitLabels;
    RenderContext AsyncRenderContext;

    ~GlobalSignDistanceFieldCustomBuffer()
    {
        WaitForDrawing();
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

    const float CascadesDistanceScales[4] = { 1.0f, 2.5f, 5.0f, 10.0f };

    void GetOptions(const RenderContext& renderContext, int32& resolution, int32& cascadesCount, int32& resolutionMip, float& distance)
    {
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
        resolutionMip = Math::DivideAndRoundUp(resolution, GLOBAL_SDF_RASTERIZE_MIP_FACTOR);
        auto& giSettings = renderContext.List->Settings.GlobalIllumination;
        distance = GraphicsSettings::Get()->GlobalSDFDistance;
        if (giSettings.Mode == GlobalIlluminationMode::DDGI)
            distance = Math::Max(distance, giSettings.Distance);
        distance = Math::Min(distance, renderContext.View.Far);
    }

    void DrawCascadeActors(const CascadeData& cascade);
    void UpdateCascadeChunks(CascadeData& cascade);
    void WriteCascadeObjects(CascadeData& cascade);
    void DrawCascadeJob(int32 cascadeIndex);

    void StartDrawing(const RenderContext& renderContext, bool enableAsync = false, bool reset = false)
    {
        if (AsyncDrawWaitLabels.HasItems())
            return; // Already started earlier this frame
        int32 resolution, cascadesCount, resolutionMip;
        float distance;
        GetOptions(renderContext, resolution, cascadesCount, resolutionMip, distance);
        if (Cascades.Count() != cascadesCount || Resolution != resolution || Origin != renderContext.View.Origin)
            return; // Not yet initialized
        PROFILE_CPU();

        // Calculate origin for Global SDF by shifting it towards the view direction to account for better view frustum coverage
        const float distanceExtent = distance / CascadesDistanceScales[cascadesCount - 1];
        Float3 viewPosition = renderContext.View.Position;
        {
            Float3 viewDirection = renderContext.View.Direction;
            const float cascade0Distance = distanceExtent * CascadesDistanceScales[0];
            const Vector2 viewRayHit = CollisionsHelper::LineHitsBox(viewPosition, viewPosition + viewDirection * (cascade0Distance * 2.0f), viewPosition - cascade0Distance, viewPosition + cascade0Distance);
            const float viewOriginOffset = (float)viewRayHit.Y * cascade0Distance * 0.6f;
            viewPosition += viewDirection * viewOriginOffset;
        }

        // Setup data for rendering
        if (FrameIndex++ > 128)
            FrameIndex = 0;
        AsyncRenderContext = renderContext;
        AsyncRenderContext.View.Pass = DrawPass::GlobalSDF;
        const bool useCache = !reset && !GLOBAL_SDF_DEBUG_FORCE_REDRAW;
        static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_GROUP_SIZE == 0, "Invalid chunk size for Global SDF rasterization group size.");
        const int32 rasterizeChunks = Math::CeilToInt((float)resolution / (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE);
        const bool updateEveryFrame = !GPU_SPREAD_WORKLOAD; // true if update all cascades every frame
        const int32 maxCascadeUpdatesPerFrame = 1; // maximum cascades to update at a single frame

        // Rasterize world geometry into Global SDF
        for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
        {
            // Reduce frequency of the updates
            auto& cascade = Cascades[cascadeIndex];
            cascade.Index = cascadeIndex;
            cascade.Dirty = !useCache || RenderTools::ShouldUpdateCascade(FrameIndex, cascadeIndex, cascadesCount, maxCascadeUpdatesPerFrame, updateEveryFrame);
            if (!cascade.Dirty)
                continue;
            const float cascadeExtent = distanceExtent * CascadesDistanceScales[cascadeIndex];
            const float cascadeSize = cascadeExtent * 2;
            const float cascadeVoxelSize = cascadeSize / (float)resolution;
            const float cascadeChunkSize = cascadeVoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
            static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_MIP_FACTOR == 0, "Adjust chunk size to match the mip factor scale.");
            const Float3 center = Float3::Floor(viewPosition / cascadeChunkSize) * cascadeChunkSize;
            //const Float3 center = Float3::Zero;
            BoundingBox cascadeBounds(center - cascadeExtent, center + cascadeExtent);

            // Clear cascade before rasterization
            cascade.Chunks.Clear();
            // TODO: consider using for RendererAllocation Chunks and RasterizeObjects to share memory with other rendering internals (ensure to release memory after SDF draw ends)
            cascade.Chunks.EnsureCapacity(rasterizeChunks * rasterizeChunks, false);
            // TODO: cache RasterizeObjects size from the previous frame (for this cascade) and preallocate it here once RendererAllocation is used
            cascade.RasterizeObjects.Clear();
            cascade.PendingSDFTextures.Clear();

            // Check if cascade center has been moved
            if (!(useCache && Float3::NearEqual(cascade.Position, center, cascadeVoxelSize)))
            {
                // TODO: optimize for moving camera (use chunkCoords scrolling)
                cascade.StaticChunks.Clear();
            }

            // Setup cascade info
            cascade.Position = center;
            cascade.VoxelSize = cascadeVoxelSize;
            cascade.Extent = cascadeExtent;
            cascade.ChunkSize = cascadeChunkSize;
            cascade.MaxDistanceTex = cascadeChunkSize * 1.5f; // Encodes SDF distance to [-maxDst; +maxDst] to be packed as normalized value, limits the max SDF trace step distance
            cascade.MaxDistanceMip = cascade.MaxDistanceTex * 2.0f; // Encode mip distance with less but covers larger area for faster jumps during tracing
            cascade.MaxDistanceTex = Math::Min(cascade.MaxDistanceTex, cascadeSize);
            cascade.MaxDistanceMip = Math::Min(cascade.MaxDistanceMip, cascadeSize);
            cascade.Bounds = cascadeBounds;
            cascade.RasterizeBounds = cascadeBounds;
            cascade.RasterizeBounds.Minimum += 0.1f; // Adjust to prevent overflowing chunk keys (cascade bounds are used for clamping object bounds)
            cascade.RasterizeBounds.Maximum -= 0.1f; // Adjust to prevent overflowing chunk keys (cascade bounds are used for clamping object bounds)
            cascade.CullingBounds = cascadeBounds.MakeOffsetted(Origin);
            const float objectMargin = cascadeVoxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
            cascade.OriginMin = -Origin - objectMargin;
            cascade.OriginMax = -Origin + objectMargin;
        }
        if (enableAsync)
        {
            // Draw all dirty cascades in async (separate job for each cascade)
            Function<void(int32)> func;
            func.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::DrawCascadeJob>(this);
            AsyncDrawWaitLabels.Add(JobSystem::Dispatch(func, cascadesCount));
        }
        else
        {
            // Synchronized drawing in sequence
            for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
                DrawCascadeJob(cascadeIndex);
        }
    }

    void WaitForDrawing()
    {
        for (int64 label : AsyncDrawWaitLabels)
            JobSystem::Wait(label);
        AsyncDrawWaitLabels.Clear();
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

    void OnSceneRenderingUpdateActor(Actor* a, const BoundingSphere& prevBounds, UpdateFlags flags) override
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
    GlobalSignDistanceFieldCustomBuffer* Current = nullptr;
    ThreadLocal<CascadeData*, 16> CurrentCascade;
}

void GlobalSignDistanceFieldCustomBuffer::DrawCascadeActors(const CascadeData& cascade)
{
    PROFILE_CPU();
    const BoundingBox cullingBounds = cascade.CullingBounds;
    const uint32 viewMask = AsyncRenderContext.View.RenderLayersMask;
    // TODO: add scene detail scale factor to PostFx settings (eg. to increase or decrease scene details and quality)
    const float minObjectRadius = Math::Max(20.0f, cascade.VoxelSize * 2.0f); // Skip too small objects for this cascade
    int32 actorsDrawn = 0;
    SceneRendering::DrawCategory drawCategories[] = { SceneRendering::SceneDraw, SceneRendering::SceneDrawAsync };
    for (auto* scene : AsyncRenderContext.List->Scenes)
    {
        for (SceneRendering::DrawCategory drawCategory : drawCategories)
        {
            auto& list = scene->Actors[drawCategory];
            for (const auto& e : list)
            {
                if (e.Bounds.Radius >= minObjectRadius && viewMask & e.LayerMask && CollisionsHelper::BoxIntersectsSphere(cullingBounds, e.Bounds))
                {
                    //PROFILE_CPU_ACTOR(e.Actor);
                    e.Actor->Draw(AsyncRenderContext);
#if COMPILE_WITH_PROFILER
                    actorsDrawn++;
#endif
                }
            }
        }
    }
    ZoneValue(actorsDrawn);
}

void GlobalSignDistanceFieldCustomBuffer::UpdateCascadeChunks(CascadeData& cascade)
{
    PROFILE_CPU();

    // Update static chunks
    for (auto it = cascade.Chunks.Begin(); it.IsNotEnd(); ++it)
    {
        auto& e = *it;
        if (e.Key.Layer != 0)
            continue;
        if (e.Value.Dynamic)
        {
            // Remove static chunk if it contains any dynamic object
            cascade.StaticChunks.Remove(e.Key);
        }
        else if (cascade.StaticChunks.Contains(e.Key))
        {
            // Remove chunk from update since it's static
            auto key = e.Key;
            while (cascade.Chunks.Remove(key))
                key.NextLayer();
        }
        else
        {
            // Add to static cache (render now but skip next frame)
            cascade.StaticChunks.Add(e.Key);
        }
    }
}

void GlobalSignDistanceFieldCustomBuffer::WriteCascadeObjects(CascadeData& cascade)
{
    PROFILE_CPU();

    // Write all objects to the buffer
    int32 objectsBufferCount = 0;
    cascade.ObjectsData.Clear();
    cascade.ObjectsTextures.Clear();
    cascade.ObjectIndexToDataIndex.Clear();
    for (const auto& e : cascade.Chunks)
    {
        auto& chunk = e.Value;
        for (int32 i = 0; i < chunk.ModelsCount; i++)
        {
            auto objectIndex = chunk.Models[i];
            if (cascade.ObjectIndexToDataIndex.ContainsKey(objectIndex))
                continue;
            const auto& object = cascade.RasterizeObjects.Get()[objectIndex];

            // Pick the SDF mip for the cascade
            int32 mipLevelIndex = 1;
            float worldUnitsPerVoxel = object.SDF->WorldUnitsPerVoxel * object.LocalToWorld.Scale.MaxValue() * 4;
            const int32 mipLevels = object.SDF->Texture->MipLevels();
            while (cascade.VoxelSize > worldUnitsPerVoxel && mipLevelIndex < mipLevels)
            {
                mipLevelIndex++;
                worldUnitsPerVoxel *= 2.0f;
            }
            mipLevelIndex--;

            // Add object data for the GPU buffer
            uint16 dataIndex = objectsBufferCount++;
            ObjectRasterizeData objectData;
            Platform::MemoryClear(&objectData, sizeof(objectData));
            Matrix localToWorld, worldToLocal, volumeToWorld;
            Matrix::Transformation(object.LocalToWorld.Scale, object.LocalToWorld.Orientation, object.LocalToWorld.Translation - Origin, localToWorld);
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
            cascade.ObjectsData.Add((const byte*)&objectData, sizeof(objectData));
            cascade.ObjectsTextures.Add(object.SDF->Texture->ViewVolume());
            cascade.PendingObjectTypes.Add(object.Actor->GetTypeHandle());
            cascade.ObjectIndexToDataIndex.Add(objectIndex, dataIndex);
        }
        for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
        {
            auto objectIndex = chunk.Heightfields[i];
            if (cascade.ObjectIndexToDataIndex.ContainsKey(objectIndex))
                continue;
            const auto& object = cascade.RasterizeObjects.Get()[objectIndex];

            // Add object data for the GPU buffer
            uint16 dataIndex = objectsBufferCount++;
            ObjectRasterizeData objectData;
            Platform::MemoryClear(&objectData, sizeof(objectData));
            Matrix localToWorld, worldToLocal;
            Matrix::Transformation(object.LocalToWorld.Scale, object.LocalToWorld.Orientation, object.LocalToWorld.Translation - Origin, localToWorld);
            Matrix::Invert(localToWorld, worldToLocal);
            objectData.WorldToVolume.SetMatrixTranspose(worldToLocal);
            objectData.VolumeToWorld.SetMatrixTranspose(localToWorld);
            objectData.VolumeToUVWMul = Float3(object.LocalToUV.X, 1.0f, object.LocalToUV.Y);
            objectData.VolumeToUVWAdd = Float3(object.LocalToUV.Z, 0.0f, object.LocalToUV.W);
            objectData.MipOffset = (float)cascade.Index * 0.5f; // Use lower-quality mip for far cascades
            cascade.ObjectsData.Add((const byte*)&objectData, sizeof(objectData));
            cascade.ObjectsTextures.Add(object.Heightfield->View());
            cascade.PendingObjectTypes.Add(object.Actor->GetTypeHandle());
            cascade.ObjectIndexToDataIndex.Add(objectIndex, dataIndex);
        }
    }
}

void GlobalSignDistanceFieldCustomBuffer::DrawCascadeJob(int32 cascadeIndex)
{
    auto& cascade = Cascades[cascadeIndex];
    if (!cascade.Dirty)
        return;
    PROFILE_CPU();
    CurrentCascade.Set(&cascade);
    DrawCascadeActors(cascade);
    UpdateCascadeChunks(cascade);
    WriteCascadeObjects(cascade);
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
        _objectsBuffer = New<DynamicStructuredBuffer>(0, (uint32)sizeof(ObjectRasterizeData), false, TEXT("GlobalSDF.ObjectsBuffer"));

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
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _shader = nullptr;
}

void GlobalSignDistanceFieldPass::OnCollectDrawCalls(RenderContextBatch& renderContextBatch)
{
    // Check if Global SDF will be used this frame
    PROFILE_CPU_NAMED("Global SDF");
    if (checkIfSkipPass())
        return;
    RenderContext& renderContext = renderContextBatch.GetMainContext();
    if (renderContext.List->Scenes.Count() == 0)
        return;
    auto& sdfData = *renderContext.Buffers->GetCustomBuffer<GlobalSignDistanceFieldCustomBuffer>(TEXT("GlobalSignDistanceField"));
    Current = &sdfData;
    sdfData.StartDrawing(renderContext, renderContextBatch.EnableAsync);
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
    int32 resolution, cascadesCount, resolutionMip;
    float distance;
    sdfData.GetOptions(renderContext, resolution, cascadesCount, resolutionMip, distance);
    const float distanceExtent = distance / sdfData.CascadesDistanceScales[cascadesCount - 1];

    // Initialize buffers
    bool reset = false;
    if (sdfData.Cascades.Count() != cascadesCount || sdfData.Resolution != resolution)
    {
        sdfData.Cascades.Resize(cascadesCount);
        sdfData.Resolution = resolution;
        sdfData.FrameIndex = 0;
        reset = true;
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
        LOG(Info, "Global SDF memory usage: {0} MB", memoryUsage / (1024 * 1024));
    }
    if (sdfData.Origin != renderContext.View.Origin)
    {
        sdfData.Origin = renderContext.View.Origin;
        reset = true;
    }
    GPUTexture* tmpMip = nullptr;
    if (reset)
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

    // Ensure that async objects drawing ended
    Current = &sdfData;
    sdfData.StartDrawing(renderContext, false, reset); // (ignored if not started earlier this frame)
    sdfData.WaitForDrawing();

    // Rasterize world geometry into Global SDF
    bool anyDraw = false;
    GPUTextureView* textureView = sdfData.Texture->ViewVolume();
    GPUTextureView* textureMipView = sdfData.TextureMip->ViewVolume();
    for (int32 cascadeIndex = 0; cascadeIndex < cascadesCount; cascadeIndex++)
    {
        auto& cascade = sdfData.Cascades[cascadeIndex];
        if (!cascade.Dirty)
            continue;

        // Process all pending SDF textures tracking
        for (auto& e : cascade.PendingSDFTextures)
        {
            GPUTexture* texture = e.Item;
            if (Current->SDFTextures.Add(texture))
            {
                texture->Deleted.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureDeleted>(Current);
                texture->ResidentMipsChanged.Bind<GlobalSignDistanceFieldCustomBuffer, &GlobalSignDistanceFieldCustomBuffer::OnSDFTextureResidentMipsChanged>(Current);
            }
        }
        cascade.PendingSDFTextures.Clear();

        // Process all pending object types tracking
        for (auto& e : cascade.PendingObjectTypes)
            sdfData.ObjectTypes.Add(e.Item);

        // Perform batched chunks rasterization
        anyDraw = true;
        context->ResetSR();
        ModelsRasterizeData data;
        data.CascadeCoordToPosMul = (Float3)cascade.Bounds.GetSize() / (float)resolution;
        data.CascadeCoordToPosAdd = (Float3)cascade.Bounds.Minimum + cascade.VoxelSize * 0.5f;
        data.MaxDistance = cascade.MaxDistanceTex;
        data.CascadeResolution = resolution;
        data.CascadeMipResolution = resolutionMip;
        data.CascadeIndex = cascadeIndex;
        data.CascadeMipFactor = GLOBAL_SDF_RASTERIZE_MIP_FACTOR;
        data.CascadeVoxelSize = cascade.VoxelSize;
        context->BindUA(0, textureView);
        context->BindCB(1, _cb1);
        constexpr int32 chunkDispatchGroups = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / GLOBAL_SDF_RASTERIZE_GROUP_SIZE;
        int32 chunkDispatches = 0;
        if (!reset)
        {
            PROFILE_GPU_CPU_NAMED("Clear Chunks");
            for (auto it = cascade.NonEmptyChunks.Begin(); it.IsNotEnd(); ++it)
            {
                auto& key = it->Item;
                if (cascade.Chunks.ContainsKey(key) || cascade.StaticChunks.Contains(key))
                    continue;

                // Clear empty chunk
                cascade.NonEmptyChunks.Remove(it);
                data.ChunkCoord = key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                context->UpdateCB(_cb1, &data);
                context->Dispatch(_csClearChunk, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                chunkDispatches++;
                // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches
            }
            ZoneValue(chunkDispatches);
        }
        {
            PROFILE_GPU_CPU_NAMED("Rasterize Chunks");

            // Send models data to the GPU
            const auto& objectIndexToDataIndex = cascade.ObjectIndexToDataIndex;
            GPUTextureView** objectsTextures = cascade.ObjectsTextures.Get();
            if (cascade.Chunks.Count() != 0)
            {
                // Flush buffer but don't allocate any CPU memory by swapping Data pointer with the cascade ObjectsData
                PROFILE_CPU_NAMED("Update Objects");
                _objectsBuffer->Data.Swap(cascade.ObjectsData);
                _objectsBuffer->Flush(context);
                _objectsBuffer->Data.Swap(cascade.ObjectsData);
            }
            context->BindSR(0, _objectsBuffer->GetBuffer() ? _objectsBuffer->GetBuffer()->View() : nullptr);

            // Rasterize non-empty chunks (first layer so can override existing chunk data)
            for (const auto& e : cascade.Chunks)
            {
                if (e.Key.Layer != 0)
                    continue;
                auto& chunk = e.Value;
                cascade.NonEmptyChunks.Add(e.Key);

                for (int32 i = 0; i < chunk.ModelsCount; i++)
                {
                    auto objectIndex = objectIndexToDataIndex.At(chunk.Models[i]);
                    data.Objects[i] = objectIndex;
                    context->BindSR(i + 1, objectsTextures[objectIndex]);
                }
                for (int32 i = chunk.ModelsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                    context->UnBindSR(i + 1);
                data.ChunkCoord = e.Key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                data.ObjectsCount = chunk.ModelsCount;
                context->UpdateCB(_cb1, &data);
                auto cs = data.ObjectsCount != 0 ? _csRasterizeModel0 : _csClearChunk; // Terrain-only chunk can be quickly cleared
                context->Dispatch(cs, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                chunkDispatches++;
                // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches (maybe cache per-shader write/read flags for all UAVs?)

                if (chunk.HeightfieldsCount != 0)
                {
                    // Inject heightfield (additive)
                    for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
                    {
                        auto objectIndex = objectIndexToDataIndex.At(chunk.Heightfields[i]);
                        data.Objects[i] = objectIndex;
                        context->BindSR(i + 1, objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.HeightfieldsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.HeightfieldsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeHeightfield, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                    chunkDispatches++;
                }

#if GLOBAL_SDF_DEBUG_CHUNKS
                // Debug draw chunk bounds in world space with number of models in it
                if (cascadeIndex + 1 == GLOBAL_SDF_DEBUG_CHUNKS)
                {
                    int32 count = chunk.ModelsCount + chunk.HeightfieldsCount;
                    RasterizeChunkKey tmp = e.Key;
                    tmp.NextLayer();
                    while (cascade.Chunks.ContainsKey(tmp))
                    {
                        count += cascade.Chunks[tmp].ModelsCount + cascade.Chunks[tmp].HeightfieldsCount;
                        tmp.NextLayer();
                    }
                    Float3 chunkMin = cascade.Bounds.Minimum + Float3(e.Key.Coord) * cascade.ChunkSize;
                    BoundingBox chunkBounds(chunkMin, chunkMin + cascade.ChunkSize);
                    DebugDraw::DrawWireBox(chunkBounds, Color::Red, 0, false);
                    DebugDraw::DrawText(StringUtils::ToString(count), chunkBounds.GetCenter(), Color::Red);
                }
#endif
            }

            // Rasterize non-empty chunks (additive layers so need combine with existing chunk data)
            for (const auto& e : cascade.Chunks)
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
                        context->BindSR(i + 1, objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.ModelsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.ModelsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeModel1, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                    chunkDispatches++;
                }

                if (chunk.HeightfieldsCount != 0)
                {
                    // Inject heightfields (additive)
                    for (int32 i = 0; i < chunk.HeightfieldsCount; i++)
                    {
                        auto objectIndex = objectIndexToDataIndex.At(chunk.Heightfields[i]);
                        data.Objects[i] = objectIndex;
                        context->BindSR(i + 1, objectsTextures[objectIndex]);
                    }
                    for (int32 i = chunk.HeightfieldsCount; i < GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT; i++)
                        context->UnBindSR(i + 1);
                    data.ObjectsCount = chunk.HeightfieldsCount;
                    context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csRasterizeHeightfield, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                    chunkDispatches++;
                }
            }

            ZoneValue(chunkDispatches);
        }

        // Generate mip out of cascade (empty chunks have distance value 1 which is incorrect so mip will be used as a fallback - lower res)
        if (reset || chunkDispatches != 0)
        {
            PROFILE_GPU_CPU_NAMED("Generate Mip");
            context->ResetUA();
            const int32 mipDispatchGroups = Math::DivideAndRoundUp(resolutionMip, GLOBAL_SDF_MIP_GROUP_SIZE);
            static_assert((GLOBAL_SDF_MIP_FLOODS % 2) == 1, "Invalid Global SDF mip flood iterations count.");
            int32 floodFillIterations = cascade.Chunks.Count() == 0 ? 1 : GLOBAL_SDF_MIP_FLOODS;
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
            data.MipMaxDistanceLoad = cascade.MaxDistanceTex; // Decode tex distance within chunk (more precision, for detailed tracing nearby geometry)
            data.MipMaxDistanceStore = cascade.MaxDistanceMip; // Encode mip distance within whole volume (less precision, for fast jumps over empty spaces)
            data.MipTexResolution = data.CascadeResolution;
            data.MipCoordScale = data.CascadeMipFactor;
            data.MipTexOffsetX = data.CascadeIndex * data.CascadeResolution;
            data.MipMipOffsetX = data.CascadeIndex * data.CascadeMipResolution;
            context->UpdateCB(_cb1, &data);
            context->BindSR(0, textureView);
            context->BindUA(0, textureMipView);
            context->Dispatch(_csGenerateMip, mipDispatchGroups, mipDispatchGroups, mipDispatchGroups);

            data.MipTexResolution = data.CascadeMipResolution;
            data.MipCoordScale = 1;
            data.MipMaxDistanceLoad = data.MipMaxDistanceStore;
            for (int32 i = 1; i < floodFillIterations; i++)
            {
                context->ResetUA();
                if ((i & 1) == 1)
                {
                    // Mip -> Tmp
                    context->BindSR(0, textureMipView);
                    context->BindUA(0, tmpMipView);
                    data.MipTexOffsetX = data.CascadeIndex * data.CascadeMipResolution;
                    data.MipMipOffsetX = 0;
                }
                else
                {
                    // Tmp -> Mip
                    context->BindSR(0, tmpMipView);
                    context->BindUA(0, textureMipView);
                    data.MipTexOffsetX = 0;
                    data.MipMipOffsetX = data.CascadeIndex * data.CascadeMipResolution;
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
        const float cascadeExtent = distanceExtent * sdfData.CascadesDistanceScales[cascadeIndex];
        result.Constants.CascadePosDistance[cascadeIndex] = Vector4(cascade.Position, cascadeExtent);
        result.Constants.CascadeVoxelSize.Raw[cascadeIndex] = cascade.VoxelSize;
        result.Constants.CascadeMaxDistance.Raw[cascadeIndex] = cascade.MaxDistanceTex;
        result.Constants.CascadeMaxDistanceMip.Raw[cascadeIndex] = cascade.MaxDistanceMip;
    }
    for (int32 cascadeIndex = cascadesCount; cascadeIndex < 4; cascadeIndex++)
    {
        result.Constants.CascadePosDistance[cascadeIndex] = result.Constants.CascadePosDistance[cascadesCount - 1];
        result.Constants.CascadeVoxelSize.Raw[cascadeIndex] = result.Constants.CascadeVoxelSize.Raw[cascadesCount - 1];
        result.Constants.CascadeMaxDistance.Raw[cascadeIndex] = result.Constants.CascadeMaxDistance.Raw[cascadesCount - 1];
    }
    result.Constants.Resolution = (float)resolution;
    result.Constants.CascadesCount = cascadesCount;
    result.Constants.Padding = Float2::Zero;
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

void GlobalSignDistanceFieldPass::GetCullingData(BoundingBox& bounds) const
{
    auto& cascade = *CurrentCascade.Get();
    bounds = cascade.CullingBounds;
}

void GlobalSignDistanceFieldPass::RasterizeModelSDF(Actor* actor, const ModelBase::SDFData& sdf, const Transform& localToWorld, const BoundingBox& objectBounds)
{
    if (!sdf.Texture)
        return;
    auto& cascade = *CurrentCascade.Get();
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    const int32 residentMipLevels = sdf.Texture->ResidentMipLevels();
    if (residentMipLevels != 0)
    {
        // Setup object data
        BoundingBox objectBoundsCascade;
        Vector3::Clamp(objectBounds.Minimum + cascade.OriginMin, cascade.RasterizeBounds.Minimum, cascade.RasterizeBounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, cascade.RasterizeBounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + cascade.OriginMax, cascade.RasterizeBounds.Minimum, cascade.RasterizeBounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, cascade.RasterizeBounds.Minimum, objectBoundsCascade.Maximum);
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / cascade.ChunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / cascade.ChunkSize);

        // Add object data
        const uint16 dataIndex = cascade.RasterizeObjects.Count();
        auto& data = cascade.RasterizeObjects.AddOne();
        data.Actor = actor;
        data.SDF = &sdf;
        data.LocalToWorld = localToWorld;
        data.ObjectBounds = objectBounds;

        // Inject object into the intersecting cascade chunks
        RasterizeChunkKey key;
        auto& chunks = cascade.Chunks;
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
    if (!dynamic && residentMipLevels != sdf.Texture->MipLevels() && !Current->SDFTextures.Contains(sdf.Texture))
    {
        cascade.PendingSDFTextures.Add(sdf.Texture);
    }
}

void GlobalSignDistanceFieldPass::RasterizeHeightfield(Actor* actor, GPUTexture* heightfield, const Transform& localToWorld, const BoundingBox& objectBounds, const Float4& localToUV)
{
    if (!heightfield)
        return;
    auto& cascade = *CurrentCascade.Get();
    const bool dynamic = !GLOBAL_SDF_ACTOR_IS_STATIC(actor);
    const int32 residentMipLevels = heightfield->ResidentMipLevels();
    if (residentMipLevels != 0)
    {
        // Setup object data
        BoundingBox objectBoundsCascade;
        Vector3::Clamp(objectBounds.Minimum + cascade.OriginMin, cascade.RasterizeBounds.Minimum, cascade.RasterizeBounds.Maximum, objectBoundsCascade.Minimum);
        Vector3::Subtract(objectBoundsCascade.Minimum, cascade.RasterizeBounds.Minimum, objectBoundsCascade.Minimum);
        Vector3::Clamp(objectBounds.Maximum + cascade.OriginMax, cascade.RasterizeBounds.Minimum, cascade.RasterizeBounds.Maximum, objectBoundsCascade.Maximum);
        Vector3::Subtract(objectBoundsCascade.Maximum, cascade.RasterizeBounds.Minimum, objectBoundsCascade.Maximum);
        const Int3 objectChunkMin(objectBoundsCascade.Minimum / cascade.ChunkSize);
        const Int3 objectChunkMax(objectBoundsCascade.Maximum / cascade.ChunkSize);

        // Add object data
        const uint16 dataIndex = cascade.RasterizeObjects.Count();
        auto& data = cascade.RasterizeObjects.AddOne();
        data.Actor = actor;
        data.Heightfield = heightfield;
        data.LocalToWorld = localToWorld;
        data.ObjectBounds = objectBounds;
        data.LocalToUV = localToUV;

        // Inject object into the intersecting cascade chunks
        RasterizeChunkKey key;
        auto& chunks = cascade.Chunks;
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
    if (!dynamic && residentMipLevels != heightfield->MipLevels() && !Current->SDFTextures.Contains(heightfield))
    {
        cascade.PendingSDFTextures.Add(heightfield);
    }
}
