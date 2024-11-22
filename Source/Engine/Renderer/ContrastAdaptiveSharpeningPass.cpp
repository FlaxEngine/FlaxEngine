// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ContrastAdaptiveSharpeningPass.h"
#include "RenderList.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"

GPU_CB_STRUCT(Data {
    Float2 InputSizeInv;
    Float2 Padding;
    float SharpeningAmount;
    float EdgeSharpening;
    float MinEdgeThreshold;
    float OverBlurLimit;
    });

String ContrastAdaptiveSharpeningPass::ToString() const
{
    return TEXT("ContrastAdaptiveSharpening");
}

void ContrastAdaptiveSharpeningPass::Dispose()
{
    RendererPass::Dispose();

    SAFE_DELETE_GPU_RESOURCE(_psCAS);
    _shader = nullptr;
}

bool ContrastAdaptiveSharpeningPass::setupResources()
{
    // Lazy-load shader
    if (_lazyInit && !_shader)
    {
        _lazyInit = false;
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/CAS"));
        if (!_shader)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<ContrastAdaptiveSharpeningPass, &ContrastAdaptiveSharpeningPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stage
    auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDesc.PS = shader->GetPS("PS_CAS");
    _psCAS = GPUDevice::Instance->CreatePipelineState();
    if (_psCAS->Init(psDesc))
        return true;

    return false;
}

bool ContrastAdaptiveSharpeningPass::CanRender(const RenderContext& renderContext)
{
    const AntiAliasingSettings& antiAliasing = renderContext.List->Settings.AntiAliasing;
    return EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::AntiAliasing) &&
            antiAliasing.CAS_SharpeningAmount > ZeroTolerance &&
            !checkIfSkipPass();
}

void ContrastAdaptiveSharpeningPass::Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
{
    ASSERT_LOW_LAYER(CanRender(renderContext));
    PROFILE_GPU_CPU("Contrast Adaptive Sharpening");
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    const AntiAliasingSettings& antiAliasing = renderContext.List->Settings.AntiAliasing;

    Data data;
    data.InputSizeInv = Float2::One / input->Size();
    data.SharpeningAmount = antiAliasing.CAS_SharpeningAmount;
    data.EdgeSharpening = antiAliasing.CAS_EdgeSharpening;
    data.MinEdgeThreshold = antiAliasing.CAS_MinEdgeThreshold;
    data.OverBlurLimit = antiAliasing.CAS_OverBlurLimit;
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, input);
    context->SetState(_psCAS);
    context->SetRenderTarget(output);
    context->DrawFullscreenTriangle();
}
