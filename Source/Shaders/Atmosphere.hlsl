// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __ATMOSPHERE__
#define __ATMOSPHERE__

/// Provides functions for atmospheric scattering and aerial perspective.
//
// Explanations:
// Scale Height = the altitude (height above ground) at which the average
//                atmospheric density is found.
// Optical Depth = also called optical length, airmass, etc.
//
// References:
// [GPUGems2] GPU Gems 2: Accurate Atmospheric Scattering by Sean O'Neil.
// [GPUPro3]  An Approximation to the Chapman Grazing-Incidence Function for
//            Atmospheric Scattering, GPU Pro3, pp. 105.
// Papers bei Bruneton, Nishita, etc.
//
// This code contains embedded portions of free sample source code from 
// http://www-evasion.imag.fr/Membres/Eric.Bruneton/PrecomputedAtmosphericScattering2.zip, Author: Eric Bruneton, 
// 08/16/2011, Copyright (c) 2008 INRIA, All Rights Reserved, which have been altered from their original version.

//-----------------------------------------------------------------------------

#include "./Flax/Common.hlsl"

// Magic numbers are based on paper (http://hal.inria.fr/docs/00/28/87/58/PDF/article.pdf)

// Simple approximation ground reflection light
const static float AverageGroundRelectance = 0.1f;

// Rayleigh scattering terms
const static float HeightScaleRayleigh = 8.0;
const static float3 BetaRayleighScattering = float3(5.8e-3, 1.35e-2, 3.31e-2); // Equation 1, REK 04

// Mie scattering terms
const static float HeightScaleMie = 1.2f; // 1.2 km, for Mie scattering
const static float3 BetaMieScattering = float3(4e-3f, 4e-3f, 4e-3f); // Equation 4
const static float BetaRatio = 0.9f; // Figure 6, BetaMScattering/BetaMExtinction = 0.9
const static float3 BetaMieExtinction = BetaMieScattering / BetaRatio.rrr;
const static float MieG = 0.8f; // Equation 4

const static float RadiusScale = 1;
const static float RadiusGround = 6360 * RadiusScale;
const static float RadiusAtmosphere = 6420 * RadiusScale;
const static float RadiusLimit = 6421 * RadiusScale;
#define Rg RadiusGround
#define Rt RadiusAtmosphere
#define RL RadiusLimit

const static int IrradianceTexWidth = 64;
const static int IrradianceTexHeight = 16;

const static int InscatterMuNum = 128;
const static int InscatterMuSNum = 32;
const static int InscatterNuNum = 8;
const static int AtmosphericFogInscatterAltitudeSampleNum = 4;

// Configuration
#define TRANSMITTANCE_NON_LINEAR 1
#define INSCATTER_NON_LINEAR 1

#ifndef ATMOSPHERIC_NO_SUN_DISK
#define	ATMOSPHERIC_NO_SUN_DISK				0
#endif

#ifndef ATMOSPHERIC_NO_GROUND_SCATTERING
#define	ATMOSPHERIC_NO_GROUND_SCATTERING	0
#endif

#ifndef ATMOSPHERIC_NO_DECAY
#define	ATMOSPHERIC_NO_DECAY				0
#endif

// Textures for rendering atmosphere
#ifdef __GBUFFER__ // Prevent from overlapping registers ( 4 * GBuffer + Depth = 5 registers )
Texture2D AtmosphereTransmittanceTexture : register(t4);
Texture2D AtmosphereIrradianceTexture    : register(t5);
Texture3D AtmosphereInscatterTexture     : register(t6);
#elif MATERIAL // For materials start from the 4th slot (3 lightmaps)
Texture2D AtmosphereTransmittanceTexture : register(t3);
Texture2D AtmosphereIrradianceTexture    : register(t4);
Texture3D AtmosphereInscatterTexture     : register(t5);
#else
Texture2D AtmosphereTransmittanceTexture : register(t0);
Texture2D AtmosphereIrradianceTexture : register(t1);
Texture3D AtmosphereInscatterTexture : register(t2);
#endif

float2 GetTransmittanceUV(float radius, float Mu)
{
    float u, v;
#if TRANSMITTANCE_NON_LINEAR
    v = sqrt((radius - RadiusGround) / (RadiusAtmosphere - RadiusGround));
    u = atan((Mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
#else
	v = (radius - RadiusGround) / (RadiusAtmosphere - RadiusGround);
	u = (Mu + 0.15) / (1.0 + 0.15);
#endif
    return float2(u, v);
}

void GetTransmittanceRMuS(float2 uv, out float radius, out float MuS)
{
    radius = uv.y;
    MuS = uv.x;
#if TRANSMITTANCE_NON_LINEAR
    radius = RadiusGround + (radius * radius) * (RadiusAtmosphere - RadiusGround);
    MuS = -0.15 + tan(1.5 * MuS) / tan(1.5) * (1.0 + 0.15);
#else
    r = RadiusGround + r * (RadiusAtmosphere - RadiusGround);
    muS = -0.15 + muS * (1.0 + 0.15);
#endif
}

float2 GetIrradianceUV(float radius, float MuS)
{
    float v = (radius - RadiusGround) / (RadiusAtmosphere - RadiusGround);
    float u = (MuS + 0.2) / (1.0 + 0.2);
    return float2(u, v);
}

void GetIrradianceRMuS(float2 uv, out float radius, out float MuS)
{
    radius = RadiusGround + (uv.y * float(IrradianceTexHeight) - 0.5) / (float(IrradianceTexHeight) - 1.0) * (RadiusAtmosphere - RadiusGround);
    MuS = -0.2 + (uv.x * float(IrradianceTexWidth) - 0.5) / (float(IrradianceTexWidth) - 1.0) * (1.0 + 0.2);
}

float4 Texture4DSample(Texture3D tex, float radius, float Mu, float MuS, float Nu)
{
    float H = sqrt(RadiusAtmosphere * RadiusAtmosphere - RadiusGround * RadiusGround);
    float Rho = sqrt(radius * radius - RadiusGround * RadiusGround);
#if INSCATTER_NON_LINEAR
    float RMu = radius * Mu;
    float Delta = RMu * RMu - radius * radius + RadiusGround * RadiusGround;
    float4 TexOffset = RMu < 0.0 && Delta > 0.0 ? float4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(InscatterMuNum)) : float4(-1.0, H * H, H, 0.5 + 0.5 / float(InscatterMuNum));
    float MuR = 0.5 / float(AtmosphericFogInscatterAltitudeSampleNum) + Rho / H * (1.0 - 1.0 / float(AtmosphericFogInscatterAltitudeSampleNum));
    float MuMu = TexOffset.w + (RMu * TexOffset.x + sqrt(Delta + TexOffset.y)) / (Rho + TexOffset.z) * (0.5 - 1.0 / float(InscatterMuNum));
    float MuMuS = 0.5 / float(InscatterMuSNum) + (atan(max(MuS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(InscatterMuSNum));
#else
	float MuR = 0.5 / float(AtmosphericFogInscatterAltitudeSampleNum) + Rho / H * (1.0 - 1.0 / float(AtmosphericFogInscatterAltitudeSampleNum));
	float MuMu = 0.5 / float(InscatterMuNum) + (Mu + 1.0) * 0.5f * (1.0 - 1.0 / float(InscatterMuNum));
	float MuMuS = 0.5 / float(InscatterMuSNum) + max(MuS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(InscatterMuSNum));
#endif
    float LerpValue = (Nu + 1.0) * 0.5f * (float(InscatterNuNum) - 1.0);
    float MuNu = floor(LerpValue);
    LerpValue = LerpValue - MuNu;

    return tex.SampleLevel(SamplerLinearClamp, float3((MuNu + MuMuS) / float(InscatterNuNum), MuMu, MuR), 0) * (1.0 - LerpValue)
            + tex.SampleLevel(SamplerLinearClamp, float3((MuNu + MuMuS + 1.0) / float(InscatterNuNum), MuMu, MuR), 0) * LerpValue;
}

float Mod(float x, float y)
{
    return x - y * floor(x / y);
}

void GetMuMuSNu(float2 uv, float radius, float4 DhdH, out float Mu, out float MuS, out float Nu)
{
    float x = uv.x * float(InscatterMuSNum * InscatterNuNum) - 0.5;
    float y = uv.y * float(InscatterMuNum) - 0.5;
#if INSCATTER_NON_LINEAR
    if (y < float(InscatterMuNum) * 0.5f)
    {
        float d = 1.0 - y / (float(InscatterMuNum) * 0.5f - 1.0);
        d = min(max(DhdH.z, d * DhdH.w), DhdH.w * 0.999);
        Mu = (RadiusGround * RadiusGround - radius * radius - d * d) / (2.0 * radius * d);
        Mu = min(Mu, -sqrt(1.0 - (RadiusGround / radius) * (RadiusGround / radius)) - 0.001);
    }
    else
    {
        float d = (y - float(InscatterMuNum) * 0.5f) / (float(InscatterMuNum) * 0.5f - 1.0);
        d = min(max(DhdH.x, d * DhdH.y), DhdH.y * 0.999);
        Mu = (RadiusAtmosphere * RadiusAtmosphere - radius * radius - d * d) / (2.0 * radius * d);
    }
    MuS = Mod(x, float(InscatterMuSNum)) / (float(InscatterMuSNum) - 1.0);
    MuS = tan((2.0 * MuS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    Nu = -1.0 + floor(x / float(InscatterMuSNum)) / (float(InscatterNuNum) - 1.0) * 2.0;
#else
    Mu = -1.0 + 2.0 * Y / (float(InscatterMuNum) - 1.0);
    MuS = Mod(X, float(InscatterMuSNum)) / (float(InscatterMuSNum) - 1.0);
    MuS = -0.2 + MuS * 1.2;
    Nu = -1.0 + floor(X / float(InscatterMuSNum)) / (float(InscatterNuNum) - 1.0) * 2.0;
#endif
}

// Nearest intersection of ray r,mu with ground or top atmosphere boundary, mu=cos(ray zenith angle at ray origin)
float Limit(float radius, float Mu)
{
    float Dout = -radius * Mu + sqrt(radius * radius * (Mu * Mu - 1.0) + RadiusLimit * RadiusLimit);
    float Delta2 = radius * radius * (Mu * Mu - 1.0) + RadiusGround * RadiusGround;
    if (Delta2 >= 0.0)
    {
        float Din = -radius * Mu - sqrt(Delta2);
        if (Din >= 0.0)
        {
            Dout = min(Dout, Din);
        }
    }
    return Dout;
}

// Transmittance(=transparency) of atmosphere for infinite ray (r,mu) (mu=cos(view zenith angle)), intersections with ground ignored
float3 Transmittance(float radius, float Mu)
{
    float2 uv = GetTransmittanceUV(radius, Mu);
    return AtmosphereTransmittanceTexture.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
}

// Transmittance(=transparency) of atmosphere for infinite ray (r,mu) (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 TransmittanceWithShadow(float radius, float Mu)
{
    return Transmittance(radius, Mu);
}

//Transmittance(=transparency) of atmosphere between x and x0. Assume segment x,x0 not intersecting ground. D = Distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
float3 TransmittanceWithDistance(float radius, float Mu, float D)
{
    float3 result;
    float R1 = sqrt(radius * radius + D * D + 2.0 * radius * Mu * D);
    float Mu1 = (radius * Mu + D) / R1;
    if (Mu > 0.0)
        result = min(Transmittance(radius, Mu) / Transmittance(R1, Mu1), 1.0);
    else
        result = min(Transmittance(R1, -Mu1) / Transmittance(radius, -Mu), 1.0);
    return result;
}

// Transmittance(=transparency) of atmosphere between x and x0. Assume segment x,x0 not intersecting ground radius=||x||, Mu=cos(zenith angle of [x,x0) ray at x), v=unit direction vector of [x,x0) ray
float3 TransmittanceWithDistance(float radius, float Mu, float3 V, float3 X0)
{
    float3 result;
    float d1 = length(X0);
    float Mu1 = dot(X0, V) / radius;
    if (Mu > 0.0)
        result = min(Transmittance(radius, Mu) / Transmittance(d1, Mu1), 1.0);
    else
        result = min(Transmittance(d1, -Mu1) / Transmittance(radius, -Mu), 1.0);
    return result;
}

// Optical depth for ray (r,mu) of length d, using analytic formula (mu=cos(view zenith angle)), intersections with ground ignored H=height scale of exponential density function
float OpticalDepthWithDistance(float H, float radius, float Mu, float D)
{
    float particleDensity = 6.2831; // REK 04, Table 2
    float a = sqrt(0.5 / H * radius);
    float2 A01 = a * float2(Mu, Mu + D / radius);
    float2 A01Sign = (float2)sign(A01);
    float2 A01Squared = A01 * A01;
    float x = A01Sign.y > A01Sign.x ? exp(A01Squared.x) : 0.0;
    float2 y = A01Sign / (2.3193 * abs(A01) + sqrt(1.52 * A01Squared + 4.0)) * float2(1.0, exp(-D / H * (D / (2.0 * radius) + Mu)));
    return sqrt((particleDensity * H) * radius) * exp((RadiusGround - radius) / H) * (x + dot(y, float2(1.0, -1.0)));
}

// Transmittance(=transparency) of atmosphere for ray (r,mu) of length d (mu=cos(view zenith angle)), intersections with ground ignored uses analytic formula instead of transmittance texture, REK 04, Atmospheric Transparency
float3 AnalyticTransmittance(float R, float Mu, float D)
{
    return exp(- BetaRayleighScattering * OpticalDepthWithDistance(HeightScaleRayleigh, R, Mu, D) - BetaMieExtinction * OpticalDepthWithDistance(HeightScaleMie, R, Mu, D));
}

float3 Irradiance(Texture2D tex, float r, float muS)
{
    float2 uv = GetIrradianceUV(r, muS);
    return tex.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
}

// Rayleigh phase function
float PhaseFunctionR(float Mu)
{
    return (3.0 / (16.0 * PI)) * (1.0 + Mu * Mu);
}

// Mie phase function
float PhaseFunctionM(float Mu)
{
    return 1.5 * 1.0 / (4.0 * PI) * (1.0 - MieG * MieG) * pow(1.0 + (MieG * MieG) - 2.0 * MieG * Mu, -3.0 / 2.0) * (1.0 + Mu * Mu) / (2.0 + MieG * MieG);
}

// Approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
float3 GetMie(float4 RayMie)
{
    // RayMie.rgb=C*, RayMie.w=Cm,r
    return RayMie.rgb * RayMie.w / max(RayMie.r, 1e-4) * (BetaRayleighScattering.rrr / BetaRayleighScattering.rgb);
}

#endif
