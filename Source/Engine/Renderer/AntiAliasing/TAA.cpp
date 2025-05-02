// Copyright (c) Wojciech Figat. All rights reserved.

#include "TAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/GBufferPass.h"
#include "Engine/Engine/Engine.h"

GPU_CB_STRUCT(Data {
    Float2 ScreenSizeInv;
    Float2 JitterInv;
    float Sharpness;
    float StationaryBlending;
    float MotionBlending;
    float Dummy0;
    ShaderGBufferData GBuffer;
    });

bool TAA::Init()
{
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
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }
    if (!_psTAA)
        _psTAA = GPUDevice::Instance->CreatePipelineState();
    GPUPipelineState::Description psDesc;
    if (!_psTAA->IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = shader->GetPS("PS");
        if (_psTAA->Init(psDesc))
            return true;
    }
    return false;
}

void TAA::Dispose()
{
    // Base
    RendererPass::Dispose();

    SAFE_DELETE_GPU_RESOURCE(_psTAA);
    _shader = nullptr;
}

void TAA::Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();

    // Ensure to have valid data
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
    data.Sharpness = settings.TAA_Sharpness;
    data.StationaryBlending = settings.TAA_StationaryBlending * blendStrength;
    data.MotionBlending = settings.TAA_MotionBlending * blendStrength;
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
    context->SetState(_psTAA);
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
