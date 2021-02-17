// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    //_psTAA.CreatePipelineStates();

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

    return false;
}

void TAA::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psTAA = nullptr;
    _shader = nullptr;
}

bool TAA::NeedMotionVectors(RenderContext& renderContext)
{
    return renderContext.List->Settings.AntiAliasing.Mode == AntialiasingMode::TemporalAntialiasing;
}

void TAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();

    // Ensure to have valid data
    //if (checkIfSkipPass())
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

    // ...
}
