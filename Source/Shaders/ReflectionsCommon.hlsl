// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __REFLECTIONS_COMMON__
#define __REFLECTIONS_COMMON__

#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/Quaternion.hlsl"
#include "./Flax/BRDF.hlsl"

// Packed env probe data
struct EnvProbeData
{
    float4 Data0; // x - Position.x         | y - Position.y   | z - Position.z   | w - Brightness (negative for BoxProjection)
    float4 Data1; // x - Radius/BoxExtent.x | y - BoxExtent.y  | z - BoxExtent.z  | w - BlendDistance
    float4 Data2; // x - BoxInvQuat.x       | y - BoxInvQuat.y | z - BoxInvQuat.z | w - BoxInvQuat.w
};

#define EnvProbePosition(data) data.Data0.xyz
#define EnvProbeBrightness(data) abs(data.Data0.w)
#define EnvProbeBoxProjection(data) (data.Data0.w < 0.0f)
#define EnvProbeBoxExtent(data) data.Data1.xyz
#define EnvProbeBoxInvQuat(data) data.Data2
#define EnvProbeSphereRadius(data) data.Data1.x
#define EnvProbeBlendDistance(data) data.Data1.w

float GetSpecularOcclusion(float NoV, float roughnessSq, float ao)
{
    return saturate(pow(NoV + ao, roughnessSq) - 1 + ao);
}

float4 SampleReflectionProbe(float3 viewPos, TextureCube probe, EnvProbeData data, float3 positionWS, float3 normal, float roughness)
{
    // Calculate fade based on distance to the probe
    float3 captureVector = positionWS - EnvProbePosition(data);
    float distanceAlpha;
    if (EnvProbeBoxProjection(data))
    {
        // Box shape
        float3 boxExtent = EnvProbeBoxExtent(data);
        float blendDistance = EnvProbeBlendDistance(data);
        float3 pos = QuaternionRotate(EnvProbeBoxInvQuat(data), captureVector);
        float3 clampedPos = clamp(pos, -boxExtent + blendDistance, boxExtent - blendDistance);
        float distanceToBox = length(clampedPos - pos);
        distanceAlpha = saturate(1 - distanceToBox / blendDistance);
    }
    else
    {
        // Sphere shape
        float normalizedDistanceToCapture = saturate(length(captureVector) / EnvProbeSphereRadius(data));
        distanceAlpha = 1.0 - smoothstep(0.7f, 1, normalizedDistanceToCapture);
    }

    // Early out without sampling texture if out of the bounds
    BRANCH
    if (distanceAlpha <= 0.0f)
        return float4(0, 0, 0, 0);

    // Calculate probe sampling coordinates
    float3 sampleVector;
    float3 V = normalize(positionWS - viewPos);
    float3 R = reflect(V, normal);
    if (EnvProbeBoxProjection(data))
    {
        // Box projection
        float3 rotatedReflection = QuaternionRotate(EnvProbeBoxInvQuat(data), R);
        float3 boxExtent = EnvProbeBoxExtent(data);
        float3 boxMinMax = select(rotatedReflection > 0.0f, boxExtent, -boxExtent);
        float3 pos = QuaternionRotate(EnvProbeBoxInvQuat(data), captureVector);
        float3 rotatedPos = float3(boxMinMax - pos) / rotatedReflection;
        float minDir = min(min(rotatedPos.x, rotatedPos.y), rotatedPos.z);
        float3 dir = pos + rotatedReflection * minDir;
        sampleVector = QuaternionRotate(float4(-EnvProbeBoxInvQuat(data).xyz, EnvProbeBoxInvQuat(data).w), dir);
    }
    else
    {
        // Sphere projection
        sampleVector = captureVector / EnvProbeSphereRadius(data) + R;
    }

    // Sample probe at valid mip level based on surface roughness value
    float mip = ProbeMipFromRoughness(roughness);
    float4 probeSample = probe.SampleLevel(SamplerLinearClamp, sampleVector, mip);

    return probeSample * (distanceAlpha * EnvProbeBrightness(data));
}

// Calculates the reflective environment lighting to multiply the raw reflection color for the specular light (eg. from Env Probe or SSR).
float3 GetReflectionSpecularLighting(float3 viewPos, GBufferSample gBuffer)
{
    // Calculate reflection color
    float3 V = normalize(viewPos - gBuffer.WorldPos);
    float NoV = saturate(dot(gBuffer.Normal, V));
    float3 specularColor = GetSpecularColor(gBuffer);
    float3 reflections = EnvBRDFApprox(specularColor, gBuffer.Roughness, NoV);

    // Apply specular occlusion
    float roughnessSq = gBuffer.Roughness * gBuffer.Roughness;
    reflections *= GetSpecularOcclusion(NoV, roughnessSq, gBuffer.AO);

    return reflections;
}
float3 GetReflectionSpecularLighting(Texture2D preIntegratedGF, float3 viewPos, GBufferSample gBuffer)
{
    // Calculate reflection color
    float3 V = normalize(viewPos - gBuffer.WorldPos);
    float NoV = saturate(dot(gBuffer.Normal, V));
    float3 specularColor = GetSpecularColor(gBuffer);
    float3 reflections = EnvBRDF(preIntegratedGF, specularColor, gBuffer.Roughness, NoV);

    // Apply specular occlusion
    float roughnessSq = gBuffer.Roughness * gBuffer.Roughness;
    reflections *= GetSpecularOcclusion(NoV, roughnessSq, gBuffer.AO);

    return reflections;
}

#endif
