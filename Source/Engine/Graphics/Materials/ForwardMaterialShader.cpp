// Copyright (c) Wojciech Figat. All rights reserved.

#include "ForwardMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderContext.h"
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

bool ForwardMaterialShader::CanUseInstancing(const RenderContext& renderContext, InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, };
    // TODO: support instancing when using ForwardShadingFeature
    return renderContext.View.Pass == DrawPass::Depth;
}

void ForwardMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.DrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    int32 srv = 2;

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

    // Bind skinning buffer
    const bool useSkinning = drawCall.Surface.Skinning != DrawCall::SkinningMode::None;
    if (useSkinning)
    {
        context->BindSR(1, drawCall.Surface.SkinningBones->View());
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Select pipeline state based on current pass and render mode
    CullMode cullMode = GetCullMode(view.Pass, drawCall.WorldDeterminant);
#if USE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif
    const auto cache = params.Instanced ? &_cacheInstanced : &_cache;
    PipelineStateCache* psCache = cache->GetPS(view.Pass, useSkinning);
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(this, cullMode);

    // Bind pipeline
    context->SetState(state);
}

#if COMPILE_WITH_PSO_PRECACHING

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Utilities/BitIterator.h"

void ForwardMaterialShader::PrecachePSO(const PrecacheParameters& params)
{
    // Precache all used shader permutations
    PROFILE_CPU();
    auto& context = *(GPUPipelineState::PrecacheContext*)params.PrecacheContext;
    uint32 canUseInstancing = params.Pass == DrawPass::Depth;
    for (uint32 instanced = 0; instanced <= canUseInstancing; instanced++)
    {
        auto vertexLayout = context.VertexLayout;
        if (instanced && vertexLayout)
        {
            // Insert instanced object index into the vertex stream
            auto elements = vertexLayout->GetElements();
            elements.Add(INSTANCED_DRAW_OBJECT_ID_ELEMENT);
            context.VertexLayout = GPUVertexLayout::Get(elements, true);
        }
        auto cache = instanced ? &_cacheInstanced : &_cache;
        for (uint32 geometry : BitIterator(params.MaterialFlags.Geometry& MaterialPrecacheFlags::GeometrySurface))
        {
            bool useSkinning = geometry == MaterialPrecacheFlags::Skinned;
            for (uint32 worldDeterminant : BitIterator(params.MaterialFlags.WorldDeterminant))
            {
                auto psCache = cache->GetPS(params.Pass, useSkinning);
                auto cullMode = GetCullMode(params.Pass, worldDeterminant == MaterialPrecacheFlags::Negative);
                PipelineStateCache::PrecachePSO(psCache, params, this, cullMode);
            }
        }
        context.VertexLayout = vertexLayout;
    }
}

#endif

void ForwardMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
    _cacheInstanced.Release();
}

bool ForwardMaterialShader::Load()
{
    _drawModes = DrawPass::Depth | DrawPass::Forward | DrawPass::QuadOverdraw | DrawPass::Wireframe;
    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthEnable = EnumHasNoneFlags(_info.FeaturesFlags, MaterialFeaturesFlags::DisableDepthTest);
    psDesc.DepthWriteEnable = EnumHasNoneFlags(_info.FeaturesFlags, MaterialFeaturesFlags::DisableDepthWrite);
    psDesc.Wireframe = EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::Wireframe);
    psDesc.CullMode = _info.CullMode;

#if GPU_ALLOW_TESSELLATION_SHADERS
    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }
#endif

#if GPU_ENABLE_DEVELOPMENT
    if (_shader->HasShader("PS_QuadOverdraw"))
    {
        // Quad Overdraw
        _drawModes |= DrawPass::QuadOverdraw;
        psDesc.VS = _shader->GetVS("VS");
        psDesc.PS = _shader->GetPS("PS_QuadOverdraw");
        _cache.QuadOverdraw.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS", 1);
        _cacheInstanced.QuadOverdraw.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS_Skinned");
        _cache.QuadOverdrawSkinned.Init(psDesc);
    }

    // Wireframe
    psDesc.Wireframe = true;
    psDesc.DepthWriteEnable = false;
    psDesc.BlendMode = BlendingMode::AlphaBlend;
    psDesc.PS = GPUDevice::Instance->QuadShader->GetPS("PS_Wireframe");
    psDesc.VS = _shader->GetVS("VS");
    _cache.Wireframe.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    _cacheInstanced.Wireframe.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.WireframeSkinned.Init(psDesc);
    psDesc.DepthWriteEnable = EnumHasNoneFlags(_info.FeaturesFlags, MaterialFeaturesFlags::DisableDepthWrite);
    psDesc.Wireframe = EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::Wireframe);
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
    psDesc.DepthFunc = ComparisonFunc::Default;
    psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::None;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_Depth");
    _cache.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    _cacheInstanced.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DepthSkinned.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned", 2);
    _cacheInstanced.DepthSkinned.Init(psDesc);

#if PLATFORM_PS5
    // Fix shader binding issues on forward shading materials on PS5
    _drawModes = DrawPass::None;
#endif

    return false;
}
