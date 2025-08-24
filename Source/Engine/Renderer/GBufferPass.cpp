// Copyright (c) Wojciech Figat. All rights reserved.

#include "GBufferPass.h"
#include "RenderList.h"
#if USE_EDITOR
#include "Engine/Renderer/Editor/VertexColors.h"
#include "Engine/Renderer/Editor/LightmapUVsDensity.h"
#include "Engine/Renderer/Editor/LODPreview.h"
#include "Engine/Renderer/Editor/MaterialComplexity.h"
#endif
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Level/Actors/Decal.h"
#include "Engine/Engine/Engine.h"

GPU_CB_STRUCT(GBufferPassData {
    ShaderGBufferData GBuffer;
    Float3 Dummy0;
    int32 ViewMode;
    });

#if USE_EDITOR
Dictionary<GPUBuffer*, const ModelLOD*> GBufferPass::IndexBufferToModelLOD;
CriticalSection GBufferPass::Locker;
#endif

String GBufferPass::ToString() const
{
    return TEXT("GBufferPass");
}

bool GBufferPass::Init()
{
    // Create pipeline state
    _psDebug = GPUDevice::Instance->CreatePipelineState();

    // Load assets
    _gBufferShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GBuffer"));
    _skyModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    _boxModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/SimpleBox"));
    if (_gBufferShader == nullptr || _skyModel == nullptr || _boxModel == nullptr)
    {
        return true;
    }
#if COMPILE_WITH_DEV_ENV
    _gBufferShader.Get()->OnReloading.Bind<GBufferPass, &GBufferPass::OnShaderReloading>(this);
#endif

    return false;
}

bool GBufferPass::setupResources()
{
    if (!_gBufferShader || !_gBufferShader->IsLoaded())
        return true;
    auto gbuffer = _gBufferShader->GetShader();

    // Validate shader constant buffers sizes
    if (gbuffer->GetCB(0)->GetSize() != sizeof(GBufferPassData))
    {
        LOG(Warning, "GBuffer shader has incorrct constant buffers sizes.");
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug->IsValid())
    {
        psDesc.PS = gbuffer->GetPS("PS_DebugView");
        if (_psDebug->Init(psDesc))
            return true;
    }

    return false;
}

void GBufferPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _gBufferShader = nullptr;
    _skyModel = nullptr;
    _boxModel = nullptr;
#if USE_EDITOR
    SAFE_DELETE(_lightmapUVsDensity);
    SAFE_DELETE(_vertexColors);
    SAFE_DELETE(_lodPreview);
    SAFE_DELETE(_materialComplexity);
    IndexBufferToModelLOD.SetCapacity(0);
#endif
}

#if USE_EDITOR

void DebugOverrideDrawCallsMaterial(const RenderContext& renderContext, IMaterial* material)
{
    if (!material->IsReady())
        return;
    PROFILE_CPU();
    IMaterial::InstancingHandler handler;
    const bool canUseInstancing = material->CanUseInstancing(handler);
    const auto drawModes = material->GetDrawModes();
    if (EnumHasAnyFlags(drawModes, DrawPass::GBuffer))
    {
        auto& drawCallsList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer];
        for (int32 i : drawCallsList.Indices)
        {
            auto& drawCall = renderContext.List->DrawCalls.Get()[i];
            if (drawCall.Material->IsSurface())
            {
                drawCall.Material = material;
            }
        }
        drawCallsList.CanUseInstancing &= canUseInstancing;
    }
    if (EnumHasAnyFlags(drawModes, DrawPass::GBuffer))
    {
        auto& drawCallsList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals];
        for (int32 i : drawCallsList.Indices)
        {
            auto& drawCall = renderContext.List->DrawCalls.Get()[i];
            if (drawCall.Material->IsSurface())
            {
                drawCall.Material = material;
            }
        }
        drawCallsList.CanUseInstancing &= canUseInstancing;
    }
}

#endif

void GBufferPass::Fill(RenderContext& renderContext, GPUTexture* lightBuffer)
{
    PROFILE_GPU_CPU("GBuffer");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    GPUTextureView* targetBuffers[5] =
    {
        lightBuffer->View(),
        renderContext.Buffers->GBuffer0->View(),
        renderContext.Buffers->GBuffer1->View(),
        renderContext.Buffers->GBuffer2->View(),
        renderContext.Buffers->GBuffer3->View(),
    };
    renderContext.View.Pass = DrawPass::GBuffer;

    // Clear GBuffer
    {
        PROFILE_GPU_CPU_NAMED("Clear");

        context->ClearDepth(*renderContext.Buffers->DepthBuffer);
        context->Clear(lightBuffer->View(), Color::Transparent);
        context->Clear(renderContext.Buffers->GBuffer0->View(), Color::Transparent);
        context->Clear(renderContext.Buffers->GBuffer1->View(), Color::Transparent);
        context->Clear(renderContext.Buffers->GBuffer2->View(), Color(1, 0, 0, 0));
        context->Clear(renderContext.Buffers->GBuffer3->View(), Color::Transparent);
    }

    // Ensure to have valid data
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering.
        return;
    }

#if USE_EDITOR
    // Special debug drawing
    if (renderContext.View.Mode == ViewMode::MaterialComplexity)
    {
        // Initialize background with complexity of the sky (uniform)
        if (renderContext.List->Sky)
        {
            renderContext.List->Sky->ApplySky(context, renderContext, Matrix::Identity);
            GPUPipelineState* materialPs = context->GetState();
            const float complexity = (float)Math::Min(materialPs->Complexity, MATERIAL_COMPLEXITY_LIMIT) / MATERIAL_COMPLEXITY_LIMIT;
            context->Clear(lightBuffer->View(), Color(complexity, complexity, complexity, 1.0f));
            renderContext.List->Sky = nullptr;
        }
    }
    else if (renderContext.View.Mode == ViewMode::PhysicsColliders)
    {
        context->ResetRenderTarget();
        return;
    }
#endif

    // Draw objects that can get decals
    context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBuffer);

    // Draw decals
    DrawDecals(renderContext, lightBuffer->View());

    // Draw objects that cannot get decals
    context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBufferNoDecals);

    GPUTexture* nullTexture = nullptr;
    renderContext.List->RunCustomPostFxPass(context, renderContext, PostProcessEffectLocation::AfterGBufferPass, lightBuffer, nullTexture);

    // Draw sky
    if (renderContext.List->Sky && _skyModel && _skyModel->CanBeRendered() && EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Sky))
    {
        PROFILE_GPU_CPU_NAMED("Sky");
        context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
        DrawSky(renderContext, context);
    }

    context->ResetRenderTarget();
}

bool SortDecal(RenderDecalData const& a, RenderDecalData const& b)
{
    if (a.SortOrder == b.SortOrder)
    {
        return (uintptr)a.Material < (uintptr)b.Material;
    }
    return a.SortOrder < b.SortOrder;
}

void GBufferPass::RenderDebug(RenderContext& renderContext)
{
    // Check if has resources loaded
    if (checkIfSkipPass())
        return;

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto lights = _gBufferShader->GetShader();
    GBufferPassData data;

    // Set constants buffer
    SetInputs(renderContext.View, data.GBuffer);
    data.ViewMode = static_cast<int32>(renderContext.View.Mode);
    auto cb = lights->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Bind inputs
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);
    context->BindSR(4, renderContext.Buffers->GBuffer3);

    // Combine frame
    context->SetState(_psDebug);
    context->DrawFullscreenTriangle();

    // Cleanup
    context->ResetSR();
}

// Custom render buffer for realtime skybox capturing (eg. used by GI).
class SkyboxCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    uint64 LastCaptureFrame = 0;
    GPUTexture* Skybox = nullptr;

    ~SkyboxCustomBuffer()
    {
        RenderTargetPool::Release(Skybox);
    }
};

GPUTextureView* GBufferPass::RenderSkybox(RenderContext& renderContext, GPUContext* context)
{
    GPUTextureView* result = nullptr;
    if (renderContext.List->Sky && _skyModel && _skyModel->CanBeRendered() && EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Sky))
    {
        // Initialize skybox texture
        auto& skyboxData = *renderContext.Buffers->GetCustomBuffer<SkyboxCustomBuffer>(TEXT("Skybox"));
        skyboxData.LastFrameUsed = Engine::FrameCount;
        bool dirty = false;
        const int32 resolution = 16;
        if (!skyboxData.Skybox)
        {
            const auto desc = GPUTextureDescription::NewCube(resolution, PixelFormat::R11G11B10_Float);
            skyboxData.Skybox = RenderTargetPool::Get(desc);
            if (!skyboxData.Skybox)
                return nullptr;
            RENDER_TARGET_POOL_SET_NAME(skyboxData.Skybox, "GBuffer.Skybox");
            dirty = true;
        }

        // Redraw sky from time-to-time (dynamic skies can be animated, static skies can have textures streamed)
        const uint32 redrawFramesCount = renderContext.List->Sky->IsDynamicSky() ? 4 : 240;
        if (Engine::FrameCount - skyboxData.LastCaptureFrame >= redrawFramesCount)
            dirty = true;

        if (dirty)
        {
            PROFILE_GPU_CPU("Skybox");
            skyboxData.LastCaptureFrame = Engine::FrameCount;
            const RenderView originalView = renderContext.View;
            renderContext.View.Pass = DrawPass::GBuffer;
            renderContext.View.SetUpCube(10.0f, 10000.0f, originalView.Position);
            for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                renderContext.View.SetFace(faceIndex);
                context->SetRenderTarget(skyboxData.Skybox->View(faceIndex));
                context->SetViewportAndScissors(resolution, resolution);
                DrawSky(renderContext, context);
            }
            renderContext.View = originalView;
            context->ResetRenderTarget();
        }

        result = skyboxData.Skybox->ViewArray();
    }
    return result;
}

#if USE_EDITOR

void GBufferPass::PreOverrideDrawCalls(RenderContext& renderContext)
{
    // Clear cache before scene drawing
    IndexBufferToModelLOD.Clear();
}

void GBufferPass::OverrideDrawCalls(RenderContext& renderContext)
{
    // Override draw calls material to use material debug shader
    if (renderContext.View.Mode == ViewMode::LightmapUVsDensity)
    {
        if (!_lightmapUVsDensity)
            _lightmapUVsDensity = New<LightmapUVsDensityMaterialShader>();
        DebugOverrideDrawCallsMaterial(renderContext, _lightmapUVsDensity);
    }
    else if (renderContext.View.Mode == ViewMode::VertexColors)
    {
        if (!_vertexColors)
            _vertexColors = New<VertexColorsMaterialShader>();
        DebugOverrideDrawCallsMaterial(renderContext, _vertexColors);
    }
    else if (renderContext.View.Mode == ViewMode::LODPreview)
    {
        if (!_lodPreview)
            _lodPreview = New<LODPreviewMaterialShader>();
        DebugOverrideDrawCallsMaterial(renderContext, _lodPreview);
    }
    else if (renderContext.View.Mode == ViewMode::MaterialComplexity)
    {
        if (!_materialComplexity)
            _materialComplexity = New<MaterialComplexityMaterialShader>();
        _materialComplexity->DebugOverrideDrawCallsMaterial(renderContext);
    }
}

void GBufferPass::DrawMaterialComplexity(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer)
{
    _materialComplexity->Draw(renderContext, context, lightBuffer);
}

#endif

bool GBufferPass::IsDebugView(ViewMode mode)
{
    switch (mode)
    {
    case ViewMode::Unlit:
    case ViewMode::Diffuse:
    case ViewMode::Normals:
    case ViewMode::Depth:
    case ViewMode::AmbientOcclusion:
    case ViewMode::Metalness:
    case ViewMode::Roughness:
    case ViewMode::Specular:
    case ViewMode::SpecularColor:
    case ViewMode::SubsurfaceColor:
    case ViewMode::ShadingModel:
        return true;
    default:
        return false;
    }
}

void GBufferPass::SetInputs(const RenderView& view, ShaderGBufferData& gBuffer)
{
    // GBuffer params:
    // ViewInfo             :  x-1/Projection[0,0]   y-1/Projection[1,1]   z-(Far / (Far - Near)   w-(-Far * Near) / (Far - Near) / Far)
    // ScreenSize           :  x-Width               y-Height              z-1 / Width             w-1 / Height
    // ViewPos,ViewFar      :  x,y,z - world space view position                                   w-Far
    // InvViewMatrix        :  inverse view matrix (4 rows by 4 columns)
    // InvProjectionMatrix  :  inverse projection matrix (4 rows by 4 columns)
    gBuffer.ViewInfo = view.ViewInfo;
    gBuffer.ScreenSize = view.ScreenSize;
    gBuffer.ViewPos = view.Position;
    gBuffer.ViewFar = view.Far;
    Matrix::Transpose(view.IV, gBuffer.InvViewMatrix);
    Matrix::Transpose(view.IP, gBuffer.InvProjectionMatrix);
}

void GBufferPass::DrawSky(RenderContext& renderContext, GPUContext* context)
{
    // Cache data
    auto model = _skyModel.Get();
    auto box = model->GetBox();

    // Calculate sphere model transform to cover far plane
    Matrix m1, m2;
    Matrix::Scaling(renderContext.View.Far / ((float)box.GetSize().Y * 0.5f) * 0.95f, m1); // Scale to fit whole view frustum
    Matrix::CreateWorld(renderContext.View.Position, Float3::Up, Float3::Backward, m2); // Rotate sphere model
    m1 *= m2;

    // Draw sky
    renderContext.List->Sky->ApplySky(context, renderContext, m1);
    model->Render(context);
}

void GBufferPass::DrawDecals(RenderContext& renderContext, GPUTextureView* lightBuffer)
{
    auto& decals = renderContext.List->Decals;
    const auto boxModel = _boxModel.Get();
    if (decals.IsEmpty() || boxModel == nullptr || !boxModel->CanBeRendered() || EnumHasNoneFlags(renderContext.View.Flags, ViewFlags::Decals))
        return;
    PROFILE_GPU_CPU("Decals");
    auto context = GPUDevice::Instance->GetMainContext();
    auto buffers = renderContext.Buffers;
    GPUTextureView* depthBuffer = EnumHasAnyFlags(buffers->DepthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView) ? buffers->DepthBuffer->ViewReadOnlyDepth() : nullptr;

    // Sort decals from the lowest order to the highest order
    Sorting::QuickSort(decals.Get(), decals.Count(), &SortDecal);

    // Prepare
    DrawCall drawCall;
    MaterialBase::BindParameters bindParams(context, renderContext, drawCall);
    bindParams.BindViewData();
    MaterialDecalBlendingMode decalBlendingMode = (MaterialDecalBlendingMode)-1;
    MaterialUsageFlags usageFlags = (MaterialUsageFlags)-1;
    boxModel->LODs.Get()->Meshes.Get()->GetDrawCallGeometry(drawCall);
    context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3));
    context->BindIB(drawCall.Geometry.IndexBuffer);
    context->ResetRenderTarget();

    // Draw all decals
    for (int32 i = 0; i < decals.Count(); i++)
    {
        const RenderDecalData& decal = decals.Get()[i];

        // Bind output (skip if won't change in-between decals)
        const MaterialInfo& info = decal.Material->GetInfo();
        const MaterialUsageFlags infoUsageFlags = info.UsageFlags & (MaterialUsageFlags::UseEmissive | MaterialUsageFlags::UseNormal);
        if (decalBlendingMode != info.DecalBlendingMode || usageFlags != infoUsageFlags)
        {
            decalBlendingMode = info.DecalBlendingMode;
            usageFlags = infoUsageFlags;
            switch (decalBlendingMode)
            {
            case MaterialDecalBlendingMode::Translucent:
            {
                GPUTextureView* targetBuffers[4];
                int32 count = 2;
                targetBuffers[0] = buffers->GBuffer0->View();
                targetBuffers[1] = buffers->GBuffer2->View();
                if (EnumHasAnyFlags(usageFlags, MaterialUsageFlags::UseEmissive))
                {
                    count++;
                    targetBuffers[2] = lightBuffer;
                    if (EnumHasAnyFlags(usageFlags, MaterialUsageFlags::UseNormal))
                    {
                        count++;
                        targetBuffers[3] = buffers->GBuffer1->View();
                    }
                }
                else if (EnumHasAnyFlags(usageFlags, MaterialUsageFlags::UseNormal))
                {
                    count++;
                    targetBuffers[2] = buffers->GBuffer1->View();
                }
                context->SetRenderTarget(depthBuffer, ToSpan(targetBuffers, count));
                break;
            }
            case MaterialDecalBlendingMode::Stain:
            {
                context->SetRenderTarget(depthBuffer, buffers->GBuffer0->View());
                break;
            }
            case MaterialDecalBlendingMode::Normal:
            {
                context->SetRenderTarget(depthBuffer, buffers->GBuffer1->View());
                break;
            }
            case MaterialDecalBlendingMode::Emissive:
            {
                context->SetRenderTarget(depthBuffer, lightBuffer);
                break;
            }
            }
        }

        // Draw decal
        drawCall.World = decal.World;
        decal.Material->Bind(bindParams);
        // TODO: use hardware instancing
        context->DrawIndexed(drawCall.Draw.IndicesCount);
    }

    context->ResetSR();
}
