// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "LODPreview.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/DrawCall.h"

PACK_STRUCT(struct LODPreviewMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Color Color;
    Vector3 WorldInvScale;
    float LODDitherFactor;
    });

LODPreviewMaterialShader::LODPreviewMaterialShader()
{
    _ps = GPUDevice::Instance->CreatePipelineState();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/LODPreview"));
    if (!_shader)
        return;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<LODPreviewMaterialShader, &LODPreviewMaterialShader::OnShaderReloading>(this);
#endif
}

#if COMPILE_WITH_DEV_ENV

void LODPreviewMaterialShader::OnShaderReloading(Asset* obj)
{
    _ps->ReleaseGPU();
}

#endif

const MaterialInfo& LODPreviewMaterialShader::GetInfo() const
{
    return _info;
}

bool LODPreviewMaterialShader::IsReady() const
{
    return _shader && _shader->IsLoaded();
}

DrawPass LODPreviewMaterialShader::GetDrawModes() const
{
    return DrawPass::GBuffer;
}

void LODPreviewMaterialShader::Bind(BindParameters& params)
{
    auto context = params.GPUContext;
    auto& drawCall = *params.FirstDrawCall;
    auto shader = _shader->GetShader();
    auto cb = shader->GetCB(0);
    if (!_ps->IsValid())
    {
        auto psDesc = GPUPipelineState::Description::Default;
        psDesc.VS = shader->GetVS("VS");
        psDesc.PS = shader->GetPS("PS");
        _ps->Init(psDesc);
    }

    // Find the LOD that produced this draw call
    int32 lodIndex = 0;
    for (auto& e : Content::GetAssetsRaw())
    {
        auto model = ScriptingObject::Cast<Model>(e.Value);
        if (!model)
            continue;
        bool found = false;
        for (const auto& lod : model->LODs)
        {
            for (const auto& mesh : lod.Meshes)
            {
                if (mesh.GetIndexBuffer() == drawCall.Geometry.IndexBuffer)
                {
                    lodIndex = mesh.GetLODIndex();
                    found = true;
                    break;
                }
            }
        }
        if (found)
            break;
    }

    // Bind
    if (cb && cb->GetSize())
    {
        ASSERT(cb->GetSize() == sizeof(LODPreviewMaterialShaderData));
        LODPreviewMaterialShaderData data;
        Matrix::Transpose(params.RenderContext.View.Frustum.GetMatrix(), data.ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data.WorldMatrix);
        const float scaleX = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        data.WorldInvScale = Vector3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);
        const Color colors[MODEL_MAX_LODS] = {
            Color::White,
            Color::Red,
            Color::Orange,
            Color::Yellow,
            Color::Green,
            Color::Blue,
        };
        ASSERT(lodIndex < MODEL_MAX_LODS);
        data.Color = colors[lodIndex];
        data.LODDitherFactor = drawCall.Surface.LODDitherFactor;
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
    }
    context->SetState(_ps);
}

#endif
