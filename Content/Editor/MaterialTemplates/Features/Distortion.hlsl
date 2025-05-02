// Copyright (c) Wojciech Figat. All rights reserved.

@0// Distortion: Defines
@1// Distortion: Includes
@2// Distortion: Constants
@3// Distortion: Resources
@4// Distortion: Utilities
@5// Distortion: Shaders
#if USE_DISTORTION

// Pixel Shader function for Distortion Pass
META_PS(USE_DISTORTION, FEATURE_LEVEL_ES2)
float4 PS_Distortion(PixelInput input) : SV_Target0
{
	MaterialInput materialInput = GetMaterialInput(input);
#if USE_DITHERED_LOD_TRANSITION
	ClipLODTransition(materialInput);
#endif

	// Get material parameters
	Material material = GetMaterialPS(materialInput);

	// Masking
#if MATERIAL_MASKED
	clip(material.Mask - MATERIAL_MASK_THRESHOLD);
#endif

	float3 viewNormal = normalize(TransformWorldVectorToView(materialInput, material.WorldNormal));
	float airIOR = 1.0f;
#if USE_PIXEL_NORMAL_OFFSET_REFRACTION
	float3 viewVertexNormal = TransformWorldVectorToView(materialInput, TransformTangentVectorToWorld(materialInput, float3(0, 0, 1)));
	float2 distortion = (viewVertexNormal.xy - viewNormal.xy) * (material.Refraction - airIOR);
#else
	float2 distortion = viewNormal.xy * (material.Refraction - airIOR);
#endif

	// Clip if the distortion distance (squared) is too small to be noticed
	clip(dot(distortion, distortion) - 0.00001);

	// Scale up for better precision in low/subtle refractions at the expense of artefacts at higher refraction
	distortion *= 4.0f;
	
	// Use separate storage for positive and negative offsets
	float2 addOffset = max(distortion, 0);
	float2 subOffset = abs(min(distortion, 0));
	return float4(addOffset.x, addOffset.y, subOffset.x, subOffset.y);
}

#endif
