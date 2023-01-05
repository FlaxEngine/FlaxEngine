// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "ForwardMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
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

#define MAX_LOCAL_LIGHTS 4

PACK_STRUCT(struct ForwardMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Matrix PrevViewProjectionMatrix;
    Matrix PrevWorldMatrix;
    Matrix MainViewProjectionMatrix;
    Float4 MainScreenSize;
    Float3 ViewPos;
    float ViewFar;
    Float3 ViewDir;
    float TimeParam;
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float3 WorldInvScale;
    float WorldDeterminantSign;
    Float2 Dummy0;
    float LODDitherFactor;
    float PerInstanceRandom;
    Float4 TemporalAAJitter;
    Float3 GeometrySize;
    float Dummy1;
    });

DrawPass ForwardMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

bool ForwardMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall, };
    return true;
}

void ForwardMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(ForwardMaterialShaderData));
    auto materialData = reinterpret_cast<ForwardMaterialShaderData*>(cb.Get());
    cb = Span<byte>(cb.Get() + sizeof(ForwardMaterialShaderData), cb.Length() - sizeof(ForwardMaterialShaderData));
    int32 srv = 2;

    // Setup features
    if (static_cast<int32>(_info.FeaturesFlags & MaterialFeaturesFlags::GlobalIllumination))
        GlobalIlluminationFeature::Bind(params, cb, srv);
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

    // Check if is using mesh skinning
    const bool useSkinning = drawCall.Surface.Skinning != nullptr;
    if (useSkinning)
    {
        // Bind skinning buffer
        ASSERT(drawCall.Surface.Skinning->IsReady());
        context->BindSR(0, drawCall.Surface.Skinning->BoneMatrices->View());
    }

    // Setup material constants
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        Matrix::Transpose(drawCall.Surface.PrevWorld, materialData->PrevWorldMatrix);
        Matrix::Transpose(view.PrevViewProjection, materialData->PrevViewProjectionMatrix);
        Matrix::Transpose(view.MainViewProjection, materialData->MainViewProjectionMatrix);
        materialData->MainScreenSize = view.MainScreenSize;
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        const float scaleX = Float3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Float3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Float3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        materialData->WorldInvScale = Float3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->GeometrySize = drawCall.Surface.GeometrySize;
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = static_cast<int32>(_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
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
    ASSERT_LOW_LAYER(!(useSkinning && params.DrawCallsCount > 1)); // No support for instancing skinned meshes
    const auto cacheObj = params.DrawCallsCount == 1 ? &_cache : &_cacheInstanced;
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
    psDesc.DepthTestEnable = static_cast<int32>(_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == 0;
    psDesc.DepthWriteEnable = static_cast<int32>(_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == 0;

    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }

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
    psDesc.DepthTestEnable = true;
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
