// Copyright (c) Wojciech Figat. All rights reserved.

#include "EyeAdaptationPass.h"
#include "HistogramPass.h"
#include "RenderList.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"

GPU_CB_STRUCT(EyeAdaptationData {
    float MinBrightness;
    float MaxBrightness;
    float SpeedUp;
    float SpeedDown;

    float PreExposure;
    float DeltaTime;
    float HistogramMul;
    float HistogramAdd;

    float HistogramLowPercent;
    float HistogramHighPercent;
    float DropHistory;
    float Dummy1;
    });

void EyeAdaptationPass::Render(RenderContext& renderContext, GPUTexture* colorBuffer)
{
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& view = renderContext.View;
    auto& settings = renderContext.List->Settings.EyeAdaptation;
    bool dropHistory = renderContext.Buffers->LastEyeAdaptationTime < ZeroTolerance || renderContext.Task->IsCameraCut;
    const float time = Time::Draw.UnscaledTime.GetTotalSeconds();
    const float frameDelta = Math::Clamp(time - renderContext.Buffers->LastEyeAdaptationTime, 0.0f, 1.0f);
    renderContext.Buffers->LastEyeAdaptationTime = time; // FIXED: Was incorrectly set to 0.0f
    if ((view.Flags & ViewFlags::EyeAdaptation) == ViewFlags::None || settings.Mode == EyeAdaptationMode::None || checkIfSkipPass())
        return;
    PROFILE_GPU_CPU("Eye Adaptation");

    // Setup constants
    GPUBuffer* histogramBuffer = nullptr;
    EyeAdaptationData data;
    // INVERSE LOGIC: Swap min/max brightness for inverse adaptation
    data.MinBrightness = Math::Max(settings.MaxBrightness, 0.0001f); // Now using max as min
    data.MaxBrightness = Math::Max(settings.MinBrightness, 0.0001f); // Now using min as max
    data.PreExposure = Math::Exp2(-settings.PreExposure); // Inverted exposure
    auto mode = settings.Mode == EyeAdaptationMode::AutomaticHistogram && !_canUseHistogram ? EyeAdaptationMode::AutomaticAverageLuminance : settings.Mode;
    
    switch (mode)
    {
    case EyeAdaptationMode::Manual:
    {
        if (Math::IsZero(settings.PreExposure))
            return;
        break;
    }
    case EyeAdaptationMode::AutomaticHistogram:
    {
        ASSERT(_canUseHistogram);
        // Invert percentile ranges for inverse adaptation
        data.HistogramLowPercent = Math::Clamp((100.0f - settings.HistogramHighPercent) * 0.01f, 0.01f, 0.99f);
        data.HistogramHighPercent = Math::Clamp((100.0f - settings.HistogramLowPercent) * 0.01f, data.HistogramLowPercent, 1.0f);
        HistogramPass::Instance()->GetHistogramMad(data.HistogramMul, data.HistogramAdd);

        histogramBuffer = HistogramPass::Instance()->Render(renderContext, colorBuffer);
        if (!histogramBuffer)
            return;
        break;
    }
    case EyeAdaptationMode::AutomaticAverageLuminance:
    {
        dropHistory |= renderContext.Buffers->LuminanceMap == nullptr;
        break;
    }
    default: ;
    }
    
    // Invert speed directions for inverse adaptation
    data.SpeedUp = settings.SpeedDown; // SpeedUp now uses SpeedDown value
    data.SpeedDown = settings.SpeedUp; // SpeedDown now uses SpeedUp value
    data.DeltaTime = frameDelta;
    data.DropHistory = dropHistory ? 1.0f : 0.0f;

    // Rest of the implementation remains structurally the same...
    const auto shader = _shader->GetShader();
    const auto cb0 = shader->GetCB(0);
    context->UpdateCB(cb0, &data);
    context->BindCB(0, cb0);

    if (mode == EyeAdaptationMode::Manual)
    {
        context->SetRenderTarget(*colorBuffer);
        context->SetViewportAndScissors((float)colorBuffer->Width(), (float)colorBuffer->Height());
        context->SetState(_psManual);
        context->DrawFullscreenTriangle();
        return;
    }

    GPUTexture* previousLuminanceMap = renderContext.Buffers->LuminanceMap;
    if (dropHistory && previousLuminanceMap)
    {
        RenderTargetPool::Release(previousLuminanceMap);
        previousLuminanceMap = nullptr;
    }
    GPUTexture* currentLuminanceMap = RenderTargetPool::Get(GPUTextureDescription::New2D(1, 1, PixelFormat::R16_Float));
    RENDER_TARGET_POOL_SET_NAME(currentLuminanceMap, "EyeAdaptation.LuminanceMap");

    switch (mode)
    {
    case EyeAdaptationMode::AutomaticHistogram:
    {
        context->BindSR(0, histogramBuffer->View());
        context->BindSR(1, previousLuminanceMap);
        context->SetRenderTarget(*currentLuminanceMap);
        context->SetViewportAndScissors(1, 1);
        context->SetState(_psHistogram);
        context->DrawFullscreenTriangle();
        context->UnBindSR(1);
        context->ResetRenderTarget();
        break;
    }
    case EyeAdaptationMode::AutomaticAverageLuminance:
    {
        const Int2 luminanceMapSize(colorBuffer->Width() / 2, colorBuffer->Height() / 2);
        GPUTexture* luminanceMap = RenderTargetPool::Get(GPUTextureDescription::New2D(luminanceMapSize.X, luminanceMapSize.Y, 0, PixelFormat::R16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews));
        RENDER_TARGET_POOL_SET_NAME(luminanceMap, "EyeAdaptation.LuminanceMap");
        
        context->BindSR(0, *colorBuffer);
        context->SetRenderTarget(luminanceMap->View(0, 0));
        context->SetViewportAndScissors((float)luminanceMapSize.X, (float)luminanceMapSize.Y);
        context->SetState(_psLuminanceMap);
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();

        int32 mipLevel = 1;
        Int2 mipSize = luminanceMapSize / 2;
        const int32 totalMips = luminanceMap->MipLevels();
        while (mipLevel < totalMips)
        {
            context->SetRenderTarget(luminanceMap->View(0, mipLevel));
            context->SetViewportAndScissors((float)mipSize.X, (float)mipSize.Y);
            context->Draw(luminanceMap->View(0, mipLevel - 1));
            context->ResetRenderTarget();

            if (mipSize.X > 1)
                mipSize.X >>= 1;
            if (mipSize.Y > 1)
                mipSize.Y >>= 1;
            mipLevel++;
        }

        if (dropHistory)
        {
            context->SetRenderTarget(*currentLuminanceMap);
            context->SetViewportAndScissors(1, 1);
            context->Draw(luminanceMap->View(0, mipLevel - 1));
            context->ResetRenderTarget();
        }
        else
        {
            context->BindSR(0, luminanceMap->View(0, mipLevel - 1));
            context->BindSR(1, previousLuminanceMap);
            context->SetRenderTarget(*currentLuminanceMap);
            context->SetViewportAndScissors(1, 1);
            context->SetState(_psBlendLuminance);
            context->DrawFullscreenTriangle();
            context->UnBindSR(1);
            context->ResetRenderTarget();
        }

        RenderTargetPool::Release(luminanceMap);
        break;
    }
    default: ;
    }

    context->BindSR(0, *currentLuminanceMap);
    context->SetRenderTarget(*colorBuffer);
    context->SetViewportAndScissors((float)colorBuffer->Width(), (float)colorBuffer->Height());
    context->SetState(_psApplyLuminance);
    context->DrawFullscreenTriangle();
    context->UnBindSR(0);

    renderContext.Buffers->LastFrameLuminanceMap = Engine::FrameCount;
    renderContext.Buffers->LuminanceMap = currentLuminanceMap;

    if (previousLuminanceMap)
        RenderTargetPool::Release(previousLuminanceMap);
}

// ... (rest of the file remains unchanged)
