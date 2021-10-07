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
#include "Engine/Renderer/RenderList.h"

PACK_STRUCT(struct SingleColorShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Color Color;
    Vector3 Dummy0;
    float LODDitherFactor;
    });

LODPreviewMaterialShader::LODPreviewMaterialShader()
{
    _psModel.CreatePipelineStates();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/SingleColor"));
    if (!_shader)
        return;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<LODPreviewMaterialShader, &LODPreviewMaterialShader::OnShaderReloading>(this);
#endif
}

#if COMPILE_WITH_DEV_ENV

void LODPreviewMaterialShader::OnShaderReloading(Asset* obj)
{
    _psModel.Release();
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

bool LODPreviewMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall, };
    return true;
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
    const int32 psIndex = params.DrawCallsCount == 1 ? 0 : 1;
    auto ps = _psModel[psIndex];
    if (!ps->IsValid())
    {
        auto psDesc = GPUPipelineState::Description::Default;
        psDesc.VS = shader->GetVS("VS_Model", psIndex);
        psDesc.PS = shader->GetPS("PS_GBuffer");
        ps->Init(psDesc);
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
        ASSERT_LOW_LAYER(cb->GetSize() == sizeof(SingleColorShaderData));
        SingleColorShaderData data;
        Matrix::Transpose(params.RenderContext.View.Frustum.GetMatrix(), data.ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data.WorldMatrix);
        data.LODDitherFactor = drawCall.Surface.LODDitherFactor;
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
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
    }
    context->SetState(ps);
}

#endif
