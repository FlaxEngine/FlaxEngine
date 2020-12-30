// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __LIGHTING_COMMON__
#define __LIGHTING_COMMON__

#include "./Flax/BRDF.hlsl"
#include "./Flax/GBufferCommon.hlsl"

// Structure that contains information about light
struct LightData
{
	float2 SpotAngles;
	float SourceRadius;
	float SourceLength;

	float3 Color;
	float MinRoughness;

	float3 Position;
	float CastShadows;

	float3 Direction;
	float Radius;

	float FalloffExponent;
	float InverseSquared;
	float Dummy0;
	float RadiusInv;
};

// Structure that contains information about shadow
struct ShadowData
{
	float SurfaceShadow;
	float TransmissionShadow;
};

// Structure that contains information about direct lighting calculations result
struct LightingData
{
	float3 Diffuse;
	float3 Specular;
	float3 Transmission;
};

// Gets a radial attenuation factor for a point light.  
// WorldLightVector is the vector from the position being shaded to the light, divided by the radius of the light. 
float RadialAttenuation(float3 worldLightVector, half falloffExponent)
{
	float t = dot(worldLightVector, worldLightVector);
	return pow(1.0f - saturate(t), falloffExponent); 
}

// Calculates attenuation for a spot light. Where L normalize vector to light.
float GetSpotAttenuation(LightData lightData, float3 L)
{
	// SpotAngles.x is CosOuterCone, SpotAngles.y is InvCosConeDifference
	return Square(saturate((dot(normalize(-L), lightData.Direction) - lightData.SpotAngles.x) * lightData.SpotAngles.y));
}

// Calculates radial light (point or spot) attenuation factors (distance, spot and radius mask)
void GetRadialLightAttenuation(
	LightData lightData,
	bool isSpotLight,
	float3 worldPosition,
	float3 N,
	float distanceBiasSqr,
	inout float3 toLight, 
	inout float3 L, 
	inout float NoL, 
	inout float distanceAttenuation, 
	inout float lightRadiusMask, 
	inout float spotAttenuation)
{
	toLight = lightData.Position - worldPosition;
	
	float distanceSqr = dot(toLight, toLight);
	L = toLight * rsqrt(distanceSqr);

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
			distanceAttenuation = rcp((lengthL0 * lengthL1 + dot(l0, l1)) * 0.5 + distanceBiasSqr);
			NoL = saturate(0.5 * (dot(N, l0) / lengthL0 + dot(N, l1) / lengthL1));
		}
		else
		{
			distanceAttenuation = rcp(distanceSqr + distanceBiasSqr);
			NoL = saturate(dot(N, L));
		}

		lightRadiusMask = Square(saturate(1 - Square(distanceSqr * Square(lightData.RadiusInv))));
	}
	else
	{
		distanceAttenuation = 1;
		NoL = saturate(dot(N, L));

		lightRadiusMask = RadialAttenuation(toLight * lightData.RadiusInv, lightData.FalloffExponent);	
	}

	if (isSpotLight)
	{
		spotAttenuation = GetSpotAttenuation(lightData, L);
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
