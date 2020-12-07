// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "PostFxMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Engine/Time.h"
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
    auto& drawCall = *params.FirstDrawCall;
    const auto cb0 = _shader->GetCB(0);
    const bool hasCb0 = cb0->GetSize() != 0;

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Buffer0 = hasCb0 ? _cb0Data.Get() + sizeof(PostFxMaterialShaderData) : nullptr;
    bindMeta.Input = params.Input;
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = true;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Setup material constants data
    if (hasCb0)
    {
        const auto materialData = reinterpret_cast<PostFxMaterialShaderData*>(_cb0Data.Get());

        Matrix::Transpose(view.View, materialData->ViewMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = Time::Draw.UnscaledTime.GetTotalSeconds();
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;
        materialData->TemporalAAJitter = view.TemporalAAJitter;
    }

    // Bind constants
    if (hasCb0)
    {
        context->UpdateCB(cb0, _cb0Data.Get());
        context->BindCB(0, cb0);
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
