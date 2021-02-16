// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DepthOfFieldPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/PostProcessBase.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/GPULimits.h"

// This must match hlsl defines
#define DOF_MAX_SAMPLE_RADIUS 10
#define DOF_GRID_SIZE 450
#define DOF_APRON_SIZE DOF_MAX_SAMPLE_RADIUS
#define DOF_THREAD_GROUP_SIZE (DOF_GRID_SIZE + (DOF_APRON_SIZE * 2))

DepthOfFieldPass::DepthOfFieldPass()
{
}

String DepthOfFieldPass::ToString() const
{
    return TEXT("DepthOfFieldPass");
}

bool DepthOfFieldPass::Init()
{
    // Disable Depth Of Field for platforms without compute shaders support
    // (in future we should support it or faster solution using pixel shaders)
    auto& limits = GPUDevice::Instance->Limits;
    _platformSupportsDoF = limits.HasCompute;
    _platformSupportsBokeh = _platformSupportsDoF && limits.HasGeometryShaders && limits.HasDrawIndirect && limits.HasAppendConsumeBuffers;

    // Create pipeline states
    if (_platformSupportsDoF)
    {
        _psDofDepthBlurGeneration = GPUDevice::Instance->CreatePipelineState();
        _psDoNotGenerateBokeh = GPUDevice::Instance->CreatePipelineState();
        if (_platformSupportsBokeh)
        {
            _psBokehGeneration = GPUDevice::Instance->CreatePipelineState();
            _psBokeh = GPUDevice::Instance->CreatePipelineState();
            _psBokehComposite = GPUDevice::Instance->CreatePipelineState();
        }
    }

    // Load shaders
    if (_platformSupportsDoF)
    {
        _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/DepthOfField"));
        if (_shader == nullptr)
            return true;
#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<DepthOfFieldPass, &DepthOfFieldPass::OnShaderReloading>(this);
#endif
    }

    return false;
}

void DepthOfFieldPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psDofDepthBlurGeneration);
    SAFE_DELETE_GPU_RESOURCE(_psBokehGeneration);
    SAFE_DELETE_GPU_RESOURCE(_psDoNotGenerateBokeh);
    SAFE_DELETE_GPU_RESOURCE(_psBokeh);
    SAFE_DELETE_GPU_RESOURCE(_psBokehComposite);
    _shader = nullptr;
    _defaultBokehHexagon = nullptr;
    _defaultBokehOctagon = nullptr;
    _defaultBokehCircle = nullptr;
    _defaultBokehCross = nullptr;
    SAFE_DELETE_GPU_RESOURCE(_bokehBuffer);
    SAFE_DELETE_GPU_RESOURCE(_bokehIndirectArgsBuffer);
}

bool DepthOfFieldPass::setupResources()
{
    // Dof
    if (_platformSupportsDoF == false)
        return false;

    // Wait for shader
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psDofDepthBlurGeneration->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_DofDepthBlurGeneration");
        if (_psDofDepthBlurGeneration->Init(psDesc))
            return true;
    }
    if (!_psDoNotGenerateBokeh->IsValid())
    {
        psDesc.PS = shader->GetPS("PS_DoNotGenerateBokeh");
        if (_psDoNotGenerateBokeh->Init(psDesc))
            return true;
    }
    if (_platformSupportsBokeh)
    {
        if (!_psBokehGeneration->IsValid())
        {
            psDesc.PS = shader->GetPS("PS_GenerateBokeh");
            if (_psBokehGeneration->Init(psDesc))
                return true;
        }
        if (!_psBokehComposite->IsValid())
        {
            psDesc.PS = shader->GetPS("PS_BokehComposite");
            if (_psBokehComposite->Init(psDesc))
                return true;
        }
        if (!_psBokeh->IsValid())
        {
            psDesc.VS = shader->GetVS("VS_Bokeh");
            psDesc.GS = shader->GetGS("GS_Bokeh");
            psDesc.PS = shader->GetPS("PS_Bokeh");
            psDesc.BlendMode = BlendingMode::Additive;
            psDesc.PrimitiveTopologyType = PrimitiveTopologyType::Point;
            if (_psBokeh->Init(psDesc))
                return true;
        }

        // Create buffer
        if (_bokehBuffer == nullptr)
            _bokehBuffer = GPUDevice::Instance->CreateBuffer(TEXT("Bokeh Buffer"));
        if (_bokehIndirectArgsBuffer == nullptr)
            _bokehIndirectArgsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("Bokeh Indirect Args Buffer"));
        uint32 indirectArgsBufferInitData[4] = { 0, 1, 0, 0 };
        if (_bokehIndirectArgsBuffer->Init(GPUBufferDescription::Argument(indirectArgsBufferInitData, sizeof(indirectArgsBufferInitData))))
            return true;
    }

    return false;
}

GPUTexture* DepthOfFieldPass::getDofBokehShape(DepthOfFieldSettings& dofSettings)
{
    Texture* result;
#define GET_AND_SET_BOKEH(property, name) if (property == nullptr) property = Content::LoadAsyncInternal<Texture>(TEXT(name)); result = property
    switch (dofSettings.BokehShape)
    {
    case BokehShapeType::Hexagon: GET_AND_SET_BOKEH(_defaultBokehHexagon, "Engine/Textures/Bokeh/Hexagon");
        break;
    case BokehShapeType::Octagon: GET_AND_SET_BOKEH(_defaultBokehOctagon, "Engine/Textures/Bokeh/Octagon");
        break;
    case BokehShapeType::Circle: GET_AND_SET_BOKEH(_defaultBokehCircle, "Engine/Textures/Bokeh/Circle");
        break;
    case BokehShapeType::Cross: GET_AND_SET_BOKEH(_defaultBokehCross, "Engine/Textures/Bokeh/Cross");
        break;
    case BokehShapeType::Custom:
        result = dofSettings.BokehShapeCustom;
        break;
    default:
        result = nullptr;
        break;
    }
#undef GET_AND_SET_BOKEH
    return result ? result->GetTexture() : nullptr;
}

GPUTexture* DepthOfFieldPass::Render(RenderContext& renderContext, GPUTexture* input)
{
    // Ensure to have valid data
    if (!_platformSupportsDoF || checkIfSkipPass())
        return nullptr;

    // Cache data
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    const auto depthBuffer = renderContext.Buffers->DepthBuffer;
    const auto shader = _shader->GetShader();
    DepthOfFieldSettings& dofSettings = renderContext.List->Settings.DepthOfField;
    const bool useDoF = _platformSupportsDoF && (renderContext.View.Flags & ViewFlags::DepthOfField) != 0 && dofSettings.Enabled;

    // Skip if disabled
    if (!useDoF)
        return nullptr;

    PROFILE_GPU_CPU("Depth Of Field");

    context->ResetSR();

    // Resolution settings
    // TODO: render depth of field in 1/4 resolution?
    const int32 cocResolutionDivider = 1;
    const int32 dofResolutionDivider = 1;
    const int32 bokehResolutionDivider = 1;

    // TODO: in low-res DoF maybe use shared HalfResDepth?

    // Cache viewport sizes
    const int32 w1 = input->Width();
    const int32 h1 = input->Height();
    const int32 cocWidth = w1 / cocResolutionDivider;
    const int32 cocHeight = h1 / cocResolutionDivider;
    const int32 dofWidth = w1 / dofResolutionDivider;
    const int32 dofHeight = h1 / dofResolutionDivider;
    const int32 bokehTargetWidth = w1 / bokehResolutionDivider;
    const int32 bokehTargetHeight = h1 / bokehResolutionDivider;

    // TODO: maybe we could render particles (whole transparency in general) to the depth buffer to apply DoF on them as well?

    // TODO: reduce amount of used temporary render targets, we could plan rendering steps in more static way and hardcode some logic to make it run faster with less memory usage (less bandwitch)

    // Setup constant buffer
    Data cbData;
    {
        float nearPlane = renderContext.View.Near;
        float farPlane = renderContext.View.Far;

        float focalRegionHalf = dofSettings.FocalRegion * 0.5f;
        float nearFocusEnd = Math::Max(0.0f, dofSettings.FocalDistance - focalRegionHalf);
        float nearFocusStart = Math::Max(0.0f, nearFocusEnd - dofSettings.NearTransitionRange);
        float farFocusStart = Math::Min(farPlane - 5.0f, dofSettings.FocalDistance + focalRegionHalf);
        float farFocusEnd = Math::Min(farPlane - 5.0f, farFocusStart + dofSettings.FarTransitionRange);
        float depthLimitMax = farPlane - 10.0f;

        cbData.DOFDepths.X = nearFocusStart;
        cbData.DOFDepths.Y = nearFocusEnd;
        cbData.DOFDepths.Z = farFocusStart;
        cbData.DOFDepths.W = farFocusEnd;
        cbData.MaxBokehSize = dofSettings.BokehSize;
        cbData.BokehBrightnessThreshold = dofSettings.BokehBrightnessThreshold;
        cbData.BokehBlurThreshold = dofSettings.BokehBlurThreshold;
        cbData.BokehFalloff = dofSettings.BokehFalloff;
        cbData.BokehDepthCutoff = dofSettings.BokehDepthCutoff;
        cbData.DepthLimit = dofSettings.DepthLimit > ZeroTolerance ? Math::Min(dofSettings.DepthLimit, depthLimitMax) : depthLimitMax;
        cbData.BlurStrength = Math::Saturate(dofSettings.BlurStrength);
        cbData.BokehBrightness = dofSettings.BokehBrightness;

        cbData.DOFTargetSize.X = static_cast<float>(dofWidth); // TODO: check if this param is binded right. maybe use w1 or bokehTargetWidth?
        cbData.DOFTargetSize.Y = static_cast<float>(dofHeight);
        cbData.InputSize.X = static_cast<float>(w1);
        cbData.InputSize.Y = static_cast<float>(h1);
        cbData.BokehTargetSize.X = static_cast<float>(bokehTargetWidth);
        cbData.BokehTargetSize.Y = static_cast<float>(bokehTargetHeight);

        // TODO: use projection matrix instead of this far and near stuff?
        cbData.ProjectionAB.X = farPlane / (farPlane - nearPlane);
        cbData.ProjectionAB.Y = (-farPlane * nearPlane) / (farPlane - nearPlane);
    }

    // Bind constant buffer
    auto cb = shader->GetCB(0);
    context->UpdateCB(cb, &cbData);
    context->BindCB(0, cb);

    // Depth/blur generation pass
    auto tempDesc = GPUTextureDescription::New2D(cocWidth, cocHeight, DOF_DEPTH_BLUR_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::UnorderedAccess);
    GPUTexture* depthBlurTarget = RenderTargetPool::Get(tempDesc);
    context->SetViewportAndScissors((float)cocWidth, (float)cocHeight);
    context->SetRenderTarget(*depthBlurTarget);
    context->BindSR(0, depthBuffer);
    context->SetState(_psDofDepthBlurGeneration);
    context->DrawFullscreenTriangle();
    context->ResetRenderTarget();

    // CoC Spread pass
    // todo: add config for CoC spread in postFx settings?
    // TODO: test it out
    bool isCoCSpreadEnabled = false;
    if (isCoCSpreadEnabled)
    {
        context->ResetRenderTarget();
        context->ResetSR();
        context->ResetUA();
        context->FlushState();

        tempDesc = GPUTextureDescription::New2D(cocWidth, cocHeight, DOF_DEPTH_BLUR_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::UnorderedAccess);
        GPUTexture* tempTarget = RenderTargetPool::Get(tempDesc);

        // Horizontal pass
        context->BindSR(0, depthBlurTarget);
        //
        context->BindUA(0, tempTarget->View());
        //
        uint32 groupCountX = (cocWidth / DOF_GRID_SIZE) + ((cocWidth % DOF_GRID_SIZE) > 0 ? 1 : 0);
        uint32 groupCountY = cocHeight;
        //
        context->Dispatch(shader->GetCS("CS_CoCSpreadH"), groupCountX, groupCountY, 1);

        // Vertical pass
        context->BindSR(0, tempTarget);
        //
        context->BindUA(0, depthBlurTarget->View());
        //
        groupCountX = cocWidth;
        groupCountY = (cocHeight / DOF_GRID_SIZE) + (cocHeight % DOF_GRID_SIZE) > 0 ? 1 : 0;
        //
        context->Dispatch(shader->GetCS("CS_CoCSpreadV"), groupCountX, groupCountY, 1);

        // Cleanup
        context->ResetRenderTarget();
        context->UnBindSR(0);
        context->UnBindUA(0);
        context->FlushState();
        RenderTargetPool::Release(tempTarget);
    }

    // Peek temporary render target for dof pass
    tempDesc = GPUTextureDescription::New2D(dofWidth, dofHeight, DOF_RT_FORMAT);
    GPUTexture* dofInput = RenderTargetPool::Get(tempDesc);

    // Do the bokeh point generation, or just do a copy if disabled
    bool isBokehGenerationEnabled = dofSettings.BokehEnabled && _platformSupportsBokeh && dofSettings.BokehBrightness > 0.0f;
    if (isBokehGenerationEnabled)
    {
        // Update bokeh buffer to have enough size for points
        // TODO: maybe add param to control this? because in most cases there won't be width*height bokehs
        const uint32 minRequiredElements = dofWidth * dofHeight / 16;
        const uint32 elementStride = sizeof(BokehPoint);
        const uint32 minRequiredSize = minRequiredElements * elementStride;
        // TODO: resize also if main viewport has been resized? or just cache maximum size during last 60 frames and then resize adaptivly to that information
        if (minRequiredSize > _bokehBuffer->GetSize())
        {
            // Resize buffer
            if (_bokehBuffer->Init(GPUBufferDescription::StructuredAppend(minRequiredElements, elementStride)))
            {
                LOG(Fatal, "Cannot create buffer {0}.", TEXT("Bokeh Buffer"));
                return nullptr;
            }
        }

        // Clear bokeh points count
        context->ResetCounter(_bokehBuffer);

        // Generate bokeh points
        context->BindSR(0, input);
        context->BindSR(1, depthBlurTarget);
        context->SetRenderTarget(*dofInput, _bokehBuffer);
        context->SetViewportAndScissors((float)dofWidth, (float)dofHeight);
        context->SetState(_psBokehGeneration);
        context->DrawFullscreenTriangle();
    }
    else
    {
        // Generate bokeh points
        context->BindSR(0, input);
        context->BindSR(1, depthBlurTarget);
        context->SetRenderTarget(*dofInput);
        context->SetViewportAndScissors((float)dofWidth, (float)dofHeight);
        context->SetState(_psDoNotGenerateBokeh);
        context->DrawFullscreenTriangle();
    }

    // Do depth of field (using compute shaders in full resolution)
    GPUTexture* dofOutput;
    context->ResetRenderTarget();
    context->ResetSR();
    context->ResetUA();
    context->FlushState();
    {
        // Peek temporary targets for two blur passes
        tempDesc = GPUTextureDescription::New2D(dofWidth, dofHeight, DOF_RT_FORMAT, GPUTextureFlags::ShaderResource | GPUTextureFlags::RenderTarget | GPUTextureFlags::UnorderedAccess);
        auto dofTargetH = RenderTargetPool::Get(tempDesc);
        auto dofTargetV = RenderTargetPool::Get(tempDesc);

        // Horizontal pass
        context->BindSR(0, dofInput);
        context->BindSR(1, depthBlurTarget);
        //
        context->BindUA(0, dofTargetH->View());
        //
        uint32 groupCountX = (dofWidth / DOF_GRID_SIZE) + ((dofWidth % DOF_GRID_SIZE) > 0 ? 1 : 0);
        uint32 groupCountY = dofHeight;
        //
        context->Dispatch(shader->GetCS("CS_DepthOfFieldH"), groupCountX, groupCountY, 1);

        // Cleanup
        context->ResetRenderTarget();
        context->UnBindSR(0);
        context->UnBindUA(0);
        context->FlushState();

        // Vertical pass
        context->BindUA(0, dofTargetV->View());
        //
        context->BindSR(0, dofTargetH);
        context->BindSR(1, depthBlurTarget);
        //
        groupCountX = dofWidth;
        groupCountY = (dofHeight / DOF_GRID_SIZE) + ((dofHeight % DOF_GRID_SIZE) > 0 ? 1 : 0);
        //
        // TODO: cache Compute Shaders
        context->Dispatch(shader->GetCS("CS_DepthOfFieldV"), groupCountX, groupCountY, 1);
        context->ResetRenderTarget();

        // Cleanup
        context->UnBindSR(0);
        context->UnBindSR(1);
        context->UnBindUA(0);
        context->FlushState();
        RenderTargetPool::Release(dofTargetH);

        dofOutput = dofTargetV;
    }

    // Cleanup temporary texture
    RenderTargetPool::Release(dofInput);

    // Render the bokeh points
    if (isBokehGenerationEnabled)
    {
        tempDesc = GPUTextureDescription::New2D(bokehTargetWidth, bokehTargetHeight, DOF_RT_FORMAT);
        auto bokehTarget = RenderTargetPool::Get(tempDesc);
        context->Clear(*bokehTarget, Color::Black);

        {
            // Copy the count from the bokeh point AppendStructuredBuffer to our indirect arguments buffer.
            // This lets us issue a draw call with number of vertices == the number of points in the buffer, without having to copy anything back to the CPU.
            context->CopyCounter(_bokehIndirectArgsBuffer, 0, _bokehBuffer);

            // Blend the points additive to an intermediate target
            context->SetRenderTarget(*bokehTarget);
            context->SetViewportAndScissors((float)bokehTargetWidth, (float)bokehTargetHeight);

            // Draw the bokeh points
            context->BindSR(0, (GPUTexture*)getDofBokehShape(dofSettings));
            context->BindSR(1, depthBlurTarget);
            context->BindSR(2, _bokehBuffer->View());
            context->SetState(_psBokeh);
            context->DrawInstancedIndirect(_bokehIndirectArgsBuffer, 0);
            context->ResetRenderTarget();
        }

        // Composite the bokeh rendering results with the depth of field result
        tempDesc = GPUTextureDescription::New2D(dofWidth, dofHeight, DOF_RT_FORMAT);
        auto compositeTarget = RenderTargetPool::Get(tempDesc);
        context->BindSR(0, bokehTarget);
        context->BindSR(1, dofOutput);
        context->SetRenderTarget(*compositeTarget);
        context->SetViewportAndScissors((float)dofWidth, (float)dofHeight);
        context->SetState(_psBokehComposite);
        context->DrawFullscreenTriangle();
        context->ResetRenderTarget();

        RenderTargetPool::Release(bokehTarget);
        RenderTargetPool::Release(dofOutput);
        dofOutput = compositeTarget;
    }

    RenderTargetPool::Release(depthBlurTarget);

    // Return output temporary render target
    return dofOutput;
}
