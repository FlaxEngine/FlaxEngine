// Copyright (c) Wojciech Figat. All rights reserved.

#include "DeferredMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Models/SkinnedMeshDrawData.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTask.h"

DrawPass DeferredMaterialShader::GetDrawModes() const
{
    return DrawPass::Depth | DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas | DrawPass::MotionVectors | DrawPass::QuadOverdraw;
}

bool DeferredMaterialShader::CanUseLightmap() const
{
    return true;
}

bool DeferredMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, };
    return true;
}

void DeferredMaterialShader::Bind(BindParameters& params)
{
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.DrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    int32 srv = 3;

    // Setup features
    const bool useLightmap = _info.BlendMode == MaterialBlendMode::Opaque && LightmapFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = false;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);
    context->BindSR(0, params.ObjectBuffer);

    // Check if using mesh skinning
    const bool useSkinning = drawCall.Surface.Skinning != nullptr;
    bool perBoneMotionBlur = false;
    if (useSkinning)
    {
        // Bind skinning buffer
        ASSERT(drawCall.Surface.Skinning->IsReady());
        context->BindSR(1, drawCall.Surface.Skinning->BoneMatrices->View());
        if (drawCall.Surface.Skinning->PrevBoneMatrices && drawCall.Surface.Skinning->PrevBoneMatrices->IsAllocated())
        {
            context->BindSR(2, drawCall.Surface.Skinning->PrevBoneMatrices->View());
            perBoneMotionBlur = true;
        }
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
    const auto cache = params.Instanced ? &_cacheInstanced : &_cache;
    PipelineStateCache* psCache = cache->GetPS(view.Pass, useLightmap, useSkinning, perBoneMotionBlur);
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void DeferredMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
    _cacheInstanced.Release();
}

bool DeferredMaterialShader::Load()
{
    bool failed = false;
    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == MaterialFeaturesFlags::None;
    if (EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::DisableDepthTest))
    {
        psDesc.DepthFunc = ComparisonFunc::Always;
        if (!psDesc.DepthWriteEnable)
            psDesc.DepthEnable = false;
    }

#if GPU_ALLOW_TESSELLATION_SHADERS
    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }
#endif

    // GBuffer Pass
    psDesc.VS = _shader->GetVS("VS");
    failed |= psDesc.VS == nullptr;
    psDesc.PS = _shader->GetPS("PS_GBuffer");
    _cache.Default.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    failed |= psDesc.VS == nullptr;
    _cacheInstanced.Default.Init(psDesc);

    // GBuffer Pass with lightmap (pixel shader permutation for USE_LIGHTMAP=1)
    psDesc.VS = _shader->GetVS("VS");
    failed |= psDesc.VS == nullptr;
    psDesc.PS = _shader->GetPS("PS_GBuffer", 1);
    _cache.DefaultLightmap.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    failed |= psDesc.VS == nullptr;
    _cacheInstanced.DefaultLightmap.Init(psDesc);

    // GBuffer Pass with skinning
    psDesc.VS = _shader->GetVS("VS_Skinned");
    psDesc.PS = _shader->GetPS("PS_GBuffer");
    _cache.DefaultSkinned.Init(psDesc);

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

    // Motion Vectors pass
    psDesc.DepthWriteEnable = false;
    psDesc.DepthEnable = true;
    psDesc.DepthFunc = ComparisonFunc::LessEqual;
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_MotionVectors");
    _cache.MotionVectors.Init(psDesc);

    // Motion Vectors pass with skinning
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.MotionVectorsSkinned.Init(psDesc);

    // Motion Vectors pass with skinning (with per-bone motion blur)
    psDesc.VS = _shader->GetVS("VS_Skinned", 1);
    _cache.MotionVectorsSkinnedPerBone.Init(psDesc);

    // Depth Pass
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    GPUShaderProgramVS* instancedDepthPassVS;
    if (EnumHasAnyFlags(_info.UsageFlags, MaterialUsageFlags::UseMask | MaterialUsageFlags::UsePositionOffset))
    {
        // Materials with masking need full vertex buffer to get texcoord used to sample textures for per pixel masking.
        // Materials with world pos offset need full VB to apply offset using texcoord etc.
        psDesc.VS = _shader->GetVS("VS");
        instancedDepthPassVS = _shader->GetVS("VS", 1);
        psDesc.PS = _shader->GetPS("PS_Depth");
    }
    else
    {
        psDesc.VS = _shader->GetVS("VS_Depth");
        instancedDepthPassVS = _shader->GetVS("VS_Depth", 1);
        psDesc.PS = nullptr;
    }
    _cache.Depth.Init(psDesc);
    psDesc.VS = instancedDepthPassVS;
    _cacheInstanced.Depth.Init(psDesc);

    // Depth Pass with skinning
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DepthSkinned.Init(psDesc);

    return failed;
}
