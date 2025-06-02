// Copyright (c) Wojciech Figat. All rights reserved.

#include "ForwardPass.h"
#include "RenderList.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"

ForwardPass::ForwardPass()
    : _shader(nullptr)
    , _psApplyDistortion(nullptr)
{
}

String ForwardPass::ToString() const
{
    return TEXT("ForwardPass");
}

bool ForwardPass::Init()
{
    // Prepare resources
    _psApplyDistortion = GPUDevice::Instance->CreatePipelineState();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Forward"));
    if (_shader == nullptr)
    {
        return true;
    }
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ForwardPass, &ForwardPass::OnShaderReloading>(this);
#endif

    return false;
}

bool ForwardPass::setupResources()
{
    // Check shader
    if (!_shader->IsLoaded())
    {
        return true;
    }
    const auto shader = _shader->GetShader();

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psApplyDistortion->IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = shader->GetPS("PS_ApplyDistortion");
        if (_psApplyDistortion->Init(psDesc))
            return true;
    }

    return false;
}

void ForwardPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psApplyDistortion);
    _shader = nullptr;
}

void ForwardPass::Render(RenderContext& renderContext, GPUTexture*& input, GPUTexture*& output)
{
    PROFILE_GPU_CPU("Forward");
    auto context = GPUDevice::Instance->GetMainContext();
    auto& view = renderContext.View;
    auto mainCache = renderContext.List;

    context->ResetRenderTarget();
    context->ResetSR();

    // Try to use read-only depth if supported
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    GPUTextureView* depthBufferHandle = depthBuffer->View();
    if (EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView))
        depthBufferHandle = depthBuffer->ViewReadOnlyDepth();

    // Check if there is no objects to render or no resources ready
    auto& forwardList = mainCache->DrawCallsLists[(int32)DrawCallsListType::Forward];
    auto& distortionList = mainCache->DrawCallsLists[(int32)DrawCallsListType::Distortion];
    if ((forwardList.IsEmpty() && distortionList.IsEmpty())
#if USE_EDITOR
        || renderContext.View.Mode == ViewMode::PhysicsColliders
#endif
        )
    {
        // Skip rendering
        Swap(input, output);
        return;
    }
    if (distortionList.IsEmpty() || checkIfSkipPass())
    {
        // Copy frame
        context->SetRenderTarget(output->View());
        context->Draw(input);
    }
    else
    {
        PROFILE_GPU_CPU("Distortion");

        // Peek temporary render target for the distortion pass
        // TODO: render distortion in half-res?
        const int32 width = renderContext.Buffers->GetWidth();
        const int32 height = renderContext.Buffers->GetHeight();
        const int32 distortionWidth = width;
        const int32 distortionHeight = height;
        const auto tempDesc = GPUTextureDescription::New2D(distortionWidth, distortionHeight, PixelFormat::R8G8B8A8_UNorm);
        auto distortionRT = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(distortionRT, "Forward.Distortion");

        // Clear distortion vectors
        context->Clear(distortionRT->View(), Color::Transparent);
        context->SetViewportAndScissors((float)distortionWidth, (float)distortionHeight);
        context->SetRenderTarget(depthBufferHandle, distortionRT->View());

        // Render distortion pass
        view.Pass = DrawPass::Distortion;
        mainCache->ExecuteDrawCalls(renderContext, distortionList);

        // Copy combined frame with distortion from transparent materials
        context->SetViewportAndScissors((float)width, (float)height);
        context->ResetRenderTarget();
        context->ResetSR();
        context->BindSR(0, input);
        context->BindSR(1, distortionRT);
        context->SetRenderTarget(output->View());
        context->SetState(_psApplyDistortion);
        context->DrawFullscreenTriangle();

        RenderTargetPool::Release(distortionRT);
    }

    if (!forwardList.IsEmpty())
    {
        // Run forward pass
        view.Pass = DrawPass::Forward;
        context->SetRenderTarget(depthBufferHandle, output->View());
        mainCache->ExecuteDrawCalls(renderContext, forwardList, input->View());
    }
}
