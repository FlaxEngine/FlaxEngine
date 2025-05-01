// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "QuadOverdrawPass.h"
#include "Engine/Engine/Time.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Level/Actors/Decal.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderTools.h"

void QuadOverdrawPass::Render(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer)
{
    if (checkIfSkipPass())
    {
        context->Clear(lightBuffer, Color(0, Math::Sin(Time::Draw.UnscaledTime.GetTotalSeconds() * 5.0f) * 0.5f + 0.5f, 0, 1.0f));
        return;
    }
    PROFILE_GPU_CPU("Quad Overdraw");

    // Setup resources
    const int32 width = renderContext.Buffers->GetWidth();
    const int32 height = renderContext.Buffers->GetHeight();
    auto tempDesc = GPUTextureDescription::New2D(width >> 1, height >> 1, PixelFormat::R32_UInt, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);
    auto lockTexture = RenderTargetPool::Get(tempDesc);
    auto overdrawTexture = RenderTargetPool::Get(tempDesc);
    auto liveCountTexture = RenderTargetPool::Get(tempDesc);
    RENDER_TARGET_POOL_SET_NAME(lockTexture, "QuadOverdraw.Lock");
    RENDER_TARGET_POOL_SET_NAME(overdrawTexture, "QuadOverdraw.Overdraw");
    RENDER_TARGET_POOL_SET_NAME(liveCountTexture, "QuadOverdraw.LiveCount");

    // Clear buffers
    uint32 clearValueUINT[4] = { 0 };
    clearValueUINT[0] = 0xffffffff;
    context->ClearUA(lockTexture, clearValueUINT);
    clearValueUINT[0] = 0x00000000;
    context->ClearUA(overdrawTexture, clearValueUINT);
    context->ClearUA(liveCountTexture, clearValueUINT);
    context->ClearDepth(*renderContext.Buffers->DepthBuffer);

    // Draw quad overdraw stats into UAVs
    context->BindUA(0, lockTexture->View());
    context->BindUA(1, overdrawTexture->View());
    context->BindUA(2, liveCountTexture->View());
    DrawCall drawCall;
    drawCall.WorldDeterminantSign = 1.0f;
    drawCall.PerInstanceRandom = 0.0f;
    MaterialBase::BindParameters bindParams(context, renderContext, drawCall);
    bindParams.BindViewData();
    renderContext.View.Pass = DrawPass::QuadOverdraw;
    context->SetRenderTarget(*renderContext.Buffers->DepthBuffer, (GPUTextureView*)nullptr);
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBuffer);
    auto boxModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/SimpleBox"));
    auto defaultMaterial = GPUDevice::Instance->GetDefaultMaterial();
    if (boxModel && boxModel->CanBeRendered() && defaultMaterial && defaultMaterial->IsReady())
    {
        // Draw decals
        for (int32 i = 0; i < renderContext.List->Decals.Count(); i++)
        {
            const RenderDecalData& decal = renderContext.List->Decals.Get()[i];
            drawCall.World = decal.World;
            defaultMaterial->Bind(bindParams);
            boxModel->Render(context);
        }
    }
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::GBufferNoDecals);
    auto skyModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    auto skyMaterial = Content::LoadAsyncInternal<Material>(TEXT("Engine/SkyboxMaterial"));
    if (renderContext.List->Sky && skyModel && skyModel->CanBeRendered() && skyMaterial && skyMaterial->IsReady())
    {
        // Draw sky
        auto box = skyModel->GetBox();
        Matrix m1, m2;
        Matrix::Scaling(renderContext.View.Far / ((float)box.GetSize().Y * 0.5f) * 0.95f, m1);
        Matrix::CreateWorld(renderContext.View.Position, Float3::Up, Float3::Backward, m2);
        m1 *= m2;
        drawCall.World = m1;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
        skyMaterial->Bind(bindParams);
        skyModel->Render(context);
    }
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    GPUTextureView* readOnlyDepthBuffer = depthBuffer->View();
    if (EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView))
        readOnlyDepthBuffer = depthBuffer->ViewReadOnlyDepth();
    context->ResetSR();
    context->ResetRenderTarget();
    context->SetRenderTarget(readOnlyDepthBuffer, (GPUTextureView*)nullptr);
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::Forward);
    renderContext.List->ExecuteDrawCalls(renderContext, DrawCallsListType::Distortion);
    // TODO: draw volumetric particles
    context->ResetRenderTarget();
    context->ResetUA();
    context->ResetSR();

    // Convert stats into debug colors
    context->BindSR(0, overdrawTexture->View());
    context->SetRenderTarget(lightBuffer);
    context->SetState(_ps);
    context->DrawFullscreenTriangle();

    // Free resources
    RenderTargetPool::Release(liveCountTexture);
    RenderTargetPool::Release(overdrawTexture);
    RenderTargetPool::Release(lockTexture);
}

String QuadOverdrawPass::ToString() const
{
    return TEXT("QuadOverdrawPass");
}

void QuadOverdrawPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    SAFE_DELETE_GPU_RESOURCE(_ps);
    _shader = nullptr;
}

bool QuadOverdrawPass::setupResources()
{
    if (GPUDevice::Instance->GetFeatureLevel() < FeatureLevel::SM5)
        return true;

    if (!_shader)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/QuadOverdraw"));
        if (!_shader)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<QuadOverdrawPass, &QuadOverdrawPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_ps)
        _ps = GPUDevice::Instance->CreatePipelineState();
    if (!_ps->IsValid())
    {
        psDesc.PS = shader->GetPS("PS");
        if (_ps->Init(psDesc))
            return true;
    }

    return false;
}

#endif
