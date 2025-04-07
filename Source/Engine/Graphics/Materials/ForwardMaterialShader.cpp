// Copyright (c) Wojciech Figat. All rights reserved.

#include "ForwardMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Models/SkinnedMeshDrawData.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

DrawPass ForwardMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

bool ForwardMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, };
    return true;
}

void ForwardMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.DrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    int32 srv = 3;

    // Setup features
    if ((_info.FeaturesFlags & MaterialFeaturesFlags::GlobalIllumination) != MaterialFeaturesFlags::None)
    {
        GlobalIlluminationFeature::Bind(params, cb, srv);
        if ((_info.FeaturesFlags & MaterialFeaturesFlags::ScreenSpaceReflections) != MaterialFeaturesFlags::None)
            SDFReflectionsFeature::Bind(params, cb, srv);
    }
    ForwardShadingFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = params.Input;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = GPUDevice::Instance->Limits.HasReadOnlyDepth;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);
    context->BindSR(0, params.ObjectBuffer);

    // Check if using mesh skinning
    const bool useSkinning = drawCall.Surface.Skinning != nullptr;
    if (useSkinning)
    {
        // Bind skinning buffer
        ASSERT(drawCall.Surface.Skinning->IsReady());
        context->BindSR(1, drawCall.Surface.Skinning->BoneMatrices->View());
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != MaterialFeaturesFlags::None || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
#if USE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    ASSERT_LOW_LAYER(!(useSkinning && params.Instanced)); // No support for instancing skinned meshes
    const auto cacheObj = params.Instanced ? &_cacheInstanced : &_cache;
    PipelineStateCache* psCache = cacheObj->GetPS(view.Pass, useSkinning);
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void ForwardMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
    _cacheInstanced.Release();
}

bool ForwardMaterialShader::Load()
{
    _drawModes = DrawPass::Depth | DrawPass::Forward | DrawPass::QuadOverdraw;

    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == MaterialFeaturesFlags::None;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == MaterialFeaturesFlags::None;

#if GPU_ALLOW_TESSELLATION_SHADERS
    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }
#endif

#if USE_EDITOR
    if (_shader->HasShader("PS_QuadOverdraw"))
    {
        // Quad Overdraw
        psDesc.VS = _shader->GetVS("VS");
        psDesc.PS = _shader->GetPS("PS_QuadOverdraw");
        _cache.QuadOverdraw.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS", 1);
        _cacheInstanced.Depth.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS_Skinned");
        _cache.QuadOverdrawSkinned.Init(psDesc);
    }
#endif

    // Check if use transparent distortion pass
    if (_shader->HasShader("PS_Distortion"))
    {
        _drawModes |= DrawPass::Distortion;

        // Accumulate Distortion Pass
        psDesc.VS = _shader->GetVS("VS");
        psDesc.PS = _shader->GetPS("PS_Distortion");
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.DepthWriteEnable = false;
        _cache.Distortion.Init(psDesc);
        //psDesc.VS = _shader->GetVS("VS", 1);
        //_cacheInstanced.Distortion.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS_Skinned");
        _cache.DistortionSkinned.Init(psDesc);
    }

    // Forward Pass
    psDesc.VS = _shader->GetVS("VS");
    if (psDesc.VS == nullptr)
        return true;
    psDesc.PS = _shader->GetPS("PS_Forward");
    psDesc.DepthWriteEnable = false;
    psDesc.BlendMode = BlendingMode::AlphaBlend;
    switch (_info.BlendMode)
    {
    case MaterialBlendMode::Transparent:
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        break;
    case MaterialBlendMode::Additive:
        psDesc.BlendMode = BlendingMode::Additive;
        break;
    case MaterialBlendMode::Multiply:
        psDesc.BlendMode = BlendingMode::Multiply;
        break;
    }
    _cache.Default.Init(psDesc);
    //psDesc.VS = _shader->GetVS("VS", 1);
    //_cacheInstanced.Default.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DefaultSkinned.Init(psDesc);

    // Depth Pass
    psDesc = GPUPipelineState::Description::Default;
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_Depth");
    _cache.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    _cacheInstanced.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DepthSkinned.Init(psDesc);

    return false;
}
