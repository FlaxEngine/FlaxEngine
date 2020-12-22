// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "MotionBlurPass.h"
#include "GBufferPass.h"
#include "Renderer.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderBuffers.h"

PACK_STRUCT(struct Data {
    GBufferData GBuffer;
    Matrix CurrentVP;
    Matrix PreviousVP;
    Vector4 TemporalAAJitter;
    });

MotionBlurPass::MotionBlurPass()
    : _motionVectorsFormat(PixelFormat::Unknown)
    , _velocityFormat(PixelFormat::Unknown)
{
}

String MotionBlurPass::ToString() const
{
    return TEXT("MotionBlurPass");
}

bool MotionBlurPass::Init()
{
    // Create pipeline state
    _psCameraMotionVectors = GPUDevice::Instance->CreatePipelineState();

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/MotionBlur"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<MotionBlurPass, &MotionBlurPass::OnShaderReloading>(this);
#endif

    // Prepare formats for the buffers
    auto format = MOTION_VECTORS_PIXEL_FORMAT;
    if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->GetFormatFeatures(format).Support, (FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D)))
    {
        if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R32G32_Float).Support, (FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D)))
            format = PixelFormat::R32G32_Float;
        else
            format = PixelFormat::R32G32B32A32_Float;
    }
    _motionVectorsFormat = format;
    format = PixelFormat::R10G10B10A2_UNorm;
    if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->FeaturesPerFormat[(int32)format].Support, (FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D)))
    {
        format = PixelFormat::R32G32B32A32_Float;
    }
    _velocityFormat = format;

    return false;
}

bool MotionBlurPass::setupResources()
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
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psCameraMotionVectors->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_CameraMotionVectors");
        if (_psCameraMotionVectors->Init(psDesc))
            return true;
    }

    return false;
}

void MotionBlurPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Delete pipeline state
    SAFE_DELETE_GPU_RESOURCE(_psCameraMotionVectors);

    // Release asset
    _shader.Unlink();
}

void MotionBlurPass::RenderMotionVectors(RenderContext& renderContext)
{
    // Prepare
    auto motionVectors = renderContext.Buffers->MotionVectors;
    ASSERT(motionVectors);
    MotionBlurSettings& settings = renderContext.List->Settings.MotionBlur;
    auto context = GPUDevice::Instance->GetMainContext();
    const int32 screenWidth = renderContext.Buffers->GetWidth();
    const int32 screenHeight = renderContext.Buffers->GetHeight();
    const int32 motionVectorsWidth = screenWidth / static_cast<int32>(settings.MotionVectorsResolution);
    const int32 motionVectorsHeight = screenHeight / static_cast<int32>(settings.MotionVectorsResolution);

    // Ensure to have valid data
    if (!Renderer::NeedMotionVectors(renderContext) || checkIfSkipPass())
    {
        // Skip pass (just clear motion vectors if texture is allocated)
        if (motionVectors->IsAllocated())
        {
            if (motionVectors->Width() == motionVectorsWidth && motionVectors->Height() == motionVectorsHeight)
                context->Clear(motionVectors->View(), Color::Black);
            else
                motionVectors->ReleaseGPU();
        }
        return;
    }

    PROFILE_GPU_CPU("Motion Vectors");

    // Ensure to have valid motion vectors texture
    if (!motionVectors->IsAllocated() || motionVectors->Width() != motionVectorsWidth || motionVectors->Height() != motionVectorsHeight)
    {
        if (motionVectors->Init(GPUTextureDescription::New2D(motionVectorsWidth, motionVectorsHeight, _motionVectorsFormat, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget)))
        {
            LOG(Warning, "Failed to create  motion vectors render target.");
            return;
        }
    }

    // Prepare
    GPUTexture* depthBuffer;
    if (settings.MotionVectorsResolution != ResolutionMode::Full)
    {
        depthBuffer = renderContext.Buffers->RequestHalfResDepth(context);
        context->SetViewportAndScissors((float)motionVectorsWidth, (float)motionVectorsHeight);
    }
    else
    {
        depthBuffer = renderContext.Buffers->DepthBuffer;
    }

    // Bind input
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    Matrix::Transpose(renderContext.View.ViewProjection(), data.CurrentVP);
    Matrix::Transpose(renderContext.View.PrevViewProjection, data.PreviousVP);
    data.TemporalAAJitter = renderContext.View.TemporalAAJitter;
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, depthBuffer);

    // Render camera motion vectors (background)
    if (!data.TemporalAAJitter.IsZero() || data.CurrentVP != data.PreviousVP)
    {
        PROFILE_GPU_CPU("Camera Motion Vectors");
        context->SetRenderTarget(motionVectors->View());
        context->SetState(_psCameraMotionVectors);
        context->DrawFullscreenTriangle();
    }
    else
    {
        // Simple clear if camera is not moving
        context->Clear(motionVectors->View(), Color::Black);
    }

    // Render per-object motion vectors (use depth buffer to discard dynamic objects pixels covered by the static geometry)
    context->ResetSR();
    context->SetRenderTarget(depthBuffer->View(), motionVectors->View());
    renderContext.View.Pass = DrawPass::MotionVectors;
    // TODO: maybe update material PerFrame data because render viewport can be other size than ScreenSize from render buffers
    renderContext.List->SortDrawCalls(renderContext, false, DrawCallsListType::MotionVectors);
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::MotionVectors);

    // Cleanup
    context->ResetRenderTarget();
    if (settings.MotionVectorsResolution != ResolutionMode::Full)
    {
        const auto viewport = renderContext.Task->GetViewport();
        context->SetViewportAndScissors(viewport);
    }
}

void MotionBlurPass::RenderDebug(RenderContext& renderContext, GPUTextureView* frame)
{
    auto context = GPUDevice::Instance->GetMainContext();
    const auto motionVectors = renderContext.Buffers->MotionVectors;
    //if (!motionVectors->IsAllocated() || setupResources())
    {
        context->Draw(frame);
        return;
    }

    // ..
}

void MotionBlurPass::Render(RenderContext& renderContext, GPUTexture*& input, GPUTexture*& output)
{
    const bool isCameraCut = renderContext.Task->IsCameraCut;
    const auto motionVectors = renderContext.Buffers->MotionVectors;
    ASSERT(motionVectors);
    auto context = GPUDevice::Instance->GetMainContext();
    MotionBlurSettings& settings = renderContext.List->Settings.MotionBlur;
    const int32 screenWidth = renderContext.Buffers->GetWidth();
    const int32 screenHeight = renderContext.Buffers->GetHeight();
    const int32 motionVectorsWidth = screenWidth / static_cast<int32>(settings.MotionVectorsResolution);
    const int32 motionVectorsHeight = screenHeight / static_cast<int32>(settings.MotionVectorsResolution);

    // Ensure to have valid data
    if ((renderContext.View.Flags & ViewFlags::MotionBlur) == 0 ||
        !_hasValidResources ||
        isCameraCut ||
        screenWidth < 16 ||
        screenHeight < 16 ||
        !settings.Enabled ||
        settings.Scale <= 0.0f)
    {
        // Skip pass
        return;
    }

    // ..
}
