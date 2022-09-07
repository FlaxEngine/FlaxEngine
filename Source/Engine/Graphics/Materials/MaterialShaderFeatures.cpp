// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "MaterialShaderFeatures.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ShadowsPass.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

void ForwardShadingFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto context = params.GPUContext;
    auto cache = params.RenderContext.List;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));
    const int32 envProbeShaderRegisterIndex = srv + 0;
    const int32 skyLightShaderRegisterIndex = srv + 1;
    const int32 dirLightShaderRegisterIndex = srv + 2;
    const bool canUseShadow = view.Pass != DrawPass::Depth;

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
        const bool useShadow = shadowPass->LastDirLightIndex == 0 && canUseShadow;
        if (useShadow)
        {
            data.DirectionalLightShadow = shadowPass->LastDirLight;
            context->BindSR(dirLightShaderRegisterIndex, shadowPass->LastDirLightShadowMap);
        }
        else
        {
            context->UnBindSR(dirLightShaderRegisterIndex);
        }
        dirLight.SetupLightData(&data.DirectionalLight, useShadow);
    }
    else
    {
        data.DirectionalLight.Color = Float3::Zero;
        data.DirectionalLight.CastShadows = 0.0f;
        context->UnBindSR(dirLightShaderRegisterIndex);
    }

    // Set sky light
    if (cache->SkyLights.HasItems())
    {
        auto& skyLight = cache->SkyLights.First();
        skyLight.SetupLightData(&data.SkyLight, false);
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
    const Vector3 drawCallOrigin = drawCall.ObjectPosition + view.Origin;
    for (int32 i = 0; i < cache->EnvironmentProbes.Count(); i++)
    {
        const auto p = cache->EnvironmentProbes[i];
        if (p->GetSphere().Contains(drawCallOrigin) != ContainmentType::Disjoint)
        {
            probe = p;
            break;
        }
    }
    if (probe && probe->GetProbe())
    {
        probe->SetupProbeData(params.RenderContext, &data.EnvironmentProbe);
        context->BindSR(envProbeShaderRegisterIndex, probe->GetProbe());
    }
    else
    {
        data.EnvironmentProbe.Data1 = Float4::Zero;
        context->UnBindSR(envProbeShaderRegisterIndex);
    }

    // Set local lights
    data.LocalLightsCount = 0;
    for (int32 i = 0; i < cache->PointLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
    {
        const auto& light = cache->PointLights[i];
        if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.ObjectPosition) != ContainmentType::Disjoint)
        {
            light.SetupLightData(&data.LocalLights[data.LocalLightsCount], false);
            data.LocalLightsCount++;
        }
    }
    for (int32 i = 0; i < cache->SpotLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
    {
        const auto& light = cache->SpotLights[i];
        if (BoundingSphere(light.Position, light.Radius).Contains(drawCall.ObjectPosition) != ContainmentType::Disjoint)
        {
            light.SetupLightData(&data.LocalLights[data.LocalLightsCount], false);
            data.LocalLightsCount++;
        }
    }

    cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
    srv += SRVs;
}

bool LightmapFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto context = params.GPUContext;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.FirstDrawCall;
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));

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

    cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
    srv += SRVs;
    return useLightmap;
}

bool GlobalIlluminationFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));

    bool useGI = false;
    if (params.RenderContext.View.Flags & ViewFlags::GI)
    {
        switch (params.RenderContext.List->Settings.GlobalIllumination.Mode)
        {
        case GlobalIlluminationMode::DDGI:
        {
            DynamicDiffuseGlobalIlluminationPass::BindingData bindingDataDDGI;
            if (!DynamicDiffuseGlobalIlluminationPass::Instance()->Get(params.RenderContext.Buffers, bindingDataDDGI))
            {
                useGI = true;

                // Bind DDGI data
                data.DDGI = bindingDataDDGI.Constants;
                params.GPUContext->BindSR(srv + 0, bindingDataDDGI.ProbesState);
                params.GPUContext->BindSR(srv + 1, bindingDataDDGI.ProbesDistance);
                params.GPUContext->BindSR(srv + 2, bindingDataDDGI.ProbesIrradiance);
            }
            break;
        }
        }
    }
    if (!useGI)
    {
        // Unbind SRVs to prevent issues
        data.DDGI.CascadesCount = 0;
        data.DDGI.FallbackIrradiance = Float3::Zero;
        params.GPUContext->UnBindSR(srv + 0);
        params.GPUContext->UnBindSR(srv + 1);
        params.GPUContext->UnBindSR(srv + 2);
    }

    cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
    srv += SRVs;
    return useGI;
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

void GlobalIlluminationFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/GlobalIllumination.hlsl");
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
