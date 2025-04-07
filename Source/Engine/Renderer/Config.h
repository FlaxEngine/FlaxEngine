// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Graphics/Config.h"

/// <summary>
/// Structure that contains information about GBuffer for shaders.
/// </summary>
GPU_CB_STRUCT(ShaderGBufferData {
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float3 ViewPos;
    float ViewFar;
    Matrix InvViewMatrix;
    Matrix InvProjectionMatrix;
    });

/// <summary>
/// Structure that contains information about exponential height fog for shaders.
/// </summary>
GPU_CB_STRUCT(ShaderExponentialHeightFogData {
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
    });

/// <summary>
/// Structure that contains information about atmosphere fog for shaders.
/// </summary>
GPU_CB_STRUCT(ShaderAtmosphericFogData {
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
    });

/// <summary>
/// Structure that contains information about light for shaders.
/// </summary>
GPU_CB_STRUCT(ShaderLightData {
    Float2 SpotAngles;
    float SourceRadius;
    float SourceLength;
    Float3 Color;
    float MinRoughness;
    Float3 Position;
    uint32 ShadowsBufferAddress;
    Float3 Direction;
    float Radius;
    float FalloffExponent;
    float InverseSquared;
    float RadiusInv;
    float Dummy0;
    });

/// <summary>
/// Packed env probe data
/// </summary>
GPU_CB_STRUCT(ShaderEnvProbeData {
    Float4 Data0; // x - Position.x,  y - Position.y,  z - Position.z,  w - unused
    Float4 Data1; // x - Radius    ,  y - 1 / Radius,  z - Brightness,  w - unused
    });

// Minimum roughness value used for shading (prevent 0 roughness which causes NaNs in Vis_SmithJointApprox)
#define MIN_ROUGHNESS 0.04f

// Maximum amount of directional light cascades (using CSM technique)
#define MAX_CSM_CASCADES 4
