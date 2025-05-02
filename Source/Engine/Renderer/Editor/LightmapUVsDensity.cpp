// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "LightmapUVsDensity.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Renderer/GBufferPass.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Foliage/Foliage.h"
#include "Engine/ShadowsOfMordor/Builder.Config.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Actors/StaticModel.h"

GPU_CB_STRUCT(LightmapUVsDensityMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Rectangle LightmapArea;
    Float3 WorldInvScale;
    float LightmapTexelsPerWorldUnit;
    Float3 Dummy0;
    float LightmapSize;
    });

LightmapUVsDensityMaterialShader::LightmapUVsDensityMaterialShader()
{
    _ps = GPUDevice::Instance->CreatePipelineState();
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Editor/LightmapUVsDensity"));
    if (!_shader)
        return;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<LightmapUVsDensityMaterialShader, &LightmapUVsDensityMaterialShader::OnShaderReloading>(this);
#endif
    _gridTexture = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/Tiles_M"));
}

#if COMPILE_WITH_DEV_ENV

void LightmapUVsDensityMaterialShader::OnShaderReloading(Asset* obj)
{
    _ps->ReleaseGPU();
}

#endif

const MaterialInfo& LightmapUVsDensityMaterialShader::GetInfo() const
{
    return _info;
}

GPUShader* LightmapUVsDensityMaterialShader::GetShader() const
{
    return _shader->GetShader();
}

bool LightmapUVsDensityMaterialShader::IsReady() const
{
    return _shader && _shader->IsLoaded();
}

DrawPass LightmapUVsDensityMaterialShader::GetDrawModes() const
{
    return DrawPass::GBuffer;
}

void LightmapUVsDensityMaterialShader::Bind(BindParameters& params)
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
        ASSERT(cb->GetSize() == sizeof(LightmapUVsDensityMaterialShaderData));
        LightmapUVsDensityMaterialShaderData data;
        Matrix::Transpose(params.RenderContext.View.Frustum.GetMatrix(), data.ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data.WorldMatrix);
        const float scaleX = Float3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Float3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Float3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        data.WorldInvScale = Float3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);
        data.LightmapTexelsPerWorldUnit = ShadowsOfMordor::LightmapTexelsPerWorldUnit;
        data.LightmapSize = 1024.0f;
        data.LightmapArea = drawCall.Surface.LightmapUVsArea;
        const ModelLOD* drawCallModelLod;
        float scaleInLightmap = drawCall.Surface.LODDitherFactor; // Reuse field
        if (scaleInLightmap < 0.0f)
            data.LightmapSize = -1.0f; // Not using lightmap
        else if (GBufferPass::IndexBufferToModelLOD.TryGet(drawCall.Geometry.IndexBuffer, drawCallModelLod))
        {
            // Calculate current lightmap slot size for the object (matches the ShadowsOfMordor calculations when baking the lighting)
            float globalObjectsScale = 1.0f;
            int32 atlasSize = 1024;
            int32 chartsPadding = 3;
            BoundingBox box = drawCallModelLod->GetBox(drawCall.World);
            Float3 size = box.GetSize();
            float dimensionsCoeff = size.AverageArithmetic();
            if (size.X <= 1.0f)
                dimensionsCoeff = Float2(size.Y, size.Z).AverageArithmetic();
            else if (size.Y <= 1.0f)
                dimensionsCoeff = Float2(size.X, size.Z).AverageArithmetic();
            else if (size.Z <= 1.0f)
                dimensionsCoeff = Float2(size.Y, size.X).AverageArithmetic();
            float scale = globalObjectsScale * scaleInLightmap * ShadowsOfMordor::LightmapTexelsPerWorldUnit * dimensionsCoeff;
            if (scale <= ZeroTolerance)
                scale = 0.0f;
            const int32 maximumChartSize = atlasSize - chartsPadding * 2;
            int32 width = Math::Clamp(Math::CeilToInt(scale), ShadowsOfMordor::LightmapMinChartSize, maximumChartSize);
            int32 height = Math::Clamp(Math::CeilToInt(scale), ShadowsOfMordor::LightmapMinChartSize, maximumChartSize);
            float invSize = 1.0f / (float)atlasSize;
            data.LightmapArea = Rectangle(0, 0, width * invSize, height * invSize);
            data.LightmapSize = (float)atlasSize;
        }
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
    }

    // Bind grid texture
    context->BindSR(0, _gridTexture ? _gridTexture->GetTexture() : GPUDevice::Instance->GetDefaultWhiteTexture());

    // Bind pipeline
    context->SetState(_ps);
}

#endif
