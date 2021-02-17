// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MultiScaler.h"
#include "Engine/Content/Content.h"

MultiScaler::MultiScaler()
    : _psHalfDepth(nullptr)
{
}

String MultiScaler::ToString() const
{
    return TEXT("MultiScaler");
}

bool MultiScaler::Init()
{
    // Create pipeline states
    _psHalfDepth = GPUDevice::Instance->CreatePipelineState();
    _psBlur5.CreatePipelineStates();
    _psBlur9.CreatePipelineStates();
    _psBlur13.CreatePipelineStates();

    // Load asset
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/MultiScaler"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<MultiScaler, &MultiScaler::OnShaderReloading>(this);
#endif

    return false;
}

bool MultiScaler::setupResources()
{
    // Check if shader has not been loaded
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline states
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psBlur5.IsValid())
    {
        if (_psBlur5.Create(psDesc, shader, "PS_Blur5"))
            return true;
    }
    if (!_psBlur9.IsValid())
    {
        if (_psBlur9.Create(psDesc, shader, "PS_Blur9"))
            return true;
    }
    if (!_psBlur13.IsValid())
    {
        if (_psBlur13.Create(psDesc, shader, "PS_Blur13"))
            return true;
    }
    if (!_psHalfDepth->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_HalfDepth");
        psDesc.DepthWriteEnable = true;
        psDesc.DepthTestEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        if (_psHalfDepth->Init(psDesc))
            return true;
    }

    return false;
}

void MultiScaler::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psHalfDepth);
    _psBlur5.Delete();
    _psBlur9.Delete();
    _psBlur13.Delete();
    _shader = nullptr;
}

void MultiScaler::Filter(const FilterMode mode, GPUContext* context, const int32 width, const int32 height, GPUTextureView* src, GPUTextureView* dst, GPUTextureView* tmp)
{
    PROFILE_GPU_CPU("MultiScaler Filter");

    context->SetViewportAndScissors((float)width, (float)height);

    // Check if has missing resources
    if (checkIfSkipPass())
    {
        // Simple copy
        context->SetRenderTarget(dst);
        context->Draw(src);
        context->ResetRenderTarget();
        return;
    }

    // Select filter
    GPUPipelineStatePermutationsPs<2>* ps;
    switch (mode)
    {
    case FilterMode::GaussianBlur5:
        ps = &_psBlur5;
        break;
    case FilterMode::GaussianBlur9:
        ps = &_psBlur9;
        break;
    case FilterMode::GaussianBlur13:
        ps = &_psBlur13;
        break;
    default:
    CRASH;
        return;
    }

    // Prepare
    Data data;
    data.TexelSize.X = 1.0f / width;
    data.TexelSize.Y = 1.0f / height;
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Convolve horizontal
    context->BindSR(0, src);
    context->SetRenderTarget(tmp);
    context->SetState(ps->Get(0));
    context->DrawFullscreenTriangle();

    context->ResetRenderTarget();

    // Convolve vertical
    context->BindSR(0, tmp);
    context->SetRenderTarget(dst);
    context->SetState(ps->Get(1));
    context->DrawFullscreenTriangle();

    context->ResetRenderTarget();
}

void MultiScaler::Filter(const FilterMode mode, GPUContext* context, const int32 width, const int32 height, GPUTextureView* srcDst, GPUTextureView* tmp)
{
    PROFILE_GPU_CPU("MultiScaler Filter");

    context->SetViewportAndScissors((float)width, (float)height);

    // Check if has missing resources
    if (checkIfSkipPass())
    {
        // Skip
        return;
    }

    // Select filter
    GPUPipelineStatePermutationsPs<2>* ps;
    switch (mode)
    {
    case FilterMode::GaussianBlur5:
        ps = &_psBlur5;
        break;
    case FilterMode::GaussianBlur9:
        ps = &_psBlur9;
        break;
    case FilterMode::GaussianBlur13:
        ps = &_psBlur13;
        break;
    default:
    CRASH;
        return;
    }

    // Prepare
    Data data;
    data.TexelSize.X = 1.0f / width;
    data.TexelSize.Y = 1.0f / height;
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Convolve horizontal
    context->BindSR(0, srcDst);
    context->SetRenderTarget(tmp);
    context->SetState(ps->Get(0));
    context->DrawFullscreenTriangle();

    context->ResetRenderTarget();

    // Convolve vertical
    context->BindSR(0, tmp);
    context->SetRenderTarget(srcDst);
    context->SetState(ps->Get(1));
    context->DrawFullscreenTriangle();

    context->ResetRenderTarget();
}

void MultiScaler::DownscaleDepth(GPUContext* context, int32 dstWidth, int32 dstHeight, GPUTextureView* src, GPUTextureView* dst)
{
    PROFILE_GPU_CPU("Downscale Depth");

    // Check if has missing resources
    if (checkIfSkipPass())
    {
        // Clear the output
        context->ClearDepth(dst);
        return;
    }

    // Prepare
    Data data;
    data.TexelSize.X = 2.0f / dstWidth;
    data.TexelSize.Y = 2.0f / dstHeight;
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Draw
    context->SetViewportAndScissors((float)dstWidth, (float)dstHeight);
    context->SetRenderTarget(dst, (GPUTextureView*)nullptr);
    context->BindSR(0, src);
    context->SetState(_psHalfDepth);
    context->DrawFullscreenTriangle();

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindCB(0);
}
