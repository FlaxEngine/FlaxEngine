// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#ifndef __REFLECTIONS_COMMON__
#define __REFLECTIONS_COMMON__

#include "./Flax/GBufferCommon.hlsl"

float GetSpecularOcclusion(float NoV, float roughnessSq, float ao)
{
	return saturate(pow(NoV + ao, roughnessSq) - 1 + ao);
}

float4 SampleReflectionProbe(float3 viewPos, TextureCube probe, ProbeData data, float3 positionWS, float3 normal, float roughness)
{
	// Calculate distance from probe to the pixel
	float3 captureVector = positionWS - data.ProbePos;
	float captureVectorLength = length(captureVector);
	
	// Check if cannot light pixel
	// TODO: maybe remove this check?? - test it out with dozens of probes
	BRANCH
	if (captureVectorLength >= data.ProbeRadius)
	{
		// End
		return 0;
	}

	// Fade out based on distance to capture
	float normalizedDistanceToCapture = saturate(captureVectorLength * data.ProbeInvRadius);
	float distanceAlpha = 1.0 - smoothstep(0.7, 1, normalizedDistanceToCapture);
	float fade = distanceAlpha * data.ProbeBrightness;

	// Calculate reflection vector
	float3 V = normalize(positionWS - viewPos);
	float3 R = reflect(V, normal);
	float3 D = data.ProbeInvRadius * captureVector + R;

	// Sample probe at valid mip level based on surface roughness value
	half mip = ProbeMipFromRoughness(roughness);
	float4 probeSample = probe.SampleLevel(SamplerLinearClamp, D, mip);

	return probeSample * fade;
}

float3 GetEnvProbeLighting(float3 viewPos, TextureCube probe, ProbeData data, GBufferSample gBuffer)
{
	// Calculate reflections
	float3 reflections = SampleReflectionProbe(viewPos, probe, data, gBuffer.WorldPos, gBuffer.Normal, gBuffer.Roughness).rgb;

	// Calculate specular color
	float3 specularColor = GetSpecularColor(gBuffer);

	// Calculate reflecion color
	float3 V = normalize(viewPos - gBuffer.WorldPos);
	float NoV = saturate(dot(gBuffer.Normal, V));
	return reflections * EnvBRDFApprox(specularColor, gBuffer.Roughness, NoV);
}

#endif
