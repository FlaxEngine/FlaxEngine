// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

@0// Deferred Shading: Defines
@1// Deferred Shading: Includes
@2// Deferred Shading: Constants
@3// Deferred Shading: Resources
@4// Deferred Shading: Utilities
@5// Deferred Shading: Shaders

// Pixel Shader function for GBuffer Pass
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(USE_LIGHTMAP=0)
META_PERMUTATION_1(USE_LIGHTMAP=1)
void PS_GBuffer(
		in PixelInput input
		,out float4 Light : SV_Target0
#if MATERIAL_BLEND == MATERIAL_BLEND_OPAQUE
		// GBuffer
		,out float4 RT0   : SV_Target1
		,out float4 RT1   : SV_Target2
		,out float4 RT2   : SV_Target3
#if USE_GBUFFER_CUSTOM_DATA
		,out float4 RT3   : SV_Target4
#endif
#endif
	)
{
	Light = 0;
	
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
	
#if USE_LIGHTMAP
	float3 diffuseColor = GetDiffuseColor(material.Color, material.Metalness);
	float3 specularColor = GetSpecularColor(material.Color, material.Specular, material.Metalness);

	// Sample lightmap
	float3 diffuseIndirectLighting = SampleLightmap(material, materialInput);

	// Apply static indirect light
	Light.rgb = diffuseColor * diffuseIndirectLighting * AOMultiBounce(material.AO, diffuseColor);
#endif

#if MATERIAL_BLEND == MATERIAL_BLEND_OPAQUE

	// Pack material properties to GBuffer
	RT0 = float4(material.Color, material.AO);
	RT1 = float4(material.WorldNormal * 0.5 + 0.5, MATERIAL_SHADING_MODEL * (1.0 / 3.0));
	RT2 = float4(material.Roughness, material.Metalness, material.Specular, 0);

	// Custom data
#if USE_GBUFFER_CUSTOM_DATA
#if MATERIAL_SHADING_MODEL == SHADING_MODEL_SUBSURFACE
	RT3 = float4(material.SubsurfaceColor, material.Opacity);
#elif MATERIAL_SHADING_MODEL == SHADING_MODEL_FOLIAGE
	RT3 = float4(material.SubsurfaceColor, material.Opacity);
#else
	RT3 = float4(0, 0, 0, 0);
#endif
#endif

	// Add light emission
#if USE_EMISSIVE
	Light.rgb += material.Emissive;
#endif

#else

	// Handle blending as faked forward pass (use Light buffer and skip GBuffer modification)
	Light = float4(material.Emissive, material.Opacity);

#endif
}
