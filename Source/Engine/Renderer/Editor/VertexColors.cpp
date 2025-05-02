// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "VertexColors.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/DrawCall.h"

GPU_CB_STRUCT(VertexColorsMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    });

VertexColorsMaterialShader::VertexColorsMaterialShader()
{
    _ps = GPUDevice::Instance->CreatePipelineState();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/VertexColors"));
    if (!_shader)
        return;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<VertexColorsMaterialShader, &VertexColorsMaterialShader::OnShaderReloading>(this);
#endif
}

#if COMPILE_WITH_DEV_ENV

void VertexColorsMaterialShader::OnShaderReloading(Asset* obj)
{
    _ps->ReleaseGPU();
}

#endif

const MaterialInfo& VertexColorsMaterialShader::GetInfo() const
{
    return _info;
}

GPUShader* VertexColorsMaterialShader::GetShader() const
{
    return _shader->GetShader();
}

bool VertexColorsMaterialShader::IsReady() const
{
    return _shader && _shader->IsLoaded();
}

DrawPass VertexColorsMaterialShader::GetDrawModes() const
{
    return DrawPass::GBuffer;
}

void VertexColorsMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& drawCall = *params.DrawCall;

    // Setup
    auto shader = _shader->GetShader();
    auto cb = shader->GetCB(0);
    if (!_ps->IsValid())
    {
        auto psDesc = GPUPipelineState::Description::Default;
        psDesc.VS = shader->GetVS("VS");
        psDesc.PS = shader->GetPS("PS");
        _ps->Init(psDesc);
    }

    // Bind constants
    if (cb && cb->GetSize())
    {
        ASSERT(cb->GetSize() == sizeof(VertexColorsMaterialShaderData));
        VertexColorsMaterialShaderData data;
        Matrix::Transpose(params.RenderContext.View.Frustum.GetMatrix(), data.ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data.WorldMatrix);
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
    }

    // Bind pipeline
    context->SetState(_ps);
}

#endif
