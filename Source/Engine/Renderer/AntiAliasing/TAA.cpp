// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "TAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Engine/Engine.h"

bool TAA::Init()
{
    // Create pipeline state
    _psTAA.CreatePipelineStates();

    // Load shader
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
    // Check shader
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

    // Delete pipeline state
    _psTAA.Delete();

    // Release asset
    _shader.Unlink();
}

bool TAA::NeedMotionVectors(RenderContext& renderContext)
{
    return renderContext.List->Settings.AntiAliasing.Mode == AntialiasingMode::TemporalAntialiasing;
}

void TAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
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
    const auto tempDesc = GPUTextureDescription::New2D((int32)renderContext.View.ScreenSize.X, (int32)renderContext.View.ScreenSize.Y, input->Format());
    if (renderContext.Buffers->TemporalAA == nullptr)
    {
        // Missing temporal buffer
        renderContext.Buffers->TemporalAA = RenderTargetPool::Get(tempDesc);
        resetHistory = true;
    }
    else if (renderContext.Buffers->TemporalAA->Width() != tempDesc.Width || renderContext.Buffers->TemporalAA->Height() != tempDesc.Height)
    {
        // Wrong size temporal buffer
        RenderTargetPool::Release(renderContext.Buffers->TemporalAA);
        renderContext.Buffers->TemporalAA = RenderTargetPool::Get(tempDesc);
        resetHistory = true;
    }
    auto inputHistory = renderContext.Buffers->TemporalAA;
    const auto outputHistory = RenderTargetPool::Get(tempDesc);

    // Duplicate the current frame to the history buffer if need to reset the temporal history
    float blendStrength = 1.0f;
    if (resetHistory)
    {
        PROFILE_GPU_CPU("Reset History");

        context->SetRenderTarget(inputHistory->View());
        context->Draw(input);
        context->ResetRenderTarget();

        blendStrength = 0.0f;
    }

    // Bind input
    Data data;
    data.ScreenSize = renderContext.View.ScreenSize;
    data.TaaJitterStrength.X = renderContext.View.TemporalAAJitter.X;
    data.TaaJitterStrength.Y = renderContext.View.TemporalAAJitter.Y;
    data.TaaJitterStrength.Z = data.TaaJitterStrength.X / tempDesc.Width;
    data.TaaJitterStrength.W = data.TaaJitterStrength.Y / tempDesc.Height;
    data.FinalBlendParameters.X = settings.TAA_StationaryBlending * blendStrength;
    data.FinalBlendParameters.Y = settings.TAA_MotionBlending * blendStrength;
    data.FinalBlendParameters.Z = 100.0f * 60.0f;
    data.FinalBlendParameters.W = settings.TAA_Sharpness;
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, input);
    context->BindSR(1, inputHistory);
    context->BindSR(2, renderContext.Buffers->MotionVectors);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);

    // Render
    GPUTextureView* rts[] = { output, outputHistory->View() };
    context->SetRenderTarget(nullptr, ToSpan(rts, ARRAY_COUNT(rts)));
    context->SetState(_psTAA.Get(renderContext.View.IsOrthographicProjection() ? 1 : 0));
    context->DrawFullscreenTriangle();

    // Swap the history
    RenderTargetPool::Release(inputHistory);
    renderContext.Buffers->TemporalAA = outputHistory;
}
