// Copyright (c) Wojciech Figat. All rights reserved.

@0// Lightmap: Defines
#define CAN_USE_LIGHTMAP 1
@1// Lightmap: Includes
@2// Lightmap: Constants
@3// Lightmap: Resources
#if USE_LIGHTMAP
// Irradiance and directionality prebaked lightmaps
Texture2D Lightmap0 : register(t__SRV__);
Texture2D Lightmap1 : register(t__SRV__);
Texture2D Lightmap2 : register(t__SRV__);
#endif
@4// Lightmap: Utilities
#if USE_LIGHTMAP

// Evaluates the H-Basis coefficients in the tangent space normal direction
float3 GetHBasisIrradiance(float3 n, float3 h0, float3 h1, float3 h2, float3 h3)
{
	// Band 0
	float3 color = h0 * (1.0f / sqrt(2.0f * PI));

	// Band 1
	color += h1 * -sqrt(1.5f / PI) * n.y;
	color += h2 *  sqrt(1.5f / PI) * (2 * n.z - 1.0f);
	color += h3 * -sqrt(1.5f / PI) * n.x;

	return color;
}

float3 SampleLightmap(Material material, MaterialInput materialInput)
{
	// Sample lightmaps
	float4 lightmap0 = Lightmap0.Sample(SamplerLinearClamp, materialInput.LightmapUV);
	float4 lightmap1 = Lightmap1.Sample(SamplerLinearClamp, materialInput.LightmapUV);
	float4 lightmap2 = Lightmap2.Sample(SamplerLinearClamp, materialInput.LightmapUV);

	// Unpack H-basis
	float3 h0 = float3(lightmap0.x, lightmap1.x, lightmap2.x);
	float3 h1 = float3(lightmap0.y, lightmap1.y, lightmap2.y);
	float3 h2 = float3(lightmap0.z, lightmap1.z, lightmap2.z);
	float3 h3 = float3(lightmap0.w, lightmap1.w, lightmap2.w);

	// Sample baked diffuse irradiance from the H-basis coefficients
	float3 normal = material.TangentNormal;
#if MATERIAL_SHADING_MODEL == SHADING_MODEL_FOLIAGE
	normal *= material.TangentNormal;
#endif
	return GetHBasisIrradiance(normal, h0, h1, h2, h3) / PI;
}

#endif
@5// Lightmap: Shaders
