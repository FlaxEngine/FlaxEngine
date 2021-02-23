// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "AmbientOcclusionPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Utilities/StringConverter.h"
#include "RenderList.h"
#include "GBufferPass.h"

AmbientOcclusionPass::ASSAO_Settings::ASSAO_Settings()
{
    Radius = 1.2f;
    ShadowMultiplier = 1.0f;
    ShadowPower = 1.50f;
    HorizonAngleThreshold = 0.06f;
    FadeOutFrom = 4500.0f;
    FadeOutTo = 5000.0f;
    QualityLevel = 2;
    BlurPassCount = 2;
    Sharpness = 0.68f;
    DetailShadowStrength = 0.5f;
    SkipHalfPixels = false;
}

// Validate config
static_assert(SSAO_DEPTH_MIP_LEVELS > 1, "Invalid amount of SSAO cache depth buffer mip levels.");
static_assert(SSAO_MAX_BLUR_PASS_COUNT >= 0, "Invalid maximum amount of SSAO blur passes.");

// Shader Resource slots mapping (must match shader source)
#define SSAO_CONSTANTS_BUFFER_SLOT 0
#define SSAO_TEXTURE_SLOT0 0
#define SSAO_TEXTURE_SLOT1 1
#define SSAO_TEXTURE_SLOT2 2
#define SSAO_TEXTURE_SLOT3 3

// Note: to boost performance a little bit we render final AO in full res to GBuffer surface which contains material AO term.
#define SSAO_APPLY_OUTPUT_FORMAT GBUFFER2_FORMAT

AmbientOcclusionPass::AmbientOcclusionPass()
{
    _psPrepareDepths = nullptr;
    _psPrepareDepthsHalf = nullptr;
    Platform::MemoryClear(_psPrepareDepthMip, sizeof(_psPrepareDepthMip));
    Platform::MemoryClear(_psGenerate, sizeof(_psGenerate));
    _psSmartBlur = nullptr;
    _psSmartBlurWide = nullptr;
    _psNonSmartBlur = nullptr;
    _psApply = nullptr;
    _psApplyHalf = nullptr;
}

String AmbientOcclusionPass::ToString() const
{
    return TEXT("AmbientOcclusionPass");
}

bool AmbientOcclusionPass::Init()
{
    // Create pipeline states
    _psPrepareDepths = GPUDevice::Instance->CreatePipelineState();
    _psPrepareDepthsHalf = GPUDevice::Instance->CreatePipelineState();
    for (int32 i = 0; i < ARRAY_COUNT(_psPrepareDepthMip); i++)
        _psPrepareDepthMip[i] = GPUDevice::Instance->CreatePipelineState();
    for (int32 i = 0; i < ARRAY_COUNT(_psGenerate); i++)
        _psGenerate[i] = GPUDevice::Instance->CreatePipelineState();
    _psSmartBlur = GPUDevice::Instance->CreatePipelineState();
    _psSmartBlurWide = GPUDevice::Instance->CreatePipelineState();
    _psNonSmartBlur = GPUDevice::Instance->CreatePipelineState();
    _psApply = GPUDevice::Instance->CreatePipelineState();
    _psApplyHalf = GPUDevice::Instance->CreatePipelineState();

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/SSAO"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<AmbientOcclusionPass, &AmbientOcclusionPass::OnShaderReloading>(this);
#endif

    return false;
}

bool AmbientOcclusionPass::setupResources()
{
    // Check shader
    if (!_shader->IsLoaded())
    {
        return true;
    }
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(ASSAOConstants))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, ASSAOConstants);
        return true;
    }

    // Create pipeline states
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    // Prepare Depths
    if (!_psPrepareDepths->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_PrepareDepths");
        if (_psPrepareDepths->Init(psDesc))
            return true;
    }
    if (!_psPrepareDepthsHalf->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_PrepareDepthsHalf");
        if (_psPrepareDepthsHalf->Init(psDesc))
            return true;
    }
    // Prepare Depth Mips
    for (int32 i = 0; i < ARRAY_COUNT(_psPrepareDepthMip); i++)
    {
        if (!_psPrepareDepthMip[i]->IsValid())
        {
            const auto str = String::Format(TEXT("PS_PrepareDepthMip{0}"), i + 1);
            const StringAsANSI<50> strAnsi(*str, str.Length());
            psDesc.PS = shader->GetPS(strAnsi.Get());
            if (_psPrepareDepthMip[i]->Init(psDesc))
                return true;
        }
    }
    // AO Generate
    for (int32 i = 0; i < ARRAY_COUNT(_psGenerate); i++)
    {
        if (!_psGenerate[i]->IsValid())
        {
            const auto str = String::Format(TEXT("PS_GenerateQ{0}"), i);
            const StringAsANSI<50> strAnsi(*str, str.Length());
            psDesc.PS = shader->GetPS(strAnsi.Get());
            if (_psGenerate[i]->Init(psDesc))
                return true;
        }
    }
    // Blur
    if (!_psSmartBlur->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_SmartBlur");
        if (_psSmartBlur->Init(psDesc))
            return true;
    }
    if (!_psSmartBlurWide->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_SmartBlurWide");
        if (_psSmartBlurWide->Init(psDesc))
            return true;
    }
    if (!_psNonSmartBlur->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_NonSmartBlur");
        if (_psNonSmartBlur->Init(psDesc))
            return true;
    }
    // Apply AO
    psDesc.BlendMode = BlendingMode::Multiply;
    psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::Alpha;
    if (!_psApply->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_Apply");
        if (_psApply->Init(psDesc))
            return true;
    }
    if (!_psApplyHalf->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_ApplyHalf");
        if (_psApplyHalf->Init(psDesc))
            return true;
    }

    return false;
}

void AmbientOcclusionPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Delete pipeline states
    SAFE_DELETE_GPU_RESOURCE(_psPrepareDepths);
    SAFE_DELETE_GPU_RESOURCE(_psPrepareDepthsHalf);
    for (int32 i = 0; i < ARRAY_COUNT(_psPrepareDepthMip); i++)
        SAFE_DELETE_GPU_RESOURCE(_psPrepareDepthMip[i]);
    for (int32 i = 0; i < ARRAY_COUNT(_psGenerate); i++)
        SAFE_DELETE_GPU_RESOURCE(_psGenerate[i]);
    SAFE_DELETE_GPU_RESOURCE(_psSmartBlur);
    SAFE_DELETE_GPU_RESOURCE(_psSmartBlurWide);
    SAFE_DELETE_GPU_RESOURCE(_psNonSmartBlur);
    SAFE_DELETE_GPU_RESOURCE(_psApply);
    SAFE_DELETE_GPU_RESOURCE(_psApplyHalf);

    // Release asset
    _shader = nullptr;
}

void AmbientOcclusionPass::Render(RenderContext& renderContext)
{
    // Check if can render the effect
    if (renderContext.List == nullptr)
        return;
    auto& aoSettings = renderContext.List->Settings.AmbientOcclusion;
    if (aoSettings.Enabled == false || (renderContext.View.Flags & ViewFlags::AO) == 0)
        return;

    // TODO: add support for SSAO in ortho projection
    if (renderContext.View.IsOrthographicProjection())
        return;

    // Ensure to have valid data
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering.
        return;
    }

    PROFILE_GPU_CPU("Ambient Occlusion");

    settings.Radius = aoSettings.Radius * 0.006f;
    settings.ShadowMultiplier = aoSettings.Intensity;
    settings.ShadowPower = aoSettings.Power;
    settings.FadeOutTo = aoSettings.FadeOutDistance;
    settings.FadeOutFrom = aoSettings.FadeOutDistance - aoSettings.FadeDistance;

    // expose param for HorizonAngleThreshold ?
    // expose param for Sharpness ?
    // expose param for DetailShadowStrength ?

    // Apply quality level ASSAO to the settings
    switch (Graphics::SSAOQuality)
    {
    case Quality::Low:
    {
        settings.QualityLevel = 1;
        settings.BlurPassCount = 2;
        settings.SkipHalfPixels = true;
        break;
    }
    case Quality::Medium:
    {
        settings.QualityLevel = 2;
        settings.BlurPassCount = 2;
        settings.SkipHalfPixels = false;
        break;
    }
    case Quality::High:
    {
        settings.QualityLevel = 2;
        settings.BlurPassCount = 3;
        settings.SkipHalfPixels = false;
        break;
    }
    case Quality::Ultra:
    {
        settings.QualityLevel = 3;
        settings.BlurPassCount = 3;
        settings.SkipHalfPixels = false;
        break;
    }
    }

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    m_sizeX = (float)renderContext.Buffers->GetWidth();
    m_sizeY = (float)renderContext.Buffers->GetHeight();
    m_halfSizeX = (m_sizeX + 1) / 2;
    m_halfSizeY = (m_sizeY + 1) / 2;

    // Request temporary buffers
    InitRTs(renderContext);

    // Update and bind constant buffer
    UpdateCB(renderContext, context, 0);
    context->BindCB(SSAO_CONSTANTS_BUFFER_SLOT, _shader->GetShader()->GetCB(SSAO_CONSTANTS_BUFFER_SLOT));

    // Generate depths
    PrepareDepths(renderContext);

    // Generate SSAO
    GenerateSSAO(renderContext);

    // Apply
    context->BindSR(SSAO_TEXTURE_SLOT0, m_finalResults->ViewArray());
    context->SetViewportAndScissors(m_sizeX, m_sizeY);
    context->SetState(settings.SkipHalfPixels ? _psApplyHalf : _psApply);
    context->SetRenderTarget(renderContext.Buffers->GBuffer0->View());
    context->DrawFullscreenTriangle();

    // Release and cleanup
    ReleaseRTs(renderContext);
    context->ResetRenderTarget();
    context->ResetSR();
}

void AmbientOcclusionPass::InitRTs(const RenderContext& renderContext)
{
    GPUTextureDescription tempDesc;
    for (int i = 0; i < 4; i++)
    {
        // TODO: maybe instead of using whole mip chain request only SSAO_DEPTH_MIP_LEVELS?
        tempDesc = GPUTextureDescription::New2D((int32)m_halfSizeX, (int32)m_halfSizeY, 0, SSAO_DEPTH_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews);
        m_halfDepths[i] = RenderTargetPool::Get(tempDesc);
    }
    tempDesc = GPUTextureDescription::New2D((int32)m_halfSizeX, (int32)m_halfSizeY, SSAO_AO_RESULT_FORMAT);
    m_pingPongHalfResultA = RenderTargetPool::Get(tempDesc);
    tempDesc = GPUTextureDescription::New2D((int32)m_halfSizeX, (int32)m_halfSizeY, SSAO_AO_RESULT_FORMAT);
    m_pingPongHalfResultB = RenderTargetPool::Get(tempDesc);
    tempDesc = GPUTextureDescription::New2D((int32)m_halfSizeX, (int32)m_halfSizeY, SSAO_AO_RESULT_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget, 4);
    m_finalResults = RenderTargetPool::Get(tempDesc);
}

void AmbientOcclusionPass::ReleaseRTs(const RenderContext& renderContext)
{
    for (int i = 0; i < 4; i++)
        RenderTargetPool::Release(m_halfDepths[i]);
    for (int i = 0; i < 4; i++)
        m_halfDepths[i] = nullptr;
    RenderTargetPool::Release(m_pingPongHalfResultA);
    m_pingPongHalfResultA = nullptr;
    RenderTargetPool::Release(m_pingPongHalfResultB);
    m_pingPongHalfResultB = nullptr;
    RenderTargetPool::Release(m_finalResults);
    m_finalResults = nullptr;
}

void AmbientOcclusionPass::UpdateCB(const RenderContext& renderContext, GPUContext* context, const int32 passIndex)
{
    // Cache data
    const auto& view = renderContext.View;
    const float nearPlane = view.Near;
    const float farPlane = view.Far;
    const Matrix& proj = view.Projection;

    GBufferPass::SetInputs(view, _constantsBufferData.GBuffer);
    Matrix::Transpose(view.View, _constantsBufferData.ViewMatrix);

    _constantsBufferData.ViewportPixelSize = Vector2(1.0f / m_sizeX, 1.0f / m_sizeY);
    _constantsBufferData.HalfViewportPixelSize = Vector2(1.0f / m_halfSizeX, 1.0f / m_halfSizeY);

    _constantsBufferData.Viewport2xPixelSize = Vector2(_constantsBufferData.ViewportPixelSize.X * 2.0f, _constantsBufferData.ViewportPixelSize.Y * 2.0f);
    _constantsBufferData.Viewport2xPixelSize_x_025 = Vector2(_constantsBufferData.Viewport2xPixelSize.X * 0.25f, _constantsBufferData.Viewport2xPixelSize.Y * 0.25f);

    const float tanHalfFOVY = 1.0f / proj.Values[1][1];

    _constantsBufferData.EffectRadius = Math::Clamp(settings.Radius / farPlane * 10000.0f, 0.0f, 100000.0f);
    _constantsBufferData.EffectShadowStrength = Math::Clamp(settings.ShadowMultiplier * 4.3f, 0.0f, 10.0f);
    _constantsBufferData.EffectShadowPow = Math::Clamp(settings.ShadowPower, 0.0f, 10.0f);
    _constantsBufferData.EffectHorizonAngleThreshold = Math::Clamp(settings.HorizonAngleThreshold, 0.0f, 1.0f);

    // Effect fade params
    const float fadeOutFrom = Math::Min(settings.FadeOutFrom, farPlane - 200);
    const float fadeOutTo = Math::Min(settings.FadeOutTo, farPlane - 50);
    _constantsBufferData.EffectMaxDistance = fadeOutTo / farPlane;
    _constantsBufferData.EffectFadeOutMul = 1.0f / ((fadeOutTo - fadeOutFrom) / farPlane);
    _constantsBufferData.EffectFadeOutAdd = (-fadeOutFrom / farPlane) * _constantsBufferData.EffectFadeOutMul;
    _constantsBufferData.EffectNearFadeMul = farPlane / (settings.Radius * 2400.0f);

    // 1.2 seems to be around the best trade off - 1.0 means on-screen radius will stop/slow growing when the camera is at 1.0 distance, so, depending on FOV, basically filling up most of the screen
    // This setting is viewspace-dependent and not screen size dependent intentionally, so that when you change FOV the effect stays (relatively) similar.
    float effectSamplingRadiusNearLimit = (settings.Radius * 1.2f);

    // if the depth precision is switched to 32bit float, this can be set to something closer to 1 (0.9999 is fine)
    _constantsBufferData.DepthPrecisionOffsetMod = 0.9992f;

    // Special settings for lowest quality level - just nerf the effect a tiny bit
    if (settings.QualityLevel == 0)
    {
        //_constantsBufferData.EffectShadowStrength *= 0.9f;
        effectSamplingRadiusNearLimit *= 1.50f;
    }
    effectSamplingRadiusNearLimit /= tanHalfFOVY; // to keep the effect same regardless of FOV
    if (settings.SkipHalfPixels)
        _constantsBufferData.EffectRadius *= 0.8f;

    _constantsBufferData.EffectSamplingRadiusNearLimitRec = 1.0f / effectSamplingRadiusNearLimit;

    _constantsBufferData.NegRecEffectRadius = -1.0f / _constantsBufferData.EffectRadius;

    _constantsBufferData.PerPassFullResCoordOffsetX = passIndex % 2;
    _constantsBufferData.PerPassFullResCoordOffsetY = passIndex / 2;

    _constantsBufferData.DetailAOStrength = settings.DetailShadowStrength;
    _constantsBufferData.InvSharpness = Math::Clamp(1.0f - settings.Sharpness, 0.0f, 1.0f);
    _constantsBufferData.PassIndex = passIndex;

    const int32 subPassCount = 5;
    const float spmap[5]{ 0.0f, 1.0f, 4.0f, 3.0f, 2.0f };
    const float a = static_cast<float>(passIndex);

    for (int32 subPass = 0; subPass < subPassCount; subPass++)
    {
        const float b = spmap[subPass];

        const float angle0 = (a + b / subPassCount) * PI * 0.5f;
        const float ca = Math::Cos(angle0);
        const float sa = Math::Sin(angle0);

        const float scale = 1.0f + (a - 1.5f + (b - (subPassCount - 1.0f) * 0.5f) / static_cast<float>(subPassCount)) * 0.07f;
        _constantsBufferData.PatternRotScaleMatrices[subPass] = Vector4(scale * ca, scale * -sa, -scale * sa, -scale * ca);
    }

    // Update buffer
    const auto cb = _shader->GetShader()->GetCB(SSAO_CONSTANTS_BUFFER_SLOT);
    context->UpdateCB(cb, &_constantsBufferData);
    context->BindCB(SSAO_CONSTANTS_BUFFER_SLOT, cb);
}

void AmbientOcclusionPass::PrepareDepths(const RenderContext& renderContext)
{
    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();

    // Bind scene depth buffer and set proper viewport
    context->BindSR(SSAO_TEXTURE_SLOT0, renderContext.Buffers->DepthBuffer);
    context->SetViewportAndScissors(m_halfSizeX, m_halfSizeY);

    // Prepare depth in half resolution
    {
        if (settings.SkipHalfPixels)
        {
            GPUTextureView* twoDepths[] =
            {
                m_halfDepths[0]->View(),
                m_halfDepths[3]->View()
            };
            context->SetRenderTarget(nullptr, ToSpan(twoDepths, ARRAY_COUNT(twoDepths)));
            context->SetState(_psPrepareDepthsHalf);
        }
        else
        {
            GPUTextureView* fourDepths[] =
            {
                m_halfDepths[0]->View(),
                m_halfDepths[1]->View(),
                m_halfDepths[2]->View(),
                m_halfDepths[3]->View()
            };
            context->SetRenderTarget(nullptr, ToSpan(fourDepths, ARRAY_COUNT(fourDepths)));
            context->SetState(_psPrepareDepths);
        }
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();
    }

    // Only do mipmaps for higher quality levels (not beneficial on quality level 1, and detrimental on quality level 0)
    if (settings.QualityLevel > 1)
    {
        for (int i = 1; i < SSAO_DEPTH_MIP_LEVELS; i++)
        {
            GPUTextureView* fourDepthMips[] =
            {
                m_halfDepths[0]->View(0, i),
                m_halfDepths[1]->View(0, i),
                m_halfDepths[2]->View(0, i),
                m_halfDepths[3]->View(0, i)
            };
            context->SetRenderTarget(nullptr, ToSpan(fourDepthMips, ARRAY_COUNT(fourDepthMips)));

            int32 mipWidth, mipHeight;
            m_halfDepths[0]->GetMipSize(i, mipWidth, mipHeight);
            context->SetViewportAndScissors((float)mipWidth, (float)mipHeight);

            context->BindSR(SSAO_TEXTURE_SLOT0, m_halfDepths[0]->View(0, i - 1));
            context->BindSR(SSAO_TEXTURE_SLOT1, m_halfDepths[1]->View(0, i - 1));
            context->BindSR(SSAO_TEXTURE_SLOT2, m_halfDepths[2]->View(0, i - 1));
            context->BindSR(SSAO_TEXTURE_SLOT3, m_halfDepths[3]->View(0, i - 1));

            context->SetState(_psPrepareDepthMip[i - 1]);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
        }
    }
}

void AmbientOcclusionPass::GenerateSSAO(const RenderContext& renderContext)
{
    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    const auto normalMapSRV = renderContext.Buffers->GBuffer1;

    // Prepare
    context->SetViewportAndScissors(m_halfSizeX, m_halfSizeY);

    // Render AO interleaved in checkerboard pattern
    for (int32 pass = 0; pass < 4; pass++)
    {
        if (settings.SkipHalfPixels && ((pass == 1) || (pass == 2)))
            continue;

        int32 blurPasses = settings.BlurPassCount;
        blurPasses = Math::Min(blurPasses, SSAO_MAX_BLUR_PASS_COUNT);

        if (settings.QualityLevel == 3)
        {
            blurPasses = Math::Max(1, blurPasses);
        }
        else if (settings.QualityLevel == 0)
        {
            // just one blur pass allowed for minimum quality 
            blurPasses = Math::Min(1, settings.BlurPassCount);
        }

        if (pass > 0)
            UpdateCB(renderContext, context, pass);

        auto pPingRT = m_pingPongHalfResultA->View();
        auto pPongRT = m_pingPongHalfResultB->View();

        // Generate Pass
        {
            auto rts = pPingRT;

            // no blur?
            if (blurPasses == 0)
                rts = m_finalResults->View(pass);

            context->SetRenderTarget(rts);
            context->BindSR(SSAO_TEXTURE_SLOT0, m_halfDepths[pass]);
            context->BindSR(SSAO_TEXTURE_SLOT1, normalMapSRV);
            context->SetState(_psGenerate[settings.QualityLevel]);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
        }

        // Blur Pass
        if (blurPasses > 0)
        {
            int wideBlursRemaining = Math::Max(0, blurPasses - 2);

            for (int i = 0; i < blurPasses; i++)
            {
                auto rts = pPongRT;

                // Last pass?
                if (i == (blurPasses - 1))
                    rts = m_finalResults->View(pass);

                context->SetRenderTarget(rts);
                context->BindSR(SSAO_TEXTURE_SLOT0, pPingRT);

                if (settings.QualityLevel == 0)
                {
                    context->SetState(_psNonSmartBlur);
                }
                else
                {
                    if (wideBlursRemaining > 0)
                    {
                        context->SetState(_psSmartBlurWide);
                        wideBlursRemaining--;
                    }
                    else
                    {
                        context->SetState(_psSmartBlur);
                    }
                }
                context->DrawFullscreenTriangle();
                context->ResetRenderTarget();

                Swap(pPingRT, pPongRT);
            }
        }
    }
}
