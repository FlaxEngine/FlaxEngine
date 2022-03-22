// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GlobalSignDistanceFieldPass.h"
#include "GBufferPass.h"
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
#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4
#define GLOBAL_SDF_MIP_GROUP_SIZE 4
#define GLOBAL_SDF_MIP_FLOODS 5
#define GLOBAL_SDF_DEBUG_CHUNKS 0

static_assert(GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT % 4 == 0, "Must be multiple of 4 due to data packing for GPU constant buffer.");
#if GLOBAL_SDF_DEBUG_CHUNKS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct ModelRasterizeData
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
    int ModelsCount;
    Vector3 CascadeCoordToPosAdd;
    int32 CascadeResolution;
    Vector2 Padding0;
    int32 CascadeMipResolution;
    int32 CascadeMipFactor;
    uint32 Models[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
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
    int32 ModelsCount = 0;
    int32 Models[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT];
};

constexpr int32 RasterizeChunkKeyHashResolution = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;

struct RasterizeChunkKey
{
    uint32 Hash;
    int32 Layer;
    Int3 Coord;

    friend bool operator==(const RasterizeChunkKey& a, const RasterizeChunkKey& b)
    {
        return a.Hash == b.Hash && a.Coord == b.Coord && a.Layer == b.Layer;
    }
};

uint32 GetHash(const RasterizeChunkKey& key)
{
    return key.Hash;
}

class GlobalSignDistanceFieldCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    GPUTexture* Cascades[4] = {};
    GPUTexture* CascadeMips[4] = {};
    Vector3 Positions[4];
    HashSet<RasterizeChunkKey> NonEmptyChunks[4];

    ~GlobalSignDistanceFieldCustomBuffer()
    {
        for (GPUTexture* cascade : Cascades)
            RenderTargetPool::Release(cascade);
        for (GPUTexture* mip : CascadeMips)
            RenderTargetPool::Release(mip);
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
    auto device = GPUDevice::Instance;
    if (device->GetFeatureLevel() < FeatureLevel::SM5 || !device->Limits.HasCompute || !device->Limits.HasTypedUAVLoad)
        return false;
    if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(device->GetFormatFeatures(GLOBAL_SDF_FORMAT).Support, FormatSupport::ShaderSample | FormatSupport::Texture3D))
        return false;

    // Create pipeline states
    _psDebug = device->CreatePipelineState();

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GlobalSignDistanceField"));
    if (_shader == nullptr)
        return false;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<GlobalSignDistanceFieldPass, &GlobalSignDistanceFieldPass::OnShaderReloading>(this);
#endif

    // Init buffer
    _modelsBuffer = New<DynamicStructuredBuffer>(64u * (uint32)sizeof(ModelRasterizeData), (uint32)sizeof(ModelRasterizeData), false, TEXT("GlobalSDF.ModelsBuffer"));

    return false;
}

bool GlobalSignDistanceFieldPass::setupResources()
{
    // Check shader
    if (!_shader || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();
    _cb0 = shader->GetCB(0);
    _cb1 = shader->GetCB(1);
    _csRasterizeModel0 = shader->GetCS("CS_RasterizeModel", 0);
    _csRasterizeModel1 = shader->GetCS("CS_RasterizeModel", 1);
    _csClearChunk = shader->GetCS("CS_ClearChunk");
    _csGenerateMip0 = shader->GetCS("CS_GenerateMip", 0);
    _csGenerateMip1 = shader->GetCS("CS_GenerateMip", 1);

    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_Debug");
        if (_psDebug->Init(psDesc))
            return true;
    }

    return false;
}

#if USE_EDITOR

void GlobalSignDistanceFieldPass::OnShaderReloading(Asset* obj)
{
    _psDebug->ReleaseGPU();
    _csRasterizeModel0 = nullptr;
    _csRasterizeModel1 = nullptr;
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
    SAFE_DELETE(_modelsBuffer);
    _modelsTextures.Resize(0);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _shader = nullptr;
    ChunksCache.Clear();
    ChunksCache.SetCapacity(0);
}

bool GlobalSignDistanceFieldPass::Render(RenderContext& renderContext, GPUContext* context, BindingData& result)
{
    // Skip if not supported
    if (setupResources())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& sdfData = *renderContext.Buffers->GetCustomBuffer<GlobalSignDistanceFieldCustomBuffer>(TEXT("GlobalSignDistanceField"));

    // TODO: configurable via graphics settings
    const int32 resolution = 256;
    const int32 mipFactor = 4;
    const int32 resolutionMip = Math::DivideAndRoundUp(resolution, mipFactor);
    // TODO: configurable via postFx settings
    const float distanceExtent = 2500.0f;
    const float cascadesDistances[] = { distanceExtent, distanceExtent * 2.0f, distanceExtent * 4.0f, distanceExtent * 8.0f };

    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (sdfData.LastFrameUsed != currentFrame)
    {
        PROFILE_GPU_CPU("Global SDF");

        // Initialize buffers
        sdfData.LastFrameUsed = currentFrame;
        auto desc = GPUTextureDescription::New3D(resolution, resolution, resolution, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
        bool updated = false;
        for (GPUTexture*& cascade : sdfData.Cascades)
        {
            if (cascade && cascade->Width() != desc.Width)
            {
                RenderTargetPool::Release(cascade);
                cascade = nullptr;
            }
            if (!cascade)
            {
                cascade = RenderTargetPool::Get(desc);
                if (!cascade)
                    return true;
                updated = true;
                PROFILE_GPU_CPU("Init");
                context->ClearUA(cascade, Vector4::One);
            }
        }
        desc = GPUTextureDescription::New3D(resolutionMip, resolutionMip, resolutionMip, GLOBAL_SDF_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess, 1);
        for (GPUTexture*& cascadeMip : sdfData.CascadeMips)
        {
            if (cascadeMip && cascadeMip->Width() != desc.Width)
            {
                RenderTargetPool::Release(cascadeMip);
                cascadeMip = nullptr;
            }
            if (!cascadeMip)
            {
                cascadeMip = RenderTargetPool::Get(desc);
                if (!cascadeMip)
                    return true;
                updated = true;
            }
        }
        GPUTexture* tmpMip = nullptr;
        if (updated)
            LOG(Info, "Global SDF memory usage: {0} MB", (sdfData.Cascades[0]->GetMemoryUsage() + sdfData.CascadeMips[0]->GetMemoryUsage()) * ARRAY_COUNT(sdfData.Cascades) / 1024 / 1024);

        // Rasterize world geometry into Global SDF
        renderContext.View.Pass = DrawPass::GlobalSDF;
        uint32 viewMask = renderContext.View.RenderLayersMask;
        const bool useCache = !updated && !renderContext.Task->IsCameraCut;
        static_assert(GLOBAL_SDF_RASTERIZE_CHUNK_SIZE % GLOBAL_SDF_RASTERIZE_GROUP_SIZE == 0, "Invalid chunk size for Global SDF rasterization group size.");
        const int32 rasterizeChunks = Math::CeilToInt((float)resolution / (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE);
        auto& chunks = ChunksCache;
        chunks.EnsureCapacity(rasterizeChunks * rasterizeChunks, false);
        bool anyDraw = false;
        const uint64 cascadeFrequencies[] = { 2, 3, 5, 11 };
        //const uint64 cascadeFrequencies[] = { 1, 1, 1, 1 };
        for (int32 cascade = 0; cascade < 4; cascade++)
        {
            // Reduce frequency of the updates
            if (useCache && (Engine::FrameCount % cascadeFrequencies[cascade]) != 0)
                continue;
            const float distance = cascadesDistances[cascade];
            const float maxDistance = distance * 2;
            const float voxelSize = maxDistance / resolution;
            const float snapping = voxelSize * mipFactor;
            const Vector3 center = Vector3::Floor(renderContext.View.Position / snapping) * snapping;
            // TODO: cascade scrolling on movement to reduce dirty chunks?
            //const Vector3 center = Vector3::Zero;
            sdfData.Positions[cascade] = center;
            BoundingBox cascadeBounds(center - distance, center + distance);
            // TODO: add scene detail scale factor to PostFx settings (eg. to increase or decrease scene details and quality)
            const float minObjectRadius = Math::Max(20.0f, voxelSize * 0.5f); // Skip too small objects for this cascade
            const float chunkSize = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
            GPUTextureView* cascadeView = sdfData.Cascades[cascade]->ViewVolume();
            GPUTextureView* cascadeMipView = sdfData.CascadeMips[cascade]->ViewVolume();

            // Clear cascade before rasterization
            {
                PROFILE_CPU_NAMED("Clear");
                chunks.Clear();
                _modelsBuffer->Clear();
                _modelsTextures.Clear();
            }
            int32 modelsBufferCount = 0;

            // Draw all objects from all scenes into the cascade
            for (auto* scene : renderContext.List->Scenes)
            {
                // TODO: optimize for moving camera (copy sdf)
                // TODO: if chunk is made of static objects only then mark it as static and skip from rendering during the next frame (will need to track objects dirty state in the SceneRendering)
                for (auto& e : scene->Actors)
                {
                    if (viewMask & e.LayerMask && e.Bounds.Radius >= minObjectRadius && CollisionsHelper::BoxIntersectsSphere(cascadeBounds, e.Bounds))
                    {
                        e.Actor->Draw(renderContext);

                        // TODO: move to be properly implemented per object type
                        auto staticModel = ScriptingObject::Cast<StaticModel>(e.Actor);
                        if (staticModel && staticModel->Model && staticModel->Model->IsLoaded() && staticModel->Model->CanBeRendered() && staticModel->DrawModes & DrawPass::GlobalSDF)
                        {
                            // okay so firstly we need SDF for this model
                            // TODO: implement SDF generation on model import
                            if (!staticModel->Model->SDF.Texture)
                                staticModel->Model->GenerateSDF();
                            ModelBase::SDFData& sdf = staticModel->Model->SDF;
                            if (sdf.Texture)
                            {
                                // Setup object data
                                BoundingBox objectBounds = staticModel->GetBox();
                                BoundingBox objectBoundsCascade;
                                const float objectMargin = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN;
                                Vector3::Clamp(objectBounds.Minimum - objectMargin, cascadeBounds.Minimum, cascadeBounds.Maximum, objectBoundsCascade.Minimum);
                                Vector3::Subtract(objectBoundsCascade.Minimum, cascadeBounds.Minimum, objectBoundsCascade.Minimum);
                                Vector3::Clamp(objectBounds.Maximum + objectMargin, cascadeBounds.Minimum, cascadeBounds.Maximum, objectBoundsCascade.Maximum);
                                Vector3::Subtract(objectBoundsCascade.Maximum, cascadeBounds.Minimum, objectBoundsCascade.Maximum);
                                Int3 objectChunkMin(objectBoundsCascade.Minimum / chunkSize);
                                Int3 objectChunkMax(objectBoundsCascade.Maximum / chunkSize);
                                Matrix localToWorld, worldToLocal, volumeToWorld;
                                staticModel->GetWorld(&localToWorld);
                                Matrix::Invert(localToWorld, worldToLocal);
                                BoundingBox localVolumeBounds(sdf.LocalBoundsMin, sdf.LocalBoundsMax);
                                Vector3 volumeLocalBoundsExtent = localVolumeBounds.GetSize() * 0.5f;
                                Matrix worldToVolume = worldToLocal * Matrix::Translation(-(localVolumeBounds.Minimum + volumeLocalBoundsExtent));
                                Matrix::Invert(worldToVolume, volumeToWorld);

                                // Pick the SDF mip for the cascade
                                int32 mipLevelIndex = 1;
                                float worldUnitsPerVoxel = sdf.WorldUnitsPerVoxel * localToWorld.GetScaleVector().MaxValue() * 2;
                                while (voxelSize > worldUnitsPerVoxel && mipLevelIndex < sdf.Texture->MipLevels())
                                {
                                    mipLevelIndex++;
                                    worldUnitsPerVoxel *= 2.0f;
                                }
                                mipLevelIndex--;

                                // Volume -> Local -> UVW
                                Vector3 volumeToUVWMul = sdf.LocalToUVWMul;
                                Vector3 volumeToUVWAdd = sdf.LocalToUVWAdd + (localVolumeBounds.Minimum + volumeLocalBoundsExtent) * sdf.LocalToUVWMul;

                                // Add model data for the GPU buffer
                                int32 modelIndex = modelsBufferCount++;
                                ModelRasterizeData modelData;
                                Matrix::Transpose(worldToVolume, modelData.WorldToVolume);
                                Matrix::Transpose(volumeToWorld, modelData.VolumeToWorld);
                                modelData.VolumeLocalBoundsExtent = volumeLocalBoundsExtent;
                                modelData.VolumeToUVWMul = volumeToUVWMul;
                                modelData.VolumeToUVWAdd = volumeToUVWAdd;
                                modelData.MipOffset = (float)mipLevelIndex;
                                modelData.DecodeMul = 2.0f * sdf.MaxDistance;
                                modelData.DecodeAdd = -sdf.MaxDistance;
                                _modelsBuffer->Write(modelData);
                                _modelsTextures.Add(sdf.Texture->ViewVolume());

                                // Inject object into the intersecting cascade chunks
                                RasterizeChunkKey key;
                                for (key.Coord.Z = objectChunkMin.Z; key.Coord.Z <= objectChunkMax.Z; key.Coord.Z++)
                                {
                                    for (key.Coord.Y = objectChunkMin.Y; key.Coord.Y <= objectChunkMax.Y; key.Coord.Y++)
                                    {
                                        for (key.Coord.X = objectChunkMin.X; key.Coord.X <= objectChunkMax.X; key.Coord.X++)
                                        {
                                            key.Layer = 0;
                                            key.Hash = key.Coord.Z * (RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution) + key.Coord.Y * RasterizeChunkKeyHashResolution + key.Coord.X;
                                            RasterizeChunk* chunk = &chunks[key];

                                            // Move to the next layer if chunk has overflown
                                            while (chunk->ModelsCount == GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT)
                                            {
                                                key.Layer++;
                                                key.Hash += RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution * RasterizeChunkKeyHashResolution;
                                                chunk = &chunks[key];
                                            }

                                            chunk->Models[chunk->ModelsCount++] = modelIndex;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Send models data to the GPU
            {
                PROFILE_GPU_CPU("Update Models");
                _modelsBuffer->Flush(context);
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
            data.CascadeMipFactor = mipFactor;
            context->BindUA(0, cascadeView);
            context->BindSR(0, _modelsBuffer->GetBuffer() ? _modelsBuffer->GetBuffer()->View() : nullptr);
            if (_cb1)
                context->BindCB(1, _cb1);
            const int32 chunkDispatchGroups = GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / GLOBAL_SDF_RASTERIZE_GROUP_SIZE;
            auto& nonEmptyChunks = sdfData.NonEmptyChunks[cascade];
            {
                PROFILE_GPU_CPU("Clear Chunks");
                for (auto it = nonEmptyChunks.Begin(); it.IsNotEnd(); ++it)
                {
                    auto& key = it->Item;
                    if (chunks.ContainsKey(key))
                        continue;

                    // Clear empty chunk
                    nonEmptyChunks.Remove(it);
                    data.ChunkCoord = key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                    if (_cb1)
                        context->UpdateCB(_cb1, &data);
                    context->Dispatch(_csClearChunk, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                    // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches
                }
            }
            // TODO: rasterize models into global sdf relative to the cascade origin to prevent fp issues on large worlds
            {
                PROFILE_GPU_CPU("Rasterize Chunks");
                for (auto& e : chunks)
                {
                    // Rasterize non-empty chunk
                    auto& key = e.Key;
                    auto& chunk = e.Value;
                    for (int32 i = 0; i < chunk.ModelsCount; i++)
                    {
                        int32 model = chunk.Models[i];
                        data.Models[i] = model;
                        context->BindSR(i + 1, _modelsTextures[model]);
                    }
                    ASSERT_LOW_LAYER(chunk.ModelsCount != 0);
                    data.ChunkCoord = key.Coord * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
                    data.ModelsCount = chunk.ModelsCount;
                    if (_cb1)
                        context->UpdateCB(_cb1, &data);
                    GPUShaderProgramCS* cs;
                    if (key.Layer == 0)
                    {
                        // First layer so can override existing chunk data
                        cs = _csRasterizeModel0;
                        nonEmptyChunks.Add(key);
                    }
                    else
                    {
                        // Another layer so need combine with existing chunk data
                        cs = _csRasterizeModel1;
                    }
                    context->Dispatch(cs, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
                    // TODO: don't stall with UAV barrier on D3D12/Vulkan if UAVs don't change between dispatches - only for a sequence of _csRasterizeModel0 dispatches (maybe cache per-shader write/read flags for all UAVs?)

#if GLOBAL_SDF_DEBUG_CHUNKS
                    // Debug draw chunk bounds in world space with number of models in it
                    if (cascade + 1 == GLOBAL_SDF_DEBUG_CHUNKS)
                    {
                        Vector3 chunkMin = cascadeBounds.Minimum + Vector3(key.Coord) * chunkSize;
                        BoundingBox chunkBounds(chunkMin, chunkMin + chunkSize);
                        DebugDraw::DrawWireBox(chunkBounds, Color::Red, 0, false);
                        DebugDraw::DrawText(StringUtils::ToString(chunk.ModelsCount), chunkBounds.GetCenter() + Vector3(0, 50.0f * key.Layer, 0), Color::Red);
                    }
#endif
                }
            }

            // Generate mip out of cascade (empty chunks have distance value 1 which is incorrect so mip will be used as a fallback - lower res)
            {
                PROFILE_GPU_CPU("Generate Mip");
                if (_cb1)
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
    }

    // Copy results
    static_assert(ARRAY_COUNT(result.Cascades) == ARRAY_COUNT(sdfData.Cascades), "Invalid cascades count.");
    static_assert(ARRAY_COUNT(result.CascadeMips) == ARRAY_COUNT(sdfData.CascadeMips), "Invalid cascades count.");
    Platform::MemoryCopy(result.Cascades, sdfData.Cascades, sizeof(result.Cascades));
    Platform::MemoryCopy(result.CascadeMips, sdfData.CascadeMips, sizeof(result.CascadeMips));
    for (int32 cascade = 0; cascade < 4; cascade++)
    {
        const float distance = cascadesDistances[cascade];
        const float maxDistance = distance * 2;
        const float voxelSize = maxDistance / resolution;
        const Vector3 center = sdfData.Positions[cascade];
        result.GlobalSDF.CascadePosDistance[cascade] = Vector4(center, distance);
        result.GlobalSDF.CascadeVoxelSize.Raw[cascade] = voxelSize;
    }
    result.GlobalSDF.Resolution = (float)resolution;
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
    if (_cb0)
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
