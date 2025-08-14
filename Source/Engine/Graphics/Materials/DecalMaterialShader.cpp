// Copyright (c) Wojciech Figat. All rights reserved.

#include "DecalMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct DecalMaterialShaderData {
    Matrix WorldMatrix;
    Matrix InvWorld;
    Matrix SvPositionToWorld;
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
    auto& drawCall = *params.DrawCall;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(DecalMaterialShaderData));
    auto materialData = reinterpret_cast<DecalMaterialShaderData*>(cb.Get());
    cb = cb.Slice(sizeof(DecalMaterialShaderData));
    const bool isCameraInside = OrientedBoundingBox(Vector3::Half, drawCall.World).Contains(view.Position) == ContainmentType::Contains;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = true;
    bindMeta.CanSampleGBuffer = false;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Decals use depth buffer to draw on top of the objects
    GPUTexture* depthBuffer = params.RenderContext.Buffers->DepthBuffer;
    GPUTextureView* depthBufferView = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView) ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
    context->BindSR(0, depthBufferView);

    // Setup material constants
    {
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);

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
        Matrix::Transpose(svPositionToWorld, materialData->SvPositionToWorld);
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
    psDesc0.VS = _shader->GetVS("VS_Decal"); // TODO: move VS_Decal to be shared (eg. in GBuffer.shader)
    if (psDesc0.VS == nullptr)
        return true;
    psDesc0.PS = _shader->GetPS("PS_Decal");
    psDesc0.CullMode = CullMode::Normal;
    if (GPUDevice::Instance->Limits.HasReadOnlyDepth)
    {
        psDesc0.DepthEnable = true;
        psDesc0.DepthWriteEnable = false;
    }

    switch (_info.DecalBlendingMode)
    {
    case MaterialDecalBlendingMode::Translucent:
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::SrcAlpha;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::Zero;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    case MaterialDecalBlendingMode::Stain:
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::DestColor;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::Zero;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    case MaterialDecalBlendingMode::Normal:
        psDesc0.BlendMode.BlendEnable = true;
        psDesc0.BlendMode.SrcBlend = BlendingMode::Blend::SrcAlpha;
        psDesc0.BlendMode.DestBlend = BlendingMode::Blend::InvSrcAlpha;
        psDesc0.BlendMode.SrcBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.DestBlendAlpha = BlendingMode::Blend::One;
        psDesc0.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        break;
    case MaterialDecalBlendingMode::Emissive:
        psDesc0.BlendMode = BlendingMode::Additive;
        break;
    }

    _cache.Outside = GPUDevice::Instance->CreatePipelineState();
    if (_cache.Outside->Init(psDesc0))
    {
        LOG(Warning, "Failed to create decal material pipeline state.");
        return true;
    }

    psDesc0.CullMode = CullMode::Inverted;
    psDesc0.DepthEnable = false;
    _cache.Inside = GPUDevice::Instance->CreatePipelineState();
    if (_cache.Inside->Init(psDesc0))
    {
        LOG(Warning, "Failed to create decal material pipeline state.");
        return true;
    }

    return false;
}
