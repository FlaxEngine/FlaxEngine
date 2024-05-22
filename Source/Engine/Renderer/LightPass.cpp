// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "LightPass.h"
#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"

PACK_STRUCT(struct PerLight {
    LightData Light;
    Matrix WVP;
});

PACK_STRUCT(struct PerFrame {
    GBufferData GBuffer;
    GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
});

String LightPass::ToString() const
{
    return TEXT("LightPass");
}

bool LightPass::Init()
{
    // Create pipeline states
    _psLightDir.CreatePipelineStates();
    _psLightPointNormal.CreatePipelineStates();
    _psLightPointInverted.CreatePipelineStates();
    _psLightSpotNormal.CreatePipelineStates();
    _psLightSpotInverted.CreatePipelineStates();
    _psLightSkyNormal.CreatePipelineStates();
    _psLightSkyInverted.CreatePipelineStates();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Lights"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/SphereLowPoly"));
    if (_shader == nullptr || _sphereModel == nullptr)
    {
        return true;
    }

#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<LightPass, &LightPass::OnShaderReloading>(this);
#endif

    auto format = PixelFormat::R8G8_UNorm;
    if (EnumHasNoneFlags(GPUDevice::Instance->GetFormatFeatures(format).Support, (FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D)))
    {
        format = PixelFormat::B8G8R8A8_UNorm;
    }
    _shadowMaskFormat = format;

    return false;
}

bool LightPass::setupResources()
{
    // Wait for the assets
    if (!_sphereModel->CanBeRendered() || !_shader->IsLoaded())
        return true;
    auto shader = _shader->GetShader();

    // Validate shader constant buffers sizes
    if (shader->GetCB(0)->GetSize() != sizeof(PerLight))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, PerLight);
        return true;
    }
    if (shader->GetCB(1)->GetSize() != sizeof(PerFrame))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 1, PerFrame);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psLightDir.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        if (_psLightDir.Create(psDesc, shader, "PS_Directional"))
            return true;
    }
    if (!_psLightPointNormal.IsValid() || !_psLightPointInverted.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.CullMode = CullMode::Inverted;
        if (_psLightPointInverted.Create(psDesc, shader, "PS_Point"))
            return true;
        psDesc.CullMode = CullMode::Normal;
        psDesc.DepthEnable = true;
        if (_psLightPointNormal.Create(psDesc, shader, "PS_Point"))
            return true;
    }
    if (!_psLightSpotNormal.IsValid() || !_psLightSpotInverted.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.CullMode = CullMode::Inverted;
        if (_psLightSpotInverted.Create(psDesc, shader, "PS_Spot"))
            return true;
        psDesc.CullMode = CullMode::Normal;
        psDesc.DepthEnable = true;
        if (_psLightSpotNormal.Create(psDesc, shader, "PS_Spot"))
            return true;
    }
    if (!_psLightSkyNormal.IsValid() || !_psLightSkyInverted.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.CullMode = CullMode::Normal;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.PS = shader->GetPS("PS_Sky");
        if (_psLightSkyNormal.Create(psDesc, shader, "PS_Sky"))
            return true;
        psDesc.CullMode = CullMode::Inverted;
        if (_psLightSkyInverted.Create(psDesc, shader, "PS_Sky"))
            return true;
    }

    return false;
}

void LightPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    _psLightDir.Delete();
    _psLightPointNormal.Delete();
    _psLightPointInverted.Delete();
    _psLightSpotNormal.Delete();
    _psLightSpotInverted.Delete();
    _psLightSkyNormal.Delete();
    _psLightSkyInverted.Delete();
    SAFE_DELETE_GPU_RESOURCE(_psClearDiffuse);
    _sphereModel = nullptr;
}

void LightPass::RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer)
{
    const float sphereModelScale = 3.0f;

    // Ensure to have valid data
    if (checkIfSkipPass())
    {
        // Resources are missing. Do not perform rendering.
        return;
    }

    PROFILE_GPU_CPU("Lights");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& renderContext = renderContextBatch.Contexts[0];
    auto& view = renderContext.View;
    auto mainCache = renderContext.List;
    const auto lightShader = _shader->GetShader();
    const bool useShadows = ShadowsPass::Instance()->IsReady() && EnumHasAnyFlags(view.Flags, ViewFlags::Shadows);
    const bool disableSpecular = (view.Flags & ViewFlags::SpecularLight) == ViewFlags::None;

    // Check if debug lights
    if (renderContext.View.Mode == ViewMode::LightBuffer)
    {
        // Clear diffuse
        if (!_psClearDiffuse)
            _psClearDiffuse = GPUDevice::Instance->CreatePipelineState();
        auto quadShader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Quad"));
        if (!_psClearDiffuse->IsValid() && quadShader)
        {
            auto psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
            psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB; // Leave AO in Alpha channel unmodified
            psDesc.PS = quadShader->GetShader()->GetPS("PS_Clear");
            _psClearDiffuse->Init(psDesc);
        }
        if (_psClearDiffuse->IsValid())
        {
            context->SetRenderTarget(renderContext.Buffers->GBuffer0->View());
            auto cb = quadShader->GetShader()->GetCB(0);
            context->UpdateCB(cb, &Color::White);
            context->BindCB(0, cb);
            context->SetState(_psClearDiffuse);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
        }
        else
        {
            context->Clear(renderContext.Buffers->GBuffer0->View(), Color::White);
        }
    }

    // Temporary data
    PerLight perLight;
    PerFrame perFrame;

    // Bind output
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    const bool depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
    GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
    GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    context->SetRenderTarget(depthBufferRTV, lightBuffer);

    GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
    bool useSdfSkylightShadows = false;
    // If SDF rendering succeeds
    if (!GlobalSignDistanceFieldPass::Instance()->Render(renderContext, context, bindingDataSDF)) {
        useSdfSkylightShadows = true;
        perFrame.GlobalSDF = bindingDataSDF.Constants;
    }

    // Set per frame data
    GBufferPass::SetInputs(renderContext.View, perFrame.GBuffer);
    auto cb0 = lightShader->GetCB(0);
    auto cb1 = lightShader->GetCB(1);
    context->UpdateCB(cb1, &perFrame);

    // Bind inputs
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, depthBufferSRV);
    context->BindSR(4, renderContext.Buffers->GBuffer3);

    // Bind SDF resources
    if (useSdfSkylightShadows) {
        context->BindSR(8, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
        context->BindSR(9, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
    }

    // Fullscreen shadow mask buffer
    GPUTexture* shadowMask = nullptr;
#define GET_SHADOW_MASK() \
    if (!shadowMask) { \
        auto rtDesc = GPUTextureDescription::New2D(renderContext.Buffers->GetWidth(), renderContext.Buffers->GetHeight(), _shadowMaskFormat); \
        shadowMask = RenderTargetPool::Get(rtDesc); \
        RENDER_TARGET_POOL_SET_NAME(shadowMask, "ShadowMask"); \
    } \
    auto shadowMaskView = shadowMask->View()

    // Render all point lights
    for (int32 lightIndex = 0; lightIndex < mainCache->PointLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Point Light");

        // Cache data
        auto& light = mainCache->PointLights[lightIndex];
        float lightRadius = light.Radius;
        Float3 lightPosition = light.Position;
        const bool renderShadow = useShadows && light.ShadowDataIndex != -1;
        bool useIES = light.IESTexture != nullptr;

        // Get distance from view center to light center less radius (check if view is inside a sphere)
        float distance = ViewToCenterLessRadius(view, lightPosition, lightRadius * sphereModelScale);
        bool isViewInside = distance < 0;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp, matrix;
        Matrix::Scaling(lightRadius * sphereModelScale, wvp);
        Matrix::Translation(lightPosition, matrix);
        Matrix::Multiply(wvp, matrix, world);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Check if render shadow
        if (renderShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadow(renderContextBatch, light, shadowMaskView);

            // Bind output
            context->SetRenderTarget(depthBufferRTV, lightBuffer);

            // Set shadow mask
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetupLightData(&perLight.Light, renderShadow);
        Matrix::Transpose(wvp, perLight.WVP);
        if (useIES)
        {
            context->BindSR(6, light.IESTexture);
        }

        // Calculate lighting
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        int32 permutationIndex = (disableSpecular ? 1 : 0) + (useIES ? 2 : 0);
        context->SetState((isViewInside ? _psLightPointInverted : _psLightPointNormal).Get(permutationIndex));
        _sphereModel->Render(context);
    }

    context->UnBindCB(0);

    // Render all spot lights
    for (int32 lightIndex = 0; lightIndex < mainCache->SpotLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Spot Light");

        // Cache data
        auto& light = mainCache->SpotLights[lightIndex];
        float lightRadius = light.Radius;
        Float3 lightPosition = light.Position;
        const bool renderShadow = useShadows && light.ShadowDataIndex != -1;
        bool useIES = light.IESTexture != nullptr;

        // Get distance from view center to light center less radius (check if view is inside a sphere)
        float distance = ViewToCenterLessRadius(view, lightPosition, lightRadius * sphereModelScale);
        bool isViewInside = distance < 0;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp, matrix;
        Matrix::Scaling(lightRadius * sphereModelScale, wvp);
        Matrix::Translation(lightPosition, matrix);
        Matrix::Multiply(wvp, matrix, world);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Check if render shadow
        if (renderShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadow(renderContextBatch, light, shadowMaskView);

            // Bind output
            context->SetRenderTarget(depthBufferRTV, lightBuffer);

            // Set shadow mask
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetupLightData(&perLight.Light, renderShadow);
        Matrix::Transpose(wvp, perLight.WVP);
        if (useIES)
        {
            context->BindSR(6, light.IESTexture);
        }

        // Calculate lighting
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        int32 permutationIndex = (disableSpecular ? 1 : 0) + (useIES ? 2 : 0);
        context->SetState((isViewInside ? _psLightSpotInverted : _psLightSpotNormal).Get(permutationIndex));
        _sphereModel->Render(context);
    }

    context->UnBindCB(0);

    // Render all directional lights
    for (int32 lightIndex = 0; lightIndex < mainCache->DirectionalLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Directional Light");

        // Cache data
        auto& light = mainCache->DirectionalLights[lightIndex];
        const bool renderShadow = useShadows && light.ShadowDataIndex != -1;

        // Check if render shadow
        if (renderShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadow(renderContextBatch, light, lightIndex, shadowMaskView);

            // Bind output
            context->SetRenderTarget(depthBufferRTV, lightBuffer);

            // Set shadow mask
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetupLightData(&perLight.Light, renderShadow);

        // Calculate lighting
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        context->SetState(_psLightDir.Get(disableSpecular));
        context->DrawFullscreenTriangle();
    }

    context->UnBindCB(0);

    // Render all sky lights
    for (int32 lightIndex = 0; lightIndex < mainCache->SkyLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Sky Light");

        // Cache data
        auto& light = mainCache->SkyLights[lightIndex];
        float lightRadius = light.Radius;
        Float3 lightPosition = light.Position;

        // Get distance from view center to light center less radius (check if view is inside a sphere)
        float distance = ViewToCenterLessRadius(view, lightPosition, lightRadius * sphereModelScale);
        bool isViewInside = distance < 0;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp, matrix;
        Matrix::Scaling(lightRadius * sphereModelScale, wvp);
        Matrix::Translation(lightPosition, matrix);
        Matrix::Multiply(wvp, matrix, world);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Pack light properties buffer
        light.SetupLightData(&perLight.Light, false);
        Matrix::Transpose(wvp, perLight.WVP);

        // Bind source image
        context->BindSR(7, light.Image ? light.Image->GetTexture() : nullptr);

        // Calculate lighting
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        int permutationIndex = useSdfSkylightShadows ? 1 : 0;
        context->SetState((isViewInside ? _psLightSkyInverted : _psLightSkyNormal).Get(permutationIndex));
        _sphereModel->Render(context);
    }

    RenderTargetPool::Release(shadowMask);

    // Restore state
    context->ResetRenderTarget();
    context->ResetSR();
    context->ResetCB();
}
