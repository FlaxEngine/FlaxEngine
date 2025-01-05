// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GUIMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct GUIMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Float3 ViewPos;
    float ViewFar;
    Float3 ViewDir;
    float TimeParam;
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float4 ViewSize;
    });

void GUIMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(GUIMaterialShaderData));
    auto materialData = reinterpret_cast<GUIMaterialShaderData*>(cb.Get());
    cb = cb.Slice(sizeof(GUIMaterialShaderData));
    int32 srv = 0;
    const auto ps = context->IsDepthBufferBinded() ? _cache.Depth : _cache.NoDepth;
    auto customData = (Render2D::CustomData*)params.CustomData;

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
        Matrix::Transpose(customData->ViewProjection, materialData->ViewProjectionMatrix);
        Matrix::Transpose(Matrix::Identity, materialData->WorldMatrix);
        Matrix::Transpose(Matrix::Identity, materialData->ViewMatrix);
        materialData->ViewPos = Float3::Zero;
        materialData->ViewFar = 0.0f;
        materialData->ViewDir = Float3::Forward;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = Float4::Zero;
        auto& viewport = Render2D::GetViewport();
        materialData->ScreenSize = Float4(viewport.Width, viewport.Height, 1.0f / viewport.Width, 1.0f / viewport.Height);
        materialData->ViewSize = Float4(customData->ViewSize.X, customData->ViewSize.Y, 1.0f / customData->ViewSize.X, 1.0f / customData->ViewSize.Y);
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Bind pipeline
    context->SetState(ps);
}

void GUIMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
}

bool GUIMaterialShader::Load()
{
    GPUPipelineState::Description psDesc0 = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDesc0.Wireframe = EnumHasAnyFlags(_info.FeaturesFlags, MaterialFeaturesFlags::Wireframe);
    psDesc0.VS = _shader->GetVS("VS_GUI");
    psDesc0.PS = _shader->GetPS("PS_GUI");
    psDesc0.BlendMode = BlendingMode::AlphaBlend;

    psDesc0.DepthEnable = psDesc0.DepthWriteEnable = true;
    _cache.Depth = GPUDevice::Instance->CreatePipelineState();
    _cache.NoDepth = GPUDevice::Instance->CreatePipelineState();

    bool failed = _cache.Depth->Init(psDesc0);

    psDesc0.DepthEnable = psDesc0.DepthWriteEnable = false;
    failed |= _cache.NoDepth->Init(psDesc0);

    if (failed)
    {
        LOG(Warning, "Failed to create GUI material pipeline state.");
        return true;
    }

    return false;
}
