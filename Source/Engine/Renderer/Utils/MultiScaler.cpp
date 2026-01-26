// Copyright (c) Wojciech Figat. All rights reserved.

#include "MultiScaler.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Content/Content.h"

GPU_CB_STRUCT(Data {
    Float2 TexelSize;
    Float2 Padding;
    });

String MultiScaler::ToString() const
{
    return TEXT("MultiScaler");
}

bool MultiScaler::Init()
{
    // Create pipeline states
    _psBlur5.CreatePipelineStates();
    _psBlur9.CreatePipelineStates();
    _psBlur13.CreatePipelineStates();
    _psHalfDepth.CreatePipelineStates();
    _psUpscale = GPUDevice::Instance->CreatePipelineState();

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
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);

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
    if (!_psUpscale->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_Upscale");
        if (_psUpscale->Init(psDesc))
            return true;
    }
    if (!_psHalfDepth.IsValid())
    {
        psDesc.PS = shader->GetPS("PS_HalfDepth", 0);
        if (_psHalfDepth[0]->Init(psDesc))
            return true;
        psDesc.PS = shader->GetPS("PS_HalfDepth", 2);
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::Red;
        if (_psHalfDepth[2]->Init(psDesc))
            return true;
        psDesc.PS = shader->GetPS("PS_HalfDepth", 1);
        psDesc.DepthWriteEnable = true;
        psDesc.DepthEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Always;
        if (_psHalfDepth[1]->Init(psDesc))
            return true;
    }

    return false;
}

void MultiScaler::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psUpscale);
    _psBlur5.Delete();
    _psBlur9.Delete();
    _psBlur13.Delete();
    _psHalfDepth.Delete();
    _shader = nullptr;
}

void MultiScaler::Filter(FilterMode mode, GPUContext* context, int32 width, int32 height, GPUTextureView* src, GPUTextureView* dst, GPUTextureView* tmp)
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
    data.TexelSize.X = 1.0f / (float)width;
    data.TexelSize.Y = 1.0f / (float)height;
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

void MultiScaler::Filter(FilterMode mode, GPUContext* context, int32 width, int32 height, GPUTextureView* srcDst, GPUTextureView* tmp)
{
    PROFILE_GPU_CPU("MultiScaler Filter");

    context->SetViewportAndScissors((float)width, (float)height);

    if (checkIfSkipPass())
        return;

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
    data.TexelSize.X = 1.0f / (float)width;
    data.TexelSize.Y = 1.0f / (float)height;
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

void MultiScaler::DownscaleDepth(GPUContext* context, int32 dstWidth, int32 dstHeight, GPUTexture* src, GPUTextureView* dst)
{
    PROFILE_GPU_CPU("Downscale Depth");
    if (checkIfSkipPass())
    {
        context->ClearDepth(dst);
        return;
    }

    // Prepare
    Data data;
    data.TexelSize.X = 1.0f / (float)src->Width();
    data.TexelSize.Y = 1.0f / (float)src->Height();
    bool outputDepth = ((GPUTexture*)dst->GetParent())->IsDepthStencil();
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Draw
    context->SetViewportAndScissors((float)dstWidth, (float)dstHeight);
    context->SetRenderTarget(dst, (GPUTextureView*)nullptr);
    context->BindSR(0, src);
    context->SetState(_psHalfDepth[outputDepth ? 1 : 0]);
    context->DrawFullscreenTriangle();

    // Cleanup
    context->ResetRenderTarget();
    context->UnBindCB(0);
}

void MultiScaler::BuildHiZ(GPUContext* context, GPUTexture* srcDepth, GPUTexture* dstHiZ)
{
    PROFILE_GPU_CPU("Build HiZ");

    int32 dstWidth = dstHiZ->Width();
    int32 dstHeight = dstHiZ->Height();

    // Copy mip0
    if (srcDepth->Size() == dstHiZ->Size() && srcDepth->Format() == dstHiZ->Format())
    {
        context->CopySubresource(dstHiZ, 0, srcDepth, 0);
    }
    else if (srcDepth->Size() == dstHiZ->Size())
    {
        context->Draw(dstHiZ, srcDepth);
    }
    else
    {
        context->SetViewportAndScissors((float)dstWidth, (float)dstHeight);
        context->SetRenderTarget(dstHiZ->View());
        context->BindSR(0, srcDepth);
        context->SetState(_psHalfDepth[2]);
        context->DrawFullscreenTriangle();
    }

    // Build mip chain
    for (int32 mip = 1; mip < dstHiZ->MipLevels(); mip++)
    {
        const int32 mipWidth = Math::Max(dstWidth >> mip, 1);
        const int32 mipHeight = Math::Max(dstHeight >> mip, 1);
        context->ResetRenderTarget();

        context->SetViewportAndScissors((float)mipWidth, (float)mipHeight);
        context->SetRenderTarget(dstHiZ->View(0, mip));
        context->BindSR(0, dstHiZ->View(0, mip - 1));
        context->SetState(_psHalfDepth[2]);
        context->DrawFullscreenTriangle();
    }

    context->ResetRenderTarget();
    context->UnBindCB(0);
}

void MultiScaler::Upscale(GPUContext* context, const Viewport& viewport, GPUTexture* src, GPUTextureView* dst)
{
    PROFILE_GPU_CPU("Upscale");

    context->SetViewportAndScissors(viewport);
    context->SetRenderTarget(dst);

    if (checkIfSkipPass())
    {
        context->Draw(src);
    }
    else
    {
        Data data;
        data.TexelSize.X = 1.0f / (float)src->Width();
        data.TexelSize.Y = 1.0f / (float)src->Height();
        auto cb = _shader->GetShader()->GetCB(0);
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
        context->BindSR(0, src);
        context->SetState(_psUpscale);
        context->DrawFullscreenTriangle();
        context->UnBindCB(0);
    }

    context->ResetRenderTarget();
}
