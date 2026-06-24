// Copyright (c) Wojciech Figat. All rights reserved.

#include "LightPass.h"
#include "ShadowsPass.h"
#include "GBufferPass.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Content.h"

GPU_CB_STRUCT(PerLight {
    ShaderLightData Light;
    Matrix WVP;
    });

GPU_CB_STRUCT(PerFrame {
    ShaderGBufferData GBuffer;
    });

String LightPass::ToString() const
{
    return TEXT("LightPass");
}

bool LightPass::Init()
{
    // Create pipeline states
    _psLightDir.CreatePipelineStates();
    _psLightLocal.CreatePipelineStates();
    _psLightLocalInside.CreatePipelineStates();
    _psLightSky = GPUDevice::Instance->CreatePipelineState();
    _psLightSkyInside = GPUDevice::Instance->CreatePipelineState();
    _depthBounds = GPUDevice::Instance->Limits.HasDepthBounds && GPUDevice::Instance->Limits.HasReadOnlyDepth;

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Lights"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    if (_shader == nullptr || _sphereModel == nullptr)
        return true;

#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<LightPass, &LightPass::OnShaderReloading>(this);
#endif

    // Pick the format for shadow mask (rendered shadow projection into screen-space)
    auto format = PixelFormat::R8G8_UNorm;
    if (EnumHasNoneFlags(GPUDevice::Instance->GetFormatFeatures(format).Support, FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D))
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
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, PerLight);
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 1, PerFrame);

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psLightDir.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.DepthWriteEnable = false;
        psDesc.DepthEnable = psDesc.DepthBoundsEnable = _depthBounds;
        if (_psLightDir.Create(psDesc, shader, "PS_Directional"))
            return true;
    }
    if (!_psLightLocal.IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.DepthEnable = true;
        psDesc.DepthBoundsEnable = _depthBounds;
        psDesc.CullMode = CullMode::Normal;
        if (_psLightLocal.Create(psDesc, shader, "PS_LocalLight"))
            return true;
        psDesc.DepthFunc = ComparisonFunc::DefaultInv;
        psDesc.CullMode = CullMode::Inverted;
        if (_psLightLocalInside.Create(psDesc, shader, "PS_LocalLight"))
            return true;
    }
    if (!_psLightSky->IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.PS = shader->GetPS("PS_Sky");
        psDesc.DepthEnable = true;
        psDesc.DepthBoundsEnable = _depthBounds;
        psDesc.CullMode = CullMode::Normal;
        if (_psLightSky->Init(psDesc))
            return true;
        psDesc.DepthFunc = ComparisonFunc::DefaultInv;
        psDesc.CullMode = CullMode::Inverted;
        if (_psLightSkyInside->Init(psDesc))
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
    _psLightLocal.Delete();
    _psLightLocalInside.Delete();
    SAFE_DELETE_GPU_RESOURCE(_psLightSky);
    SAFE_DELETE_GPU_RESOURCE(_psLightSkyInside);
    SAFE_DELETE_GPU_RESOURCE(_psClearDiffuse);
    SAFE_DELETE_GPU_RESOURCE(_psComplexity);
    SAFE_DELETE_GPU_RESOURCE(_psLightOverlap[0]);
    SAFE_DELETE_GPU_RESOURCE(_psLightOverlap[1]);
    _sphereModel = nullptr;
}

template<typename T = RenderLightData>
bool SortLights(T const& p1, T const& p2)
{
    // Compare by screen size
    int32 res = static_cast<int32>(p2.ScreenSize * 100 - p1.ScreenSize * 100);
    if (res == 0)
    {
        // Compare by brightness
        res = static_cast<int32>(p2.Color.SumValues() * 100 - p1.Color.SumValues() * 100);
        if (res == 0)
        {
            // Compare by ID to stabilize order
            res = GetHash(p2.ID) - GetHash(p1.ID);
        }
    }
    return res < 0;
}

void LightPass::SetupLights(RenderContext& renderContext, RenderContextBatch& renderContextBatch)
{
    PROFILE_CPU();

    // Sort lights
    Sorting::QuickSort(renderContext.List->DirectionalLights.Get(), renderContext.List->DirectionalLights.Count(), &SortLights);
    Sorting::QuickSort(renderContext.List->PointLights.Get(), renderContext.List->PointLights.Count(), &SortLights);
    Sorting::QuickSort(renderContext.List->SpotLights.Get(), renderContext.List->SpotLights.Count(), &SortLights);
}

void LightPass::RenderLights(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer)
{
    if (checkIfSkipPass())
        return;
    PROFILE_GPU_CPU("Lights");

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    auto& renderContext = renderContextBatch.GetMainContext();
    auto& view = renderContext.View;
    auto mainCache = renderContext.List;
    const auto lightShader = _shader->GetShader();
    const bool disableSpecular = (view.Flags & ViewFlags::SpecularLight) == ViewFlags::None;

    // Check if debug lights
    if (renderContext.View.Mode == ViewMode::LightBuffer || renderContext.View.Mode == ViewMode::Lighting)
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
    auto& sphereMesh = _sphereModel->LODs.Get()[0].Meshes.Get()[0];

    // Bind output
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    const bool depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
    GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
    GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    context->SetRenderTarget(depthBufferRTV, lightBuffer);

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
        auto& light = mainCache->PointLights[lightIndex];
        bool useIES = light.IESTexture != nullptr;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp;
        bool isViewInside;
        RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, light.Radius, world, isViewInside);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Fullscreen shadow mask rendering
        if (light.HasShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadowMask(renderContextBatch, light, shadowMaskView);
            context->SetRenderTarget(depthBufferRTV, lightBuffer);
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetShaderData(perLight.Light, light.HasShadow);
        Matrix::Transpose(wvp, perLight.WVP);
        if (useIES)
        {
            context->BindSR(6, light.IESTexture);
        }

        // Calculate lighting
        if (_depthBounds)
        {
            Float2 minMaxDepth = RenderTools::GetDepthBounds(view, BoundingSphere(light.Position, light.Radius));
            context->SetDepthBounds(GPU_DEPTH_RANGE_BOUNDS(minMaxDepth.X, minMaxDepth.Y));
        }
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        int32 permutationIndex = (disableSpecular ? 1 : 0) + (useIES ? 2 : 0);
        context->SetState((isViewInside ? _psLightLocalInside : _psLightLocal).Get(permutationIndex));
        sphereMesh.Render(context);
    }

    context->UnBindCB(0);

    // Render all spot lights
    for (int32 lightIndex = 0; lightIndex < mainCache->SpotLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Spot Light");
        auto& light = mainCache->SpotLights[lightIndex];
        bool useIES = light.IESTexture != nullptr;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp;
        bool isViewInside;
        RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, light.Radius, world, isViewInside);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Fullscreen shadow mask rendering
        if (light.HasShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadowMask(renderContextBatch, light, shadowMaskView);
            context->SetRenderTarget(depthBufferRTV, lightBuffer);
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetShaderData(perLight.Light, light.HasShadow);
        Matrix::Transpose(wvp, perLight.WVP);
        if (useIES)
        {
            context->BindSR(6, light.IESTexture);
        }

        // Calculate lighting
        if (_depthBounds)
        {
            Float2 minMaxDepth = RenderTools::GetDepthBounds(view, BoundingSphere(light.Position, light.Radius));
            context->SetDepthBounds(GPU_DEPTH_RANGE_BOUNDS(minMaxDepth.X, minMaxDepth.Y));
        }
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        int32 permutationIndex = (disableSpecular ? 1 : 0) + (useIES ? 2 : 0);
        context->SetState((isViewInside ? _psLightLocalInside : _psLightLocal).Get(permutationIndex));
        sphereMesh.Render(context);
    }

    context->UnBindCB(0);

    // Render all directional lights
    for (int32 lightIndex = 0; lightIndex < mainCache->DirectionalLights.Count(); lightIndex++)
    {
        PROFILE_GPU_CPU_NAMED("Directional Light");
        auto& light = mainCache->DirectionalLights[lightIndex];

        // Fullscreen shadow mask rendering
        if (light.HasShadow)
        {
            GET_SHADOW_MASK();
            ShadowsPass::Instance()->RenderShadowMask(renderContextBatch, light, shadowMaskView);
            context->SetRenderTarget(depthBufferRTV, lightBuffer);
            context->BindSR(5, shadowMaskView);
        }
        else
            context->UnBindSR(5);

        // Pack light properties buffer
        light.SetShaderData(perLight.Light, light.HasShadow);

        // Calculate lighting
        if (_depthBounds)
            context->SetDepthBounds(GPU_DEPTH_RANGE_BOUNDS(GPU_DEPTH_RANGE_MIN, RenderTools::DepthBoundMaxBackground));
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
        auto& light = mainCache->SkyLights[lightIndex];

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp;
        bool isViewInside;
        RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, light.Radius, world, isViewInside);
        Matrix::Multiply(world, view.ViewProjection(), wvp);

        // Pack light properties buffer
        light.SetShaderData(perLight.Light, false);
        Matrix::Transpose(wvp, perLight.WVP);

        // Bind source image
        context->BindSR(7, light.CubemapImageView);

        // Calculate lighting
        if (_depthBounds)
        {
            Float2 minMaxDepth = RenderTools::GetDepthBounds(view, BoundingSphere(light.Position, light.Radius));
            context->SetDepthBounds(GPU_DEPTH_RANGE_BOUNDS(minMaxDepth.X, minMaxDepth.Y));
        }
        context->UpdateCB(cb0, &perLight);
        context->BindCB(0, cb0);
        context->BindCB(1, cb1);
        context->SetState(isViewInside ? _psLightSkyInside : _psLightSky);
        sphereMesh.Render(context);
    }

    RenderTargetPool::Release(shadowMask);

    // Restore state
    if (_depthBounds)
        context->SetDepthBounds();
    context->ResetRenderTarget();
    context->ResetSR();
    context->ResetCB();
}

// Config for light complexity
#define LIGHT_COMPLEXITY_DIR_COST 0.05f
#define LIGHT_COMPLEXITY_LOCAL_COST 0.08f
#define LIGHT_COMPLEXITY_SHADOW_COST 0.1f

void LightPass::RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output)
{
    if (checkIfSkipPass())
        return;
    if (!_psComplexity)
    {
        // Lazy-init PSOs
        auto psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.VS = _shader->GPU->GetVS("VS_Model");
        psDesc.PS = _shader->GPU->GetPS("PS_Overlap");
        psDesc.DepthEnable = true;
        psDesc.DepthBoundsEnable = _depthBounds;
        psDesc.CullMode = CullMode::Normal;
        _psLightOverlap[0] = GPUDevice::Instance->CreatePipelineState();
        _psLightOverlap[0]->Init(psDesc);
        psDesc.DepthFunc = ComparisonFunc::DefaultInv;
        psDesc.CullMode = CullMode::Inverted;
        _psLightOverlap[1] = GPUDevice::Instance->CreatePipelineState();
        _psLightOverlap[1]->Init(psDesc);
        psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc.PS = _shader->GPU->GetPS("PS_Complexity");
        _psComplexity = GPUDevice::Instance->CreatePipelineState();
        _psComplexity->Init(psDesc);
    }
    PROFILE_GPU_CPU("Lights Overlap");

    // Draw lights with additive mode to sum up complexity per-pixel
    auto complexity = RenderTargetPool::Get(GPUTextureDescription::New2D(output->Width(), output->Height(), PixelFormat::R16G16B16A16_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget));
    float baseCost = 0;
    for (auto& light : renderContext.List->DirectionalLights)
        baseCost += LIGHT_COMPLEXITY_DIR_COST + (light.HasShadow ? LIGHT_COMPLEXITY_SHADOW_COST : 0);
    context->Clear(complexity->View(), Color(baseCost));
    GPUTexture* depthBuffer = renderContext.Buffers->DepthBuffer;
    const bool depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
    GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
    GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    context->SetRenderTarget(depthBufferRTV, complexity->View());
    context->SetViewportAndScissors((float)complexity->Width(), (float)complexity->Height());
    for (auto& light : renderContext.List->PointLights)
        RenderDebugSphere(renderContext, context, light, light.Radius);
    for (auto& light : renderContext.List->SpotLights)
        RenderDebugSphere(renderContext, context, light, light.Radius);
    for (auto& light : renderContext.List->SkyLights)
        RenderDebugSphere(renderContext, context, light, light.Radius);
    context->ResetRenderTarget();

    // Draw complexity visualization based on accumulated complexity
    context->SetRenderTarget(output->View());
    context->BindSR(0, complexity->View());
    context->BindSR(3, depthBufferSRV);
    context->SetState(_psComplexity);
    context->DrawFullscreenTriangle();

    if (_depthBounds)
        context->SetDepthBounds();
    RenderTargetPool::Release(complexity);
}

void LightPass::RenderDebugSphere(RenderContext& renderContext, GPUContext* context, const RenderLightData& light, float radius) const
{
    PerLight perLight;
    Matrix world, wvp;
    bool isViewInside;
    RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, light.Position, radius, world, isViewInside);
    Matrix::Multiply(world, renderContext.View.ViewProjection(), wvp);
    Matrix::Transpose(wvp, perLight.WVP);
    if (_depthBounds)
    {
        Float2 minMaxDepth = RenderTools::GetDepthBounds(renderContext.View, BoundingSphere(light.Position, radius));
        context->SetDepthBounds(GPU_DEPTH_RANGE_BOUNDS(minMaxDepth.X, minMaxDepth.Y));
    }
    perLight.Light.Radius = LIGHT_COMPLEXITY_LOCAL_COST + (light.HasShadow ? LIGHT_COMPLEXITY_SHADOW_COST : 0);
    auto cb0 = _shader->GPU->GetCB(0);
    context->UpdateCB(cb0, &perLight);
    context->BindCB(0, cb0);
    context->SetState(_psLightOverlap[isViewInside]);
    _sphereModel->LODs[0].Meshes[0].Render(context);
}
