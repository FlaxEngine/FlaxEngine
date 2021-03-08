// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GUIMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Core/Math/Viewport.h"
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
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    });

void GUIMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<GUIMaterialShaderData*>(cb);
    cb += sizeof(GUIMaterialShaderData);
    int32 srv = 0;
    const auto ps = context->IsDepthBufferBinded() ? _cache.Depth : _cache.NoDepth;

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
        const auto viewProjectionMatrix = (Matrix*)params.CustomData;
        Matrix::Transpose(*viewProjectionMatrix, materialData->ViewProjectionMatrix);
        Matrix::Transpose(Matrix::Identity, materialData->WorldMatrix);
        Matrix::Transpose(Matrix::Identity, materialData->ViewMatrix);
        materialData->ViewPos = Vector3::Zero;
        materialData->ViewFar = 0.0f;
        materialData->ViewDir = Vector3::Forward;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = Vector4::Zero;
        auto& viewport = Render2D::GetViewport();
        materialData->ScreenSize = Vector4(viewport.Width, viewport.Height, 1.0f / viewport.Width, 1.0f / viewport.Height);
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
    psDesc0.Wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0;
    psDesc0.VS = _shader->GetVS("VS_GUI");
    psDesc0.PS = _shader->GetPS("PS_GUI");
    psDesc0.BlendMode = BlendingMode::AlphaBlend;

    psDesc0.DepthTestEnable = psDesc0.DepthWriteEnable = true;
    _cache.Depth = GPUDevice::Instance->CreatePipelineState();
    _cache.NoDepth = GPUDevice::Instance->CreatePipelineState();

    bool failed = _cache.Depth->Init(psDesc0);

    psDesc0.DepthTestEnable = psDesc0.DepthWriteEnable = false;
    failed |= _cache.NoDepth->Init(psDesc0);

    if (failed)
    {
        LOG(Warning, "Failed to create GUI material pipeline state.");
        return true;
    }

    return false;
}
