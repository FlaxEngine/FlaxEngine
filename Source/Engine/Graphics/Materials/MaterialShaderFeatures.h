// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"
#include "Engine/Core/Math/Rectangle.h"

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

    enum { SRVs = 3 };

    PACK_STRUCT(struct Data
        {
        LightData DirectionalLight;
        LightShadowData DirectionalLightShadow;
        LightData SkyLight;
        ProbeData EnvironmentProbe;
        ExponentialHeightFogData ExponentialHeightFog;
        Vector3 Dummy2;
        uint32 LocalLightsCount;
        LightData LocalLights[MaxLocalLights];
        });

    static void Bind(MaterialShader::BindParameters& params, byte*& cb, int32& srv);
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

    PACK_STRUCT(struct Data
        {
        Rectangle LightmapArea;
        });

    static bool Bind(MaterialShader::BindParameters& params, byte*& cb, int32& srv);
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
