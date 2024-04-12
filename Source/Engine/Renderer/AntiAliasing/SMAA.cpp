// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "SMAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/RenderList.h"


PACK_STRUCT(struct Data
{
    Float4 RtSize;
    float CAS_SharpeningAmount;
    float CAS_EdgeSharpening;
    float CAS_MinEdgeThreshold;
    float CAS_OverBlurLimit;
});


bool SMAA::setupResources()
{
    // Load textures
    if (_areaTex == nullptr)
    {
        _areaTex = Content::LoadAsyncInternal<Texture>(SMAA_AREA_TEX);
        if (_areaTex == nullptr)
            return true;
    }
    if (_searchTex == nullptr)
    {
        _searchTex = Content::LoadAsyncInternal<Texture>(SMAA_SEARCH_TEX);
        if (_searchTex == nullptr)
            return true;
    }

    // Check shader
    if (_shader == nullptr)
    {
        // Create pipeline states
        _psEdge.CreatePipelineStates();
        _psBlend.CreatePipelineStates();
        _psNeighbor = GPUDevice::Instance->CreatePipelineState();

        // Load shader
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/SMAA"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<SMAA, &SMAA::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
    {
        return true;
    }
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }


    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psEdge.IsValid())
    {
        psDesc.VS = shader->GetVS("VS_Edge");
        if (_psEdge.Create(psDesc, shader, "PS_Edge"))
            return true;
    }
    if (!_psBlend.IsValid())
    {
        psDesc.VS = shader->GetVS("VS_Blend");
        if (_psBlend.Create(psDesc, shader, "PS_Blend"))
            return true;
    }
    if (!_psNeighbor->IsValid())
    {
        psDesc.VS = shader->GetVS("VS_Neighbor");
        psDesc.PS = shader->GetPS("PS_Neighbor");
        if (_psNeighbor->Init(psDesc))
            return true;
    }

    if (!_psCAS)
        _psCAS = GPUDevice::Instance->CreatePipelineState();
    if (!_psCAS->IsValid())
    {
        psDesc.VS = shader->GetVS("VS_CAS");
        psDesc.PS = shader->GetPS("PS_CAS");
        if (_psCAS->Init(psDesc))
            return true;
    }

    return false;
}

void SMAA::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psEdge.Delete();
    _psBlend.Delete();
    SAFE_DELETE_GPU_RESOURCE(_psNeighbor);
    SAFE_DELETE_GPU_RESOURCE(_psCAS);
    _shader = nullptr;
    _areaTex = nullptr;
    _searchTex = nullptr;
}


void SMAA::Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();
    const auto qualityLevel = Math::Clamp(static_cast<int32>(Graphics::AAQuality), 0, static_cast<int32>(Quality::MAX) - 1);

    //Ensure to have valid data
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering, just copy input frame.
        context->SetRenderTarget(output);
        context->Draw(input);
        return;
    }

    PROFILE_GPU_CPU("Subpixel Morphological Antialiasing");

    const auto& settings = renderContext.List->Settings.AntiAliasing;

    // Get temporary targets
    const auto tempDesc = GPUTextureDescription::New2D((int32)renderContext.View.ScreenSize.X, (int32)renderContext.View.ScreenSize.Y, PixelFormat::R8G8B8A8_UNorm);
    auto edges = RenderTargetPool::Get(tempDesc);
    auto weights = RenderTargetPool::Get(tempDesc);
    auto casOutput = RenderTargetPool::Get(tempDesc);
    RENDER_TARGET_POOL_SET_NAME(edges, "SMAA.Edges");
    RENDER_TARGET_POOL_SET_NAME(weights, "SMAA.Weights");
    RENDER_TARGET_POOL_SET_NAME(casOutput, "SMAA.CASOutput");

    // Bind constants
    Data data;
    data.RtSize.X = 1.0f / tempDesc.Width;
    data.RtSize.Y = 1.0f / tempDesc.Height;
    data.RtSize.Z = (float)tempDesc.Width;
    data.RtSize.W = (float)tempDesc.Height;
    data.CAS_SharpeningAmount = settings.CAS_SharpeningAmount;
    data.CAS_EdgeSharpening = settings.CAS_EdgeSharpening;
    data.CAS_MinEdgeThreshold = settings.CAS_MinEdgeThreshold;
    data.CAS_OverBlurLimit = settings.CAS_OverBlurLimit;
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Edge Detection
    context->BindSR(0, input);
    context->SetRenderTarget(*edges);
    context->Clear(*edges, Color::Transparent);
    context->SetState(_psEdge.Get(qualityLevel));
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Blend Weights
    context->BindSR(0, edges);
    context->BindSR(1, _areaTex->GetTexture());
    context->BindSR(2, _searchTex->GetTexture());
    context->SetRenderTarget(*weights);
    context->SetState(_psBlend.Get(qualityLevel));
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Neighborhood Blending
    context->BindSR(0, input);
    context->BindSR(1, weights);
    context->UnBindSR(2);
    context->SetRenderTarget(*casOutput);
    context->SetState(_psNeighbor);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Contrast Adaptive Sharpening (CAS)
    context->BindSR(0, casOutput);
    context->SetRenderTarget(output);
    context->SetState(_psCAS);
    context->DrawFullscreenTriangle();

    // Cleanup
    context->UnBindSR(0);
    context->UnBindSR(1);
    context->UnBindSR(2);
    RenderTargetPool::Release(edges);
    RenderTargetPool::Release(weights);
    RenderTargetPool::Release(casOutput);
}
