// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __BRDF__
#define __BRDF__

#include "./Flax/Math.hlsl"

// Bidirectional reflectance distribution functions
// Physically based shading model:
// Microfacet specular = D*G*F / (4*NoL*NoV) = D*Vis*F
// Vis = G / (4*NoL*NoV)

float3 Diffuse_Lambert(float3 diffuseColor)
{
	return diffuseColor * (1 / PI);
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float3 Diffuse_Burley(float3 diffuseColor, float roughness, float NoV, float NoL, float VoH)
{
	float FD90 = 0.5 + 2 * VoH * VoH * roughness;
	float FdV = 1 + (FD90 - 1) * Pow5(1 - NoV);
	float FdL = 1 + (FD90 - 1) * Pow5(1 - NoL);
	return diffuseColor * ((1 / PI) * FdV * FdL);
}

// [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
float3 Diffuse_OrenNayar(float3 diffuseColor, float roughness, float NoV, float NoL, float VoH)
{
	float a = roughness * roughness;
	float s = a;
	float s2 = s * s;
	float VoL = 2 * VoH * VoH - 1;
	float Cosri = VoL - NoV * NoL;
	float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
	float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * (Cosri >= 0 ? rcp( max(NoL, NoV)) : 1);
	return diffuseColor / PI * (C1 + C2) * (1 + roughness * 0.5);
}

float PhongShadingPow(float x, float y)
{
	return ClampedPow(x, y);
}

// [Blinn 1977, "Models of light reflection for computer synthesized pictures"]
float D_Blinn(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float n = 2 / a2 - 2;
	return (n + 2) / (2 * PI) * PhongShadingPow(NoH, n);
}

// [Beckmann 1963, "The scattering of electromagnetic waves from rough surfaces"]
float D_Beckmann(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	return exp((NoH2 - 1) / (a2 * NoH2)) / (PI * a2 * NoH2 * NoH2);
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float d = (NoH * a2 - NoH) * NoH + 1;
	return a2 / (PI * d * d);
}

// Anisotropic GGX
// [Burley 2012, "Physically-Based Shading at Disney"]
float D_GGXaniso(float roughnessX, float roughnessY, float NoH, float3 H, float3 X, float3 Y)
{
	float ax = roughnessX * roughnessX;
	float ay = roughnessY * roughnessY;
	float XoH = dot(X, H);
	float YoH = dot(Y, H);
	float d = XoH * XoH / (ax * ax) + YoH * YoH / (ay * ay) + NoH * NoH;
	return 1 / (PI * ax * ay * d * d);
}

float Vis_Implicit()
{
	return 0.25;
}

// [Neumann et al. 1999, "Compact metallic reflectance models"]
float Vis_Neumann(float NoV, float NoL)
{
	return 1 / (4 * max(NoL, NoV));
}

// [Kelemen 2001, "A microfacet based coupled specular-matte brdf model with importance sampling"]
float Vis_Kelemen(float VoH)
{
	// constant to prevent NaN
	return rcp(4 * VoH * VoH + 1e-5);
}

// Tuned to match behavior of Vis_Smith
// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float Vis_Schlick(float roughness, float NoV, float NoL)
{
	float k = Square(roughness) * 0.5;
	float vis_SchlickV = NoV * (1 - k) + k;
	float vis_SchlickL = NoL * (1 - k) + k;
	return 0.25 / (vis_SchlickV * vis_SchlickL);
}

// Smith term for GGX
// [Smith 1967, "Geometrical shadowing of a random rough surface"]
float Vis_Smith(float roughness, float NoV, float NoL)
{
	float a = Square(roughness);
	float a2 = a * a;
	float vis_SmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2);
	float vis_SmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2);
	return rcp(vis_SmithV * vis_SmithL);
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox(float roughness, float NoV, float NoL)
{
	float a = Square(roughness);
	float vis_SmithV = NoL * (NoV * (1 - a) + a);
	float vis_SmithL = NoV * (NoL * (1 - a) + a);
	return 0.5 * rcp(vis_SmithV + vis_SmithL);
}

float3 F_None(float3 specularColor)
{
	return specularColor;
}

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 F_Schlick(float3 specularColor, float VoH)
{
	float fc = Pow5(1 - VoH);
	return saturate(50.0 * specularColor.g) * fc + (1 - fc) * specularColor;
}

float3 F_Fresnel(float3 specularColor, float VoH)
{
	float3 specularColorSqrt = sqrt(clamp(float3(0, 0, 0), float3(0.99, 0.99, 0.99), specularColor));
	float3 n = (1 + specularColorSqrt ) / (1 - specularColorSqrt);
	float3 g = sqrt(n * n + VoH * VoH - 1);
	return 0.5 * Square((g - VoH) / (g + VoH)) * (1 + Square(((g + VoH) * VoH - 1) / ((g - VoH) * VoH + 1)));
}

float D_InvBlinn(float roughness, float NoH)
{
	float m = roughness * roughness;
	float m2 = m * m;
	float A = 4;
	float cos2h = NoH * NoH;
	return rcp( PI * (1 + A * m2)) * (1 + A * exp(-cos2h / m2));
}

float D_InvBeckmann(float roughness, float NoH)
{
	float m = roughness * roughness;
	float m2 = m * m;
	float A = 4;
	float cos2h = NoH * NoH;
	float sin2h = 1 - cos2h;
	float sin4h = sin2h * sin2h;
	return rcp(PI * (1 + A * m2) * sin4h) * (sin4h + A * exp(-cos2h / (m2 * sin2h)));
}

float D_InvGGX(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float A = 4;
	float d = (NoH - a2 * NoH) * NoH + a2;
	return rcp(PI * (1 + A * a2)) * (1 + 4 * a2 * a2 / (d * d));
}

float Vis_Cloth(float NoV, float NoL)
{
	return rcp(4 * (NoL + NoV - NoL * NoV));
}

#define REFLECTION_CAPTURE_NUM_MIPS 7
#define REFLECTION_CAPTURE_ROUGHEST_MIP 1
#define REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE 1.2

half ProbeMipFromRoughness(half roughness)
{
	half levelFrom1x1 = REFLECTION_CAPTURE_ROUGHEST_MIP - REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE * log2(roughness);
	return REFLECTION_CAPTURE_NUM_MIPS - 1 - levelFrom1x1;
}

half SSRMipFromRoughness(half roughness)
{
	half levelFrom1x1 = 4 - REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE * log2(roughness);
	return max(1, 10 - levelFrom1x1);
}

float ComputeReflectionCaptureRoughnessFromMip(float mip)
{
	float levelFrom1x1 = REFLECTION_CAPTURE_NUM_MIPS - 1 - mip;
	return exp2((REFLECTION_CAPTURE_ROUGHEST_MIP - levelFrom1x1) / REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE);
}

// [Lazarov 2013, "Getting More Physical in Call of Duty: Black Ops II"]
float3 EnvBRDFApprox(float3 specularColor, float roughness, float NoV)
{
	// Approximate version, base for pre integrated version
	const half4 c0 = {-1, -0.0275, -0.572, 0.022};
	const half4 c1 = {1, 0.0425, 1.04, -0.04};
	half4 r = roughness * c0 + c1;
	half a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	half2 ab = half2(-1.04, 1.04) * a004 + r.zw;
	return specularColor * ab.x + saturate(50.0 * specularColor.g) * ab.y;
}

// Pre integrated environment GF
float3 EnvBRDF(Texture2D preIntegratedGF, float3 specularColor, float roughness, float NoV)
{
	// Importance sampled preintegrated G * F
	float2 ab = preIntegratedGF.SampleLevel(SamplerLinearClamp, float2(NoV, roughness), 0).rg;
	return specularColor * ab.x + saturate(50.0 * specularColor.g) * ab.y;
}

#define MAX_SPECULAR_POWER 10000000000.0f

float RoughnessToSpecularPower(float roughness)
{
	// TODO: use path tracer as a reference and calculate valid params for this conversion
	
	return pow(2, 13 * (1 - roughness));
	
#if 0
	
	float coeff = pow(4, roughness);
	coeff = max(coeff, 2.0f / (MAX_SPECULAR_POWER + 2.0f));
	return 2.0f / coeff - 2.0f;
	
	//const float Log2Of1OnLn2_Plus1 = 1.52876637294; // log2(1 / ln(2)) + 1
	//return exp2(10 * roughness + Log2Of1OnLn2_Plus1);
	
	//return pow(2, 2 * (1 - roughness));
	return pow(2, 13 * (1 - roughness));
	
	//return exp2(10 * roughness + 1);
#endif
}
/*
float SpecularPowerToRoughness(float specularPower)
{
	return pow(-0.25f, specularPower * 0.5f + 1.0f);
}
*/
#endif
