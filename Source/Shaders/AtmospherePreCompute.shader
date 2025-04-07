// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Atmosphere.hlsl"

// Provides functions for atmospheric scattering and aerial perspective.
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

const static int TransmittanceIntegralSamples = 500;
const static int InscatterIntegralSamples = 50;
const static int IrradianceIntegralSamples = 32;
const static int IrradianceIntegralSamplesHalf = IrradianceIntegralSamples / 2;
const static int InscatterSphericalIntegralSamples = 16;
const static float IrradianceDeltaPhi = PI / float(IrradianceIntegralSamples);
const static float IrradianceDeltaTheta = PI / float(IrradianceIntegralSamples);
const static float InscatterDeltaPhi = PI / float(InscatterSphericalIntegralSamples);
const static float InscatterDeltaTheta = PI / float(InscatterSphericalIntegralSamples);

META_CB_BEGIN(0, Data)
float First;
float AtmosphereR;
int AtmosphereLayer;
float Dummy0;
float4 dhdh;
META_CB_END

Texture2D AtmosphereDeltaETexture  : register(t3);
Texture3D AtmosphereDeltaSRTexture : register(t4);
Texture3D AtmosphereDeltaSMTexture : register(t5);
Texture3D AtmosphereDeltaJTexture  : register(t6);

float GetOpticalDepth(float h, float radius, float mu) 
{
    float result = 0.0f;
    float ti = Limit(radius, mu) / float(TransmittanceIntegralSamples);
    float xi = 0.0f;
    float yi = exp(-(radius - RadiusGround) / h);
	LOOP
    for (int i = 1; i <= TransmittanceIntegralSamples; i++) 
	{
        float xj = float(i) * ti;
        float yj = exp(-(sqrt(radius * radius + xj * xj + 2.0f * xj * radius * mu) - RadiusGround) / h);
        result += (yi + yj) * 0.5f * ti;
        xi = xj;
        yi = yj;
    }
    return mu < -sqrt(1.0f - (RadiusGround / radius) * (RadiusGround / radius)) ? 1e9 : result;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Transmittance(Quad_VS2PS input) : SV_Target0
{
	float radius, mus;
	GetTransmittanceRMuS(input.TexCoord, radius, mus);
	float3 depth = BetaRayleighScattering * GetOpticalDepth(HeightScaleRayleigh, radius, mus) + BetaMieExtinction * GetOpticalDepth(HeightScaleMie, radius, mus);
	return float4(exp(-depth), 0.0f);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Irradiance1(Quad_VS2PS input) : SV_Target0
{
	float radius, mus;
	GetIrradianceRMuS(input.TexCoord, radius, mus);
	return float4(Transmittance(radius, mus) * max(mus, 0.0f), 0.0f);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_IrradianceN(Quad_VS2PS input) : SV_Target0
{
	float radius, mus;
	GetIrradianceRMuS(input.TexCoord, radius, mus);
	float3 s = float3(sqrt(max(1.0f - mus * mus, 0.0f)), 0.0f, mus);
	float3 result = float3(0, 0, 0);
	for (int iPhi = 0; iPhi < 4 * IrradianceIntegralSamplesHalf; iPhi++) 
	{
		float phi = (float(iPhi) + 0.5f) * IrradianceDeltaPhi;
		for (int iTheta = 0; iTheta < IrradianceIntegralSamplesHalf; iTheta++) 
		{
			float theta = (float(iTheta) + 0.5f) * IrradianceDeltaTheta;
			float dw = IrradianceDeltaTheta * IrradianceDeltaPhi * sin(theta);
			float3 w = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
			float nu = dot(s, w);

			if (First == 1.0f)
			{
				float pr1 = PhaseFunctionR(nu);
				float pm1 = PhaseFunctionM(nu);
				float3 ray1 = Texture4DSample(AtmosphereDeltaSRTexture, radius, w.z, mus, nu).xyz;
				float3 mie1 = Texture4DSample(AtmosphereDeltaSMTexture, radius, w.z, mus, nu).xyz;
				result += (ray1 * pr1 + mie1 * pm1) * w.z * dw;
			}
			else 
			{
				result += Texture4DSample(AtmosphereDeltaSRTexture, radius, w.z, mus, nu).xyz * w.z * dw;
			}
		}
	}
	return float4(result, 0.0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyIrradiance1(Quad_VS2PS input) : SV_Target0
{
	return AtmosphereDeltaETexture.Sample(SamplerLinearClamp, input.TexCoord);
}

void Integrand(float radius, float mu, float mus, float nu, float t, out float3 ray, out float3 mie) 
{
    ray = float3(0, 0, 0);
    mie = float3(0, 0, 0);
    float ri = sqrt(radius * radius + t * t + 2.0f * radius * mu * t);
    float musi = (nu * t + mus * radius) / ri;
	ri = max(RadiusGround, ri);
	if (musi >= -sqrt(1.0 - RadiusGround * RadiusGround / (ri * ri)) ) 
	{
		float3 ti = TransmittanceWithDistance(radius, mu, t) * Transmittance(ri, musi);
		ray = exp(-(ri - RadiusGround) / HeightScaleRayleigh) * ti;
		mie = exp(-(ri - RadiusGround) / HeightScaleMie) * ti;
	}
}

void Inscatter(float radius, float mu, float mus, float nu, out float3 ray, out float3 mie)
{
    ray = float3(0, 0, 0);
    mie = float3(0, 0, 0);
    float dx = Limit(radius, mu) / float(InscatterIntegralSamples);
    float xi = 0.0f;
    float3 rayi;
    float3 miei;
    Integrand(radius, mu, mus, nu, 0.0f, rayi, miei);
    for (int i = 1; i <= InscatterIntegralSamples; i++) 
	{
        float xj = float(i) * dx;
        float3 Rayj;
        float3 Miej;
        Integrand(radius, mu, mus, nu, xj, Rayj, Miej);
        ray += (rayi + Rayj) * 0.5f * dx;
        mie += (miei + Miej) * 0.5f * dx;
        xi = xj;
        rayi = Rayj;
        miei = Miej;
    }
    ray *= BetaRayleighScattering;
    mie *= BetaMieScattering;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Inscatter1_A(Quad_VS2PS input) : SV_Target
{
    float3 ray;
    float3 mie;
    float mu, mus, nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, dhdh, mu, mus, nu);
    Inscatter(AtmosphereR, mu, mus, nu, ray, mie);
    return float4(ray, 1);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyInscatter1(Quad_VS2PS input) : SV_Target0
{
	float3 uvw = float3(input.TexCoord, (float(AtmosphereLayer) + 0.5f) / float(AtmosphericFogInscatterAltitudeSampleNum));
    float4 ray = AtmosphereDeltaSRTexture.Sample(SamplerLinearClamp, uvw);
    float4 mie = AtmosphereDeltaSMTexture.Sample(SamplerLinearClamp, uvw);
	return float4(ray.xyz, mie.x);
}

void Inscatter(float radius, float mu, float mus, float nu, out float3 rayMie) 
{
	radius = clamp(radius, RadiusGround, RadiusAtmosphere);
	mu = clamp(mu, -1.0f, 1.0f);
	mus = clamp(mus, -1.0f, 1.0f);
	float variation = sqrt(1.0f - mu * mu) * sqrt(1.0f - mus * mus);
	nu = clamp(nu, mus * mu - variation, mus * mu + variation);
	float cThetaMin = -sqrt(1.0f - (RadiusGround / radius) * (RadiusGround / radius));
	float3 v = float3(sqrt(1.0f - mu * mu), 0.0f, mu);
	float sx = v.x == 0.0f ? 0.0f : (nu - mus * mu) / v.x;
	float3 s = float3(sx, sqrt(max(0.0f, 1.0f - sx * sx - mus * mus)), mus);
	rayMie = float3(0, 0, 0);

	for (int iTheta = 0; iTheta < InscatterSphericalIntegralSamples; iTheta++)
	{
		float theta = (float(iTheta) + 0.5f) * InscatterDeltaTheta;
		float cTheta = cos(theta);

		float ground = 0.0f;
		float3 transmittance = float3(0, 0, 0);
		float reflectance = 0.0f;
		if (cTheta < cThetaMin)
		{
			ground = -radius * cTheta - sqrt(radius * radius * (cTheta * cTheta - 1.0f) + RadiusGround * RadiusGround);
			transmittance = TransmittanceWithDistance(RadiusGround, -(radius * cTheta + ground) / RadiusGround, ground);
			reflectance = AverageGroundRelectance / PI;
		}
		
		for (int iPhi = 0; iPhi < 2 * InscatterSphericalIntegralSamples; iPhi++)
		{
			float phi = (float(iPhi) + 0.5) * InscatterDeltaPhi;
			float dw = InscatterDeltaTheta * InscatterDeltaPhi * sin(theta);
			float3 w = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cTheta);
			float nu1 = dot(s, w);
			float nu2 = dot(v, w);
			float pr2 = PhaseFunctionR(nu2);
			float pm2 = PhaseFunctionM(nu2);
			float3 normal = (float3(0.0f, 0.0f, radius) + ground * w) / RadiusGround;
			float3 irradiance = Irradiance(AtmosphereDeltaETexture, RadiusGround, dot(normal, s));
			float3 rayMie1 = reflectance * irradiance * transmittance;

			if (First == 1.0f)
			{
				float pr1 = PhaseFunctionR(nu1);
				float pm1 = PhaseFunctionM(nu1);
				float3 ray1 = Texture4DSample(AtmosphereDeltaSRTexture, radius, w.z, mus, nu1).xyz;
				float3 mie1 = Texture4DSample(AtmosphereDeltaSMTexture, radius, w.z, mus, nu1).xyz;
				rayMie1 += ray1 * pr1 + mie1 * pm1;
			}
			else
			{
				rayMie1 += Texture4DSample(AtmosphereDeltaSRTexture, radius, w.z, mus, nu1).xyz;
			}

			rayMie += rayMie1 * (BetaRayleighScattering * exp(-(radius - RadiusGround) / HeightScaleRayleigh) * pr2 + BetaMieScattering * exp(-(radius - RadiusGround) / HeightScaleMie) * pm2) * dw;
		}
	}
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_InscatterS(Quad_VS2PS input) : SV_Target0
{
	float3 rayMie;
	float mu, mus, nu;
	GetMuMuSNu(input.TexCoord, AtmosphereR, dhdh, mu, mus, nu);
	Inscatter(AtmosphereR, mu, mus, nu, rayMie);
	return float4(rayMie, 0);
}

float3 Integrand(float radius, float mu, float mus, float nu, float t) 
{
    float ri = sqrt(radius * radius + t * t + 2.0 * radius * mu * t);
    float mui = (radius * mu + t) / ri;
    float musi = (nu * t + mus * radius) / ri;
    return Texture4DSample(AtmosphereDeltaJTexture, ri, mui, musi, nu).xyz * TransmittanceWithDistance(radius, mu, t);
}

float3 Inscatter(float radius, float mu, float mus, float nu) 
{
	float3 rayMie = float3(0, 0, 0);
	float dx = Limit(radius, mu) / float(InscatterIntegralSamples);
	float xi = 0.0f;
	float3 raymiei = Integrand(radius, mu, mus, nu, 0.0f);
	for (int i = 1; i <= InscatterIntegralSamples; i++) 
	{
		float xj = float(i) * dx;
		float3 RayMiej = Integrand(radius, mu, mus, nu, xj);
		rayMie += (raymiei + RayMiej) * 0.5f * dx;
		xi = xj;
		raymiei = RayMiej;
	}
	return rayMie;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_InscatterN(Quad_VS2PS input) : SV_Target0
{
	float mu, mus, nu;
	GetMuMuSNu(input.TexCoord, AtmosphereR, dhdh, mu, mus, nu);
	return float4(Inscatter(AtmosphereR, mu, mus, nu), 0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyInscatterN(Quad_VS2PS input) : SV_Target0
{
    float mu, mus, nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, dhdh, mu, mus, nu);
	float3 uvw = float3(input.TexCoord, (float(AtmosphereLayer) + 0.5f) / float(AtmosphericFogInscatterAltitudeSampleNum));
	float4 ray = AtmosphereDeltaSRTexture.Sample(SamplerLinearClamp, uvw) / PhaseFunctionR(nu);
	return float4(ray.xyz, 0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Inscatter1_B(Quad_VS2PS input) : SV_Target
{
    float3 ray;
    float3 mie;
    float mu, mus, nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, dhdh, mu, mus, nu);
    Inscatter(AtmosphereR, mu, mus, nu, ray, mie);
    return float4(mie, 1);
}
