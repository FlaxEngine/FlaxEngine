// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MaterialShaderFeatures.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ShadowsPass.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

void ForwardShadingFeature::Bind(MaterialShader::BindParameters& params, byte*& cb, int32& srv)
{
    auto context = params.GPUContext;
    auto cache = params.RenderContext.List;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    auto& data = *(Data*)cb;
    const int32 envProbeShaderRegisterIndex = srv + 0;
    const int32 skyLightShaderRegisterIndex = srv + 1;
    const int32 dirLightShaderRegisterIndex = srv + 2;

    // Set fog input
    if (cache->Fog)
    {
        cache->Fog->GetExponentialHeightFogData(view, data.ExponentialHeightFog);
    }
    else
    {
        data.ExponentialHeightFog.FogMinOpacity = 1.0f;
        data.ExponentialHeightFog.ApplyDirectionalInscattering = 0.0f;
    }

    // Set directional light input
    if (cache->DirectionalLights.HasItems())
    {
        const auto& dirLight = cache->DirectionalLights.First();
        const auto shadowPass = ShadowsPass::Instance();
        const bool useShadow = shadowPass->LastDirLightIndex == 0;
        if (useShadow)
        {
            data.DirectionalLightShadow = shadowPass->LastDirLight;
            context->BindSR(dirLightShaderRegisterIndex, shadowPass->LastDirLightShadowMap);
        }
        else
        {
            context->UnBindSR(dirLightShaderRegisterIndex);
        }
        dirLight.SetupLightData(&data.DirectionalLight, view, useShadow);
    }
    else
    {
        data.DirectionalLight.Color = Vector3::Zero;
        data.DirectionalLight.CastShadows = 0.0f;
        context->UnBindSR(dirLightShaderRegisterIndex);
    }

    // Set sky light
    if (cache->SkyLights.HasItems())
    {
        auto& skyLight = cache->SkyLights.First();
        skyLight.SetupLightData(&data.SkyLight, view, false);
        const auto texture = skyLight.Image ? skyLight.Image->GetTexture() : nullptr;
        context->BindSR(skyLightShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));
    }
    else
    {
        Platform::MemoryClear(&data.SkyLight, sizeof(data.SkyLight));
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
        probe->SetupProbeData(&data.EnvironmentProbe);
        const auto texture = probe->GetProbe()->GetTexture();
        context->BindSR(envProbeShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));
    }
    else
    {
        data.EnvironmentProbe.Data1 = Vector4::Zero;
        context->UnBindSR(envProbeShaderRegisterIndex);
    }

    // Set local lights
    data.LocalLightsCount = 0;
    for (int32 i = 0; i < cache->PointLights.Count(); i++)
    {
        const auto& light = cache->PointLights[i];
        if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
        {
            light.SetupLightData(&data.LocalLights[data.LocalLightsCount], view, false);
            data.LocalLightsCount++;
            if (data.LocalLightsCount == MaxLocalLights)
                break;
        }
    }
    for (int32 i = 0; i < cache->SpotLights.Count(); i++)
    {
        const auto& light = cache->SpotLights[i];
        if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.World.GetTranslation()) != ContainmentType::Disjoint)
        {
            light.SetupLightData(&data.LocalLights[data.LocalLightsCount], view, false);
            data.LocalLightsCount++;
            if (data.LocalLightsCount == MaxLocalLights)
                break;
        }
    }

    cb += sizeof(Data);
    srv += SRVs;
}

bool LightmapFeature::Bind(MaterialShader::BindParameters& params, byte*& cb, int32& srv)
{
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    auto& data = *(Data*)cb;

    const bool useLightmap = view.Flags & ViewFlags::GI
#if USE_EDITOR
            && EnableLightmapsUsage
#endif
            && drawCall.Surface.Lightmap != nullptr;
    if (useLightmap)
    {
        // Bind lightmap textures
        GPUTexture *lightmap0, *lightmap1, *lightmap2;
        drawCall.Features.Lightmap->GetTextures(&lightmap0, &lightmap1, &lightmap2);
        context->BindSR(srv + 0, lightmap0);
        context->BindSR(srv + 1, lightmap1);
        context->BindSR(srv + 2, lightmap2);

        // Set lightmap data
        data.LightmapArea = drawCall.Features.LightmapUVsArea;
    }

    srv += SRVs;
    cb += sizeof(Data);
    return useLightmap;
}

#if USE_EDITOR

void ForwardShadingFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/ForwardShading.hlsl");
}

void DeferredShadingFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/DeferredShading.hlsl");
}

void TessellationFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/Tessellation.hlsl");
}

void LightmapFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/Lightmap.hlsl");
}

void DistortionFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/Distortion.hlsl");
}

void MotionVectorsFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/MotionVectors.hlsl");
}

#endif
