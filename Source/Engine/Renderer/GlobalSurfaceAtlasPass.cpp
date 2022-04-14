// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GlobalSurfaceAtlasPass.h"
#include "GlobalSignDistanceFieldPass.h"
#include "RenderList.h"
#include "ShadowsPass.h"
#include "Engine/Core/Math/Matrix3x3.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Utilities/RectPack.h"

// This must match HLSL
#define GLOBAL_SURFACE_ATLAS_OBJECT_SIZE (5 + 6 * 5) // Amount of float4s per-object
#define GLOBAL_SURFACE_ATLAS_TILE_PADDING 1 // 1px padding to prevent color bleeding between tiles
#define GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET 0.1f // Small offset to prevent clipping with the closest triangles (shifts near and far planes)
#define GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES 0 // Forces to redraw all object tiles every frame
#define GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS 0 // Debug draws object bounds on redraw (and tile draw projection locations)

#if GLOBAL_SURFACE_ATLAS_DEBUG_DRAW_OBJECTS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct Data0
    {
    Vector3 ViewWorldPos;
    float ViewNearPlane;
    Vector2 Padding00;
    float LightShadowsStrength;
    float ViewFarPlane;
    Vector4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::GlobalSDFData GlobalSDF;
    GlobalSurfaceAtlasPass::GlobalSurfaceAtlasData GlobalSurfaceAtlas;
    LightData Light;
    });

PACK_STRUCT(struct AtlasTileVertex
    {
    Half2 Position;
    Half2 TileUV;
    uint16 ObjectIndex;
    uint16 TileIndex;
    });

struct GlobalSurfaceAtlasTile : RectPack<GlobalSurfaceAtlasTile, uint16>
{
    Vector3 ViewDirection;
    Vector3 ViewPosition;
    Vector3 ViewBoundsSize;
    Matrix ViewMatrix;

    GlobalSurfaceAtlasTile(uint16 x, uint16 y, uint16 width, uint16 height)
        : RectPack<GlobalSurfaceAtlasTile, uint16>(x, y, width, height)
    {
    }

    void OnInsert(class GlobalSurfaceAtlasCustomBuffer* buffer, Actor* actor, int32 tileIndex);

    void OnFree()
    {
    }
};

struct GlobalSurfaceAtlasObject
{
    uint64 LastFrameUsed;
    uint64 LastFrameDirty;
    GlobalSurfaceAtlasTile* Tiles[6];
    uint32 Index;
    float Radius;
    OrientedBoundingBox Bounds;

    GlobalSurfaceAtlasObject()
    {
        Platform::MemoryClear(this, sizeof(GlobalSurfaceAtlasObject));
    }

    GlobalSurfaceAtlasObject(const GlobalSurfaceAtlasObject& other)
    {
        Platform::MemoryCopy(this, &other, sizeof(GlobalSurfaceAtlasObject));
    }

    GlobalSurfaceAtlasObject(GlobalSurfaceAtlasObject&& other) noexcept
    {
        Platform::MemoryCopy(this, &other, sizeof(GlobalSurfaceAtlasObject));
    }

    GlobalSurfaceAtlasObject& operator=(const GlobalSurfaceAtlasObject& other)
    {
        Platform::MemoryCopy(this, &other, sizeof(GlobalSurfaceAtlasObject));
        return *this;
    }

    GlobalSurfaceAtlasObject& operator=(GlobalSurfaceAtlasObject&& other) noexcept
    {
        Platform::MemoryCopy(this, &other, sizeof(GlobalSurfaceAtlasObject));
        return *this;
    }
};

class GlobalSurfaceAtlasCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    int32 Resolution = 0;
    uint64 LastFrameAtlasInsertFail = 0;
    uint64 LastFrameAtlasDefragmentation = 0;
    GPUTexture* AtlasDepth = nullptr;
    GPUTexture* AtlasEmissive = nullptr;
    GPUTexture* AtlasGBuffer0 = nullptr;
    GPUTexture* AtlasGBuffer1 = nullptr;
    GPUTexture* AtlasGBuffer2 = nullptr;
    GPUTexture* AtlasDirectLight = nullptr;
    DynamicTypedBuffer ObjectsBuffer;
    uint32 ObjectIndexCounter;
    GlobalSurfaceAtlasPass::BindingData Result;
    GlobalSurfaceAtlasTile* AtlasTiles = nullptr; // TODO: optimize with a single allocation for atlas tiles
    Dictionary<Actor*, GlobalSurfaceAtlasObject> Objects;

    GlobalSurfaceAtlasCustomBuffer()
        : ObjectsBuffer(256 * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE, PixelFormat::R32G32B32A32_Float, false, TEXT("GlobalSurfaceAtlas.ObjectsBuffer"))
    {
    }

    FORCE_INLINE void ClearObjects()
    {
        LastFrameAtlasDefragmentation = Engine::FrameCount;
        SAFE_DELETE(AtlasTiles);
        ObjectsBuffer.Clear();
        Objects.Clear();
    }

    FORCE_INLINE void Clear()
    {
        RenderTargetPool::Release(AtlasDepth);
        RenderTargetPool::Release(AtlasEmissive);
        RenderTargetPool::Release(AtlasGBuffer0);
        RenderTargetPool::Release(AtlasGBuffer1);
        RenderTargetPool::Release(AtlasGBuffer2);
        RenderTargetPool::Release(AtlasDirectLight);
        ClearObjects();
    }

    ~GlobalSurfaceAtlasCustomBuffer()
    {
        Clear();
    }
};

void GlobalSurfaceAtlasTile::OnInsert(GlobalSurfaceAtlasCustomBuffer* buffer, Actor* actor, int32 tileIndex)
{
    buffer->Objects[actor].Tiles[tileIndex] = this;
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
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GlobalSurfaceAtlas"));
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

    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug)
    {
        _psDebug = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Debug");
        if (_psDebug->Init(psDesc))
            return true;
    }
    if (!_psClear)
    {
        _psClear = device->CreatePipelineState();
        psDesc.DepthTestEnable = true;
        psDesc.DepthWriteEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        psDesc.VS = shader->GetVS("VS_Atlas");
        psDesc.PS = shader->GetPS("PS_Clear");
        if (_psClear->Init(psDesc))
            return true;
    }
    if (!_psDirectLighting0)
    {
        _psDirectLighting0 = device->CreatePipelineState();
        psDesc.DepthTestEnable = false;
        psDesc.DepthWriteEnable = false;
        psDesc.DepthFunc = ComparisonFunc::Never;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.PS = shader->GetPS("PS_DirectLighting", 0);
        if (_psDirectLighting0->Init(psDesc))
            return true;
        _psDirectLighting1 = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_DirectLighting", 1);
        if (_psDirectLighting1->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void GlobalSurfaceAtlasPass::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting0);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting1);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    invalidateResources();
}

#endif

void GlobalSurfaceAtlasPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE(_vertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting0);
    SAFE_DELETE_GPU_RESOURCE(_psDirectLighting1);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
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
    PROFILE_GPU_CPU("Global Surface Atlas");

    // TODO: configurable via graphics settings
    const int32 resolution = 2048;
    const float resolutionInv = 1.0f / resolution;
    // TODO: configurable via postFx settings (maybe use Global SDF distance?)
    const float distance = 20000;

    // Initialize buffers
    bool noCache = surfaceAtlasData.Resolution != resolution;
    if (noCache)
    {
        surfaceAtlasData.Clear();

        auto desc = GPUTextureDescription::New2D(resolution, resolution, PixelFormat::Unknown);
        uint64 memUsage = 0;
        // TODO: try using BC4/BC5/BC7 block compression for Surface Atlas (eg. for Tiles material properties)
#define INIT_ATLAS_TEXTURE(texture, format) desc.Format = format; surfaceAtlasData.texture = RenderTargetPool::Get(desc); if (!surfaceAtlasData.texture) return true; memUsage += surfaceAtlasData.texture->GetMemoryUsage()
        INIT_ATLAS_TEXTURE(AtlasEmissive, LIGHT_BUFFER_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasGBuffer0, GBUFFER0_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasGBuffer1, GBUFFER1_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasGBuffer2, GBUFFER2_FORMAT);
        INIT_ATLAS_TEXTURE(AtlasDirectLight, LIGHT_BUFFER_FORMAT);
        desc.Flags = GPUTextureFlags::DepthStencil | GPUTextureFlags::ShaderResource;
        INIT_ATLAS_TEXTURE(AtlasDepth, PixelFormat::D16_UNorm);
#undef INIT_ATLAS_TEXTURE
        surfaceAtlasData.Resolution = resolution;
        LOG(Info, "Global Surface Atlas resolution: {0}, memory usage: {1} MB", resolution, memUsage / 1024 / 1024);
    }
    else
    {
        // Perform atlas defragmentation if needed
        // TODO: track atlas used vs free ratio to skip defragmentation if it's nearly full (then maybe auto resize up?)
        if (currentFrame - surfaceAtlasData.LastFrameAtlasInsertFail < 10 &&
            currentFrame - surfaceAtlasData.LastFrameAtlasDefragmentation > 60)
        {
            surfaceAtlasData.ClearObjects();
        }
    }
    if (!surfaceAtlasData.AtlasTiles)
        surfaceAtlasData.AtlasTiles = New<GlobalSurfaceAtlasTile>(0, 0, resolution, resolution);
    if (!_vertexBuffer)
        _vertexBuffer = New<DynamicVertexBuffer>(0u, (uint32)sizeof(AtlasTileVertex), TEXT("GlobalSurfaceAtlas.VertexBuffer"));
    const Vector2 posToClipMul(2.0f * resolutionInv, -2.0f * resolutionInv);
    const Vector2 posToClipAdd(-1.0f, 1.0f);
#define VB_WRITE_TILE_POS_ONLY(tile) \
        Vector2 minPos((float)tile->X, (float)tile->Y), maxPos((float)(tile->X + tile->Width), (float)(tile->Y + tile->Height)); \
        Half2 min(minPos * posToClipMul + posToClipAdd), max(maxPos * posToClipMul + posToClipAdd); \
        auto* quad = _vertexBuffer->WriteReserve<AtlasTileVertex>(6); \
        quad[0].Position  = max; \
        quad[1].Position = { min.X, max.Y }; \
        quad[2].Position = min; \
        quad[3].Position = quad[2].Position; \
        quad[4].Position = { max.X, min.Y }; \
        quad[5].Position = quad[0].Position
#define VB_WRITE_TILE(tile) \
        Vector2 minPos((float)tile->X, (float)tile->Y), maxPos((float)(tile->X + tile->Width), (float)(tile->Y + tile->Height)); \
        Half2 min(minPos * posToClipMul + posToClipAdd), max(maxPos * posToClipMul + posToClipAdd); \
        Vector2 minUV(0, 0), maxUV(1, 1); \
        auto* quad = _vertexBuffer->WriteReserve<AtlasTileVertex>(6); \
        quad[0] = { { max }, { maxUV }, (uint16)object.Index, (uint16)tileIndex }; \
        quad[1] = { { min.X, max.Y }, { minUV.X, maxUV.Y }, (uint16)object.Index, (uint16)tileIndex }; \
        quad[2] = { { min }, { minUV }, (uint16)object.Index, (uint16)tileIndex }; \
        quad[3] = quad[2]; \
        quad[4] = { { max.X, min.Y }, { maxUV.X, minUV.Y }, (uint16)object.Index, (uint16)tileIndex }; \
        quad[5] = quad[0]
#define VB_DRAW() \
        _vertexBuffer->Flush(context); \
        auto vb = _vertexBuffer->GetBuffer(); \
        context->BindVB(ToSpan(&vb, 1)); \
        context->DrawInstanced(_vertexBuffer->Data.Count() / sizeof(AtlasTileVertex), 1);
    // Add objects into the atlas
    surfaceAtlasData.ObjectsBuffer.Clear();
    surfaceAtlasData.ObjectIndexCounter = 0;
    _dirtyObjectsBuffer.Clear();
    {
        PROFILE_CPU_NAMED("Draw");
        const uint32 viewMask = renderContext.View.RenderLayersMask;
        const Vector3 viewPosition = renderContext.View.Position;
        const uint16 minTileResolution = 8; // Minimum size (in texels) of the tile in atlas
        const uint16 maxTileResolution = 128; // Maximum size (in texels) of the tile in atlas
        const uint16 tileResolutionAlignment = 8; // Alignment to snap (down) tiles resolution which allows to reuse atlas slots once object gets resizes/replaced by other object
        const float minObjectRadius = 20.0f; // Skip too small objects
        const float tileTexelsPerWorldUnit = 1.0f / 10.0f; // Scales the tiles resolution
        const float distanceScalingStart = 2000.0f; // Distance from camera at which the tiles resolution starts to be scaled down
        const float distanceScalingEnd = 5000.0f; // Distance from camera at which the tiles resolution end to be scaled down
        const float distanceScaling = 0.1f; // The scale for tiles at distanceScalingEnd and further away
        static_assert(GLOBAL_SURFACE_ATLAS_TILE_PADDING < minTileResolution, "Invalid tile size configuration.");
        for (auto* scene : renderContext.List->Scenes)
        {
            // TODO: optimize for static objects (SceneRendering could have separate and optimized caching for static actors)
            for (auto& e : scene->Actors)
            {
                if (viewMask & e.LayerMask && e.Bounds.Radius >= minObjectRadius && CollisionsHelper::DistanceSpherePoint(e.Bounds, viewPosition) < distance)
                {
                    // TODO: move into actor-specific Draw() impl (eg. via GlobalSurfaceAtlas pass)
                    auto* staticModel = ScriptingObject::Cast<StaticModel>(e.Actor);
                    if (staticModel && staticModel->Model && staticModel->Model->IsLoaded() && staticModel->Model->CanBeRendered())
                    {
                        Matrix localToWorld;
                        staticModel->GetWorld(&localToWorld);
                        bool anyTile = false, dirty = false;
                        GlobalSurfaceAtlasObject* object = surfaceAtlasData.Objects.TryGet(e.Actor);
                        auto& lod = staticModel->Model->LODs.Last();
                        BoundingBox localBounds = lod.GetBox();
                        Vector3 boundsSize = localBounds.GetSize() * staticModel->GetScale();
                        const float distanceScale = Math::Lerp(1.0f, distanceScaling, Math::InverseLerp(distanceScalingStart, distanceScalingEnd, CollisionsHelper::DistanceSpherePoint(e.Bounds, viewPosition)));
                        const float tilesScale = tileTexelsPerWorldUnit * distanceScale;
                        for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                        {
                            // Calculate optimal tile resolution for the object side
                            Vector3 boundsSizeTile = boundsSize;
                            boundsSizeTile.Raw[tileIndex / 2] = MAX_float; // Ignore depth size
                            boundsSizeTile.Absolute();
                            uint16 tileResolution = (uint16)(boundsSizeTile.MinValue() * tilesScale);
                            if (tileResolution < 4)
                            {
                                // Skip too small surfaces
                                if (object && object->Tiles[tileIndex])
                                {
                                    object->Tiles[tileIndex]->Free();
                                    object->Tiles[tileIndex] = nullptr;
                                }
                                continue;
                            }

                            // Clamp and snap to reduce atlas fragmentation
                            tileResolution = Math::Clamp(tileResolution, minTileResolution, maxTileResolution);
                            tileResolution = Math::AlignDown(tileResolution, tileResolutionAlignment);

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
                                object->Tiles[tileIndex]->Free();
                            }

                            // Insert tile into atlas
                            auto* tile = surfaceAtlasData.AtlasTiles->Insert(tileResolution, tileResolution, 0, &surfaceAtlasData, e.Actor, tileIndex);
                            if (tile)
                            {
                                if (!object)
                                    object = &surfaceAtlasData.Objects[e.Actor];
                                object->Tiles[tileIndex] = tile;
                                anyTile = true;
                                dirty = true;
                            }
                            else
                            {
                                if (object)
                                    object->Tiles[tileIndex] = nullptr;
                                surfaceAtlasData.LastFrameAtlasInsertFail = currentFrame;
                            }
                        }
                        if (anyTile)
                        {
                            // Redraw objects from time-to-time (dynamic objects can be animated, static objects can have textures streamed)
                            uint32 redrawFramesCount = staticModel->HasStaticFlag(StaticFlags::Lightmap) ? 120 : 4;
                            if (currentFrame - object->LastFrameDirty >= (redrawFramesCount + (e.Actor->GetID().D & redrawFramesCount)))
                                dirty = true;

                            // Mark object as used
                            object->LastFrameUsed = currentFrame;
                            object->Bounds = OrientedBoundingBox(localBounds);
                            object->Bounds.Transform(localToWorld);
                            object->Radius = e.Bounds.Radius;
                            if (dirty || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_REDRAW_TILES)
                            {
                                object->LastFrameDirty = currentFrame;
                                _dirtyObjectsBuffer.Add(e.Actor);
                            }

                            // Write to objects buffer (this must match unpacking logic in HLSL)
                            Matrix worldToLocalBounds;
                            Matrix::Invert(object->Bounds.Transformation, worldToLocalBounds);
                            // TODO: cache data for static objects to optimize CPU perf (move ObjectsBuffer into surfaceAtlasData)
                            object->Index = surfaceAtlasData.ObjectIndexCounter++;
                            auto* objectData = surfaceAtlasData.ObjectsBuffer.WriteReserve<Vector4>(GLOBAL_SURFACE_ATLAS_OBJECT_SIZE);
                            objectData[0] = *(Vector4*)&e.Bounds;
                            objectData[1] = Vector4(worldToLocalBounds.M11, worldToLocalBounds.M12, worldToLocalBounds.M13, worldToLocalBounds.M41);
                            objectData[2] = Vector4(worldToLocalBounds.M21, worldToLocalBounds.M22, worldToLocalBounds.M23, worldToLocalBounds.M42);
                            objectData[3] = Vector4(worldToLocalBounds.M31, worldToLocalBounds.M32, worldToLocalBounds.M33, worldToLocalBounds.M43);
                            objectData[4] = Vector4(object->Bounds.Extents, 0.0f);
                            // TODO: try to optimize memory footprint (eg. merge scale into extents and use rotation+offset but reconstruct rotation from two axes with sign)
                            for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                            {
                                auto* tile = object->Tiles[tileIndex];
                                const int32 tileStart = 5 + tileIndex * 5;
                                if (!tile)
                                {
                                    // Disable tile
                                    objectData[tileStart + 4] = Vector4::Zero;
                                    continue;
                                }

                                // Setup view to render object from the side
                                Vector3 xAxis, yAxis, zAxis = Vector3::Zero;
                                zAxis.Raw[tileIndex / 2] = tileIndex & 1 ? 1.0f : -1.0f;
                                yAxis = tileIndex == 2 || tileIndex == 3 ? Vector3::Right : Vector3::Up;
                                Vector3::Cross(yAxis, zAxis, xAxis);
                                Vector3 localSpaceOffset = -zAxis * object->Bounds.Extents;
                                Vector3::TransformNormal(xAxis, object->Bounds.Transformation, xAxis);
                                Vector3::TransformNormal(yAxis, object->Bounds.Transformation, yAxis);
                                Vector3::TransformNormal(zAxis, object->Bounds.Transformation, zAxis);
                                xAxis.NormalizeFast();
                                yAxis.NormalizeFast();
                                zAxis.NormalizeFast();
                                Vector3::Transform(localSpaceOffset, object->Bounds.Transformation, tile->ViewPosition);
                                tile->ViewDirection = zAxis;

                                // Create view matrix
                                tile->ViewMatrix.SetColumn1(Vector4(xAxis, -Vector3::Dot(xAxis, tile->ViewPosition)));
                                tile->ViewMatrix.SetColumn2(Vector4(yAxis, -Vector3::Dot(yAxis, tile->ViewPosition)));
                                tile->ViewMatrix.SetColumn3(Vector4(zAxis, -Vector3::Dot(zAxis, tile->ViewPosition)));
                                tile->ViewMatrix.SetColumn4(Vector4(0, 0, 0, 1));

                                // Calculate object bounds size in the view
                                OrientedBoundingBox viewBounds(object->Bounds);
                                viewBounds.Transform(tile->ViewMatrix);
                                Vector3 viewExtent;
                                Vector3::TransformNormal(viewBounds.Extents, viewBounds.Transformation, viewExtent);
                                tile->ViewBoundsSize = viewExtent.GetAbsolute() * 2.0f;

                                // Per-tile data
                                const float tileWidth = (float)tile->Width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
                                const float tileHeight = (float)tile->Height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
                                objectData[tileStart + 0] = Vector4(tile->X, tile->Y, tileWidth, tileHeight) * resolutionInv;
                                objectData[tileStart + 1] = Vector4(tile->ViewMatrix.M11, tile->ViewMatrix.M12, tile->ViewMatrix.M13, tile->ViewMatrix.M41);
                                objectData[tileStart + 2] = Vector4(tile->ViewMatrix.M21, tile->ViewMatrix.M22, tile->ViewMatrix.M23, tile->ViewMatrix.M42);
                                objectData[tileStart + 3] = Vector4(tile->ViewMatrix.M31, tile->ViewMatrix.M32, tile->ViewMatrix.M33, tile->ViewMatrix.M43);
                                objectData[tileStart + 4] = Vector4(tile->ViewBoundsSize, 1.0f);
                            }
                        }
                    }
                }
            }
        }
    }

    // Remove unused objects
    for (auto it = surfaceAtlasData.Objects.Begin(); it.IsNotEnd(); ++it)
    {
        if (it->Value.LastFrameUsed != currentFrame)
        {
            for (auto& tile : it->Value.Tiles)
            {
                if (tile)
                    tile->Free();
            }
            surfaceAtlasData.Objects.Remove(it);
        }
    }

    // Send objects data to the GPU
    {
        PROFILE_GPU_CPU("Update Objects");
        surfaceAtlasData.ObjectsBuffer.Flush(context);
    }

    // Rasterize world geometry material properties into Global Surface Atlas
    if (_dirtyObjectsBuffer.Count() != 0)
    {
        PROFILE_GPU_CPU("Rasterize Tiles");

        RenderContext renderContextTiles = renderContext;
        renderContextTiles.List = RenderList::GetFromPool();
        renderContextTiles.View.Pass = DrawPass::GBuffer;
        renderContextTiles.View.Mode = ViewMode::Default;
        renderContextTiles.View.ModelLODBias += 100000;
        renderContextTiles.View.ShadowModelLODBias += 100000;
        renderContextTiles.View.IsSingleFrame = true;
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
            PROFILE_GPU_CPU("Clear");
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
                for (Actor* actor : _dirtyObjectsBuffer)
                {
                    const auto& object = ((const Dictionary<Actor*, GlobalSurfaceAtlasObject>&)surfaceAtlasData.Objects)[actor];
                    for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                    {
                        auto* tile = object.Tiles[tileIndex];
                        if (!tile)
                            continue;
                        VB_WRITE_TILE_POS_ONLY(tile);
                    }
                }
                context->SetState(_psClear);
                context->SetViewportAndScissors(Viewport(0, 0, resolution, resolution));
                VB_DRAW();
            }
        }
        // TODO: limit dirty objects count on a first frame (eg. collect overflown objects to be redirty next frame)
        auto& drawCallsListGBuffer = renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer];
        auto& drawCallsListGBufferNoDecals = renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals];
        drawCallsListGBuffer.CanUseInstancing = false;
        drawCallsListGBufferNoDecals.CanUseInstancing = false;
        for (Actor* actor : _dirtyObjectsBuffer)
        {
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
            actor->Draw(renderContextTiles);

            // Render all tiles into the atlas
            const auto& object = ((const Dictionary<Actor*, GlobalSurfaceAtlasObject>&)surfaceAtlasData.Objects)[actor];
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
            }
        }
        context->ResetRenderTarget();
        RenderList::ReturnToPool(renderContextTiles.List);
    }

    // Copy results
    result.Atlas[0] = surfaceAtlasData.AtlasDepth;
    result.Atlas[1] = surfaceAtlasData.AtlasGBuffer0;
    result.Atlas[2] = surfaceAtlasData.AtlasGBuffer1;
    result.Atlas[3] = surfaceAtlasData.AtlasGBuffer2;
    result.Atlas[4] = surfaceAtlasData.AtlasDirectLight;
    result.Objects = surfaceAtlasData.ObjectsBuffer.GetBuffer();
    result.GlobalSurfaceAtlas.Resolution = (float)resolution;
    result.GlobalSurfaceAtlas.ObjectsCount = surfaceAtlasData.Objects.Count();
    surfaceAtlasData.Result = result;

    // Render direct lighting into atlas
    if (surfaceAtlasData.Objects.Count() != 0)
    {
        PROFILE_GPU_CPU("Direct Lighting");

        // Copy emissive light into the final direct lighting atlas
        // TODO: test perf diff when manually copying only dirty object tiles and dirty light tiles
        {
            PROFILE_GPU_CPU("Copy Emissive");
            context->CopyTexture(surfaceAtlasData.AtlasDirectLight, 0, 0, 0, 0, surfaceAtlasData.AtlasEmissive, 0);
        }

        context->SetViewportAndScissors(Viewport(0, 0, resolution, resolution));
        context->SetRenderTarget(surfaceAtlasData.AtlasDirectLight->View());
        context->BindSR(0, surfaceAtlasData.AtlasGBuffer0->View());
        context->BindSR(1, surfaceAtlasData.AtlasGBuffer1->View());
        context->BindSR(2, surfaceAtlasData.AtlasGBuffer2->View());
        context->BindSR(3, surfaceAtlasData.AtlasDepth->View());
        context->BindSR(4, surfaceAtlasData.ObjectsBuffer.GetBuffer()->View());
        for (int32 i = 0; i < 4; i++)
        {
            context->BindSR(i + 5, bindingDataSDF.Cascades[i]->ViewVolume());
            context->BindSR(i + 9, bindingDataSDF.CascadeMips[i]->ViewVolume());
        }
        context->BindCB(0, _cb0);
        Data0 data;
        data.ViewWorldPos = renderContext.View.Position;
        data.GlobalSDF = bindingDataSDF.GlobalSDF;
        data.GlobalSurfaceAtlas = result.GlobalSurfaceAtlas;

        // Shade object tiles influenced by lights to calculate direct lighting
        // TODO: reduce redraw frequency for static lights (StaticFlags::Lightmap)
        for (auto& light : renderContext.List->DirectionalLights)
        {
            // Collect tiles to shade
            _vertexBuffer->Clear();
            for (const auto& e : surfaceAtlasData.Objects)
            {
                const auto& object = e.Value;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile || Vector3::Dot(tile->ViewDirection, light.Direction) < ZeroTolerance)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }

            // Draw draw light
            const bool useShadow = CanRenderShadow(renderContext.View, light);
            // TODO: test perf/quality when using Shadow Map for directional light (ShadowsPass::Instance()->LastDirLightShadowMap) instead of Global SDF trace
            light.SetupLightData(&data.Light, useShadow);
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
                Vector3 lightToObject = object.Bounds.GetCenter() - light.Position;
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

            // Draw draw light
            const bool useShadow = CanRenderShadow(renderContext.View, light);
            light.SetupLightData(&data.Light, useShadow);
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
                Vector3 lightToObject = object.Bounds.GetCenter() - light.Position;
                if (lightToObject.LengthSquared() >= Math::Square(object.Radius + light.Radius))
                    continue;
                for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                {
                    auto* tile = object.Tiles[tileIndex];
                    if (!tile || Vector3::Dot(tile->ViewDirection, light.Direction) < ZeroTolerance)
                        continue;
                    VB_WRITE_TILE(tile);
                }
            }

            // Draw draw light
            const bool useShadow = CanRenderShadow(renderContext.View, light);
            light.SetupLightData(&data.Light, useShadow);
            data.LightShadowsStrength = 1.0f - light.ShadowsStrength;
            context->UpdateCB(_cb0, &data);
            context->SetState(_psDirectLighting1);
            VB_DRAW();
        }

        context->ResetRenderTarget();
    }

    // TODO: indirect lighting apply to get infinite bounces for GI

    // TODO: explore atlas tiles optimization with feedback from renderer (eg. when tile is sampled by GI/Reflections mark it as used, then sort tiles by importance and prioritize updates for ones frequently used)

#undef WRITE_TILE
    return false;
}

void GlobalSurfaceAtlasPass::RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output)
{
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    BindingData bindingData;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF) || Render(renderContext, context, bindingData))
    {
        context->Draw(output, renderContext.Buffers->GBuffer0);
        return;
    }

    PROFILE_GPU_CPU("Global Surface Atlas Debug");
    const Vector2 outputSize(output->Size());
    {
        Data0 data;
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        for (int32 i = 0; i < 4; i++)
            data.ViewFrustumWorldRays[i] = Vector4(renderContext.List->FrustumCornersWs[i + 4], 0);
        data.GlobalSDF = bindingDataSDF.GlobalSDF;
        data.GlobalSurfaceAtlas = bindingData.GlobalSurfaceAtlas;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }
    for (int32 i = 0; i < 4; i++)
    {
        context->BindSR(i, bindingDataSDF.Cascades[i]->ViewVolume());
        context->BindSR(i + 4, bindingDataSDF.CascadeMips[i]->ViewVolume());
    }
    context->BindSR(8, bindingData.Objects ? bindingData.Objects->View() : nullptr);
    context->BindSR(9, bindingData.Atlas[0]->View());
    {
        //GPUTexture* tex = bindingData.Atlas[1]; // Preview diffuse
        //GPUTexture* tex = bindingData.Atlas[2]; // Preview normals
        //GPUTexture* tex = bindingData.Atlas[3]; // Preview roughness/metalness/ao
        GPUTexture* tex = bindingData.Atlas[4]; // Preview direct light
        context->BindSR(10, tex->View());
    }
    context->SetState(_psDebug);
    context->SetRenderTarget(output->View());
    context->SetViewportAndScissors(outputSize.X, outputSize.Y);
    context->DrawFullscreenTriangle();
}
