// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ScreenSpaceReflectionsPass.h"
#include "ReflectionsPass.h"
#include "GBufferPass.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Engine/Time.h"
#include "Engine/Platform/Window.h"
#include "Utils/MultiScaler.h"
#include "Engine/Engine/Engine.h"

#define REFLECTIONS_FORMAT PixelFormat::R11G11B10_Float
#define RESOLVE_PASS_OUTPUT_FORMAT PixelFormat::R16G16B16A16_Float

// Shader input texture slots mapping
#define TEXTURE0 4
#define TEXTURE1 5
#define TEXTURE2 6

PACK_STRUCT(struct Data
    {
    GBufferData GBuffer;

    float MaxColorMiplevel;
    float TraceSizeMax;
    float MaxTraceSamples;
    float RoughnessFade;

    Vector2 SSRtexelSize;
    float TemporalTime;
    float BRDFBias;

    float WorldAntiSelfOcclusionBias;
    float EdgeFadeFactor;
    float TemporalResponse;
    float TemporalScale;

    float RayTraceStep;
    float NoTemporalEffect;
    float Intensity;
    float FadeOutDistance;

    Vector3 Dummy0;
    float InvFadeDistance;

    Matrix ViewMatrix;
    Matrix ViewProjectionMatrix;
    });

ScreenSpaceReflectionsPass::ScreenSpaceReflectionsPass()
    : _psRayTracePass(nullptr)
    , _psCombinePass(nullptr)
    , _psTemporalPass(nullptr)
    , _psMixPass(nullptr)
{
}

bool ScreenSpaceReflectionsPass::NeedMotionVectors(RenderContext& renderContext)
{
    auto& settings = renderContext.List->Settings.ScreenSpaceReflections;
    return settings.TemporalEffect && renderContext.View.Flags & ViewFlags::SSR;
}

String ScreenSpaceReflectionsPass::ToString() const
{
    return TEXT("ScreenSpaceReflectionsPass");
}

bool ScreenSpaceReflectionsPass::Init()
{
    // Create pipeline states
    _psRayTracePass = GPUDevice::Instance->CreatePipelineState();
    _psCombinePass = GPUDevice::Instance->CreatePipelineState();
    _psResolvePass.CreatePipelineStates();
    _psTemporalPass = GPUDevice::Instance->CreatePipelineState();
    _psMixPass = GPUDevice::Instance->CreatePipelineState();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/SSR"));
    _preIntegratedGF = Content::LoadAsyncInternal<Texture>(PRE_INTEGRATED_GF_ASSET_NAME);
    if (_shader == nullptr || _preIntegratedGF == nullptr)
    {
        return true;
    }
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ScreenSpaceReflectionsPass, &ScreenSpaceReflectionsPass::OnShaderReloading>(this);
#endif

    return false;
}

bool ScreenSpaceReflectionsPass::setupResources()
{
    // Wait for the assets
    if (!_preIntegratedGF->IsLoaded())
        return true;

    // Shader
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psRayTracePass->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_RayTracePass");
        if (_psRayTracePass->Init(psDesc))
            return true;
    }
    if (!_psCombinePass->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_CombinePass");
        if (_psCombinePass->Init(psDesc))
            return true;
    }
    if (!_psResolvePass.IsValid())
    {
        if (_psResolvePass.Create(psDesc, shader, "PS_ResolvePass"))
            return true;
    }
    if (!_psTemporalPass->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TemporalPass");
        if (_psTemporalPass->Init(psDesc))
            return true;
    }
    if (!_psMixPass->IsValid())
    {
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        psDesc.PS = shader->GetPS("PS_MixPass");
        if (_psMixPass->Init(psDesc))
            return true;
    }

    return false;
}

void ScreenSpaceReflectionsPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psRayTracePass);
    SAFE_DELETE_GPU_RESOURCE(_psCombinePass);
    SAFE_DELETE_GPU_RESOURCE(_psTemporalPass);
    SAFE_DELETE_GPU_RESOURCE(_psMixPass);
    _psResolvePass.Delete();
    _shader = nullptr;
    _preIntegratedGF = nullptr;
}

void ScreenSpaceReflectionsPass::Render(RenderContext& renderContext, GPUTextureView* reflectionsRT, GPUTextureView* lightBuffer)
{
    // Skip pass if resources aren't ready
    if (checkIfSkipPass())
        return;
    auto& view = renderContext.View;
    auto buffers = renderContext.Buffers;

    // TODO: add support for SSR in ortho projection
    if (view.IsOrthographicProjection())
        return;

    PROFILE_GPU_CPU("Screen Space Reflections");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    const auto shader = _shader->GetShader();
    auto cb = shader->GetCB(0);
    auto& settings = renderContext.List->Settings.ScreenSpaceReflections;
    const bool useTemporal = settings.TemporalEffect && !renderContext.Task->IsCameraCut;

    // Prepare resolutions for passes
    const int32 width = renderContext.Buffers->GetWidth();
    const int32 height = renderContext.Buffers->GetHeight();
    const int32 traceWidth = width / static_cast<int32>(settings.RayTracePassResolution);
    const int32 traceHeight = height / static_cast<int32>(settings.RayTracePassResolution);
    const int32 resolveWidth = width / static_cast<int32>(settings.RayTracePassResolution);
    const int32 resolveHeight = height / static_cast<int32>(settings.RayTracePassResolution);
    const int32 colorBufferWidth = width / 2;
    const int32 colorBufferHeight = height / 2;
    const int32 temporalWidth = width;
    const int32 temporalHeight = height;
    const auto colorBufferMips = MipLevelsCount(colorBufferWidth, colorBufferHeight);

    // Prepare buffers
    auto tempDesc = GPUTextureDescription::New2D(width / 2, height / 2, 0, PixelFormat::R11G11B10_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews);
    auto colorBuffer0 = RenderTargetPool::Get(tempDesc);
    // TODO: maybe allocate colorBuffer1 smaller because mip0 is not used (the same as PostProcessingPass for Bloom), keep in sync to use the same buffer in frame
    auto colorBuffer1 = RenderTargetPool::Get(tempDesc);
    tempDesc = GPUTextureDescription::New2D(traceWidth, traceHeight, REFLECTIONS_FORMAT);
    auto traceBuffer = RenderTargetPool::Get(tempDesc);
    tempDesc = GPUTextureDescription::New2D(resolveWidth, resolveHeight, RESOLVE_PASS_OUTPUT_FORMAT);
    auto resolveBuffer = RenderTargetPool::Get(tempDesc);

    // Pick effect settings
    int32 maxTraceSamples = 60;
    switch (Graphics::SSRQuality)
    {
    case Quality::Low:
        maxTraceSamples = 20;
        break;
    case Quality::Medium:
        maxTraceSamples = 55;
        break;
    case Quality::High:
        maxTraceSamples = 70;
        break;
    case Quality::Ultra:
        maxTraceSamples = 120;
        break;
    }
    const int32 resolveSamples = settings.ResolveSamples;
    int32 resolvePassIndex = 0;
    if (resolveSamples >= 8)
        resolvePassIndex = 3;
    else if (resolveSamples >= 4)
        resolvePassIndex = 2;
    else if (resolveSamples >= 2)
        resolvePassIndex = 1;

    // Setup data
    Data data;
    GBufferPass::SetInputs(view, data.GBuffer);
    data.RoughnessFade = Math::Saturate(settings.RoughnessThreshold);
    data.MaxTraceSamples = static_cast<float>(maxTraceSamples);
    data.BRDFBias = settings.BRDFBias;
    data.WorldAntiSelfOcclusionBias = settings.WorldAntiSelfOcclusionBias;
    data.EdgeFadeFactor = settings.EdgeFadeFactor;
    data.SSRtexelSize = Vector2(1.0f / (float)traceWidth, 1.0f / (float)traceHeight);
    data.TraceSizeMax = (float)Math::Max(traceWidth, traceHeight);
    data.MaxColorMiplevel = settings.UseColorBufferMips ? (float)colorBufferMips - 2.0f : 0.0f;
    data.RayTraceStep = static_cast<float>(settings.DepthResolution) / (float)width;
    data.Intensity = settings.Intensity;
    data.FadeOutDistance = settings.FadeOutDistance;
    data.InvFadeDistance = 1.0f / settings.FadeDistance;
    data.TemporalScale = settings.TemporalScale;
    data.TemporalResponse = settings.TemporalResponse;
    data.NoTemporalEffect = useTemporal ? 0.0f : 1.0f;
    if (useTemporal)
    {
        const float time = Time::Draw.UnscaledTime.GetTotalSeconds();

        // Keep time in smaller range to prevent temporal noise errors
        const double scale = 10;
        const double integral = round(time / scale) * scale;
        data.TemporalTime = static_cast<float>(time - integral);

        renderContext.Buffers->LastFrameTemporalSSR = Engine::FrameCount;
        if (buffers->TemporalSSR == nullptr)
        {
            // Missing temporal buffer
            tempDesc = GPUTextureDescription::New2D(temporalWidth, temporalHeight, RESOLVE_PASS_OUTPUT_FORMAT);
            buffers->TemporalSSR = RenderTargetPool::Get(tempDesc);
        }
        else if (buffers->TemporalSSR->Width() != temporalWidth || buffers->TemporalSSR->Height() != temporalHeight)
        {
            // Wrong size temporal buffer
            RenderTargetPool::Release(buffers->TemporalSSR);
            tempDesc = GPUTextureDescription::New2D(temporalWidth, temporalHeight, RESOLVE_PASS_OUTPUT_FORMAT);
            buffers->TemporalSSR = RenderTargetPool::Get(tempDesc);
        }
    }
    else
    {
        data.TemporalTime = 0;
    }
    Matrix::Transpose(view.View, data.ViewMatrix);
    Matrix::Transpose(view.ViewProjection(), data.ViewProjectionMatrix);

    // Check if resize depth
    GPUTexture* originalDepthBuffer = buffers->DepthBuffer;
    GPUTexture* smallerDepthBuffer = originalDepthBuffer;
    if (settings.DepthResolution != ResolutionMode::Full)
    {
        // Smaller depth buffer improves ray tracing performance
        smallerDepthBuffer = buffers->RequestHalfResDepth(context);
    }

    // Prepare constants
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Bind GBuffer inputs
    context->BindSR(0, buffers->GBuffer0);
    context->BindSR(1, buffers->GBuffer1);
    context->BindSR(2, buffers->GBuffer2);
    context->BindSR(3, smallerDepthBuffer);

    // Combine pass
    context->BindSR(TEXTURE0, lightBuffer);
    context->BindSR(TEXTURE1, reflectionsRT);
    context->BindSR(TEXTURE2, _preIntegratedGF->GetTexture());
    context->SetViewportAndScissors((float)colorBufferWidth, (float)colorBufferHeight);
    context->SetRenderTarget(colorBuffer0->View(0));
    context->SetState(_psCombinePass);
    context->DrawFullscreenTriangle();
    context->UnBindSR(TEXTURE1);
    context->UnBindSR(TEXTURE2);
    context->ResetRenderTarget();

    // Blur Pass
    GPUTexture* blurPassBuffer;
    if (settings.UseColorBufferMips)
    {
        // Note: using color buffer mips maps helps with reducing artifacts
        // and improves resolve pass performance (faster color texture lookups, less cache misses)
        // Also for high surface roughness values it adds more blur to the reflection tail which looks more realistic.

        const auto filterMode = MultiScaler::FilterMode::GaussianBlur9;

        // Downscale with gaussian blur
        for (int32 mipLevel = 1; mipLevel < colorBufferMips; mipLevel++)
        {
            const int32 mipWidth = Math::Max(colorBufferWidth >> mipLevel, 1);
            const int32 mipHeight = Math::Max(colorBufferHeight >> mipLevel, 1);

            const auto srcMip = colorBuffer0->View(0, mipLevel - 1);
            const auto tmpMip = colorBuffer1->View(0, mipLevel);
            const auto dstMip = colorBuffer0->View(0, mipLevel);

            MultiScaler::Instance()->Filter(filterMode, context, mipWidth, mipHeight, srcMip, dstMip, tmpMip);
        }

        // Restore state
        context->BindCB(0, cb);
        context->BindSR(0, buffers->GBuffer0);

        // Use color buffer with full mip chain
        blurPassBuffer = colorBuffer0;
    }
    else
    {
        // Don't use color buffer with mip maps
        blurPassBuffer = colorBuffer0;
    }

    // Ray Trace Pass
    context->SetViewportAndScissors((float)traceWidth, (float)traceHeight);
    context->SetRenderTarget(*traceBuffer);
    context->SetState(_psRayTracePass);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Resolve Pass
    context->SetRenderTarget(resolveBuffer->View());
    context->BindSR(TEXTURE0, blurPassBuffer->View());
    context->BindSR(TEXTURE1, traceBuffer->View());
    context->SetState(_psResolvePass.Get(resolvePassIndex));
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Temporal Pass
    GPUTexture* reflectionsBuffer = resolveBuffer;
    if (useTemporal)
    {
        tempDesc = GPUTextureDescription::New2D(temporalWidth, temporalHeight, RESOLVE_PASS_OUTPUT_FORMAT);
        auto newTemporal = RenderTargetPool::Get(tempDesc);
        const auto oldTemporal = buffers->TemporalSSR;
        const auto motionVectors = buffers->MotionVectors;

        context->SetViewportAndScissors((float)temporalWidth, (float)temporalHeight);
        context->SetRenderTarget(newTemporal->View());
        context->BindSR(TEXTURE0, resolveBuffer);
        context->BindSR(TEXTURE1, oldTemporal);
        context->BindSR(TEXTURE2, motionVectors && motionVectors->IsAllocated() ? motionVectors->View() : nullptr);
        context->SetState(_psTemporalPass);
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();

        context->UnBindSR(TEXTURE2);

        // TODO: if those 2 buffers are the same maybe we could swap them internally to prevent data copy?

        context->CopyResource(oldTemporal, newTemporal);
        RenderTargetPool::Release(newTemporal);
        reflectionsBuffer = oldTemporal;
    }

    context->UnBindSR(TEXTURE1);

    // Mix Pass
    context->SetViewportAndScissors((float)width, (float)height);
    context->BindSR(TEXTURE0, reflectionsBuffer);
    context->SetRenderTarget(reflectionsRT);
    context->SetState(_psMixPass);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // Cleanup
    RenderTargetPool::Release(colorBuffer0);
    RenderTargetPool::Release(colorBuffer1);
    RenderTargetPool::Release(traceBuffer);
    RenderTargetPool::Release(resolveBuffer);
}
