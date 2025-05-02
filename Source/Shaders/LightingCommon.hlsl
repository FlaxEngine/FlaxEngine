// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __LIGHTING_COMMON__
#define __LIGHTING_COMMON__

#include "./Flax/BRDF.hlsl"
#include "./Flax/GBufferCommon.hlsl"

// Disables directional lighting (no shadowing with dot(N, L), eg. for smoke particles)
#ifndef LIGHTING_NO_DIRECTIONAL
#define LIGHTING_NO_DIRECTIONAL 0
#endif

// Disables specular lighting (diffuse-only)
#ifndef LIGHTING_NO_SPECULAR
#define LIGHTING_NO_SPECULAR 0
#endif

// Structure that contains information about light
struct LightData
{
    float2 SpotAngles;
    float SourceRadius;
    float SourceLength;

    float3 Color;
    float MinRoughness;

    float3 Position;
    uint ShadowsBufferAddress;

    float3 Direction;
    float Radius;

    float FalloffExponent;
    float InverseSquared;
    float RadiusInv;
    float Dummy0;
};

// Structure that contains information about shadow sampling result
struct ShadowSample
{
    float SurfaceShadow;
    float TransmissionShadow;
};

// Structure that contains information about direct lighting calculations result
struct LightSample
{
    float3 Diffuse;
    float3 Specular;
    float3 Transmission;
};

// Calculates radial light (point or spot) attenuation factors (distance, spot and radius mask)
void GetRadialLightAttenuation(
    LightData lightData,
    bool isSpotLight,
    float3 N,
    float distanceSqr,
    float distanceBiasSqr,
    float3 toLight,
    float3 L,
    inout float NoL,
    inout float attenuation)
{
    // Distance attenuation
    if (lightData.InverseSquared)
    {
        BRANCH
        if (lightData.SourceLength > 0)
        {
            float3 l01 = lightData.Direction * lightData.SourceLength;
            float3 l0 = toLight - 0.5 * l01;
            float3 l1 = toLight + 0.5 * l01;
            float lengthL0 = length(l0);
            float lengthL1 = length(l1);
            attenuation = rcp((lengthL0 * lengthL1 + dot(l0, l1)) * 0.5 + distanceBiasSqr);
            NoL = saturate(0.5 * (dot(N, l0) / lengthL0 + dot(N, l1) / lengthL1));
        }
        else
        {
            attenuation = rcp(distanceSqr + distanceBiasSqr);
            NoL = saturate(dot(N, L));
        }
        attenuation *= Square(saturate(1 - Square(distanceSqr * Square(lightData.RadiusInv))));
    }
    else
    {
        attenuation = 1;
        NoL = saturate(dot(N, L));
        float3 worldLightVector = toLight * lightData.RadiusInv;
        float t = dot(worldLightVector, worldLightVector);
        attenuation *= pow(1.0f - saturate(t), lightData.FalloffExponent);
    }

    // Spot mask attenuation
    if (isSpotLight)
    {
        float cosOuterCone = lightData.SpotAngles.x;
        float invCosConeDifference = lightData.SpotAngles.y;
        attenuation *= Square(saturate((dot(normalize(-L), lightData.Direction) - cosOuterCone) * invCosConeDifference));
    }
}

// Find representative incoming light direction and energy modification
float AreaLightSpecular(LightData lightData, float roughness, inout float3 toLight, inout float3 L, float3 V, half3 N)
{
    float energy = 1;
    float m = roughness * roughness;
    float3 r = reflect(-V, N);
    float invDistToLight = rsqrt(dot(toLight, toLight));

    BRANCH
    if (lightData.SourceLength > 0)
    {
        float lineAngle = saturate(lightData.SourceLength * invDistToLight);
        energy *= m / saturate(m + 0.5 * lineAngle);
        float3 l01 = lightData.Direction * lightData.SourceLength;
        float3 l0 = toLight - 0.5 * l01;
        float a = Square(lightData.SourceLength);
        float b = dot(r, l01);
        float t = saturate(dot(l0, b * r - l01) / (a - b * b));
        toLight = l0 + t * l01;
    }

    BRANCH
    if (lightData.SourceRadius > 0)
    {
        float sphereAngle = saturate(lightData.SourceRadius * invDistToLight);
        energy *= Square(m / saturate(m + 0.5 * sphereAngle));
        float3 closestPointOnRay = dot(toLight, r) * r;
        float3 centerToRay = closestPointOnRay - toLight;
        float3 closestPointOnSphere = toLight + centerToRay * saturate(lightData.SourceRadius * rsqrt(dot(centerToRay, centerToRay)));
        toLight = closestPointOnSphere;
    }

    L = normalize(toLight);
    return energy;
}

#endif
