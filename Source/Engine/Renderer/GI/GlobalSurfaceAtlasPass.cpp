// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GlobalSurfaceAtlasPass.h"
#include "DynamicDiffuseGlobalIllumination.h"
#include "../GlobalSignDistanceFieldPass.h"
#include "../GBufferPass.h"
#include "../RenderList.h"
#include "../ShadowsPass.h"
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/Async/GPUSyncPoint.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Renderer/ColorGradingPass.h"
#include "Engine/Renderer/EyeAdaptationPass.h"
#include "Engine/Renderer/PostProcessingPass.h"
#include "Engine/Utilities/RectPack.h"

// This must match HLSL
#define GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION 40 // Amount of chunks (in each direction) to split atlas draw distance for objects culling
#define GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE 4
#define GLOBAL_SURFACE_ATLAS_OBJECT_DATA_STRIDE 6 // Amount of float4s per-object
#define GLOBAL_SURFACE_ATLAS_TILE_DATA_STRIDE 5 // Amount of float4s per-tile
#define GLOBAL_SURFACE_ATLAS_TILE_PADDING 1 // 1px padding to prevent color bleeding between tiles
#define GLOBAL_SURFACE_ATLAS_TILE_SIZE_MIN 8 // The minimum size of the tile
#define GLOBAL_SURFACE_ATLAS_TILE_SIZE_MAX 192 // The maximum size of the tile
#define GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET 0.1f // Small offset to prevent clipping with the closest triangles (shifts near and far planes)
#define GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES 0 // Forces to redraw all object tiles every frame
#define GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS 0 // Debug draws object bounds on redraw (and tile draw projection locations)
#define GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_CHUNKS 0 // Debug draws culled chunks bounds (non-empty)

#if GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS || GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_CHUNKS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct alignas(GPU_SHADER_DATA_ALIGNMENT) Data0
    {
    Float3 ViewWorldPos;
    float ViewNearPlane;
    float SkyboxIntensity;
    uint32 CulledObjectsCapacity;
    float LightShadowsStrength;
    float ViewFarPlane;
    Float4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
    GlobalSurfaceAtlasPass::ConstantsData GlobalSurfaceAtlas;
    DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
    ShaderLightData Light;
    });

PACK_STRUCT(struct AtlasTileVertex
    {
    Half2 Position;
    Half2 TileUV;
    uint32 TileAddress;
    });

struct GlobalSurfaceAtlasTile : RectPack<GlobalSurfaceAtlasTile, uint16>
{
    Float3 ViewDirection;
    Float3 ViewPosition;
    Float3 ViewBoundsSize;
    Matrix ViewMatrix;
    uint32 Address;
    uint32 ObjectAddressOffset;

    GlobalSurfaceAtlasTile(uint16 x, uint16 y, uint16 width, uint16 height)
        : RectPack<GlobalSurfaceAtlasTile, uint16>(x, y, width, height)
    {
    }

    void OnInsert(class GlobalSurfaceAtlasCustomBuffer* buffer, void* actorObject, int32 tileIndex);
    void OnFree(GlobalSurfaceAtlasCustomBuffer* buffer);
};

struct GlobalSurfaceAtlasObject
{
    uint64 LastFrameUsed;
    uint64 LastFrameUpdated;
    uint64 LightingUpdateFrame; // Index of the frame to update lighting for this object (calculated when object gets dirty or overriden by dynamic lights)
    Actor* Actor;
    GlobalSurfaceAtlasTile* Tiles[6];
    float Radius;
    OrientedBoundingBox Bounds;

    GlobalSurfaceAtlasObject()
    {
        Platform::MemoryClear(this, sizeof(GlobalSurfaceAtlasObject));
    }

    POD_COPYABLE(GlobalSurfaceAtlasObject);
};

struct GlobalSurfaceAtlasLight
{
    uint64 LastFrameUsed = 0;
    uint64 LastFrameUpdated = 0;
};

class GlobalSurfaceAtlasCustomBuffer : public RenderBuffers::CustomBuffer, public ISceneRenderingListener
{
public:
    int32 Resolution = 0;
    int32 AtlasPixelsUsed = 0;
    uint64 LastFrameAtlasInsertFail = 0;
    uint64 LastFrameAtlasDefragmentation = 0;
    GPUTexture* AtlasDepth = nullptr;
    GPUTexture* AtlasEmissive = nullptr;
    GPUTexture* AtlasGBuffer0 = nullptr;
    GPUTexture* AtlasGBuffer1 = nullptr;
    GPUTexture* AtlasGBuffer2 = nullptr;
    GPUTexture* AtlasLighting = nullptr;
    GPUBuffer* ChunksBuffer = nullptr;
    GPUBuffer* CulledObjectsBuffer = nullptr;
    DynamicTypedBuffer ObjectsBuffer;
    int32 CulledObjectsCounterIndex = -1;
    GlobalSurfaceAtlasPass::BindingData Result;
    GlobalSurfaceAtlasTile* AtlasTiles = nullptr; // TODO: optimize with a single allocation for atlas tiles
    Dictionary<void*, GlobalSurfaceAtlasObject> Objects;
    Dictionary<Guid, GlobalSurfaceAtlasLight> Lights;
    SamplesBuffer<uint32, 30> CulledObjectsUsageHistory;

    // Cached data to be reused during RasterizeActor
    uint64 CurrentFrame;
    float ResolutionInv;
    Float3 ViewPosition;
    float TileTexelsPerWorldUnit;
    float DistanceScalingStart;
    float DistanceScalingEnd;
    float DistanceScaling;

    GlobalSurfaceAtlasCustomBuffer()
        : ObjectsBuffer(256 * (GLOBAL_SURFACE_ATLAS_OBJECT_DATA_STRIDE + GLOBAL_SURFACE_ATLAS_TILE_DATA_STRIDE * 3 / 4), PixelFormat::R32G32B32A32_Float, false, TEXT("GlobalSurfaceAtlas.ObjectsBuffer"))
    {
    }

    void ClearObjects()
    {
        CulledObjectsCounterIndex = -1;
        CulledObjectsUsageHistory.Clear();
        LastFrameAtlasDefragmentation = Engine::FrameCount;
        AtlasPixelsUsed = 0;
        SAFE_DELETE(AtlasTiles);
        Objects.Clear();
        Lights.Clear();
    }

    void Reset()
    {
        RenderTargetPool::Release(AtlasDepth);
        RenderTargetPool::Release(AtlasEmissive);
        RenderTargetPool::Release(AtlasGBuffer0);
        RenderTargetPool::Release(AtlasGBuffer1);
        RenderTargetPool::Release(AtlasGBuffer2);
        RenderTargetPool::Release(AtlasLighting);
        ClearObjects();
    }

    ~GlobalSurfaceAtlasCustomBuffer()
    {
        SAFE_DELETE_GPU_RESOURCE(ChunksBuffer);
        SAFE_DELETE_GPU_RESOURCE(CulledObjectsBuffer);
        Reset();
    }

    // [ISceneRenderingListener]
    void OnSceneRenderingAddActor(Actor* a) override
    {
    }

    void OnSceneRenderingUpdateActor(Actor* a, const BoundingSphere& prevBounds) override
    {
        // Dirty static objects to redraw when changed (eg. material modification)
        if (a->HasStaticFlag(StaticFlags::Lightmap))
        {
            GlobalSurfaceAtlasObject* object = Objects.TryGet(a);
            if (object)
            {
                // Dirty object to redraw
                object->LastFrameUpdated = 0;
            }
            GlobalSurfaceAtlasLight* light = Lights.TryGet(a->GetID());
            if (light)
            {
                // Dirty light to redraw
                light->LastFrameUpdated = 0;
            }
        }
    }

    void OnSceneRenderingRemoveActor(Actor* a) override
    {
    }

    void OnSceneRenderingClear(SceneRendering* scene) override
    {
    }
};

void GlobalSurfaceAtlasTile::OnInsert(GlobalSurfaceAtlasCustomBuffer* buffer, void* actorObject, int32 tileIndex)
{
    buffer->Objects[actorObject].Tiles[tileIndex] = this;
    buffer->AtlasPixelsUsed += (int32)Width * (int32)Height;
}

void GlobalSurfaceAtlasTile::OnFree(GlobalSurfaceAtlasCustomBuffer* buffer)
{
    buffer->AtlasPixelsUsed -= (int32)Width * (int32)Height;
}

String GlobalSurfaceAtlasPass::ToString() const
{
    return TEXT("GlobalSurfaceAtlasPass");
}

bool GlobalSurfaceAtlasPass::Init()
{
    // Check platform support
    const auto device = GPUDevice::Instance;
    _supported = device->GetFeatureLevel() >= FeatureLevel::SM5 && device->Limits.HasCompute && device->Limits.HasTypedUAVLoad;
    return false;
}

bool GlobalSurfaceAtlasPass::setupResources()
{
    if (!_supported)
        return true;

    // Load shader
    if (!_shader)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GI/GlobalSurfaceAtlas"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<GlobalSurfaceAtlasPass, &GlobalSurfaceAtlasPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;

    const auto device = GPUDevice::Instance;
    const auto shader = _shader->GetShader();
    _cb0 = shader->GetCB(0);
    if (!_cb0)
        return true;
    _csCullObjects = shader->GetCS("CS_CullObjects");

    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug0)
    {
        _psDebug0 = device->CreatePipelineState();
        _psDebug1 = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Debug", 0);
        if (_psDebug0->Init(psDesc))
            return true;
        psDesc.PS = shader->GetPS("PS_Debug", 1);
        if (_psDebug1->Init(psDesc))
            return true;
    }
    if (!_psClear)
    {
        _psClear = device->CreatePipelineState();
        psDesc.DepthEnable = true;
        psDesc.DepthWriteEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        psDesc.VS = shader->GetVS("VS_Atlas");
        psDesc.PS = shader->GetPS("PS_Clear");
        if (_psClear->Init(psDesc))
            return true;
    }
    psDesc.DepthEnable = false;
    psDesc.DepthWriteEnable = false;
    psDesc.DepthFunc = ComparisonFunc::Never;
    if (!_psClearLighting)
    {
        _psClearLighting = device->CreatePipelineState();
        psDesc.VS = shader->GetVS("VS_Atlas");
        psDesc.PS = shader->GetPS("PS_ClearLighting");
        if (_psClearLighting->Init(psDesc))
            return true;
    }
    if (!_psDirectLighting0)
    {
        _psDirectLighting0 = device->CreatePipelineState();
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.PS = shader->GetPS("PS_Lighting", 0);
        if (_psDirectLighting0->Init(psDesc))
            return true;
        _psDirectLighting1 = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Lighting", 1);
        if (_psDirectLighting1->Init(psDesc))
            return true;
        _psIndirectLighting = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Lighting", 2);
        if (_psIndirectLighting->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void GlobalSurfaceAtlasPass::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psClearLighting);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting0);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting1);
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting);
    SAFE_DELETE_GPU_RESOURCE(_psDebug0);
    SAFE_DELETE_GPU_RESOURCE(_psDebug1);
    invalidateResources();
}

#endif

void GlobalSurfaceAtlasPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE(_vertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(_culledObjectsSizeBuffer);
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psClearLighting);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting0);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting1);
    SAFE_DELETE_GPU_RESOURCE(_psIndirectLighting);
    SAFE_DELETE_GPU_RESOURCE(_psDebug0);
    SAFE_DELETE_GPU_RESOURCE(_psDebug1);
    _cb0 = nullptr;
    _shader = nullptr;
}

bool GlobalSurfaceAtlasPass::Render(RenderContext& renderContext, GPUContext* context, BindingData& result)
{
    // Skip if not supported
    if (checkIfSkipPass())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& surfaceAtlasData = *renderContext.Buffers->GetCustomBuffer<GlobalSurfaceAtlasCustomBuffer>(TEXT("GlobalSurfaceAtlas"));

    // Render Global SDF (used for direct shadowing)
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF))
        return true;

    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (surfaceAtlasData.LastFrameUsed == currentFrame)
    {
        result = surfaceAtlasData.Result;
        return false;
    }
    surfaceAtlasData.LastFrameUsed = currentFrame;
    PROFILE_GPU_CPU_NAMED("Global Surface Atlas");

    // Setup options
    auto* graphicsSettings = GraphicsSettings::Get();
    const int32 resolution = Math::Clamp(graphicsSettings->GlobalSurfaceAtlasResolution, 256, GPU_MAX_TEXTURE_SIZE);
    const float resolutionInv = 1.0f / (float)resolution;
    auto& giSettings = renderContext.List->Settings.GlobalIllumination;
    const float distance = giSettings.Distance;

    // Initialize buffers
    bool noCache = surfaceAtlasData.Resolution != resolution;
    if (noCache)
    {
        surfaceAtlasData.Reset();

        auto desc = GPUTextureDescription::New2D(resolution, resolution, PixelFormat::Unknown);
        uint64 memUsage = 0;
        // TODO: try using BC4/BC5/BC7 block compression for Surface Atlas (eg. for Tiles material properties)
#define INIT_ATLAS_TEXTURE(texture, format) desc.Format = format; surfaceAtlasData.texture = RenderTargetPool::Get(desc); if (!surfaceAtlasData.texture) return true; memUsage += surfaceAtlasData.texture->GetMemoryUsage(); RENDER_TARGET_POOL_SET_NAME(surfaceAtlasData.texture, "GlobalSurfaceAtlas." #texture);
        INIT_ATLAS_TEXTURE(AtlasEmissive, PixelFormat::R11G11B10_Float);
        INIT_ATLAS_TEXTURE(AtlasGBuffer0, GBUFFER0_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasGBuffer1, GBUFFER1_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasGBuffer2, GBUFFER2_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasLighting, PixelFormat::R11G11B10_Float);
        desc.Flags = GPUTextureFlags::DepthStencil | GPUTextureFlags::ShaderResource;
        INIT_ATLAS_TEXTURE(AtlasDepth, PixelFormat::D16_UNorm);
#undef INIT_ATLAS_TEXTURE
        surfaceAtlasData.Resolution = resolution;
        if (!surfaceAtlasData.ChunksBuffer)
        {
            surfaceAtlasData.ChunksBuffer = GPUDevice::Instance->CreateBuffer(TEXT("GlobalSurfaceAtlas.ChunksBuffer"));
            if (surfaceAtlasData.ChunksBuffer->Init(GPUBufferDescription::Raw(sizeof(uint32) * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION, GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess)))
                return true;
            memUsage += surfaceAtlasData.ChunksBuffer->GetMemoryUsage();
        }
        LOG(Info, "Global Surface Atlas resolution: {0}, memory usage: {1} MB", resolution, memUsage / 1024 / 1024);
    }
    else
    {
        // Perform atlas defragmentation if needed
        constexpr float maxUsageToDefrag = 0.8f;
        if (currentFrame - surfaceAtlasData.LastFrameAtlasInsertFail < 10 &&
            currentFrame - surfaceAtlasData.LastFrameAtlasDefragmentation > 60 &&
            (float)surfaceAtlasData.AtlasPixelsUsed / (resolution * resolution) < maxUsageToDefrag)
        {
            surfaceAtlasData.ClearObjects();
        }
    }
    for (SceneRendering* scene : renderContext.List->Scenes)
        surfaceAtlasData.ListenSceneRendering(scene);
    if (!surfaceAtlasData.AtlasTiles)
        surfaceAtlasData.AtlasTiles = New<GlobalSurfaceAtlasTile>(0, 0, resolution, resolution);
    if (!_vertexBuffer)
        _vertexBuffer = New<DynamicVertexBuffer>(0u, (uint32)sizeof(AtlasTileVertex), TEXT("GlobalSurfaceAtlas.VertexBuffer"));

    // Utility for writing into tiles vertex buffer
    const Float2 posToClipMul(2.0f * resolutionInv, -2.0f * resolutionInv);
    const Float2 posToClipAdd(-1.0f, 1.0f);
#define VB_WRITE_TILE_POS_ONLY(tile) \
        Float2 minPos((float)tile->X, (float)tile->Y), maxPos((float)(tile->X + tile->Width), (float)(tile->Y + tile->Height)); \
        Half2 min(minPos * posToClipMul + posToClipAdd), max(maxPos * posToClipMul + posToClipAdd); \
        auto* quad = _vertexBuffer->WriteReserve<AtlasTileVertex>(6); \
        quad[0].Position  = max; \
        quad[1].Position = { min.X, max.Y }; \
        quad[2].Position = min; \
        quad[3].Position = quad[2].Position; \
        quad[4].Position = { max.X, min.Y }; \
        quad[5].Position = quad[0].Position
#define VB_WRITE_TILE(tile) \
        Float2 minPos((float)tile->X, (float)tile->Y), maxPos((float)(tile->X + tile->Width), (float)(tile->Y + tile->Height)); \
        Half2 min(minPos * posToClipMul + posToClipAdd), max(maxPos * posToClipMul + posToClipAdd); \
        Float2 minUV(0, 0), maxUV(1, 1); \
        auto* quad = _vertexBuffer->WriteReserve<AtlasTileVertex>(6); \
        quad[0] = { { max }, { maxUV }, tile->Address }; \
        quad[1] = { { min.X, max.Y }, { minUV.X, maxUV.Y }, tile->Address }; \
        quad[2] = { { min }, { minUV }, tile->Address }; \
        quad[3] = quad[2]; \
        quad[4] = { { max.X, min.Y }, { maxUV.X, minUV.Y }, tile->Address }; \
        quad[5] = quad[0]
#define VB_DRAW() \
        _vertexBuffer->Flush(context); \
        auto vb = _vertexBuffer->GetBuffer(); \
        context->BindVB(ToSpan(&vb, 1)); \
        context->DrawInstanced(_vertexBuffer->Data.Count() / sizeof(AtlasTileVertex), 1);

    // Add objects into the atlas
    {
        PROFILE_CPU_NAMED("Draw");
        surfaceAtlasData.ObjectsBuffer.Clear();
        _dirtyObjectsBuffer.Clear();
        _surfaceAtlasData = &surfaceAtlasData;
        renderContext.View.Pass = DrawPass::GlobalSurfaceAtlas;
        surfaceAtlasData.CurrentFrame = currentFrame;
        surfaceAtlasData.ResolutionInv = resolutionInv;
        surfaceAtlasData.ViewPosition = renderContext.View.Position;
        surfaceAtlasData.TileTexelsPerWorldUnit = 1.0f / 10.0f; // Scales the tiles resolution
        surfaceAtlasData.DistanceScalingStart = 2000.0f; // Distance from camera at which the tiles resolution starts to be scaled down
        surfaceAtlasData.DistanceScalingEnd = 5000.0f; // Distance from camera at which the tiles resolution end to be scaled down
        surfaceAtlasData.DistanceScaling = 0.2f; // The scale for tiles at distanceScalingEnd and further away
        // TODO: add DetailsScale param to adjust quality of scene details in Global Surface Atlas
        const uint32 viewMask = renderContext.View.RenderLayersMask;
        const Float3 viewPosition = renderContext.View.Position;
        const float minObjectRadius = 20.0f; // Skip too small objects
        _cullingPosDistance = Vector4(viewPosition, distance);
        int32 actorsDrawn = 0;
        SceneRendering::DrawCategory drawCategories[] = { SceneRendering::SceneDraw, SceneRendering::SceneDrawAsync };
        for (auto* scene : renderContext.List->Scenes)
        {
            for (SceneRendering::DrawCategory drawCategory : drawCategories)
            {
                auto& list = scene->Actors[drawCategory];
                for (auto& e : list)
                {
                    if (e.Bounds.Radius >= minObjectRadius && viewMask & e.LayerMask && CollisionsHelper::DistanceSpherePoint(e.Bounds, viewPosition) < distance)
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

    // Remove unused objects
    {
        PROFILE_GPU_CPU_NAMED("Compact Objects");
        for (auto it = surfaceAtlasData.Objects.Begin(); it.IsNotEnd(); ++it)
        {
            if (it->Value.LastFrameUsed != currentFrame)
            {
                for (auto& tile : it->Value.Tiles)
                {
                    if (tile)
                        tile->Free(&surfaceAtlasData);
                }
                surfaceAtlasData.Objects.Remove(it);
            }
        }
    }

    // Rasterize world geometry material properties into Global Surface Atlas
    if (_dirtyObjectsBuffer.Count() != 0)
    {
        PROFILE_GPU_CPU_NAMED("Rasterize Tiles");

        RenderContext renderContextTiles = renderContext;
        renderContextTiles.List = RenderList::GetFromPool();
        renderContextTiles.View.Pass = DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas;
        renderContextTiles.View.Mode = ViewMode::Default;
        renderContextTiles.View.ModelLODBias += 100000;
        renderContextTiles.View.IsSingleFrame = true;
        renderContextTiles.View.IsCullingDisabled = true;
        renderContextTiles.View.Near = 0.0f;
        renderContextTiles.View.Prepare(renderContextTiles);

        GPUTextureView* depthBuffer = surfaceAtlasData.AtlasDepth->View();
        GPUTextureView* targetBuffers[4] =
        {
            surfaceAtlasData.AtlasEmissive->View(),
            surfaceAtlasData.AtlasGBuffer0->View(),
            surfaceAtlasData.AtlasGBuffer1->View(),
            surfaceAtlasData.AtlasGBuffer2->View(),
        };
        context->SetRenderTarget(depthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
        {
            PROFILE_GPU_CPU_NAMED("Clear");
            if (noCache || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES)
            {
                // Full-atlas hardware clear
                context->ClearDepth(depthBuffer);
                context->Clear(targetBuffers[0], Color::Transparent);
                context->Clear(targetBuffers[1], Color::Transparent);
                context->Clear(targetBuffers[2], Color::Transparent);
                context->Clear(targetBuffers[3], Color(1, 0, 0, 0));
            }
            else
            {
                // Per-tile clear (with a single draw call)
                _vertexBuffer->Clear();
                _vertexBuffer->Data.EnsureCapacity(_dirtyObjectsBuffer.Count() * 6 * sizeof(AtlasTileVertex));
                for (void* actorObject : _dirtyObjectsBuffer)
                {
                    const GlobalSurfaceAtlasObject* objectPtr = surfaceAtlasData.Objects.TryGet(actorObject);
                    if (!objectPtr)
                        continue;
                    const GlobalSurfaceAtlasObject& object = *objectPtr;
                    for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                    {
                        auto* tile = object.Tiles[tileIndex];
                        if (!tile)
                            continue;
                        VB_WRITE_TILE_POS_ONLY(tile);
                    }
                }
                context->SetState(_psClear);
                context->SetViewportAndScissors(Viewport(0, 0, (float)resolution, (float)resolution));
                VB_DRAW();
            }
        }
        // TODO: limit dirty objects count on a first frame (eg. collect overflown objects to be redirty next frame)
        auto& drawCallsListGBuffer = renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer];
        auto& drawCallsListGBufferNoDecals = renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals];
        drawCallsListGBuffer.CanUseInstancing = false;
        drawCallsListGBufferNoDecals.CanUseInstancing = false;
        int32 tilesDrawn = 0;
        for (void* actorObject : _dirtyObjectsBuffer)
        {
            const GlobalSurfaceAtlasObject* objectPtr = surfaceAtlasData.Objects.TryGet(actorObject);
            if (!objectPtr)
                continue;
            const GlobalSurfaceAtlasObject& object = *objectPtr;

            // Clear draw calls list
            renderContextTiles.List->DrawCalls.Clear();
            renderContextTiles.List->BatchedDrawCalls.Clear();
            drawCallsListGBuffer.Indices.Clear();
            drawCallsListGBuffer.PreBatchedDrawCalls.Clear();
            drawCallsListGBufferNoDecals.Indices.Clear();
            drawCallsListGBufferNoDecals.PreBatchedDrawCalls.Clear();

            // Fake projection matrix to disable Screen Size culling based on RenderTools::ComputeBoundsScreenRadiusSquared
            renderContextTiles.View.Projection.Values[0][0] = 10000.0f;

            // Collect draw calls for the object
            _currentActorObject = actorObject;
            object.Actor->Draw(renderContextTiles);

            // Render all tiles into the atlas
#if GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS
            DebugDraw::DrawBox(object.Bounds, Color::Red.AlphaMultiplied(0.4f));
#endif
            for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
            {
                auto* tile = object.Tiles[tileIndex];
                if (!tile)
                    continue;
                const float tileWidth = (float)tile->Width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
                const float tileHeight = (float)tile->Height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;

                // Setup projection to capture object from the side
                renderContextTiles.View.Position = tile->ViewPosition;
                renderContextTiles.View.Direction = tile->ViewDirection;
                renderContextTiles.View.Near = -GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET;
                renderContextTiles.View.Far = tile->ViewBoundsSize.Z + 2 * GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET;
                Matrix projectionMatrix;
                Matrix::Ortho(tile->ViewBoundsSize.X, tile->ViewBoundsSize.Y, renderContextTiles.View.Near, renderContextTiles.View.Far, projectionMatrix);
                renderContextTiles.View.SetUp(tile->ViewMatrix, projectionMatrix);
#if GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS
                DebugDraw::DrawLine(renderContextTiles.View.Position, renderContextTiles.View.Position + renderContextTiles.View.Direction * 20.0f, Color::Orange);
                DebugDraw::DrawWireSphere(BoundingSphere(renderContextTiles.View.Position, 10.0f), Color::Green);
#endif

                // Draw
                context->SetViewportAndScissors(Viewport(tile->X, tile->Y, tileWidth, tileHeight));
                renderContextTiles.List->ExecuteDrawCalls(renderContextTiles, drawCallsListGBuffer);
                renderContextTiles.List->ExecuteDrawCalls(renderContextTiles, drawCallsListGBufferNoDecals);
                tilesDrawn++;
            }
        }
        ZoneValue(tilesDrawn);
        context->ResetRenderTarget();
        RenderList::ReturnToPool(renderContextTiles.List);
    }

    // Send objects data to the GPU
    {
        PROFILE_GPU_CPU_NAMED("Update Objects");
        surfaceAtlasData.ObjectsBuffer.Flush(context);
    }

    // Init constants
    result.Constants.ViewPos = renderContext.View.Position;
    result.Constants.Resolution = (float)resolution;
    result.Constants.ChunkSize = distance / (float)GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION;
    result.Constants.ObjectsCount = surfaceAtlasData.Objects.Count();

    // If we don't know the culled objects buffer capacity then we shouldn't use atlas results as many objects are still missing (see CulledObjectsCounterIndex usage)
    bool notReady = false;

    // Cull objects into chunks (for faster Atlas sampling)
    if (surfaceAtlasData.Objects.Count() != 0)
    {
        // Each chunk (ChunksBuffer) contains uint with address of the culled objects data start in CulledObjectsBuffer.
        // If chunk has address=0 then it's unused/empty.
        // Chunk [0,0,0] is unused and it's address=0 is used for atomic counter for writing into CulledObjectsBuffer.
        // Each chunk data contains objects count + all objects addresses.
        // This allows to quickly convert world-space position into chunk, then read chunk data start and loop over culled objects.
        PROFILE_GPU_CPU_NAMED("Cull Objects");
        uint32 objectsBufferCapacity = (uint32)((float)surfaceAtlasData.Objects.Count() * 1.3f);

        // Copy counter from ChunksBuffer into staging buffer to access current chunks memory usage to adapt dynamically to the scene complexity
        if (surfaceAtlasData.ChunksBuffer)
        {
            if (!_culledObjectsSizeBuffer)
            {
                Platform::MemoryClear(_culledObjectsSizeFrames, sizeof(_culledObjectsSizeFrames));
                _culledObjectsSizeBuffer = GPUDevice::Instance->CreateBuffer(TEXT("GlobalSurfaceAtlas.CulledObjectsSizeBuffer"));
                const GPUBufferDescription desc = GPUBufferDescription::Buffer(ARRAY_COUNT(_culledObjectsSizeFrames) * sizeof(uint32), GPUBufferFlags::None, PixelFormat::R32_UInt, _culledObjectsSizeFrames, sizeof(uint32), GPUResourceUsage::StagingReadback);
                if (_culledObjectsSizeBuffer->Init(desc))
                    return true;
            }
            if (surfaceAtlasData.CulledObjectsCounterIndex != -1)
            {
                // Get the last counter value (accept staging readback delay or not available data yet)
                notReady = true;
                auto data = (uint32*)_culledObjectsSizeBuffer->Map(GPUResourceMapMode::Read);
                if (data)
                {
                    uint32 counter = data[surfaceAtlasData.CulledObjectsCounterIndex];
                    if (counter > 0)
                    {
                        objectsBufferCapacity = counter;
                        notReady = false;
                    }
                    _culledObjectsSizeBuffer->Unmap();
                }

                // Allow to be ready if the buffer was already used
                if (notReady && surfaceAtlasData.CulledObjectsBuffer && surfaceAtlasData.CulledObjectsBuffer->IsAllocated())
                    notReady = false;
            }
            if (surfaceAtlasData.CulledObjectsCounterIndex == -1)
            {
                // Find a free timer slot
                notReady = true;
                for (int32 i = 0; i < ARRAY_COUNT(_culledObjectsSizeFrames); i++)
                {
                    if (currentFrame - _culledObjectsSizeFrames[i] > GPU_ASYNC_LATENCY)
                    {
                        surfaceAtlasData.CulledObjectsCounterIndex = i;
                        break;
                    }
                }
            }
            if (surfaceAtlasData.CulledObjectsCounterIndex != -1 && surfaceAtlasData.CulledObjectsBuffer)
            {
                // Copy current counter value
                _culledObjectsSizeFrames[surfaceAtlasData.CulledObjectsCounterIndex] = currentFrame;
                context->CopyBuffer(_culledObjectsSizeBuffer, surfaceAtlasData.CulledObjectsBuffer, sizeof(uint32), surfaceAtlasData.CulledObjectsCounterIndex * sizeof(uint32), 0);
            }
        }

        // Calculate optimal capacity for the objects buffer
        objectsBufferCapacity *= sizeof(uint32) * 2; // Convert to bytes and add safe margin
        objectsBufferCapacity = Math::Clamp(Math::AlignUp<uint32>(objectsBufferCapacity, 4096u), 32u * 1024u, 1024u * 1024u); // Align up to 4kB, clamp 32kB - 1MB
        surfaceAtlasData.CulledObjectsUsageHistory.Add(objectsBufferCapacity); // Record history
        objectsBufferCapacity = surfaceAtlasData.CulledObjectsUsageHistory.Maximum(); // Use biggest value from history
        if (surfaceAtlasData.CulledObjectsUsageHistory.Count() == surfaceAtlasData.CulledObjectsUsageHistory.Capacity())
            notReady = false; // Always ready when rendering for some time

        // Allocate buffer for culled objects (estimated size)
        if (!surfaceAtlasData.CulledObjectsBuffer)
            surfaceAtlasData.CulledObjectsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("GlobalSurfaceAtlas.CulledObjectsBuffer"));
        if (surfaceAtlasData.CulledObjectsBuffer->GetSize() < objectsBufferCapacity)
        {
            const auto desc = GPUBufferDescription::Raw(objectsBufferCapacity, GPUBufferFlags::UnorderedAccess | GPUBufferFlags::ShaderResource);
            if (surfaceAtlasData.CulledObjectsBuffer->Init(desc))
                return true;
        }
        objectsBufferCapacity = surfaceAtlasData.CulledObjectsBuffer->GetSize();
        ZoneValue(objectsBufferCapacity / 1024); // CulledObjectsBuffer size in kB

        // Clear chunks counter (uint at 0 is used for a counter)
        uint32 counter = 1; // Move write location for culled objects after counter
        context->UpdateBuffer(surfaceAtlasData.CulledObjectsBuffer, &counter, sizeof(counter), 0);

        // Cull objects into chunks (1 thread per chunk)
        Data0 data;
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        data.CulledObjectsCapacity = objectsBufferCapacity / sizeof(uint32); // Capacity in items, not bytes
        data.GlobalSurfaceAtlas = result.Constants;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
        static_assert(GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION % GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE == 0, "Invalid chunks resolution/groups setting.");
        const int32 chunkDispatchGroups = GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION / GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE;
        context->BindSR(0, surfaceAtlasData.ObjectsBuffer.GetBuffer()->View());
        context->BindUA(0, surfaceAtlasData.ChunksBuffer->View());
        context->BindUA(1, surfaceAtlasData.CulledObjectsBuffer->View());
        context->Dispatch(_csCullObjects, chunkDispatchGroups, chunkDispatchGroups, chunkDispatchGroups);
        context->ResetUA();

#if GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_CHUNKS
        // Debug draw tiles that have any objects inside
        for (int32 z = 0; z < GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION; z++)
        {
            for (int32 y = 0; y < GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION; y++)
            {
                for (int32 x = 0; x < GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION; x++)
                {
                    Float3 chunkCoord((float)x, (float)y, (float)z);
                    Float3 chunkMin = result.Constants.ViewPos + (chunkCoord - (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 0.5f)) * result.Constants.ChunkSize;
                    Float3 chunkMax = chunkMin + result.Constants.ChunkSize;
                    BoundingBox chunkBounds(chunkMin, chunkMax);
                    if (Float3::Distance(chunkBounds.GetCenter(), result.Constants.ViewPos) >= 2000.0f)
                        continue;

                    int32 count = 0;
                    for (auto& e : surfaceAtlasData.Objects)
                    {
                        BoundingSphere objectBounds(e.Value.Bounds.GetCenter(), e.Value.Radius);
                        if (chunkBounds.Intersects(objectBounds))
                            count++;
                    }
                    if (count != 0)
                    {
                        DebugDraw::DrawText(String::Format(TEXT("{} Objects"), count), chunkBounds.GetCenter(), Color::Green);
                        DebugDraw::DrawWireBox(chunkBounds, Color::Green);
                    }
                }
            }
        }
#endif
    }

    // Copy results
    result.Atlas[0] = surfaceAtlasData.AtlasDepth;
    result.Atlas[1] = surfaceAtlasData.AtlasGBuffer0;
    result.Atlas[2] = surfaceAtlasData.AtlasGBuffer1;
    result.Atlas[3] = surfaceAtlasData.AtlasGBuffer2;
    result.Atlas[4] = surfaceAtlasData.AtlasLighting;
    result.Chunks = surfaceAtlasData.ChunksBuffer;
    result.CulledObjects = surfaceAtlasData.CulledObjectsBuffer;
    result.Objects = surfaceAtlasData.ObjectsBuffer.GetBuffer();
    surfaceAtlasData.Result = result;

    // Render direct lighting into atlas
    if (surfaceAtlasData.Objects.Count() != 0)
    {
        PROFILE_GPU_CPU_NAMED("Direct Lighting");
        context->SetViewportAndScissors(Viewport(0, 0, (float)resolution, (float)resolution));
        context->SetRenderTarget(surfaceAtlasData.AtlasLighting->View());
        context->BindSR(0, surfaceAtlasData.AtlasGBuffer0->View());
        context->BindSR(1, surfaceAtlasData.AtlasGBuffer1->View());
        context->BindSR(2, surfaceAtlasData.AtlasGBuffer2->View());
        context->BindSR(3, surfaceAtlasData.AtlasDepth->View());
        context->BindSR(4, surfaceAtlasData.ObjectsBuffer.GetBuffer()->View());
        context->BindSR(5, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
        context->BindSR(6, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
        context->BindCB(0, _cb0);
        Data0 data;
        data.ViewWorldPos = renderContext.View.Position;
        data.GlobalSDF = bindingDataSDF.Constants;
        data.GlobalSurfaceAtlas = result.Constants;

        // Collect objects to update lighting this frame (dirty objects and dirty lights)
        bool allLightingDirty = false;
        for (auto& light : renderContext.List->DirectionalLights)
        {
            GlobalSurfaceAtlasLight& lightData = surfaceAtlasData.Lights[light.ID];
            lightData.LastFrameUsed = currentFrame;
            uint32 redrawFramesCount = EnumHasAnyFlags(light.StaticFlags, StaticFlags::Lightmap) ? 120 : 4;
            if (surfaceAtlasData.CurrentFrame - lightData.LastFrameUpdated < (redrawFramesCount + (light.ID.D & redrawFramesCount)))
                continue;
            lightData.LastFrameUpdated = currentFrame;

            // Mark all objects to shade
            allLightingDirty = true;
        }
        if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::GI) && (renderContext.List->DirectionalLights.Count() != 1 || EnumHasAnyFlags(renderContext.List->DirectionalLights[0].StaticFlags, StaticFlags::Lightmap)))
        {
            switch (renderContext.List->Settings.GlobalIllumination.Mode)
            {
            case GlobalIlluminationMode::DDGI:
            {
                DynamicDiffuseGlobalIlluminationPass::BindingData bindingDataDDGI;
                if (!DynamicDiffuseGlobalIlluminationPass::Instance()->Get(renderContext.Buffers, bindingDataDDGI))
                {
                    GlobalSurfaceAtlasLight& lightData = surfaceAtlasData.Lights[Guid(0, 0, 0, 1)];
                    lightData.LastFrameUsed = currentFrame;
                    uint32 redrawFramesCount = 4; // GI Bounce redraw minimum frequency
                    if (surfaceAtlasData.CurrentFrame - lightData.LastFrameUpdated < redrawFramesCount)
                        break;
                    lightData.LastFrameUpdated = currentFrame;

                    // Mark all objects to shade
                    allLightingDirty = true;
                }
                break;
            }
            }
        }
        for (auto& light : renderContext.List->PointLights)
        {
            GlobalSurfaceAtlasLight& lightData = surfaceAtlasData.Lights[light.ID];
            lightData.LastFrameUsed = currentFrame;
            uint32 redrawFramesCount = EnumHasAnyFlags(light.StaticFlags, StaticFlags::Lightmap) ? 120 : 4;
            if (surfaceAtlasData.CurrentFrame - lightData.LastFrameUpdated < (redrawFramesCount + (light.ID.D & redrawFramesCount)))
                continue;
            lightData.LastFrameUpdated = currentFrame;

            if (!allLightingDirty)
            {
                // Mark objects to shade
                for (auto& e : surfaceAtlasData.Objects)
                {
                    auto& object = e.Value;
                    Float3 lightToObject = object.Bounds.GetCenter() - light.Position;
                    if (lightToObject.LengthSquared() >= Math::Square(object.Radius + light.Radius))
                        continue;
                    object.LightingUpdateFrame = currentFrame;
                }
            }
        }
        for (auto& light : renderContext.List->SpotLights)
        {
            GlobalSurfaceAtlasLight& lightData = surfaceAtlasData.Lights[light.ID];
            lightData.LastFrameUsed = currentFrame;
            uint32 redrawFramesCount = EnumHasAnyFlags(light.StaticFlags, StaticFlags::Lightmap) ? 120 : 4;
            if (surfaceAtlasData.CurrentFrame - lightData.LastFrameUpdated < (redrawFramesCount + (light.ID.D & redrawFramesCount)))
                continue;
            lightData.LastFrameUpdated = currentFrame;

            if (!allLightingDirty)
            {
                // Mark objects to shade
                for (auto& e : surfaceAtlasData.Objects)
                {
                    auto& object = e.Value;
                    Float3 lightToObject = object.Bounds.GetCenter() - light.Position;
                    if (lightToObject.LengthSquared() >= Math::Square(object.Radius + light.Radius))
                        continue;
                    object.LightingUpdateFrame = currentFrame;
                }
            }
        }

        // Copy emissive light into the final direct lighting atlas
        {
            PROFILE_GPU_CPU_NAMED("Copy Emissive");
            _vertexBuffer->Clear();
            for (const auto& e : surfaceAtlasData.Objects)
            {
                const auto& object = e.Value;
                if (!allLightingDirty && object.LightingUpdateFrame != currentFrame)
                    continue;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }
            if (_vertexBuffer->Data.Count() != 0)
            {
                context->BindSR(7, surfaceAtlasData.AtlasEmissive);
                context->SetState(_psClearLighting);
                VB_DRAW();
            }
        }

        // Shade object tiles influenced by lights to calculate direct lighting
        for (auto& light : renderContext.List->DirectionalLights)
        {
            // Collect tiles to shade
            _vertexBuffer->Clear();
            for (const auto& e : surfaceAtlasData.Objects)
            {
                const auto& object = e.Value;
                if (!allLightingDirty && object.LightingUpdateFrame != currentFrame)
                    continue;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile || Float3::Dot(tile->ViewDirection, light.Direction) < ZeroTolerance)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }
            if (_vertexBuffer->Data.Count() == 0)
                continue;

            // Draw light
            PROFILE_GPU_CPU_NAMED("Directional Light");
            const bool useShadow = light.CanRenderShadow(renderContext.View);
            // TODO: test perf/quality when using Shadow Map for directional light (ShadowsPass::Instance()->LastDirLightShadowMap) instead of Global SDF trace
            light.SetShaderData(data.Light, useShadow);
            data.Light.Color *= light.IndirectLightingIntensity;
            data.LightShadowsStrength = 1.0f - light.ShadowsStrength;
            context->UpdateCB(_cb0, &data);
            context->SetState(_psDirectLighting0);
            VB_DRAW();
        }
        for (auto& light : renderContext.List->PointLights)
        {
            // Collect tiles to shade
            _vertexBuffer->Clear();
            for (const auto& e : surfaceAtlasData.Objects)
            {
                const auto& object = e.Value;
                if (!allLightingDirty && object.LightingUpdateFrame != currentFrame)
                    continue;
                Float3 lightToObject = object.Bounds.GetCenter() - light.Position;
                if (lightToObject.LengthSquared() >= Math::Square(object.Radius + light.Radius))
                    continue;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }
            if (_vertexBuffer->Data.Count() == 0)
                continue;

            // Draw light
            PROFILE_GPU_CPU_NAMED("Point Light");
            const bool useShadow = light.CanRenderShadow(renderContext.View);
            light.SetShaderData(data.Light, useShadow);
            data.Light.Color *= light.IndirectLightingIntensity;
            data.LightShadowsStrength = 1.0f - light.ShadowsStrength;
            context->UpdateCB(_cb0, &data);
            context->SetState(_psDirectLighting1);
            VB_DRAW();
        }
        for (auto& light : renderContext.List->SpotLights)
        {
            // Collect tiles to shade
            _vertexBuffer->Clear();
            for (const auto& e : surfaceAtlasData.Objects)
            {
                const auto& object = e.Value;
                if (!allLightingDirty && object.LightingUpdateFrame != currentFrame)
                    continue;
                Float3 lightToObject = object.Bounds.GetCenter() - light.Position;
                if (lightToObject.LengthSquared() >= Math::Square(object.Radius + light.Radius))
                    continue;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile || Float3::Dot(tile->ViewDirection, light.Direction) < ZeroTolerance)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }
            if (_vertexBuffer->Data.Count() == 0)
                continue;

            // Draw light
            PROFILE_GPU_CPU_NAMED("Spot Light");
            const bool useShadow = light.CanRenderShadow(renderContext.View);
            light.SetShaderData(data.Light, useShadow);
            data.Light.Color *= light.IndirectLightingIntensity;
            data.LightShadowsStrength = 1.0f - light.ShadowsStrength;
            context->UpdateCB(_cb0, &data);
            context->SetState(_psDirectLighting1);
            VB_DRAW();
        }

        // Remove unused lights
        for (auto it = surfaceAtlasData.Lights.Begin(); it.IsNotEnd(); ++it)
        {
            if (it->Value.LastFrameUsed != currentFrame)
                surfaceAtlasData.Lights.Remove(it);
        }

        // Draw indirect light from Global Illumination
        if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::GI))
        {
            switch (giSettings.Mode)
            {
            case GlobalIlluminationMode::DDGI:
            {
                DynamicDiffuseGlobalIlluminationPass::BindingData bindingDataDDGI;
                if (giSettings.BounceIntensity > ZeroTolerance && !DynamicDiffuseGlobalIlluminationPass::Instance()->Get(renderContext.Buffers, bindingDataDDGI))
                {
                    _vertexBuffer->Clear();
                    for (const auto& e : surfaceAtlasData.Objects)
                    {
                        const auto& object = e.Value;
                        if (!allLightingDirty && object.LightingUpdateFrame != currentFrame)
                            continue;
                        for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                        {
                            auto* tile = object.Tiles[tileIndex];
                            if (!tile)
                                continue;
                            VB_WRITE_TILE(tile);
                        }
                    }
                    if (_vertexBuffer->Data.Count() == 0)
                        break;
                    PROFILE_GPU_CPU_NAMED("DDGI");
                    data.DDGI = bindingDataDDGI.Constants;
                    data.Light.Radius = giSettings.BounceIntensity / bindingDataDDGI.Constants.IndirectLightingIntensity; // Reuse for smaller CB
                    context->BindSR(5, bindingDataDDGI.ProbesData);
                    context->BindSR(6, bindingDataDDGI.ProbesDistance);
                    context->BindSR(7, bindingDataDDGI.ProbesIrradiance);
                    context->UpdateCB(_cb0, &data);
                    context->SetState(_psIndirectLighting);
                    VB_DRAW();
                }
                break;
            }
            }
        }
    }

    // TODO: explore atlas tiles optimization with feedback from renderer (eg. when tile is sampled by GI/Reflections mark it as used, then sort tiles by importance and prioritize updates for ones frequently used)

#undef WRITE_TILE
    context->ResetSR();
    context->ResetRenderTarget();
    context->SetViewportAndScissors(renderContext.View.ScreenSize.X, renderContext.View.ScreenSize.Y);
    return notReady;
}

void GlobalSurfaceAtlasPass::RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output)
{
    // Render all dependant effects before
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::GI))
    {
        switch (renderContext.List->Settings.GlobalIllumination.Mode)
        {
        case GlobalIlluminationMode::DDGI:
            DynamicDiffuseGlobalIlluminationPass::Instance()->Render(renderContext, context, nullptr);
            break;
        }
    }
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    BindingData bindingData;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF) || Render(renderContext, context, bindingData))
    {
        context->Draw(output, renderContext.Buffers->GBuffer0);
        return;
    }
    GPUTextureView* skybox = GBufferPass::Instance()->RenderSkybox(renderContext, context);

    PROFILE_GPU_CPU("Global Surface Atlas Debug");
    const Float2 outputSize(output->Size());
    Data0 data;
    {
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        for (int32 i = 0; i < 4; i++)
            data.ViewFrustumWorldRays[i] = Float4(renderContext.List->FrustumCornersWs[i + 4], 0);
        data.GlobalSDF = bindingDataSDF.Constants;
        data.GlobalSurfaceAtlas = bindingData.Constants;
        data.SkyboxIntensity = 1.0f;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }
    context->BindSR(0, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
    context->BindSR(1, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
    context->BindSR(2, bindingData.Chunks ? bindingData.Chunks->View() : nullptr);
    context->BindSR(3, bindingData.CulledObjects ? bindingData.CulledObjects->View() : nullptr);
    context->BindSR(4, bindingData.Objects ? bindingData.Objects->View() : nullptr);
    context->BindSR(6, bindingData.AtlasDepth->View());
    context->BindSR(7, skybox);
    context->SetState(_psDebug0);
    {
        Float2 outputSizeThird = outputSize * 0.333f;
        Float2 outputSizeTwoThird = outputSize * 0.666f;

        auto tempBuffer = RenderTargetPool::Get(output->GetDescription());
        RENDER_TARGET_POOL_SET_NAME(tempBuffer, "GlobalSurfaceAtlas.TempBuffer");
        context->Clear(tempBuffer->View(), Color::Black);
        context->SetRenderTarget(tempBuffer->View());

        // Full screen - direct light
        context->BindSR(5, bindingData.AtlasLighting->View());
        context->SetViewport(outputSize.X, outputSize.Y);
        context->SetScissor(Rectangle(0, 0, outputSizeTwoThird.X, outputSize.Y));
        context->DrawFullscreenTriangle();

        // Color Grading and Post-Processing to improve readability in bright/dark scenes
        context->ResetRenderTarget();
        auto colorGradingLUT = ColorGradingPass::Instance()->RenderLUT(renderContext);
        EyeAdaptationPass::Instance()->Render(renderContext, tempBuffer);
        PostProcessingPass::Instance()->Render(renderContext, tempBuffer, output, colorGradingLUT);
        RenderTargetPool::Release(colorGradingLUT);
        RenderTargetPool::Release(tempBuffer);
        context->ResetRenderTarget();

        // Rebind resources
        context->BindSR(0, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
        context->BindSR(1, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
        context->BindSR(2, bindingData.Chunks ? bindingData.Chunks->View() : nullptr);
        context->BindSR(3, bindingData.CulledObjects ? bindingData.CulledObjects->View() : nullptr);
        context->BindSR(4, bindingData.Objects ? bindingData.Objects->View() : nullptr);
        context->BindSR(6, bindingData.AtlasDepth->View());
        context->BindSR(7, skybox);
        context->BindCB(0, _cb0);
        context->SetRenderTarget(output->View());

        // Disable skybox
        data.SkyboxIntensity = 0.0f;
        context->UpdateCB(_cb0, &data);

        // Bottom left - diffuse (with missing surface coverage debug)
        context->SetState(_psDebug1);
        context->BindSR(5, bindingData.AtlasGBuffer0->View());
        context->SetViewportAndScissors(Viewport(outputSizeTwoThird.X, 0, outputSizeThird.X, outputSizeThird.Y));
        context->DrawFullscreenTriangle();

        // Bottom middle - normals
        context->SetState(_psDebug0);
        context->BindSR(5, bindingData.AtlasGBuffer1->View());
        context->SetViewportAndScissors(Viewport(outputSizeTwoThird.X, outputSizeThird.Y, outputSizeThird.X, outputSizeThird.Y));
        context->DrawFullscreenTriangle();

        // Bottom right - roughness/metalness/ao
        context->BindSR(5, bindingData.AtlasGBuffer2->View());
        context->SetViewportAndScissors(Viewport(outputSizeTwoThird.X, outputSizeTwoThird.Y, outputSizeThird.X, outputSizeThird.Y));
        context->DrawFullscreenTriangle();
    }
}

void GlobalSurfaceAtlasPass::RasterizeActor(Actor* actor, void* actorObject, const BoundingSphere& actorObjectBounds, const Transform& localToWorld, const BoundingBox& localBounds, uint32 tilesMask, bool useVisibility, float qualityScale)
{
    GlobalSurfaceAtlasCustomBuffer& surfaceAtlasData = *_surfaceAtlasData;
    Float3 boundsSize = localBounds.GetSize() * actor->GetScale();
    const float distanceScale = Math::Lerp(1.0f, surfaceAtlasData.DistanceScaling, Math::InverseLerp(surfaceAtlasData.DistanceScalingStart, surfaceAtlasData.DistanceScalingEnd, (float)CollisionsHelper::DistanceSpherePoint(actorObjectBounds, surfaceAtlasData.ViewPosition)));
    const float tilesScale = surfaceAtlasData.TileTexelsPerWorldUnit * distanceScale * qualityScale;
    GlobalSurfaceAtlasObject* object = surfaceAtlasData.Objects.TryGet(actorObject);
    bool anyTile = false, dirty = false;
    for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
    {
        if (((1 << tileIndex) & tilesMask) == 0)
            continue;

        // Calculate optimal tile resolution for the object side
        Float3 boundsSizeTile = boundsSize;
        boundsSizeTile.Raw[tileIndex / 2] = MAX_float; // Ignore depth size
        uint16 tileResolution = (uint16)(boundsSizeTile.GetAbsolute().MinValue() * tilesScale);
        if (tileResolution < 4)
        {
            // Skip too small surfaces
            if (object && object->Tiles[tileIndex])
            {
                object->Tiles[tileIndex]->Free(&surfaceAtlasData);
                object->Tiles[tileIndex] = nullptr;
            }
            continue;
        }

        // Clamp tile resolution (in pixels)
        static_assert(GLOBAL_SURFACE_ATLAS_TILE_PADDING < GLOBAL_SURFACE_ATLAS_TILE_SIZE_MIN, "Invalid tile size configuration. Minimum tile size must be larger than padding.");
        tileResolution = Math::Clamp<uint16>(tileResolution, GLOBAL_SURFACE_ATLAS_TILE_SIZE_MIN, GLOBAL_SURFACE_ATLAS_TILE_SIZE_MAX);

        // Snap tiles resolution (down) which allows to reuse atlas slots once object gets resizes/replaced by other object
        tileResolution = Math::AlignDown<uint16>(tileResolution, 8);

        // Reuse current tile (refit only on a significant resolution change)
        if (object && object->Tiles[tileIndex])
        {
            const uint16 tileRefitResolutionStep = 32;
            const uint16 currentSize = object->Tiles[tileIndex]->Width;
            if (Math::Abs(tileResolution - currentSize) < tileRefitResolutionStep)
            {
                anyTile = true;
                continue;
            }
            object->Tiles[tileIndex]->Free(&surfaceAtlasData);
        }

        // Insert tile into atlas
        auto* tile = surfaceAtlasData.AtlasTiles->Insert(tileResolution, tileResolution, 0, &surfaceAtlasData, actorObject, tileIndex);
        if (tile)
        {
            if (!object)
                object = &surfaceAtlasData.Objects[actorObject];
            object->Tiles[tileIndex] = tile;
            anyTile = true;
            dirty = true;
        }
        else
        {
            if (object)
                object->Tiles[tileIndex] = nullptr;
            surfaceAtlasData.LastFrameAtlasInsertFail = surfaceAtlasData.CurrentFrame;
        }
    }
    if (!anyTile)
        return;

    // Redraw objects from time-to-time (dynamic objects can be animated, static objects can have textures streamed)
    uint32 redrawFramesCount = actor->HasStaticFlag(StaticFlags::Lightmap) ? 120 : 4;
    if (surfaceAtlasData.CurrentFrame - object->LastFrameUpdated >= (redrawFramesCount + (actor->GetID().D & redrawFramesCount)))
        dirty = true;

    // Mark object as used
    object->Actor = actor;
    object->LastFrameUsed = surfaceAtlasData.CurrentFrame;
    object->Bounds = OrientedBoundingBox(localBounds);
    object->Bounds.Transform(localToWorld);
    object->Radius = (float)actorObjectBounds.Radius;
    if (dirty || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES)
    {
        object->LastFrameUpdated = surfaceAtlasData.CurrentFrame;
        object->LightingUpdateFrame = surfaceAtlasData.CurrentFrame;
        _dirtyObjectsBuffer.Add(actorObject);
    }

    Matrix3x3 worldToLocalRotation;
    Matrix3x3::RotationQuaternion(object->Bounds.Transformation.Orientation.Conjugated(), worldToLocalRotation);
    Float3 worldPosition = object->Bounds.Transformation.Translation;
    Float3 worldExtents = object->Bounds.Extents * object->Bounds.Transformation.Scale;

    // Write to objects buffer (this must match unpacking logic in HLSL)
    uint32 objectAddress = surfaceAtlasData.ObjectsBuffer.Data.Count() / sizeof(Float4);
    auto* objectData = surfaceAtlasData.ObjectsBuffer.WriteReserve<Float4>(GLOBAL_SURFACE_ATLAS_OBJECT_DATA_STRIDE);
    objectData[0] = *(Float4*)&actorObjectBounds;
    objectData[1] = Float4::Zero;
    objectData[2] = Float4(worldToLocalRotation.M11, worldToLocalRotation.M12, worldToLocalRotation.M13, worldPosition.X);
    objectData[3] = Float4(worldToLocalRotation.M21, worldToLocalRotation.M22, worldToLocalRotation.M23, worldPosition.Y);
    objectData[4] = Float4(worldToLocalRotation.M31, worldToLocalRotation.M32, worldToLocalRotation.M33, worldPosition.Z);
    objectData[5] = Float4(worldExtents, useVisibility ? 1.0f : 0.0f);
    auto tileOffsets = reinterpret_cast<uint16*>(&objectData[1]); // xyz used for tile offsets packed into uint16
    auto objectDataSize = reinterpret_cast<uint32*>(&objectData[1].W); // w used for object size (count of Float4s for object+tiles)
    *objectDataSize = GLOBAL_SURFACE_ATLAS_OBJECT_DATA_STRIDE;
    for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
    {
        auto* tile = object->Tiles[tileIndex];
        if (!tile)
            continue;
        tile->ObjectAddressOffset = *objectDataSize;
        tile->Address = objectAddress + tile->ObjectAddressOffset;
        tileOffsets[tileIndex] = tile->ObjectAddressOffset;
        *objectDataSize += GLOBAL_SURFACE_ATLAS_TILE_DATA_STRIDE;

        // Setup view to render object from the side
        Float3 xAxis, yAxis, zAxis = Float3::Zero;
        zAxis.Raw[tileIndex / 2] = tileIndex & 1 ? 1.0f : -1.0f;
        yAxis = tileIndex == 2 || tileIndex == 3 ? Float3::Right : Float3::Up;
        Float3::Cross(yAxis, zAxis, xAxis);
        Float3 localSpaceOffset = -zAxis * object->Bounds.Extents;
        xAxis = object->Bounds.Transformation.LocalToWorldVector(xAxis);
        yAxis = object->Bounds.Transformation.LocalToWorldVector(yAxis);
        zAxis = object->Bounds.Transformation.LocalToWorldVector(zAxis);
        xAxis.NormalizeFast();
        yAxis.NormalizeFast();
        zAxis.NormalizeFast();
        tile->ViewPosition = object->Bounds.Transformation.LocalToWorld(localSpaceOffset);
        tile->ViewDirection = zAxis;

        // Create view matrix
        tile->ViewMatrix.SetColumn1(Float4(xAxis, -Float3::Dot(xAxis, tile->ViewPosition)));
        tile->ViewMatrix.SetColumn2(Float4(yAxis, -Float3::Dot(yAxis, tile->ViewPosition)));
        tile->ViewMatrix.SetColumn3(Float4(zAxis, -Float3::Dot(zAxis, tile->ViewPosition)));
        tile->ViewMatrix.SetColumn4(Float4(0, 0, 0, 1));

        // Calculate object bounds size in the view
        OrientedBoundingBox viewBounds(object->Bounds);
        viewBounds.Transform(tile->ViewMatrix);
        Float3 viewExtent = viewBounds.Transformation.LocalToWorldVector(viewBounds.Extents);
        tile->ViewBoundsSize = viewExtent.GetAbsolute() * 2.0f;

        // Per-tile data
        const float tileWidth = (float)tile->Width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
        const float tileHeight = (float)tile->Height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
        auto* tileData = surfaceAtlasData.ObjectsBuffer.WriteReserve<Float4>(GLOBAL_SURFACE_ATLAS_TILE_DATA_STRIDE);
        tileData[0] = Float4(tile->X, tile->Y, tileWidth, tileHeight) * surfaceAtlasData.ResolutionInv;
        tileData[1] = Float4(tile->ViewMatrix.M11, tile->ViewMatrix.M12, tile->ViewMatrix.M13, tile->ViewMatrix.M41);
        tileData[2] = Float4(tile->ViewMatrix.M21, tile->ViewMatrix.M22, tile->ViewMatrix.M23, tile->ViewMatrix.M42);
        tileData[3] = Float4(tile->ViewMatrix.M31, tile->ViewMatrix.M32, tile->ViewMatrix.M33, tile->ViewMatrix.M43);
        tileData[4] = Float4(tile->ViewBoundsSize, 0.0f); // w unused
    }
}
