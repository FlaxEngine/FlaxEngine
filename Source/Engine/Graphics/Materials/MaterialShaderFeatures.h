// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Renderer/GI/DynamicDiffuseGlobalIllumination.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"
#include "Engine/Renderer/GI/GlobalSurfaceAtlasPass.h"

// Material shader features are plugin-based functionalities that are reusable between different material domains.
struct MaterialShaderFeature
{
#if USE_EDITOR
    struct GeneratorData
    {
        const Char* Template;
    };
#endif
};

// Material shader feature that add support for Forward shading inside the material shader.
struct ForwardShadingFeature : MaterialShaderFeature
{
    enum { MaxLocalLights = 4 };

    enum { SRVs = 4 };

    PACK_STRUCT(struct Data
        {
        ShaderLightData DirectionalLight;
        ShaderLightData SkyLight;
        ShaderEnvProbeData EnvironmentProbe;
        ShaderExponentialHeightFogData ExponentialHeightFog;
        Float3 Dummy2;
        uint32 LocalLightsCount;
        ShaderLightData LocalLights[MaxLocalLights];
        });

    static void Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that add support for Deferred shading inside the material shader.
struct DeferredShadingFeature : MaterialShaderFeature
{
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds geometry hardware tessellation (using Hull and Domain shaders).
struct TessellationFeature : MaterialShaderFeature
{
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds lightmap sampling feature.
struct LightmapFeature : MaterialShaderFeature
{
    enum { SRVs = 3 };

    static bool Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds Global Illumination sampling feature (light probes).
struct GlobalIlluminationFeature : MaterialShaderFeature
{
    enum { SRVs = 3 };

    PACK_STRUCT(struct Data
        {
        DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
        });

    static bool Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds SDF Reflections feature (software reflections). 
struct SDFReflectionsFeature : MaterialShaderFeature
{
    enum { SRVs = 7 };

    PACK_STRUCT(struct Data
    {
        GlobalSignDistanceFieldPass::ConstantsData GlobalSDF;
        GlobalSurfaceAtlasPass::ConstantsData GlobalSurfaceAtlas;
    });

    

    static bool Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds distortion vectors rendering pass.
struct DistortionFeature : MaterialShaderFeature
{
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};

// Material shader feature that adds motion vectors rendering pass.
struct MotionVectorsFeature : MaterialShaderFeature
{
#if USE_EDITOR
    static void Generate(GeneratorData& data);
#endif
};
