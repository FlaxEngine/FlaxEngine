// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "FXAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Graphics.h"

bool FXAA::Init()
{
    // Create pipeline state
    _psFXAA.CreatePipelineStates();

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/FXAA"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<FXAA, &FXAA::OnShaderReloading>(this);
#endif

    return false;
}

bool FXAA::setupResources()
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
    if (!_psFXAA.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        if (_psFXAA.Create(psDesc, shader, "PS"))
            return true;
    }

    return false;
}

void FXAA::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psFXAA.Delete();
    _shader = nullptr;
}

void FXAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();

    // Ensure to have valid data
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering, just copy input frame.
        context->SetRenderTarget(output);
        context->Draw(input);
        return;
    }

    PROFILE_GPU_CPU("Fast Approximate Antialiasing");

    // Bind input
    Data data;
    data.ScreenSize = renderContext.View.ScreenSize;
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, input);

    // Render
    context->SetRenderTarget(output);
    context->SetState(_psFXAA.Get(static_cast<int32>(Graphics::AAQuality)));
    context->DrawFullscreenTriangle();
}
