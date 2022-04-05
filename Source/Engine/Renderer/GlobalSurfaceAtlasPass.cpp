// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GlobalSurfaceAtlasPass.h"
#include "GlobalSignDistanceFieldPass.h"
#include "RenderList.h"
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
#define GLOBAL_SURFACE_ATLAS_OBJECT_SIZE (1)
#define GLOBAL_SURFACE_ATLAS_OBJECT_STRIDE (16 * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE)
#define GLOBAL_SURFACE_ATLAS_TILE_PADDING 1 // 1px padding to prevent color bleeding between tiles
#define GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_TILES_REDRAW 0 // Forces to redraw all object tiles every frame
#define GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_DRAW_OBJECTS 0 // Debug draws object bounds on redraw (and tile draw projection locations)

#if GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_DRAW_OBJECTS
#include "Engine/Debug/DebugDraw.h"
#endif

PACK_STRUCT(struct Data0
    {
    Vector3 ViewWorldPos;
    float ViewNearPlane;
    Vector3 Padding00;
    float ViewFarPlane;
    Vector4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::GlobalSDFData GlobalSDF;
    GlobalSurfaceAtlasPass::GlobalSurfaceAtlasData GlobalSurfaceAtlas;
    });

PACK_STRUCT(struct AtlasTileVertex
    {
    Half2 Position;
    });

struct GlobalSurfaceAtlasTile : RectPack<GlobalSurfaceAtlasTile, uint16>
{
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
    GlobalSurfaceAtlasTile* Tiles[6] = {};
    OrientedBoundingBox Bounds;
};

class GlobalSurfaceAtlasCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    int32 Resolution = 0;
    GPUTexture* AtlasDepth = nullptr;
    GPUTexture* AtlasGBuffer0 = nullptr;
    GPUTexture* AtlasGBuffer1 = nullptr;
    GPUTexture* AtlasGBuffer2 = nullptr;
    GPUTexture* AtlasDirectLight = nullptr;
    GlobalSurfaceAtlasPass::BindingData Result;
    GlobalSurfaceAtlasTile* AtlasTiles = nullptr; // TODO: optimize with a single allocation for atlas tiles
    Dictionary<Actor*, GlobalSurfaceAtlasObject> Objects;

    FORCE_INLINE void Clear()
    {
        RenderTargetPool::Release(AtlasDepth);
        RenderTargetPool::Release(AtlasGBuffer0);
        RenderTargetPool::Release(AtlasGBuffer1);
        RenderTargetPool::Release(AtlasGBuffer2);
        RenderTargetPool::Release(AtlasDirectLight);
        SAFE_DELETE(AtlasTiles);
        Objects.Clear();
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
        psDesc.VS = shader->GetVS("VS_Clear");
        psDesc.PS = shader->GetPS("PS_Clear");
        if (_psClear->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void GlobalSurfaceAtlasPass::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    invalidateResources();
}

#endif

void GlobalSurfaceAtlasPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE(_vertexBuffer);
    SAFE_DELETE(_objectsBuffer);
    SAFE_DELETE_GPU_RESOURCE(_psClear);
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _cb0 = nullptr;
    _shader = nullptr;
}

bool GlobalSurfaceAtlasPass::Render(RenderContext& renderContext, GPUContext* context, BindingData& result)
{
    // Skip if not supported
    if (setupResources())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& surfaceAtlasData = *renderContext.Buffers->GetCustomBuffer<GlobalSurfaceAtlasCustomBuffer>(TEXT("GlobalSurfaceAtlas"));

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
    const int32 resolution = 4096;
    // TODO: configurable via postFx settings (maybe use Global SDF distance?)
    const float distance = 20000;

    // Initialize buffers
    bool noCache = surfaceAtlasData.Resolution != resolution;
    if (noCache)
    {
        surfaceAtlasData.Clear();
        surfaceAtlasData.AtlasTiles = New<GlobalSurfaceAtlasTile>(0, 0, resolution, resolution);

        auto desc = GPUTextureDescription::New2D(resolution, resolution, PixelFormat::Unknown);
        uint64 memUsage = 0;
        // TODO: try using BC4/BC5/BC7 block compression for Surface Atlas (eg. for Tiles material properties)
#define INIT_ATLAS_TEXTURE(texture, format) desc.Format = format; surfaceAtlasData.texture = RenderTargetPool::Get(desc); if (!surfaceAtlasData.texture) return true; memUsage += surfaceAtlasData.texture->GetMemoryUsage()
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
    if (!_vertexBuffer)
        _vertexBuffer = New<DynamicVertexBuffer>(0u, (uint32)sizeof(AtlasTileVertex), TEXT("GlobalSurfaceAtlas.VertexBuffer"));

    // Add objects into the atlas
    if (_objectsBuffer)
        _objectsBuffer->Clear();
    else
        _objectsBuffer = New<DynamicTypedBuffer>(256 * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE, PixelFormat::R32G32B32A32_Float, false, TEXT("GlobalSurfaceAtlas.ObjectsBuffer"));
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
                        const bool staticLight = staticModel->HasStaticFlag(StaticFlags::Lightmap);
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
                            if (tileResolution < minTileResolution)
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
                                    if (!staticLight)
                                    {
                                        // TODO: collect dirty tile to be rasterized once every X frames
                                    }
                                    anyTile = true;
                                    continue;
                                }
                                object->Tiles[tileIndex]->Free();
                            }

                            // Insert tile into atlas
                            auto* tile = surfaceAtlasData.AtlasTiles->Insert(tileResolution, tileResolution, 0, &surfaceAtlasData, e.Actor, tileIndex);
                            // TODO: try to perform atlas defragmentation if it's full (eg. max once per ~10s)
                            if (tile)
                            {
                                if (!object)
                                    object = &surfaceAtlasData.Objects[e.Actor];
                                object->Tiles[tileIndex] = tile;
                                anyTile = true;
                                dirty = true;
                            }
                            else if (object)
                            {
                                object->Tiles[tileIndex] = nullptr;
                            }
                        }
                        if (anyTile)
                        {
                            // Mark object as used
                            object->LastFrameUsed = currentFrame;
                            object->Bounds = OrientedBoundingBox(localBounds);
                            object->Bounds.Transform(localToWorld);
                            if (dirty || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_TILES_REDRAW)
                                _dirtyObjectsBuffer.Add(ToPair(e.Actor, object));
                            // TODO: populate ObjectsBuffer with objects tiles data

                            // Write to objects buffer (this must match unpacking logic in HLSL)
                            // TODO: cache data for static objects to optimize CPU perf (move ObjectsBuffer into surfaceAtlasData)
                            Vector4 objectData[GLOBAL_SURFACE_ATLAS_OBJECT_SIZE];
                            objectData[0] = *(Vector4*)&e.Bounds;
                            _objectsBuffer->Write(objectData);
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
    // TODO: perform atlas defragmentation after certain amount of tiles removal

    // Send objects data to the GPU
    {
        PROFILE_GPU_CPU("Update Objects");
        // TODO: cache objects data in surfaceAtlasData to reduce memory transfer
        _objectsBuffer->Flush(context);
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
            surfaceAtlasData.AtlasDirectLight->View(),
            surfaceAtlasData.AtlasGBuffer0->View(),
            surfaceAtlasData.AtlasGBuffer1->View(),
            surfaceAtlasData.AtlasGBuffer2->View(),
        };
        context->SetRenderTarget(depthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
        {
            PROFILE_GPU_CPU("Clear");
            if (noCache || GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_TILES_REDRAW)
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
                const Vector2 posToClipMul(2.0f / resolution, -2.0f / resolution);
                const Vector2 posToClipAdd(-1.0f, 1.0f);
                for (const auto& e : _dirtyObjectsBuffer)
                {
                    const auto& object = *e.Second;
                    for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
                    {
                        auto* tile = object.Tiles[tileIndex];
                        if (!tile)
                            continue;
                        Vector2 minPos((float)tile->X, (float)tile->Y), maxPos((float)(tile->X + tile->Width), (float)(tile->Y + tile->Height));
                        Half2 min(minPos * posToClipMul + posToClipAdd), max(maxPos * posToClipMul + posToClipAdd);
                        auto* quad = _vertexBuffer->WriteReserve<AtlasTileVertex>(6);
                        quad[0] = { { max } };
                        quad[1] = { { min.X, max.Y } };
                        quad[2] = { { min } };
                        quad[3] = quad[2];
                        quad[4] = { { max.X, min.Y } };
                        quad[5] = quad[0];
                    }
                }
                _vertexBuffer->Flush(context);
                auto vb = _vertexBuffer->GetBuffer();
                context->SetState(_psClear);
                context->SetViewportAndScissors(Viewport(0, 0, resolution, resolution));
                context->BindVB(ToSpan(&vb, 1));
                context->DrawInstanced(_vertexBuffer->Data.Count() / sizeof(AtlasTileVertex), 1);
            }
        }
        renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer].CanUseInstancing = false;
        renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].CanUseInstancing = false;
        for (const auto& e : _dirtyObjectsBuffer)
        {
            renderContextTiles.List->Clear();
            renderContextTiles.List->DrawCalls.Clear();
            renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Clear();
            renderContextTiles.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Clear();

            // Fake projection matrix to disable Screen Size culling based on RenderTools::ComputeBoundsScreenRadiusSquared
            renderContextTiles.View.Projection.Values[0][0] = 10000.0f;

            // Collect draw calls for the object
            e.First->Draw(renderContextTiles);

            // Render all tiles into the atlas
            GlobalSurfaceAtlasObject& object = *e.Second;
#if GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_DRAW_OBJECTS
            DebugDraw::DrawBox(object.Bounds, Color::Red.AlphaMultiplied(0.4f));
#endif
            for (int32 tileIndex = 0; tileIndex < 6; tileIndex++)
            {
                auto* tile = object.Tiles[tileIndex];
                if (!tile)
                    continue;
                const float tileWidth = (float)tile->Width - GLOBAL_SURFACE_ATLAS_TILE_PADDING;
                const float tileHeight = (float)tile->Height - GLOBAL_SURFACE_ATLAS_TILE_PADDING;

                // Setup view to render object from the side
                Vector3 xAxis, yAxis, zAxis = Vector3::Zero;
                zAxis.Raw[tileIndex / 2] = tileIndex & 1 ? 1.0f : -1.0f;
                yAxis = tileIndex == 2 || tileIndex == 3 ? Vector3::Right : Vector3::Up;
                Vector3::Cross(yAxis, zAxis, xAxis);
                Vector3 localSpaceOffset = -zAxis * object.Bounds.Extents;
                Vector3::TransformNormal(xAxis, object.Bounds.Transformation, xAxis);
                Vector3::TransformNormal(yAxis, object.Bounds.Transformation, yAxis);
                Vector3::TransformNormal(zAxis, object.Bounds.Transformation, zAxis);
                xAxis.NormalizeFast();
                yAxis.NormalizeFast();
                zAxis.NormalizeFast();
                Vector3::Transform(localSpaceOffset, object.Bounds.Transformation, renderContextTiles.View.Position);
                renderContextTiles.View.Direction = zAxis;

                // Create view matrix
                Matrix viewMatrix;
                viewMatrix.SetColumn1(Vector4(xAxis, -Vector3::Dot(xAxis, renderContextTiles.View.Position)));
                viewMatrix.SetColumn2(Vector4(yAxis, -Vector3::Dot(yAxis, renderContextTiles.View.Position)));
                viewMatrix.SetColumn3(Vector4(zAxis, -Vector3::Dot(zAxis, renderContextTiles.View.Position)));
                viewMatrix.SetColumn4(Vector4(0, 0, 0, 1));

                // Calculate object bounds size in the view
                OrientedBoundingBox viewBounds(object.Bounds);
                viewBounds.Transform(viewMatrix);
                Vector3 viewExtent;
                Vector3::TransformNormal(viewBounds.Extents, viewBounds.Transformation, viewExtent);
                Vector3 viewBoundsSize = viewExtent.GetAbsolute() * 2.0f;

                // Setup projection to capture object from the side
                renderContextTiles.View.Near = -0.1f; // Small offset to prevent clipping with the closest triangles
                renderContextTiles.View.Far = viewBoundsSize.Z + 0.2f;
                Matrix projectionMatrix;
                Matrix::Ortho(viewBoundsSize.X, viewBoundsSize.Y, renderContextTiles.View.Near, renderContextTiles.View.Far, projectionMatrix);
                renderContextTiles.View.SetUp(viewMatrix, projectionMatrix);
#if GLOBAL_SURFACE_ATLAS_DEBUG_FORCE_DRAW_OBJECTS
                DebugDraw::DrawLine(renderContextTiles.View.Position, renderContextTiles.View.Position + renderContextTiles.View.Direction * 20.0f, Color::Orange);
                DebugDraw::DrawWireSphere(BoundingSphere(renderContextTiles.View.Position, 10.0f), Color::Green);
#endif

                // Draw
                context->SetViewportAndScissors(Viewport(tile->X, tile->Y, tileWidth, tileHeight));
                renderContextTiles.List->ExecuteDrawCalls(renderContextTiles, DrawCallsListType::GBuffer);
                renderContextTiles.List->ExecuteDrawCalls(renderContextTiles, DrawCallsListType::GBufferNoDecals);
            }
        }
        context->ResetRenderTarget();
        RenderList::ReturnToPool(renderContextTiles.List);
    }

    // TODO: update direct lighting atlas (for modified tiles and lights)
    // TODO: update static lights only for dirty tiles (dynamic lights every X frames)
    // TODO: use custom dynamic vertex buffer to decide which atlas tiles to shade with a light

    // TODO: indirect lighting apply to get infinite bounces for GI

    // Copy results
    result.Atlas[0] = surfaceAtlasData.AtlasDepth;
    result.Atlas[1] = surfaceAtlasData.AtlasGBuffer0;
    result.Atlas[2] = surfaceAtlasData.AtlasGBuffer1;
    result.Atlas[3] = surfaceAtlasData.AtlasGBuffer2;
    result.Atlas[4] = surfaceAtlasData.AtlasDirectLight;
    result.GlobalSurfaceAtlas.ObjectsCount = surfaceAtlasData.Objects.Count();
    surfaceAtlasData.Result = result;
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
    if (_cb0)
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
    context->BindSR(8, bindingData.Atlas[1]->View()); // TODO: pass Atlas[4]=AtlasDirectLight
    context->SetState(_psDebug);
    context->SetRenderTarget(output->View());
    context->SetViewportAndScissors(outputSize.X, outputSize.Y);
    context->DrawFullscreenTriangle();
}
