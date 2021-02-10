// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GBufferPass.h"
#include "Engine/Renderer/Editor/VertexColors.h"
#include "Engine/Renderer/Editor/LightmapUVsDensity.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Level/Actors/Decal.h"

PACK_STRUCT(struct GBufferPassData{
    GBufferData GBuffer;
    Vector3 Dummy0;
    int32 ViewMode;
    });

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
    ASSERT(_gBufferShader);

    // Check if shader has not been loaded
    if (!_gBufferShader->IsLoaded())
    {
        return true;
    }
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
    SAFE_DELETE(_lightmapUVsDensityMaterialShader);
    SAFE_DELETE(_vertexColorsMaterialShader);
#endif
}

void GBufferPass::Fill(RenderContext& renderContext, GPUTextureView* lightBuffer)
{
    PROFILE_GPU_CPU("GBuffer");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& view = renderContext.View;
    GPUTextureView* targetBuffers[5] =
    {
        lightBuffer,
        renderContext.Buffers->GBuffer0->View(),
        renderContext.Buffers->GBuffer1->View(),
        renderContext.Buffers->GBuffer2->View(),
        renderContext.Buffers->GBuffer3->View(),
    };
    view.Pass = DrawPass::GBuffer;

    // Clear GBuffer
    {
        PROFILE_GPU_CPU("Clear");

        context->ClearDepth(*renderContext.Buffers->DepthBuffer);
        context->Clear(lightBuffer, Color::Transparent);
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
    // Override draw calls material to use material debug shader
    if (renderContext.View.Mode == ViewMode::LightmapUVsDensity)
    {
        if (!_lightmapUVsDensityMaterialShader)
            _lightmapUVsDensityMaterialShader = New<LightmapUVsDensityMaterialShader>();
        if (_lightmapUVsDensityMaterialShader->IsReady())
        {
            auto& drawCallsList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer];
            for (int32 i : drawCallsList.Indices)
            {
                auto& drawCall = renderContext.List->DrawCalls[i];
                if (drawCall.Material->IsSurface())
                {
                    drawCall.Material = _lightmapUVsDensityMaterialShader;
                }
            }
            IMaterial::InstancingHandler handler;
            if (!_lightmapUVsDensityMaterialShader->CanUseInstancing(handler))
            {
                drawCallsList.CanUseInstancing = false;
            }
        }
    }
    else if (renderContext.View.Mode == ViewMode::VertexColors)
    {
        if (!_vertexColorsMaterialShader)
            _vertexColorsMaterialShader = New<VertexColorsMaterialShader>();
        if (_vertexColorsMaterialShader->IsReady())
        {
            auto& drawCallsList = renderContext.List->DrawCallsLists[(int32)DrawCallsListType::GBuffer];
            for (int32 i : drawCallsList.Indices)
            {
                auto& drawCall = renderContext.List->DrawCalls[i];
                if (drawCall.Material->IsSurface())
                {
                    drawCall.Material = _vertexColorsMaterialShader;
                }
            }
            IMaterial::InstancingHandler handler;
            if (!_vertexColorsMaterialShader->CanUseInstancing(handler))
            {
                drawCallsList.CanUseInstancing = false;
            }
        }
    }
#endif

    // Draw objects that can get decals
    context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBuffer);

    // Draw decals
    DrawDecals(renderContext, lightBuffer);

    // Draw objects that cannot get decals
    context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, ToSpan(targetBuffers, ARRAY_COUNT(targetBuffers)));
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBufferNoDecals);

    // Draw sky
    if (renderContext.List->Sky && _skyModel && _skyModel->CanBeRendered())
    {
        PROFILE_GPU_CPU("Sky");

        // Cache data
        auto model = _skyModel.Get();
        auto box = model->GetBox();

        // Calculate sphere model transform to cover far plane
        Matrix m1, m2;
        Matrix::Scaling(view.Far / (box.GetSize().Y * 0.5f) * 0.95f, m1); // Scale to fit whole view frustum
        Matrix::CreateWorld(view.Position, Vector3::Up, Vector3::Backward, m2); // Rotate sphere model
        m1 *= m2;

        // Draw sky
        renderContext.List->Sky->ApplySky(context, renderContext, m1);
        model->Render(context);
    }

    context->ResetRenderTarget();
}

bool SortDecal(Decal* const& a, Decal* const& b)
{
    return a->SortOrder < b->SortOrder;
}

void GBufferPass::RenderDebug(RenderContext& renderContext)
{
    // Check if has resources loaded
    if (setupResources())
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

void GBufferPass::SetInputs(const RenderView& view, GBufferData& gBuffer)
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

void GBufferPass::DrawDecals(RenderContext& renderContext, GPUTextureView* lightBuffer)
{
    // Skip if no decals to render
    auto& decals = renderContext.List->Decals;
    if (decals.IsEmpty() || _boxModel == nullptr || !_boxModel->CanBeRendered())
        return;

    PROFILE_GPU_CPU("Decals");

    // Cache data
    auto device = GPUDevice::Instance;
    auto gpuContext = device->GetMainContext();
    auto& view = renderContext.View;
    auto model = _boxModel.Get();
    auto buffers = renderContext.Buffers;

    // Sort decals from the lowest order to the highest order
    Sorting::QuickSort(decals.Get(), (int32)decals.Count(), &SortDecal);

    // TODO: batch decals using the same material

    // TODO: sort decals by the blending mode within the same order

    // Prepare
    DrawCall drawCall;
    MaterialBase::BindParameters bindParams(gpuContext, renderContext, drawCall);
    drawCall.Material = nullptr;
    drawCall.WorldDeterminantSign = 1.0f;

    // Draw all decals
    for (int32 i = 0; i < decals.Count(); i++)
    {
        const auto decal = decals[i];
        ASSERT(decal && decal->Material);
        decal->GetWorld(&drawCall.World);
        drawCall.ObjectPosition = drawCall.World.GetTranslation();

        gpuContext->ResetRenderTarget();

        // Bind output
        const MaterialInfo& info = decal->Material->GetInfo();
        switch (info.DecalBlendingMode)
        {
        case MaterialDecalBlendingMode::Translucent:
        {
            GPUTextureView* targetBuffers[4];
            int32 count = 2;
            targetBuffers[0] = buffers->GBuffer0->View();
            targetBuffers[1] = buffers->GBuffer2->View();
            if (info.UsageFlags & MaterialUsageFlags::UseEmissive)
            {
                count++;
                targetBuffers[2] = lightBuffer;

                if (info.UsageFlags & MaterialUsageFlags::UseNormal)
                {
                    count++;
                    targetBuffers[3] = buffers->GBuffer1->View();
                }
            }
            else if (info.UsageFlags & MaterialUsageFlags::UseNormal)
            {
                count++;
                targetBuffers[2] = buffers->GBuffer1->View();
            }
            gpuContext->SetRenderTarget(nullptr, ToSpan(targetBuffers, count));
            break;
        }
        case MaterialDecalBlendingMode::Stain:
        {
            gpuContext->SetRenderTarget(buffers->GBuffer0->View());
            break;
        }
        case MaterialDecalBlendingMode::Normal:
        {
            gpuContext->SetRenderTarget(buffers->GBuffer1->View());
            break;
        }
        case MaterialDecalBlendingMode::Emissive:
        {
            gpuContext->SetRenderTarget(lightBuffer);
            break;
        }
        }

        // Draw decal
        drawCall.PerInstanceRandom = decal->GetPerInstanceRandom();
        decal->Material->Bind(bindParams);
        model->Render(gpuContext);
    }

    gpuContext->ResetSR();
}
