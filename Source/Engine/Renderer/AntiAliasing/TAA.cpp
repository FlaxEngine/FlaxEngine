// Copyright (c) Wojciech Figat. All rights reserved.

#include "TAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/GBufferPass.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTools.h"

GPU_CB_STRUCT(Data {
    Float2 ScreenSizeInv;
    Float2 JitterInv;
    Float2 MotionScale;
    float StationaryBlending;
    float MotionBlending;
    Float3 QuantizationError;
    float Sharpness;
    ShaderGBufferData GBuffer;
    });

bool TAA::Init()
{
    _psTAA.CreatePipelineStates();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/TAA"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<TAA, &TAA::OnShaderReloading>(this);
#endif
    return false;
}

bool TAA::setupResources()
{
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
    GPUPipelineState::Description psDesc;
    if (!_psTAA.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        if (_psTAA.Create(psDesc, shader, "PS"))
            return true;
    }
    return false;
}

void TAA::Dispose()
{
    // Base
    RendererPass::Dispose();

    _psTAA.Delete();
    _shader = nullptr;
}

void TAA::Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering, just copy source frame.
        context->SetRenderTarget(output);
        context->Draw(input);
        return;
    }
    const auto& settings = renderContext.List->Settings.AntiAliasing;

    PROFILE_GPU_CPU("Temporal Antialiasing");

    // Get history buffers
    bool resetHistory = renderContext.Task->IsCameraCut;
    renderContext.Buffers->LastFrameTemporalAA = Engine::FrameCount;
    const auto tempDesc = GPUTextureDescription::New2D(input->Width(), input->Height(), input->Format());
    if (renderContext.Buffers->TemporalAA == nullptr)
    {
        // Missing temporal buffer
        renderContext.Buffers->TemporalAA = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(renderContext.Buffers->TemporalAA, "TemporalAA");
        resetHistory = true;
    }
    else if (renderContext.Buffers->TemporalAA->Width() != tempDesc.Width || renderContext.Buffers->TemporalAA->Height() != tempDesc.Height)
    {
        // Wrong size temporal buffer
        RenderTargetPool::Release(renderContext.Buffers->TemporalAA);
        renderContext.Buffers->TemporalAA = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(renderContext.Buffers->TemporalAA, "TemporalAA");
        resetHistory = true;
    }
    auto inputHistory = renderContext.Buffers->TemporalAA;
    const auto outputHistory = RenderTargetPool::Get(tempDesc);
    RENDER_TARGET_POOL_SET_NAME(outputHistory, "TemporalAA");

    // Duplicate the current frame to the history buffer if need to reset the temporal history
    float blendStrength = 1.0f;
    if (resetHistory)
    {
#if 0
        context->CopyTexture(inputHistory, 0, 0, 0, 0, input, 0);
#else
        context->SetRenderTarget(inputHistory->View());
        context->Draw(input);
        context->ResetRenderTarget();
#endif
        blendStrength = 0.0f;
    }

    // Bind input
    Data data;
    data.ScreenSizeInv.X = renderContext.View.ScreenSize.Z;
    data.ScreenSizeInv.Y = renderContext.View.ScreenSize.W;
    data.JitterInv.X = renderContext.View.TemporalAAJitter.X / (float)tempDesc.Width;
    data.JitterInv.Y = renderContext.View.TemporalAAJitter.Y / (float)tempDesc.Height;
    data.Sharpness = settings.TAA_Sharpness * 3; // Hardcoded scale
    data.StationaryBlending = settings.TAA_StationaryBlending * blendStrength;
    data.MotionBlending = settings.TAA_MotionBlending * blendStrength;
    data.MotionScale = 0.1f / data.ScreenSizeInv; // Hardcoded scale
    data.QuantizationError = RenderTools::GetColorQuantizationError(tempDesc.Format);
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, input);
    context->BindSR(1, inputHistory);
    context->BindSR(2, renderContext.Buffers->MotionVectors);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);

    // Render
    context->SetRenderTarget(output);
    int qualityLevel;
    switch (Graphics::AAQuality)
    {
    case Quality::Low:
        qualityLevel = 0;
        break;
    case Quality::Medium:
        qualityLevel = 1;
        break;
    case Quality::High:
    case Quality::Ultra:
        qualityLevel = 2;
        break;
    }
    context->SetState(_psTAA.Get(qualityLevel));
    context->DrawFullscreenTriangle();

    // Update the history
    {
        RenderTargetPool::Release(inputHistory);
        context->ResetRenderTarget();
        context->SetRenderTarget(outputHistory->View());
        context->Draw(output);
        renderContext.Buffers->TemporalAA = outputHistory;
    }

    // Mark TAA jitter as resolved for future drawing
    (bool&)renderContext.View.IsTaaResolved = true;
}
