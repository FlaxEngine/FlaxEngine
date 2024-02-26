// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

/// <summary>
/// Structure that contains information about GBuffer for shaders.
/// </summary>
struct GBufferData
{
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float3 ViewPos;
    float ViewFar;
    Matrix InvViewMatrix;
    Matrix InvProjectionMatrix;
};

/// <summary>
/// Structure that contains information about exponential height fog for shaders.
/// </summary>
struct ExponentialHeightFogData
{
    Float3 FogInscatteringColor;
    float FogMinOpacity;

    float FogDensity;
    float FogHeight;
    float FogHeightFalloff;
    float FogAtViewPosition;

    Float3 InscatteringLightDirection;
    float ApplyDirectionalInscattering;

    Float3 DirectionalInscatteringColor;
    float DirectionalInscatteringExponent;

    float FogCutoffDistance;
    float VolumetricFogMaxDistance;
    float DirectionalInscatteringStartDistance;
    float StartDistance;
};

/// <summary>
/// Structure that contains information about atmosphere fog for shaders.
/// </summary>
struct AtmosphericFogData
{
    float AtmosphericFogDensityScale;
    float AtmosphericFogSunDiscScale;
    float AtmosphericFogDistanceScale;
    float AtmosphericFogGroundOffset;

    float AtmosphericFogAltitudeScale;
    float AtmosphericFogStartDistance;
    float AtmosphericFogPower;
    float AtmosphericFogDistanceOffset;

    Float3 AtmosphericFogSunDirection;
    float AtmosphericFogSunPower;

    Float3 AtmosphericFogSunColor;
    float AtmosphericFogDensityOffset;
};

/// <summary>
/// Structure that contains information about light for shaders.
/// </summary>
PACK_STRUCT(struct LightData {
    Float2 SpotAngles;
    float SourceRadius;
    float SourceLength;
    Float3 Color;
    float MinRoughness;
    Float3 Position;
    float CastShadows;
    Float3 Direction;
    float Radius;
    float FalloffExponent;
    float InverseSquared;
    float Dummy0;
    float RadiusInv;
    });

/// <summary>
/// Structure that contains information about light for shaders.
/// </summary>
PACK_STRUCT(struct LightShadowData {
    Float2 ShadowMapSize;
    float Sharpness;
    float Fade;
    float NormalOffsetScale;
    float Bias;
    float FadeDistance;
    uint32 NumCascades;
    Float4 CascadeSplits;
    Matrix ShadowVP[6];
    });

/// <summary>
/// Packed env probe data
/// </summary>
PACK_STRUCT(struct ProbeData {
    Float4 Data0; // x - Position.x,  y - Position.y,  z - Position.z,  w - unused
    Float4 Data1; // x - Radius    ,  y - 1 / Radius,  z - Brightness,  w - unused
    });

// Minimum roughness value used for shading (prevent 0 roughness which causes NaNs in Vis_SmithJointApprox)
#define MIN_ROUGHNESS 0.04f

// Maximum amount of directional light cascades (using CSM technique)
#define MAX_CSM_CASCADES 4
