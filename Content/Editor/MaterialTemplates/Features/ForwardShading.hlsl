// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

@0// Forward Shading: Defines
#define MAX_LOCAL_LIGHTS 4
@1// Forward Shading: Includes
#include "./Flax/LightingCommon.hlsl"
#if USE_REFLECTIONS
#include "./Flax/ReflectionsCommon.hlsl"
#endif
#include "./Flax/Lighting.hlsl"
#include "./Flax/ShadowsSampling.hlsl"
#include "./Flax/ExponentialHeightFog.hlsl"
@2// Forward Shading: Constants
LightData DirectionalLight;
LightShadowData DirectionalLightShadow;
LightData SkyLight;
ProbeData EnvironmentProbe;
ExponentialHeightFogData ExponentialHeightFog;
float3 Dummy2;
uint LocalLightsCount;
LightData LocalLights[MAX_LOCAL_LIGHTS];
@3// Forward Shading: Resources
TextureCube EnvProbe : register(t__SRV__);
TextureCube SkyLightTexture : register(t__SRV__);
Texture2DArray DirectionalLightShadowMap : register(t__SRV__);
@4// Forward Shading: Utilities
DECLARE_LIGHTSHADOWDATA_ACCESS(DirectionalLightShadow);
@5// Forward Shading: Shaders

// Pixel Shader function for Forward Pass
META_PS(USE_FORWARD, FEATURE_LEVEL_ES2)
float4 PS_Forward(PixelInput input) : SV_Target0
{
	float4 output = 0;

#if USE_DITHERED_LOD_TRANSITION
	// LOD masking
	ClipLODTransition(input);
#endif

	// Get material parameters
	MaterialInput materialInput = GetMaterialInput(input);
	Material material = GetMaterialPS(materialInput);

	// Masking
#if MATERIAL_MASKED
	clip(material.Mask - MATERIAL_MASK_THRESHOLD);
#endif

	// Add emissive light
	output = float4(material.Emissive, material.Opacity);

#if MATERIAL_SHADING_MODEL != SHADING_MODEL_UNLIT

	// Setup GBuffer data as proxy for lighting
	GBufferSample gBuffer;
	gBuffer.Normal = material.WorldNormal;
	gBuffer.Roughness = material.Roughness;
	gBuffer.Metalness = material.Metalness;
	gBuffer.Color = material.Color;
	gBuffer.Specular = material.Specular;
	gBuffer.AO = material.AO;
	gBuffer.ViewPos = mul(float4(materialInput.WorldPosition, 1), ViewMatrix).xyz;
#if MATERIAL_SHADING_MODEL == SHADING_MODEL_SUBSURFACE
	gBuffer.CustomData = float4(material.SubsurfaceColor, material.Opacity);
#elif MATERIAL_SHADING_MODEL == SHADING_MODEL_FOLIAGE
	gBuffer.CustomData = float4(material.SubsurfaceColor, material.Opacity);
#else
	gBuffer.CustomData = float4(0, 0, 0, 0);
#endif
	gBuffer.WorldPos = materialInput.WorldPosition;
	gBuffer.ShadingModel = MATERIAL_SHADING_MODEL;

	// Calculate lighting from a single directional light
	float4 shadowMask = 1.0f;
	if (DirectionalLight.CastShadows > 0)
	{
		LightShadowData directionalLightShadowData = GetDirectionalLightShadowData();
		shadowMask.r = SampleShadow(DirectionalLight, directionalLightShadowData, DirectionalLightShadowMap, gBuffer, shadowMask.g);
	}
	float4 light = GetLighting(ViewPos, DirectionalLight, gBuffer, shadowMask, false, false);

	// Calculate lighting from sky light
	light += GetSkyLightLighting(SkyLight, gBuffer, SkyLightTexture);

	// Calculate lighting from local lights
	LOOP
	for (uint localLightIndex = 0; localLightIndex < LocalLightsCount; localLightIndex++)
	{
		const LightData localLight = LocalLights[localLightIndex];
		bool isSpotLight = localLight.SpotAngles.x > -2.0f;
		shadowMask = 1.0f;
		light += GetLighting(ViewPos, localLight, gBuffer, shadowMask, true, isSpotLight);
	}

#if USE_REFLECTIONS
	// Calculate reflections
	light.rgb += GetEnvProbeLighting(ViewPos, EnvProbe, EnvironmentProbe, gBuffer) * light.a;	
#endif

	// Add lighting (apply ambient occlusion)
	output.rgb += light.rgb * gBuffer.AO;

#if USE_FOG
	// Calculate exponential height fog
	float4 fog = GetExponentialHeightFog(ExponentialHeightFog, materialInput.WorldPosition, ViewPos, 0);

	// Apply fog to the output color
#if MATERIAL_BLEND == MATERIAL_BLEND_OPAQUE
	output = float4(output.rgb * fog.a + fog.rgb, output.a);
#elif MATERIAL_BLEND == MATERIAL_BLEND_TRANSPARENT
	output = float4(output.rgb * fog.a + fog.rgb, output.a);
#elif MATERIAL_BLEND == MATERIAL_BLEND_ADDITIVE
	output = float4(output.rgb * fog.a + fog.rgb, output.a * fog.a);
#elif MATERIAL_BLEND == MATERIAL_BLEND_MULTIPLY
	output = float4(lerp(float3(1, 1, 1), output.rgb, fog.aaa * fog.aaa), output.a);
#endif

#endif
	
#endif

	return output;
}
