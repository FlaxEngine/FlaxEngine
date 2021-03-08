// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DecalMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct DecalMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Matrix InvWorld;
    Matrix SVPositionToWorld;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    });

DrawPass DecalMaterialShader::GetDrawModes() const
{
    return DrawPass::GBuffer;
}

void DecalMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<DecalMaterialShaderData*>(cb);
    cb += sizeof(DecalMaterialShaderData);
    int32 srv = 0;
    const bool isCameraInside = OrientedBoundingBox(Vector3::Half, params.FirstDrawCall->World).Contains(view.Position) == ContainmentType::Contains;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = nullptr;
    bindMeta.CanSampleDepth = true;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Decals use depth buffer to draw on top of the objects
    context->BindSR(0, GET_TEXTURE_VIEW_SAFE(params.RenderContext.Buffers->DepthBuffer));

    // Setup material constants
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;

        // Matrix for transformation from world space to decal object space
        Matrix invWorld;
        Matrix::Invert(drawCall.World, invWorld);
        Matrix::Transpose(invWorld, materialData->InvWorld);

        // Matrix for transformation from SV Position space to world space
        const Matrix offsetMatrix(
            2.0f * view.ScreenSize.Z, 0, 0, 0,
            0, -2.0f * view.ScreenSize.W, 0, 0,
            0, 0, 1, 0,
            -1.0f, 1.0f, 0, 1);
        const Matrix svPositionToWorld = offsetMatrix * view.IVP;
        Matrix::Transpose(svPositionToWorld, materialData->SVPositionToWorld);
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Bind pipeline
    context->SetState(isCameraInside ? _cache.Inside : _cache.Outside);
}

void DecalMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
}

bool DecalMaterialShader::Load()
{
    GPUPipelineState::Description psDesc0 = GPUPipelineState::Description::DefaultNoDepth;
    psDesc0.VS = _shader->GetVS("VS_Decal");
    psDesc0.PS = _shader->GetPS("PS_Decal");
    psDesc0.CullMode = CullMode::Normal;

    switch (_info.DecalBlendingMode)
    {
    case MaterialDecalBlendingMode::Translucent:
    {
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::SrcAlpha;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::Zero;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    }
    case MaterialDecalBlendingMode::Stain:
    {
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::DestColor;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::Zero;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    }
    case MaterialDecalBlendingMode::Normal:
    {
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::SrcAlpha;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    }
    case MaterialDecalBlendingMode::Emissive:
    {
        psDesc0.BlendMode = BlendingMode::Additive;
        break;
    }
    }

    _cache.Outside = GPUDevice::Instance->CreatePipelineState();
    if (_cache.Outside->Init(psDesc0))
    {
        LOG(Warning, "Failed to create decal material pipeline state.");
        return true;
    }

    psDesc0.CullMode = CullMode::Inverted;
    _cache.Inside = GPUDevice::Instance->CreatePipelineState();
    if (_cache.Inside->Init(psDesc0))
    {
        LOG(Warning, "Failed to create decal material pipeline state.");
        return true;
    }

    return false;
}
