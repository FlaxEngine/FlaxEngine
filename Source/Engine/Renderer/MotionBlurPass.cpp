// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#include "Engine/Engine/Time.h"

PACK_STRUCT(struct Data {
    GBufferData GBuffer;
    Matrix CurrentVP;
    Matrix PreviousVP;
    Vector4 TemporalAAJitter;

    float VelocityScale;
    float Dummy0;
    int32 MaxBlurSamples;
    uint32 VariableTileLoopCount;

    Vector2 Input0SizeInv;
    Vector2 Input2SizeInv;
    });

MotionBlurPass::MotionBlurPass()
    : _motionVectorsFormat(PixelFormat::Unknown)
{
}

String MotionBlurPass::ToString() const
{
    return TEXT("MotionBlurPass");
}

bool MotionBlurPass::Init()
{
    // Create pipeline states
    _psCameraMotionVectors = GPUDevice::Instance->CreatePipelineState();
    _psMotionVectorsDebug = GPUDevice::Instance->CreatePipelineState();
    _psTileMax = GPUDevice::Instance->CreatePipelineState();
    _psTileMaxVariable = GPUDevice::Instance->CreatePipelineState();
    _psNeighborMax = GPUDevice::Instance->CreatePipelineState();
    _psMotionBlur = GPUDevice::Instance->CreatePipelineState();

    // Load shader
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/MotionBlur"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<MotionBlurPass, &MotionBlurPass::OnShaderReloading>(this);
#endif

    // Prepare formats for the buffers
    auto format = MOTION_VECTORS_PIXEL_FORMAT;
    if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->GetFormatFeatures(format).Support, FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D))
    {
        if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R32G32_Float).Support, FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D))
            format = PixelFormat::R32G32_Float;
        else if (FORMAT_FEATURES_ARE_NOT_SUPPORTED(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R16G16B16A16_Float).Support, FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D))
            format = PixelFormat::R16G16B16A16_Float;
        else
            format = PixelFormat::R32G32B32A32_Float;
    }
    _motionVectorsFormat = format;

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
    if (!_psMotionVectorsDebug->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_MotionVectorsDebug");
        if (_psMotionVectorsDebug->Init(psDesc))
            return true;
    }
    if (!_psTileMax->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMax");
        if (_psTileMax->Init(psDesc))
            return true;
    }
    if (!_psTileMaxVariable->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMaxVariable");
        if (_psTileMaxVariable->Init(psDesc))
            return true;
    }
    if (!_psNeighborMax->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_NeighborMax");
        if (_psNeighborMax->Init(psDesc))
            return true;
    }
    if (!_psMotionBlur->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_MotionBlur");
        if (_psMotionBlur->Init(psDesc))
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
    SAFE_DELETE_GPU_RESOURCE(_psMotionVectorsDebug);
    SAFE_DELETE_GPU_RESOURCE(_psTileMax);
    SAFE_DELETE_GPU_RESOURCE(_psTileMaxVariable);
    SAFE_DELETE_GPU_RESOURCE(_psNeighborMax);
    SAFE_DELETE_GPU_RESOURCE(_psMotionBlur);

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
    if (!motionVectors->IsAllocated() || setupResources())
    {
        context->Draw(frame);
        return;
    }

    PROFILE_GPU_CPU("Motion Vectors Debug");
    context->BindSR(0, frame);
    context->BindSR(1, renderContext.Buffers->MotionVectors->View());
    context->SetState(_psMotionVectorsDebug);
    context->DrawFullscreenTriangle();
    context->ResetSR();
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

    // Need to have valid motion vectors created and rendered before
    ASSERT(motionVectors->IsAllocated());

    PROFILE_GPU_CPU("Motion Blur");

    // Setup shader inputs
    const int32 maxBlurSize = Math::Max((int32)((float)motionVectorsHeight * 0.05f), 1);
    const int32 tileSize = Math::AlignUp(maxBlurSize, 8);
    const float timeScale = renderContext.Task->View.IsOfflinePass ? 1.0f : 1.0f / Time::Draw.UnscaledDeltaTime.GetTotalSeconds() / 60.0f; // 60fps as a reference
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    data.TemporalAAJitter = renderContext.View.TemporalAAJitter;
    data.VelocityScale = settings.Scale * 0.5f * timeScale; // 2x samples in loop
    data.MaxBlurSamples = Math::Clamp(settings.SampleCount / 2, 1, 64); // 2x samples in loop
    data.VariableTileLoopCount = tileSize / 8;
    data.Input0SizeInv = Vector2(1.0f / (float)motionVectorsWidth, 1.0f / (float)motionVectorsWidth);
    const auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);

    // Downscale motion vectors texture down to 1/2 (with max velocity calculation 2x2 kernel)
    auto rtDesc = GPUTextureDescription::New2D(motionVectorsWidth / 2, motionVectorsHeight / 2, _motionVectorsFormat);
    const auto vMaxBuffer2 = RenderTargetPool::Get(rtDesc);
    context->SetRenderTarget(vMaxBuffer2->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, motionVectors->View());
    context->SetState(_psTileMax);
    context->DrawFullscreenTriangle();

    // Downscale motion vectors texture down to 1/4 (with max velocity calculation 2x2 kernel)
    rtDesc.Width /= 2;
    rtDesc.Height /= 2;
    const auto vMaxBuffer4 = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(vMaxBuffer4->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, vMaxBuffer2->View());
    data.Input0SizeInv = Vector2(1.0f / (float)vMaxBuffer2->Width(), 1.0f / (float)vMaxBuffer2->Height());
    context->UpdateCB(cb, &data);
    context->SetState(_psTileMax);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(vMaxBuffer2);

    // Downscale motion vectors texture down to 1/8 (with max velocity calculation 2x2 kernel)
    rtDesc.Width /= 2;
    rtDesc.Height /= 2;
    const auto vMaxBuffer8 = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(vMaxBuffer8->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, vMaxBuffer4->View());
    data.Input0SizeInv = Vector2(1.0f / (float)vMaxBuffer4->Width(), 1.0f / (float)vMaxBuffer4->Height());
    context->UpdateCB(cb, &data);
    context->SetState(_psTileMax);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(vMaxBuffer4);

    // Downscale motion vectors texture down to tileSize/tileSize (with max velocity calculation NxN kernel)
    rtDesc.Width = Math::Max(motionVectorsWidth / tileSize, 1);
    rtDesc.Height = Math::Max(motionVectorsHeight / tileSize, 1);
    auto vMaxBuffer = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(vMaxBuffer->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, vMaxBuffer8->View());
    data.Input0SizeInv = Vector2(1.0f / (float)vMaxBuffer8->Width(), 1.0f / (float)vMaxBuffer8->Height());
    context->UpdateCB(cb, &data);
    context->SetState(_psTileMaxVariable);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(vMaxBuffer8);

    // Extract maximum velocities for the tiles based on their neighbors
    context->ResetRenderTarget();
    auto vMaxNeighborBuffer = RenderTargetPool::Get(rtDesc);
    context->SetRenderTarget(vMaxNeighborBuffer->View());
    context->BindSR(0, vMaxBuffer->View());
    context->SetState(_psNeighborMax);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(vMaxBuffer);

    // Render motion blur
    context->ResetRenderTarget();
    context->SetRenderTarget(*output);
    context->SetViewportAndScissors((float)screenWidth, (float)screenHeight);
    context->BindSR(0, input->View());
    context->BindSR(1, motionVectors->View());
    context->BindSR(2, vMaxNeighborBuffer->View());
    context->BindSR(3, renderContext.Buffers->DepthBuffer->View());
    data.Input0SizeInv = Vector2(1.0f / (float)input->Width(), 1.0f / (float)input->Height());
    data.Input2SizeInv = Vector2(1.0f / (float)renderContext.Buffers->DepthBuffer->Width(), 1.0f / (float)renderContext.Buffers->DepthBuffer->Height());
    context->UpdateCB(cb, &data);
    context->SetState(_psMotionBlur);
    context->DrawFullscreenTriangle();

    // Cleanup
    RenderTargetPool::Release(vMaxNeighborBuffer);
    context->ResetSR();
    context->ResetRenderTarget();
    Swap(output, input);
}
