// Copyright (c) Wojciech Figat. All rights reserved.

#include "MaterialShaderFeatures.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ShadowsPass.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#endif
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

void ForwardShadingFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto cache = params.RenderContext.List;
    auto& view = params.RenderContext.View;
    auto& drawCall = *params.DrawCall;
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));
    const int32 envProbeShaderRegisterIndex = srv + 0;
    const int32 skyLightShaderRegisterIndex = srv + 1;
    const int32 shadowsBufferRegisterIndex = srv + 2;
    const int32 shadowMapShaderRegisterIndex = srv + 3;
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
        GPUTexture* shadowMapAtlas;
        GPUBufferView* shadowsBuffer;
        ShadowsPass::GetShadowAtlas(params.RenderContext.Buffers, shadowMapAtlas, shadowsBuffer);
        const bool useShadow = shadowMapAtlas && canUseShadow && dirLight.HasShadow;
        dirLight.SetShaderData(data.DirectionalLight, useShadow);
        params.GPUContext->BindSR(shadowsBufferRegisterIndex, shadowsBuffer);
        params.GPUContext->BindSR(shadowMapShaderRegisterIndex, shadowMapAtlas);
    }
    else
    {
        Platform::MemoryClear(&data.DirectionalLight, sizeof(data.DirectionalLight));
        params.GPUContext->UnBindSR(shadowsBufferRegisterIndex);
        params.GPUContext->UnBindSR(shadowMapShaderRegisterIndex);
    }

    // Set sky light
    if (cache->SkyLights.HasItems())
    {
        auto& skyLight = cache->SkyLights.First();
        skyLight.SetShaderData(data.SkyLight, false);
        const auto texture = skyLight.Image ? skyLight.Image->GetTexture() : nullptr;
        params.GPUContext->BindSR(skyLightShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));
    }
    else
    {
        Platform::MemoryClear(&data.SkyLight, sizeof(data.SkyLight));
        params.GPUContext->UnBindSR(skyLightShaderRegisterIndex);
    }

    // Set reflection probe data
    bool noEnvProbe = true;
    // TODO: optimize env probe searching for a transparent material - use spatial cache for renderer to find it
    const BoundingSphere objectBounds(drawCall.ObjectPosition, drawCall.ObjectRadius);
    for (int32 i = 0; i < cache->EnvironmentProbes.Count(); i++)
    {
        const RenderEnvironmentProbeData& probe = cache->EnvironmentProbes.Get()[i];
        if (objectBounds.Intersects(BoundingSphere(probe.Position, probe.Radius)))
        {
            noEnvProbe = false;
            probe.SetShaderData(data.EnvironmentProbe);
            params.GPUContext->BindSR(envProbeShaderRegisterIndex, probe.Texture);
            break;
        }
    }
    if (noEnvProbe)
    {
        data.EnvironmentProbe.Data1 = Float4::Zero;
        params.GPUContext->UnBindSR(envProbeShaderRegisterIndex);
    }

    // Set local lights
    data.LocalLightsCount = 0;
    // TODO: optimize lights searching for a transparent material - use spatial cache for renderer to find it
    for (int32 i = 0; i < cache->PointLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
    {
        const auto& light = cache->PointLights[i];
        if (objectBounds.Intersects(BoundingSphere(light.Position, light.Radius)))
        {
            light.SetShaderData(data.LocalLights[data.LocalLightsCount], false);
            data.LocalLightsCount++;
        }
    }
    for (int32 i = 0; i < cache->SpotLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
    {
        const auto& light = cache->SpotLights[i];
        if (objectBounds.Intersects(BoundingSphere(light.Position, light.Radius)))
        {
            light.SetShaderData(data.LocalLights[data.LocalLightsCount], false);
            data.LocalLightsCount++;
        }
    }

    cb = cb.Slice(sizeof(Data));
    srv += SRVs;
}

bool LightmapFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto& drawCall = *params.DrawCall;

    const bool useLightmap = EnumHasAnyFlags(params.RenderContext.View.Flags, ViewFlags::GI)
#if USE_EDITOR
            && EnableLightmapsUsage
#endif
            && drawCall.Surface.Lightmap != nullptr;
    if (useLightmap)
    {
        // Bind lightmap textures
        GPUTexture *lightmap0, *lightmap1, *lightmap2;
        drawCall.Features.Lightmap->GetTextures(&lightmap0, &lightmap1, &lightmap2);
        params.GPUContext->BindSR(srv + 0, lightmap0);
        params.GPUContext->BindSR(srv + 1, lightmap1);
        params.GPUContext->BindSR(srv + 2, lightmap2);
    }
    else
    {
        // Free texture slots
        params.GPUContext->UnBindSR(srv + 0);
        params.GPUContext->UnBindSR(srv + 1);
        params.GPUContext->UnBindSR(srv + 2);
    }

    srv += SRVs;
    return useLightmap;
}

bool GlobalIlluminationFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));

    bool useGI = false;
    if (EnumHasAnyFlags(params.RenderContext.View.Flags, ViewFlags::GI))
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
                params.GPUContext->BindSR(srv + 0, bindingDataDDGI.ProbesData);
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

    cb = cb.Slice(sizeof(Data));
    srv += SRVs;
    return useGI;
}

bool SDFReflectionsFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
{
    auto& data = *(Data*)cb.Get();
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));

    bool useSDFReflections = false;
    if (EnumHasAnyFlags(params.RenderContext.View.Flags, ViewFlags::Reflections))
    {
        switch (params.RenderContext.List->Settings.ScreenSpaceReflections.TraceMode)
        {
        case ReflectionsTraceMode::SoftwareTracing:
        {
            GlobalSignDistanceFieldPass::BindingData bindingDataSDF;
            GlobalSurfaceAtlasPass::BindingData bindingDataSurfaceAtlas;
            if (!GlobalSignDistanceFieldPass::Instance()->Get(params.RenderContext.Buffers, bindingDataSDF) &&
                !GlobalSurfaceAtlasPass::Instance()->Get(params.RenderContext.Buffers, bindingDataSurfaceAtlas))
            {
                useSDFReflections = true;

                // Bind SDF and Surface Atlas data
                data.GlobalSDF = bindingDataSDF.Constants;
                data.GlobalSurfaceAtlas = bindingDataSurfaceAtlas.Constants;
                params.GPUContext->BindSR(srv + 0, bindingDataSDF.Texture ? bindingDataSDF.Texture->ViewVolume() : nullptr);
                params.GPUContext->BindSR(srv + 1, bindingDataSDF.TextureMip ? bindingDataSDF.TextureMip->ViewVolume() : nullptr);
                params.GPUContext->BindSR(srv + 2, bindingDataSurfaceAtlas.Chunks ? bindingDataSurfaceAtlas.Chunks->View() : nullptr);
                params.GPUContext->BindSR(srv + 3, bindingDataSurfaceAtlas.CulledObjects ? bindingDataSurfaceAtlas.CulledObjects->View() : nullptr);
                params.GPUContext->BindSR(srv + 4, bindingDataSurfaceAtlas.Objects ? bindingDataSurfaceAtlas.Objects->View() : nullptr);
                params.GPUContext->BindSR(srv + 5, bindingDataSurfaceAtlas.AtlasDepth->View());
                params.GPUContext->BindSR(srv + 6, bindingDataSurfaceAtlas.AtlasLighting->View());
            }
            break;
        }
        }
    }
    if (!useSDFReflections)
    {
        // Unbind SRVs to prevent issues
        data.GlobalSDF.CascadesCount = 0;
        params.GPUContext->UnBindSR(srv + 0);
        params.GPUContext->UnBindSR(srv + 1);
        params.GPUContext->UnBindSR(srv + 2);
        params.GPUContext->UnBindSR(srv + 3);
        params.GPUContext->UnBindSR(srv + 4);
        params.GPUContext->UnBindSR(srv + 5);
        params.GPUContext->UnBindSR(srv + 6);
    }

    cb = cb.Slice(sizeof(Data));
    srv += SRVs;
    return useSDFReflections;
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

void SDFReflectionsFeature::Generate(GeneratorData& data)
{
    data.Template = TEXT("Features/SDFReflections.hlsl");
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
