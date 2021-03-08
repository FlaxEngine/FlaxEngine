// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DeformableMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTask.h"

PACK_STRUCT(struct DeformableMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix LocalMatrix;
    Matrix ViewMatrix;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    Vector3 Dummy0;
    float WorldDeterminantSign;
    float MeshMinZ;
    float Segment;
    float ChunksPerSegment;
    float PerInstanceRandom;
    Vector4 TemporalAAJitter;
    Vector3 GeometrySize;
    float MeshMaxZ;
    });

DrawPass DeformableMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

void DeformableMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<DeformableMaterialShaderData*>(cb);
    cb += sizeof(DeformableMaterialShaderData);
    int32 srv = 1;

    // Setup features
    if (_info.BlendMode != MaterialBlendMode::Opaque)
        ForwardShadingFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = nullptr;
    bindMeta.CanSampleDepth = false;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup material constants
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(drawCall.Deformable.LocalMatrix, materialData->LocalMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->Segment = drawCall.Deformable.Segment;
        materialData->ChunksPerSegment = drawCall.Deformable.ChunksPerSegment;
        materialData->MeshMinZ = drawCall.Deformable.MeshMinZ;
        materialData->MeshMaxZ = drawCall.Deformable.MeshMaxZ;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->TemporalAAJitter = view.TemporalAAJitter;
        materialData->GeometrySize = drawCall.Deformable.GeometrySize;
    }

    // Bind spline deformation buffer
    context->BindSR(0, drawCall.Deformable.SplineDeformation->View());

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    PipelineStateCache* psCache = _cache.GetPS(view.Pass);
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void DeformableMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
}

bool DeformableMaterialShader::Load()
{
    _drawModes = DrawPass::Depth;
    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthTestEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == 0;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == 0;

    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }

    if (_info.BlendMode == MaterialBlendMode::Opaque)
    {
        _drawModes = DrawPass::GBuffer;

        // GBuffer Pass
        psDesc.VS = _shader->GetVS("VS_SplineModel");
        psDesc.PS = _shader->GetPS("PS_GBuffer");
        _cache.Default.Init(psDesc);
    }
    else
    {
        _drawModes = DrawPass::Forward;

        // Forward Pass
        psDesc.VS = _shader->GetVS("VS_SplineModel");
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
    }

    // Depth Pass
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthTestEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    _cache.Depth.Init(psDesc);

    return false;
}
