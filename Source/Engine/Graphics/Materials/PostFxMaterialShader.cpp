// Copyright (c) Wojciech Figat. All rights reserved.

#include "PostFxMaterialShader.h"
#include "MaterialParams.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct PostFxMaterialShaderData {
    Matrix ViewMatrix;
    Float3 ViewPos;
    float ViewFar;
    Float3 ViewDir;
    float TimeParam;
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float4 TemporalAAJitter;
    Matrix InverseViewProjectionMatrix;
    Float3 ViewPadding0;
    float UnscaledTimeParam;
    });

void PostFxMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    Span<byte> cb(_cbData.Get(), _cbData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(PostFxMaterialShaderData));
    auto materialData = reinterpret_cast<PostFxMaterialShaderData*>(cb.Get());
    cb = cb.Slice(sizeof(PostFxMaterialShaderData));
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
        Matrix::Transpose(view.IVP, materialData->InverseViewProjectionMatrix);
        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = params.Time;
        materialData->UnscaledTimeParam = params.UnscaledTime;
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
