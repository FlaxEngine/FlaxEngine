// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "GlobalSurfaceAtlasPass.h"
#include "GlobalSignDistanceFieldPass.h"
#include "RenderList.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/Shaders/GPUShader.h"

PACK_STRUCT(struct Data0
    {
    Vector3 ViewWorldPos;
    float ViewNearPlane;
    Vector3 Padding00;
    float ViewFarPlane;
    Vector4 ViewFrustumWorldRays[4];
    GlobalSignDistanceFieldPass::GlobalSDFData GlobalSDF;
    });

class GlobalSurfaceAtlasCustomBuffer : public RenderBuffers::CustomBuffer
{
public:
    GPUTexture* Dummy = nullptr; // TODO use some actual atlas textures
    GlobalSurfaceAtlasPass::BindingData Result;

    ~GlobalSurfaceAtlasCustomBuffer()
    {
        RenderTargetPool::Release(Dummy);
    }
};

String GlobalSurfaceAtlasPass::ToString() const
{
    return TEXT("GlobalSurfaceAtlasPass");
}

bool GlobalSurfaceAtlasPass::Init()
{
    // Check platform support
    const auto device = GPUDevice::Instance;
    _supported = device->GetFeatureLevel() >= FeatureLevel::SM5 && device->Limits.HasCompute && device->Limits.HasTypedUAVLoad;
    return false;
}

bool GlobalSurfaceAtlasPass::setupResources()
{
    // Load shader
    if (!_shader)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/GlobalSurfaceAtlas"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<GlobalSurfaceAtlasPass, &GlobalSurfaceAtlasPass::OnShaderReloading>(this);
#endif
    }
    if (!_shader->IsLoaded())
        return true;

    const auto device = GPUDevice::Instance;
    const auto shader = _shader->GetShader();
    _cb0 = shader->GetCB(0);

    // Create pipeline state
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDebug)
    {
        _psDebug = device->CreatePipelineState();
        psDesc.PS = shader->GetPS("PS_Debug");
        if (_psDebug->Init(psDesc))
            return true;
    }

    return false;
}

#if COMPILE_WITH_DEV_ENV

void GlobalSurfaceAtlasPass::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    invalidateResources();
}

#endif

void GlobalSurfaceAtlasPass::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psDebug);
    _cb0 = nullptr;
    _shader = nullptr;
}

bool GlobalSurfaceAtlasPass::Render(RenderContext& renderContext, GPUContext* context, BindingData& result)
{
    // Skip if not supported
    if (setupResources())
        return true;
    if (renderContext.List->Scenes.Count() == 0)
        return true;
    auto& surfaceAtlasData = *renderContext.Buffers->GetCustomBuffer<GlobalSurfaceAtlasCustomBuffer>(TEXT("GlobalSurfaceAtlas"));

    // Skip if already done in the current frame
    const auto currentFrame = Engine::FrameCount;
    if (surfaceAtlasData.LastFrameUsed == currentFrame)
    {
        result = surfaceAtlasData.Result;
        return false;
    }

    PROFILE_GPU_CPU("Global Surface Atlas");

    return false;
    // TODO: configurable via graphics settings
    const int32 resolution = 4096;
    // TODO: configurable via postFx settings (maybe use Global SDF distance?)
    const float distance = 20000;

    // TODO: Initialize buffers
    surfaceAtlasData.LastFrameUsed = currentFrame;

    // TODO: Rasterize world geometry into Global Surface Atlas

    // Copy results
    result.Dummy = surfaceAtlasData.Dummy;
    surfaceAtlasData.Result = result;
    return false;
}

void GlobalSurfaceAtlasPass::RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output)
{
    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    BindingData bindingData;
    if (GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF) || Render(renderContext, context, bindingData))
    {
        context->Draw(output, renderContext.Buffers->GBuffer0);
        return;
    }

    PROFILE_GPU_CPU("Global Surface Atlas Debug");
    const Vector2 outputSize(output->Size());
    if (_cb0)
    {
        Data0 data;
        data.ViewWorldPos = renderContext.View.Position;
        data.ViewNearPlane = renderContext.View.Near;
        data.ViewFarPlane = renderContext.View.Far;
        for (int32 i = 0; i < 4; i++)
            data.ViewFrustumWorldRays[i] = Vector4(renderContext.List->FrustumCornersWs[i + 4], 0);
        data.GlobalSDF = bindingDataSDF.GlobalSDF;
        context->UpdateCB(_cb0, &data);
        context->BindCB(0, _cb0);
    }
    for (int32 i = 0; i < 4; i++)
    {
        context->BindSR(i, bindingDataSDF.Cascades[i]->ViewVolume());
        context->BindSR(i + 4, bindingDataSDF.CascadeMips[i]->ViewVolume());
    }
    context->SetState(_psDebug);
    context->SetRenderTarget(output->View());
    context->SetViewportAndScissors(outputSize.X, outputSize.Y);
    context->DrawFullscreenTriangle();
}
