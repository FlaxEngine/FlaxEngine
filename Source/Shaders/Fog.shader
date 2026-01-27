// Copyright (c) Wojciech Figat. All rights reserved.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/VolumetricFog.hlsl"
#include "./Flax/ExponentialHeightFog.hlsl"

// Disable Volumetric Fog if is not supported
#if VOLUMETRIC_FOG && !CAN_USE_COMPUTE_SHADER
#define VOLUMETRIC_FOG 0
#endif

META_CB_BEGIN(0, Data)
GBufferData GBuffer;
ExponentialHeightFogData ExponentialHeightFog;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Depth : register(t0);
#if VOLUMETRIC_FOG
Texture3D VolumetricFogTexture : register(t1);
#endif

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(VOLUMETRIC_FOG=0)
META_PERMUTATION_1(VOLUMETRIC_FOG=1)
float4 PS_Fog(Quad_VS2PS input) : SV_Target0
{
    // Get world space position at given pixel coordinate
	float rawDepth = SAMPLE_RT(Depth, input.TexCoord).r;
	GBufferData gBufferData = GetGBufferData();
	float3 viewPos = GetViewPos(gBufferData, input.TexCoord, rawDepth);
	float3 worldPos = mul(float4(viewPos, 1), gBufferData.InvViewMatrix).xyz;

    float skipDistance = 0;
#if VOLUMETRIC_FOG
	skipDistance = max(ExponentialHeightFog.VolumetricFogMaxDistance - 100, 0);
#endif

	// Calculate exponential fog color
	float4 fog = GetExponentialHeightFog(ExponentialHeightFog, worldPos, GBuffer.ViewPos, skipDistance, viewPos.z);

#if VOLUMETRIC_FOG
    // Sample volumetric fog and mix it in
	float4 volumetricFog = SampleVolumetricFog(VolumetricFogTexture, worldPos - GBuffer.ViewPos, ExponentialHeightFog.VolumetricFogMaxDistance, input.TexCoord);
	fog = CombineVolumetricFog(fog, volumetricFog);
#endif

	return fog;
}
