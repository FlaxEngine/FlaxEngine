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

MotionBlurPass::MotionBlurPass()
    : _motionVectorsFormat(PixelFormat::Unknown)
    , _velocityFormat(PixelFormat::Unknown)
    , _psCameraMotionVectors(nullptr)
    , _psMotionVectorsDebug(nullptr)
    , _psMotionVectorsDebugArrow(nullptr)
    , _psVelocitySetup(nullptr)
    , _psTileMax1(nullptr)
    , _psTileMax2(nullptr)
    , _psTileMax4(nullptr)
    , _psTileMaxV(nullptr)
    , _psNeighborMax(nullptr)
    , _psReconstruction(nullptr)
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
    _psMotionVectorsDebug = GPUDevice::Instance->CreatePipelineState();
    _psMotionVectorsDebugArrow = GPUDevice::Instance->CreatePipelineState();
    _psVelocitySetup = GPUDevice::Instance->CreatePipelineState();
    _psTileMax1 = GPUDevice::Instance->CreatePipelineState();
    _psTileMax2 = GPUDevice::Instance->CreatePipelineState();
    _psTileMax4 = GPUDevice::Instance->CreatePipelineState();
    _psTileMaxV = GPUDevice::Instance->CreatePipelineState();
    _psNeighborMax = GPUDevice::Instance->CreatePipelineState();
    _psReconstruction = GPUDevice::Instance->CreatePipelineState();

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
    if (!_psVelocitySetup->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_VelocitySetup");
        if (_psVelocitySetup->Init(psDesc))
            return true;
    }
    if (!_psTileMax1->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMax1");
        if (_psTileMax1->Init(psDesc))
            return true;
    }
    if (!_psTileMax2->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMax2");
        if (_psTileMax2->Init(psDesc))
            return true;
    }
    if (!_psTileMax4->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMax4");
        if (_psTileMax4->Init(psDesc))
            return true;
    }
    if (!_psTileMaxV->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_TileMaxV");
        if (_psTileMaxV->Init(psDesc))
            return true;
    }
    if (!_psNeighborMax->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_NeighborMax");
        if (_psNeighborMax->Init(psDesc))
            return true;
    }
    if (!_psReconstruction->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_Reconstruction");
        if (_psReconstruction->Init(psDesc))
            return true;
    }
    if (!_psMotionVectorsDebug->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_MotionVectorsDebug");
        if (_psMotionVectorsDebug->Init(psDesc))
            return true;
    }
    if (!_psMotionVectorsDebugArrow->IsValid())
    {
        psDesc.PrimitiveTopologyType = PrimitiveTopologyType::Line;
        psDesc.VS = shader->GetVS("VS_DebugArrow");
        psDesc.PS = shader->GetPS("PS_DebugArrow");
        if (_psMotionVectorsDebugArrow->Init(psDesc))
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
    SAFE_DELETE_GPU_RESOURCE(_psMotionVectorsDebugArrow);
    SAFE_DELETE_GPU_RESOURCE(_psVelocitySetup);
    SAFE_DELETE_GPU_RESOURCE(_psTileMax1);
    SAFE_DELETE_GPU_RESOURCE(_psTileMax2);
    SAFE_DELETE_GPU_RESOURCE(_psTileMax4);
    SAFE_DELETE_GPU_RESOURCE(_psTileMaxV);
    SAFE_DELETE_GPU_RESOURCE(_psNeighborMax);
    SAFE_DELETE_GPU_RESOURCE(_psReconstruction);

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

    // Bind input
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    const float rows = 16.0f;
    const float cols = rows * renderContext.Buffers->GetWidth() / renderContext.Buffers->GetHeight();
    data.DebugBlend = 0.7f;
    data.DebugAmplitude = 2.0f;
    data.DebugRowCount = static_cast<int32>(rows);
    data.DebugColumnCount = static_cast<int32>(cols);
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, frame);
    context->BindSR(1, renderContext.Buffers->MotionVectors);

    // Draw motion gradient
    context->SetState(_psMotionVectorsDebug);
    context->DrawFullscreenTriangle();

    // Draw arrows
    context->SetState(_psMotionVectorsDebugArrow);
    context->Draw(0, static_cast<uint32>(cols * rows * 6));

    // Cleanup
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

    // Need to have valid motion vectors created and rendered before
    ASSERT(motionVectors->IsAllocated());

    PROFILE_GPU_CPU("Motion Blur");

    // Calculate the maximum blur radius in pixels
    const float maxBlurRadius = 5.0f;
    const int32 maxBlurPixels = static_cast<int32>(maxBlurRadius * motionVectorsHeight / 100.0f);

    // Calculate the TileMax size (it should be a multiple of 8 and larger than maxBlur)
    const int32 tileSize = ((maxBlurPixels - 1) / 8 + 1) * 8;

    // Bind input
    Data data;
    GBufferPass::SetInputs(renderContext.View, data.GBuffer);
    Matrix::Transpose(renderContext.View.ViewProjection(), data.CurrentVP);
    Matrix::Transpose(renderContext.View.PrevViewProjection, data.PreviousVP);
    data.TemporalAAJitter = renderContext.View.TemporalAAJitter;
    data.VelocityScale = settings.Scale;
    data.MaxBlurRadius = static_cast<float>(maxBlurPixels);
    data.RcpMaxBlurRadius = 1.0f / maxBlurPixels;
    data.TileMaxOffs = Vector2::One * (tileSize / 8.0f - 1.0f) * -0.5f;
    data.TileMaxLoop = static_cast<int32>(tileSize / 8.0f);
    data.LoopCount = Math::Clamp(settings.SampleCount / 2.0f, 1.0f, 64.0f);
    const float invWidth = 1.0f / motionVectorsWidth;
    const float invHeight = 1.0f / motionVectorsHeight;
    data.TexelSize1 = Vector2(invWidth, invHeight);
    data.TexelSize2 = Vector2(invWidth * 2.0f, invHeight * 2.0f);
    data.TexelSize4 = Vector2(invWidth * 4.0f, invHeight * 4.0f);
    data.TexelSizeV = Vector2(invWidth * 8.0f, invHeight * 8.0f);
    data.TexelSizeNM = Vector2(invWidth * tileSize, invHeight * tileSize);
    data.MotionVectorsTexelSize = Vector2(1.0f / motionVectorsWidth, invHeight * tileSize);
    auto cb = _shader->GetShader()->GetCB(0);
    context->UpdateCB(cb, &data);
    context->BindCB(0, cb);
    context->BindSR(0, renderContext.Buffers->DepthBuffer);
    auto rtDesc = GPUTextureDescription::New2D(motionVectorsWidth, motionVectorsHeight, _velocityFormat);

    // Pass 1 - Velocity/depth packing
    auto vBuffer = RenderTargetPool::Get(rtDesc);
    context->SetRenderTarget(*vBuffer);
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, motionVectors->View());
    context->BindSR(1, renderContext.Buffers->DepthBuffer->View());
    context->SetState(_psVelocitySetup);
    context->DrawFullscreenTriangle();
    context->UnBindSR(1);

    // Pass 2 - First TileMax filter (1/2 downsize)
    rtDesc.Format = _motionVectorsFormat;
    rtDesc.Width = motionVectorsWidth / 2;
    rtDesc.Height = motionVectorsHeight / 2;
    auto tile2 = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(tile2->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, vBuffer->View());
    context->SetState(_psTileMax1);
    context->DrawFullscreenTriangle();

    // Pass 3 - Second TileMax filter (1/4 downsize)
    rtDesc.Width = motionVectorsWidth / 4;
    rtDesc.Height = motionVectorsHeight / 4;
    auto tile4 = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(tile4->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, tile2->View());
    context->SetState(_psTileMax2);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(tile2);

    // Pass 4 - Third TileMax filter (1/8 downsize)
    rtDesc.Width = motionVectorsWidth / 8;
    rtDesc.Height = motionVectorsHeight / 8;
    auto tile8 = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(tile8->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, tile4->View());
    context->SetState(_psTileMax4);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(tile4);

    // Pass 5 - Fourth TileMax filter (reduce to tileSize)
    rtDesc.Width = motionVectorsWidth / tileSize;
    rtDesc.Height = motionVectorsHeight / tileSize;
    auto tile = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(tile->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, tile8->View());
    context->SetState(_psTileMaxV);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(tile8);

    // Pass 6 - NeighborMax filter
    rtDesc.Width = motionVectorsWidth / tileSize;
    rtDesc.Height = motionVectorsHeight / tileSize;
    auto neighborMax = RenderTargetPool::Get(rtDesc);
    context->ResetRenderTarget();
    context->SetRenderTarget(neighborMax->View());
    context->SetViewportAndScissors((float)rtDesc.Width, (float)rtDesc.Height);
    context->BindSR(0, tile->View());
    context->SetState(_psNeighborMax);
    context->DrawFullscreenTriangle();
    RenderTargetPool::Release(tile);

    // Pass 7 - Reconstruction pass
    context->ResetRenderTarget();
    context->SetRenderTarget(*output);
    context->SetViewportAndScissors((float)screenWidth, (float)screenHeight);
    context->BindSR(0, input->View());
    context->BindSR(1, vBuffer->View());
    context->BindSR(2, neighborMax->View());
    context->SetState(_psReconstruction);
    context->DrawFullscreenTriangle();

    // Cleanup
    context->ResetSR();
    context->ResetRenderTarget();
    RenderTargetPool::Release(vBuffer);
    RenderTargetPool::Release(neighborMax);
    Swap(output, input);
}
