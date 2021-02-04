// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ForwardMaterialShader.h"
#include "MaterialShaderFeatures.h"
#include "MaterialParams.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/Models/SkinnedMeshDrawData.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"
#include "Engine/Renderer/DepthOfFieldPass.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/ShadowsPass.h"
#include "Engine/Renderer/RenderList.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif

#define MAX_LOCAL_LIGHTS 4

PACK_STRUCT(struct ForwardMaterialShaderData {
    Matrix ViewProjectionMatrix;
    Matrix WorldMatrix;
    Matrix ViewMatrix;
    Matrix PrevViewProjectionMatrix;
    Matrix PrevWorldMatrix;
    Vector3 ViewPos;
    float ViewFar;
    Vector3 ViewDir;
    float TimeParam;
    Vector4 ViewInfo;
    Vector4 ScreenSize;
    Vector3 WorldInvScale;
    float WorldDeterminantSign;
    Vector2 Dummy0;
    float LODDitherFactor;
    float PerInstanceRandom;
    Vector3 GeometrySize;
    float Dummy1;
    });

PACK_STRUCT(struct ForwardMaterialShaderLightingData {
    LightData DirectionalLight;
    LightShadowData DirectionalLightShadow;
    LightData SkyLight;
    ProbeData EnvironmentProbe;
    ExponentialHeightFogData ExponentialHeightFog;
    Vector3 Dummy2;
    uint32 LocalLightsCount;
    LightData LocalLights[MAX_LOCAL_LIGHTS];
    });

DrawPass ForwardMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

bool ForwardMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall, };
    return true;
}

void ForwardMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto cache = params.RenderContext.List;
    auto& drawCall = *params.FirstDrawCall;
    const auto cb0 = _shader->GetCB(0);
    const bool hasCb0 = cb0 && cb0->GetSize() != 0;
    ASSERT(hasCb0 && "TODO: fix it"); // TODO: always make cb pointer valid even if cb is missing
    const auto cb1 = _shader->GetCB(1);
    const bool hasCb1 = cb1 && cb1->GetSize() != 0;
    byte* cb = _cb0Data.Get();
    auto materialData = reinterpret_cast<ForwardMaterialShaderData*>(cb);
    cb += sizeof(ForwardMaterialShaderData);
    int32 srv = 0;

    // Setup features

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = nullptr; // forward pass materials cannot sample scene color for now
    bindMeta.Buffers = params.RenderContext.Buffers;
    bindMeta.CanSampleDepth = GPUDevice::Instance->Limits.HasReadOnlyDepth;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.ParamsLink, bindMeta);

    // Check if is using mesh skinning
    const bool useSkinning = drawCall.Surface.Skinning != nullptr;
    if (useSkinning)
    {
        // Bind skinning buffer
        ASSERT(drawCall.Surface.Skinning->IsReady());
        context->BindSR(0, drawCall.Surface.Skinning->BoneMatrices->View());
    }

    // Setup material constants data
    if (hasCb0)
    {
        Matrix::Transpose(view.Frustum.GetMatrix(), materialData->ViewProjectionMatrix);
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(view.View, materialData->ViewMatrix);
        Matrix::Transpose(drawCall.Surface.PrevWorld, materialData->PrevWorldMatrix);
        Matrix::Transpose(view.PrevViewProjection, materialData->PrevViewProjectionMatrix);

        materialData->ViewPos = view.Position;
        materialData->ViewFar = view.Far;
        materialData->ViewDir = view.Direction;
        materialData->TimeParam = Time::Draw.UnscaledTime.GetTotalSeconds();
        materialData->ViewInfo = view.ViewInfo;
        materialData->ScreenSize = view.ScreenSize;

        // Extract per axis scales from LocalToWorld transform
        const float scaleX = Vector3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13).Length();
        const float scaleY = Vector3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23).Length();
        const float scaleZ = Vector3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33).Length();
        const Vector3 worldInvScale = Vector3(
            scaleX > 0.00001f ? 1.0f / scaleX : 0.0f,
            scaleY > 0.00001f ? 1.0f / scaleY : 0.0f,
            scaleZ > 0.00001f ? 1.0f / scaleZ : 0.0f);

        materialData->WorldInvScale = worldInvScale;
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->GeometrySize = drawCall.Surface.GeometrySize;
    }

    // Setup lighting constants data
    if (hasCb1)
    {
        auto& lightingData = *reinterpret_cast<ForwardMaterialShaderLightingData*>(_cb1Data.Get());
        const int32 envProbeShaderRegisterIndex = 0;
        const int32 skyLightShaderRegisterIndex = 1;
        const int32 dirLightShaderRegisterIndex = 2;

        // Set fog input
        if (cache->Fog)
        {
            cache->Fog->GetExponentialHeightFogData(view, lightingData.ExponentialHeightFog);
        }
        else
        {
            lightingData.ExponentialHeightFog.FogMinOpacity = 1.0f;
            lightingData.ExponentialHeightFog.ApplyDirectionalInscattering = 0.0f;
        }

        // Set directional light input
        if (cache->DirectionalLights.HasItems())
        {
            const auto& dirLight = cache->DirectionalLights.First();
            const auto shadowPass = ShadowsPass::Instance();
            const bool useShadow = shadowPass->LastDirLightIndex == 0;
            if (useShadow)
            {
                lightingData.DirectionalLightShadow = shadowPass->LastDirLight;
                context->BindSR(dirLightShaderRegisterIndex, shadowPass->LastDirLightShadowMap);
            }
            else
            {
                context->UnBindSR(dirLightShaderRegisterIndex);
            }
            dirLight.SetupLightData(&lightingData.DirectionalLight, view, useShadow);
        }
        else
        {
            lightingData.DirectionalLight.Color = Vector3::Zero;
            lightingData.DirectionalLight.CastShadows = 0.0f;
            context->UnBindSR(dirLightShaderRegisterIndex);
        }

        // Set sky light
        if (cache->SkyLights.HasItems())
        {
            auto& skyLight = cache->SkyLights.First();
            skyLight.SetupLightData(&lightingData.SkyLight, view, false);
            const auto texture = skyLight.Image ? skyLight.Image->GetTexture() : nullptr;
            context->BindSR(skyLightShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));
        }
        else
        {
            Platform::MemoryClear(&lightingData.SkyLight, sizeof(lightingData.SkyLight));
            context->UnBindSR(skyLightShaderRegisterIndex);
        }

        // Set reflection probe data
        EnvironmentProbe* probe = nullptr;
        // TODO: optimize env probe searching for a transparent material - use spatial cache for renderer to find it
        for (int32 i = 0; i < cache->EnvironmentProbes.Count(); i++)
        {
            const auto p = cache->EnvironmentProbes[i];
            if (p->GetSphere().Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                probe = p;
                break;
            }
        }
        if (probe && probe->GetProbe())
        {
            probe->SetupProbeData(&lightingData.EnvironmentProbe);
            const auto texture = probe->GetProbe()->GetTexture();
            context->BindSR(envProbeShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));
        }
        else
        {
            lightingData.EnvironmentProbe.Data1 = Vector4::Zero;
            context->UnBindSR(envProbeShaderRegisterIndex);
        }

        // Set local lights
        lightingData.LocalLightsCount = 0;
        for (int32 i = 0; i < cache->PointLights.Count(); i++)
        {
            const auto& light = cache->PointLights[i];
            if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                light.SetupLightData(&lightingData.LocalLights[lightingData.LocalLightsCount], view, false);
                lightingData.LocalLightsCount++;
                if (lightingData.LocalLightsCount == MAX_LOCAL_LIGHTS)
                    break;
            }
        }
        for (int32 i = 0; i < cache->SpotLights.Count(); i++)
        {
            const auto& light = cache->SpotLights[i];
            if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
            {
                light.SetupLightData(&lightingData.LocalLights[lightingData.LocalLightsCount], view, false);
                lightingData.LocalLightsCount++;
                if (lightingData.LocalLightsCount == MAX_LOCAL_LIGHTS)
                    break;
            }
        }
    }

    // Bind constants
    if (hasCb0)
    {
        context->UpdateCB(cb0, _cb0Data.Get());
        context->BindCB(0, cb0);
    }
    if (hasCb1)
    {
        context->UpdateCB(cb1, _cb1Data.Get());
        context->BindCB(1, cb1);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = (_info.FeaturesFlags & MaterialFeaturesFlags::Wireframe) != 0 || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
#if USE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    ASSERT_LOW_LAYER(!(useSkinning && params.DrawCallsCount > 1)); // No support for instancing skinned meshes
    const auto cacheObj = params.DrawCallsCount == 1 ? &_cache : &_cacheInstanced;
    PipelineStateCache* psCache = cacheObj->GetPS(view.Pass, useSkinning);
    ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void ForwardMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
    _cacheInstanced.Release();
}

bool ForwardMaterialShader::Load()
{
    _drawModes = DrawPass::Depth | DrawPass::Forward;

    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthTestEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthTest) == 0;
    psDesc.DepthWriteEnable = (_info.FeaturesFlags & MaterialFeaturesFlags::DisableDepthWrite) == 0;

    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = _info.TessellationMode != TessellationMethod::None && GPUDevice::Instance->Limits.HasTessellation;
    if (useTess)
    {
        psDesc.HS = _shader->GetHS("HS");
        psDesc.DS = _shader->GetDS("DS");
    }

    // Check if use transparent distortion pass
    if (_shader->HasShader("PS_Distortion"))
    {
        _drawModes |= DrawPass::Distortion;

        // Accumulate Distortion Pass
        psDesc.VS = _shader->GetVS("VS");
        psDesc.PS = _shader->GetPS("PS_Distortion");
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.DepthWriteEnable = false;
        _cache.Distortion.Init(psDesc);
        //psDesc.VS = _shader->GetVS("VS", 1);
        //_cacheInstanced.Distortion.Init(psDesc);
        psDesc.VS = _shader->GetVS("VS_Skinned");
        _cache.DistortionSkinned.Init(psDesc);
    }

    // Forward Pass
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_Forward");
    psDesc.DepthWriteEnable = false;
    psDesc.BlendMode = BlendingMode::AlphaBlend;
    switch (_info.BlendMode)
    {
    case MaterialBlendMode::Transparent:
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        break;
    case MaterialBlendMode::Additive:
        psDesc.BlendMode = BlendingMode::Additive;
        break;
    case MaterialBlendMode::Multiply:
        psDesc.BlendMode = BlendingMode::Multiply;
        break;
    }
    _cache.Default.Init(psDesc);
    //psDesc.VS = _shader->GetVS("VS", 1);
    //_cacheInstanced.Default.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DefaultSkinned.Init(psDesc);

    // Depth Pass
    psDesc = GPUPipelineState::Description::Default;
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthTestEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    psDesc.VS = _shader->GetVS("VS");
    psDesc.PS = _shader->GetPS("PS_Depth");
    _cache.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS", 1);
    _cacheInstanced.Depth.Init(psDesc);
    psDesc.VS = _shader->GetVS("VS_Skinned");
    _cache.DepthSkinned.Init(psDesc);

    return false;
}
