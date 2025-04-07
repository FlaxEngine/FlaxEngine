// Copyright (c) Wojciech Figat. All rights reserved.

#include "FXAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTask.h"

GPU_CB_STRUCT(Data {
    Float4 ScreenSize;
    });

String FXAA::ToString() const
{
    return TEXT("FXAA");
}

bool FXAA::Init()
{
    _psFXAA.CreatePipelineStates();
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
    if (!_shader->IsLoaded())
    {
        return true;
    }
    const auto shader = _shader->GetShader();
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

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

    _psFXAA.Delete();
    _shader = nullptr;
}

void FXAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    auto context = GPUDevice::Instance->GetMainContext();
    context->SetRenderTarget(output);
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering, just copy input frame.
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
    const auto qualityLevel = Math::Clamp(static_cast<int32>(Graphics::AAQuality), 0, static_cast<int32>(Quality::MAX) - 1);
    context->SetState(_psFXAA.Get(qualityLevel));
    context->DrawFullscreenTriangle();
}
