// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __EXPONENTIAL_HEIGHT_FOG__
#define __EXPONENTIAL_HEIGHT_FOG__

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"

// Structure that contains information about exponential height fog
struct ExponentialHeightFogData
{
	float3 FogInscatteringColor;
	float FogMinOpacity;

	float FogDensity;
	float FogHeight;
	float FogHeightFalloff;
	float FogAtViewPosition;

	float3 InscatteringLightDirection;
	float ApplyDirectionalInscattering;

	float3 DirectionalInscatteringColor;
	float DirectionalInscatteringExponent;

	float FogCutoffDistance;
	float VolumetricFogMaxDistance;
	float DirectionalInscatteringStartDistance;
	float StartDistance;
};

float4 GetExponentialHeightFog(ExponentialHeightFogData exponentialHeightFog, float3 posWS, float3 camWS, float skipDistance)
{
	float3 cameraToPos = posWS - camWS;
	float cameraToPosSqr = dot(cameraToPos, cameraToPos);
	float cameraToPosLenInv = rsqrt(cameraToPosSqr);
	float cameraToPosLen = cameraToPosSqr * cameraToPosLenInv;
	float3 cameraToReceiverNorm = cameraToPos * cameraToPosLenInv;

	float rayOriginTerms = exponentialHeightFog.FogAtViewPosition;	
	float rayLength = cameraToPosLen;
	float rayDirectionY = cameraToPos.y;

	// Apply start distance offset
	skipDistance = max(skipDistance, exponentialHeightFog.StartDistance);
	if (skipDistance > 0)
	{
		float excludeIntersectionTime = skipDistance * cameraToPosLenInv;
		float cameraToExclusionIntersectionY = excludeIntersectionTime * cameraToPos.y;
		float exclusionIntersectionY = camWS.y + cameraToExclusionIntersectionY;
		float exclusionIntersectionToReceiverY = cameraToPos.y - cameraToExclusionIntersectionY;

		// Calculate fog off of the ray starting from the exclusion distance, instead of starting from the camera
		rayLength = (1.0f - excludeIntersectionTime) * cameraToPosLen;
		rayDirectionY = exclusionIntersectionToReceiverY;

		// Move off the viewer
		float exponent = exponentialHeightFog.FogHeightFalloff * (exclusionIntersectionY - exponentialHeightFog.FogHeight);
		rayOriginTerms = exponentialHeightFog.FogDensity * exp2(-exponent);
	}

	// Calculate the line integral of the ray from the camera to the receiver position through the fog density function
	float falloff = max(-127.0f, exponentialHeightFog.FogHeightFalloff * rayDirectionY);
	float lineIntegral = (1.0f - exp2(-falloff)) / falloff;
	float lineIntegralTaylor = log(2.0) - (0.5 * Pow2(log(2.0))) * falloff;
	float exponentialHeightLineIntegralCalc = rayOriginTerms * (abs(falloff) > 0.01f ? lineIntegral : lineIntegralTaylor);
	float exponentialHeightLineIntegral = exponentialHeightLineIntegralCalc * rayLength;

	// Calculate the amount of light that made it through the fog using the transmission equation
	float expFogFactor = max(saturate(exp2(-exponentialHeightLineIntegral)), exponentialHeightFog.FogMinOpacity);

	// Calculate the directional light inscattering
	float3 inscatteringColor = exponentialHeightFog.FogInscatteringColor;
	float3 directionalInscattering = 0;
	BRANCH
	if (exponentialHeightFog.ApplyDirectionalInscattering > 0)
	{
		// Setup a cosine lobe around the light direction to approximate inscattering from the directional light off of the ambient haze
		float3 directionalLightInscattering = exponentialHeightFog.DirectionalInscatteringColor * pow(saturate(dot(cameraToReceiverNorm, exponentialHeightFog.InscatteringLightDirection)), exponentialHeightFog.DirectionalInscatteringExponent);
		
		// Calculate the line integral of the eye ray through the haze, using a special starting distance to limit the inscattering to the distance
		float dirExponentialHeightLineIntegral = exponentialHeightLineIntegralCalc * max(rayLength - exponentialHeightFog.DirectionalInscatteringStartDistance, 0.0f);
		
		// Calculate the amount of light that made it through the fog using the transmission equation
		float directionalInscatteringFogFactor = saturate(exp2(-dirExponentialHeightLineIntegral));
		
		// Final inscattering from the light
		directionalInscattering = directionalLightInscattering * (1 - directionalInscatteringFogFactor);
	}

	// Disable fog after a certain distance
	FLATTEN
	if (exponentialHeightFog.FogCutoffDistance > 0 && cameraToPosLen > exponentialHeightFog.FogCutoffDistance)
	{
		expFogFactor = 1;
		directionalInscattering = 0;
	}

	return float4(inscatteringColor * (1.0f - expFogFactor) + directionalInscattering, expFogFactor);
}

#endif
