// Copyright (c) Wojciech Figat. All rights reserved.

#include "FXAA.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUPass.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderContext.h"

GPU_CB_STRUCT(Data {
    Float4 ScreenSize;
    });

static int32 GetPermutationIndex()
{
    const auto qualityLevel = Math::Clamp(static_cast<int32>(Graphics::AAQuality), 0, static_cast<int32>(Quality::MAX) - 1);
    return qualityLevel;
}

String FXAA::ToString() const
{
    return TEXT("FXAA");
}

bool FXAA::Init()
{
    _psFXAA.CreatePipelineStates();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/AntiAliasing/FXAA"));
    if (_shader == nullptr)
        return true;
    BIND_SHADER_RELOADING(_shader, FXAA, OnShaderReloading);
    return false;
}

bool FXAA::setupResources()
{
    if (!_shader->IsLoaded())
    {
        return true;
    }
    const auto shader = _shader->GPU;
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);

    GPUPipelineState::Description psDesc;
    if (!_psFXAA.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        if (_psFXAA.Create(psDesc, shader, "PS"))
            return true;
    }

    return false;
}

#if COMPILE_WITH_PSO_PRECACHING

#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"

void FXAA::PrecachePSO()
{
    const auto& settings = Graphics::PostProcessSettings.AntiAliasing;
    if (_shader->WaitForLoaded() ||
        settings.Mode != AntialiasingMode::FastApproximateAntialiasing ||
        checkIfSkipPass(true))
        return;
    GPUPipelineState::PrecacheContext context;
    context.RTCount = 1;
    context.RTFormats[0] = MainRenderTask::Instance->Buffers->GetOutputFormat();
    context.SetReadOnlyDepthFormat();
    _psFXAA[GetPermutationIndex()]->PrecachePSO(context);
}

#endif

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

    auto rtAction = GPUDrawPassAction::Store;
    GPUDrawPass drawPass(context, ToSpan(&output, 1), ToSpan(&rtAction, 1));
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
    const auto cb = _shader->GPU->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, input);

    // Render
    context->SetState(_psFXAA.Get(GetPermutationIndex()));
    context->DrawFullscreenTriangle();
}
