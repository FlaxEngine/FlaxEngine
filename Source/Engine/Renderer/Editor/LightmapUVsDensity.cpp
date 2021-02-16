// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "LightmapUVsDensity.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Foliage/Foliage.h"
#include "Engine/ShadowsOfMordor/Builder.Config.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Actors/StaticModel.h"

PACK_STRUCT(struct LightmapUVsDensityMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Rectangle LightmapArea;
    Vector3 WorldInvScale;
    float LightmapTexelsPerWorldUnit;
    Vector3 Dummy0;
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

bool LightmapUVsDensityMaterialShader::IsReady() const
{
    return _shader && _shader->IsLoaded();
}

DrawPass LightmapUVsDensityMaterialShader::GetDrawModes() const
{
    return DrawPass::GBuffer;
}

namespace
{
    Actor* FindActorByDrawCall(Actor* actor, const DrawCall& drawCall, float& scaleInLightmap)
    {
        const auto asStaticModel = ScriptingObject::Cast<StaticModel>(actor);
        if (asStaticModel && asStaticModel->GetPerInstanceRandom() == drawCall.PerInstanceRandom && asStaticModel->GetPosition() == drawCall.ObjectPosition)
        {
            scaleInLightmap = asStaticModel->GetScaleInLightmap();
            return asStaticModel;
        }
        const auto asFoliage = ScriptingObject::Cast<Foliage>(actor);
        if (asFoliage)
        {
            for (auto i = asFoliage->Instances.Begin(); i.IsNotEnd(); ++i)
            {
                auto& instance = *i;
                if (instance.Random == drawCall.PerInstanceRandom && instance.Transform.Translation == drawCall.ObjectPosition)
                {
                    scaleInLightmap = asFoliage->FoliageTypes[instance.Type].ScaleInLightmap;
                    return asFoliage;
                }
            }
        }
        for (Actor* child : actor->Children)
        {
            const auto other = FindActorByDrawCall(child, drawCall, scaleInLightmap);
            if (other)
                return other;
        }
        return nullptr;
    }
}

void LightmapUVsDensityMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& drawCall = *params.FirstDrawCall;

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

    // Find the static model that produced this draw call
    const Actor* drawCallActor = nullptr;
    float scaleInLightmap = 1.0f;
    if (params.RenderContext.Task)
    {
        if (params.RenderContext.Task->ActorsSource & ActorsSources::CustomActors)
        {
            for (auto actor : params.RenderContext.Task->CustomActors)
            {
                drawCallActor = FindActorByDrawCall(actor, drawCall, scaleInLightmap);
                if (drawCallActor)
                    break;
            }
        }
        if (!drawCallActor && params.RenderContext.Task->ActorsSource & ActorsSources::Scenes)
        {
            for (auto& scene : Level::Scenes)
            {
                drawCallActor = FindActorByDrawCall(scene, drawCall, scaleInLightmap);
                if (drawCallActor)
                    break;
            }
        }
    }

    // Find the model that produced this draw call
    const Model* drawCallModel = nullptr;
    const ModelLOD* drawCallModelLod = nullptr;
    const Mesh* drawCallMesh = nullptr;
    for (auto& e : Content::GetAssetsRaw())
    {
        auto model = ScriptingObject::Cast<Model>(e.Value);
        if (!model)
            continue;
        for (const auto& lod : model->LODs)
        {
            for (const auto& mesh : lod.Meshes)
            {
                if (mesh.GetIndexBuffer() == drawCall.Geometry.IndexBuffer)
                {
                    drawCallModel = model;
                    drawCallModelLod = &lod;
                    drawCallMesh = &mesh;
                    break;
                }
            }
        }
        if (drawCallModel)
            break;
    }

    // Bind constants
    if (cb && cb->GetSize())
    {
        ASSERT(cb->GetSize() == sizeof(LightmapUVsDensityMaterialShaderData));
        LightmapUVsDensityMaterialShaderData data;
        Matrix::Transpose(params.RenderContext.View.Frustum.GetMatrix(), data.ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, data.WorldMatrix);
        const float scaleX = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        data.WorldInvScale = Vector3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);
        data.LightmapTexelsPerWorldUnit = ShadowsOfMordor::LightmapTexelsPerWorldUnit;
        data.LightmapSize = 1024.0f;
        data.LightmapArea = drawCall.Surface.LightmapUVsArea;
        if (drawCallModel)
        {
            // Calculate current lightmap slot size for the object (matches the ShadowsOfMordor calculations when baking the lighting)
            float globalObjectsScale = 1.0f;
            int32 atlasSize = 1024;
            int32 chartsPadding = 3;
            if (drawCallActor)
            {
                const Scene* drawCallScene = drawCallActor->GetScene();
                if (drawCallScene)
                {
                    globalObjectsScale = drawCallScene->Info.LightmapSettings.GlobalObjectsScale;
                    atlasSize = (int32)drawCallScene->Info.LightmapSettings.AtlasSize;
                    chartsPadding = drawCallScene->Info.LightmapSettings.ChartsPadding;
                }
            }
            BoundingBox box = drawCallModelLod->GetBox(drawCall.World);
            Vector3 size = box.GetSize();
            float dimensionsCoeff = size.AverageArithmetic();
            if (size.X <= 1.0f)
                dimensionsCoeff = Vector2(size.Y, size.Z).AverageArithmetic();
            else if (size.Y <= 1.0f)
                dimensionsCoeff = Vector2(size.X, size.Z).AverageArithmetic();
            else if (size.Z <= 1.0f)
                dimensionsCoeff = Vector2(size.Y, size.X).AverageArithmetic();
            float scale = globalObjectsScale * scaleInLightmap * ShadowsOfMordor::LightmapTexelsPerWorldUnit * dimensionsCoeff;
            if (scale <= ZeroTolerance)
                scale = 0.0f;
            const int32 maximumChartSize = atlasSize - chartsPadding * 2;
            int32 width = Math::Clamp(Math::CeilToInt(scale), ShadowsOfMordor::LightmapMinChartSize, maximumChartSize);
            int32 height = Math::Clamp(Math::CeilToInt(scale), ShadowsOfMordor::LightmapMinChartSize, maximumChartSize);
            float invSize = 1.0f / atlasSize;
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
