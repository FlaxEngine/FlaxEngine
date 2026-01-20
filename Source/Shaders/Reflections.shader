// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/BRDF.hlsl"
#include "./Flax/Noise.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/ReflectionsCommon.hlsl"

// Enable/disable blurring SSR during sampling results and mixing with reflections buffer
#define SSR_MIX_BLUR (!defined(PLATFORM_ANDROID) && !defined(PLATFORM_IOS) && !defined(PLATFORM_SWITCH))

META_CB_BEGIN(0, Data)
EnvProbeData PData;
float4x4 WVP;
GBufferData GBuffer;
float2 SSRTexelSize;
float2 Dummy0;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

TextureCube Probe : register(t4);
Texture2D Reflections : register(t5);
Texture2D PreIntegratedGF : register(t6);
Texture2D SSR : register(t7);

// Vertex Shader for probe shape rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = mul(float4(input.Position.xyz, 1), WVP);
	output.ScreenPos = output.Position;
	return output;
}

// Pixel Shader for enviroment probes rendering
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_EnvProbe(Model_VS2PS input) : SV_Target0
{
	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if cannot light a pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return 0;
	}

	// Sample probe
	float4 color = SampleReflectionProbe(gBufferData.ViewPos, Probe, PData, gBuffer.WorldPos, gBuffer.Normal, gBuffer.Roughness);

    // Apply dithering to hide banding artifacts
    color.rgb += rand2dTo1d(uv) * 0.02f * Luminance(saturate(color.rgb));

    return color;
}

// Pixel Shader for reflections combine pass (additive rendering to the light buffer)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CombinePass(Quad_VS2PS input) : SV_Target0
{
	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Check if cannot light pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		return 0;
	}

	// Sample reflections buffer
	float3 reflections = SAMPLE_RT(Reflections, input.TexCoord).rgb;

    // Blend with Screen Space Reflections
    float4 ssr = SSR.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
#if SSR_MIX_BLUR
    ssr += SSR.SampleLevel(SamplerLinearClamp, input.TexCoord + float2(0, SSRTexelSize.y), 0);
    ssr += SSR.SampleLevel(SamplerLinearClamp, input.TexCoord - float2(0, SSRTexelSize.y), 0);
    ssr += SSR.SampleLevel(SamplerLinearClamp, input.TexCoord + float2(SSRTexelSize.x, 0), 0);
    ssr += SSR.SampleLevel(SamplerLinearClamp, input.TexCoord - float2(SSRTexelSize.x, 0), 0);
    ssr *= (1.0f / 5.0f);
#endif
    reflections = lerp(reflections, ssr.rgb, saturate(ssr.a));

	// Calculate specular color
	float3 specularColor = GetSpecularColor(gBuffer);

	// Calculate reflecion color
	float3 V = normalize(gBufferData.ViewPos - gBuffer.WorldPos);
	float NoV = saturate(dot(gBuffer.Normal, V));
	reflections *= EnvBRDF(PreIntegratedGF, specularColor, gBuffer.Roughness, NoV);

	// Apply specular occlusion
	float roughnessSq = gBuffer.Roughness * gBuffer.Roughness;
	float specularOcclusion = GetSpecularOcclusion(NoV, roughnessSq, gBuffer.AO);
	reflections *= specularOcclusion;

	return float4(reflections, 0);
}
