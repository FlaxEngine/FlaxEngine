// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PostFxMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct PostFxMaterialShaderData {
    Matrix ViewMatrix;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    Vector4 TemporalAAJitter;
    });

void PostFxMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    byte* cb = _cbData.Get();
    auto materialData = reinterpret_cast<PostFxMaterialShaderData*>(cb);
    cb += sizeof(PostFxMaterialShaderData);
    int32 srv = 0;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = params.Input;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = true;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup material constants
    {
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.TimeParam;
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        materialData->TemporalAAJitter = view.TemporalAAJitter;
    }

    // Bind constants
    if (_cb)
    {
        context->UpdateCB(_cb, _cbData.Get());
        context->BindCB(0, _cb);
    }

    // Bind pipeline
    context->SetState(_cache.Default);
}

void PostFxMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
}

bool PostFxMaterialShader::Load()
{
    // PostFx material uses 'PS_PostFx' pixel shader and default simple shared quad vertex shader
    GPUPipelineState::Description psDesc0 = GPUPipelineState::Description::DefaultFullscreenTriangle;
    psDesc0.VS = GPUDevice::Instance->QuadShader->GetVS("VS_PostFx");
    psDesc0.PS = _shader->GetPS("PS_PostFx");
    _cache.Default = GPUDevice::Instance->CreatePipelineState();
    if (_cache.Default->Init(psDesc0))
    {
        LOG(Warning, "Failed to create postFx material pipeline state.");
        return true;
    }

    return false;
}
