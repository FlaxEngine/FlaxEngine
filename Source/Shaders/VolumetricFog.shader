// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

// Implementation based on:
// "Volumetric fog: Unified, compute shader based solution to atmospheric scattering" - Bart Wronski at Siggraph 2014
// and
// "Physically Based and Unified Volumetric Rendering in Frostbite" - Sebastien Hillaire at Siggraph 2015

#define NO_GBUFFER_SAMPLING
#define LIGHTING_NO_DIRECTIONAL 1
#define LIGHTING_NO_SPECULAR 0
#define SHADOWS_QUALITY 0

// Debug voxels world space positions
#define DEBUG_VOXEL_WS_POS 0

// Debug voxels so CS_FinalIntegration will just copy data without modifications
#define DEBUG_VOXELS 0

#include "./Flax/Common.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/ShadowsSampling.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/GI/DDGI.hlsl"

struct SkyLightData
{
	float3 MultiplyColor;
	float VolumetricScatteringIntensity;
	float3 AdditiveColor;
	float Dummy0;	
};

META_CB_BEGIN(0, Data)
GBufferData GBuffer;

float3 GlobalAlbedo;
float GlobalExtinctionScale;

float3 GlobalEmissive;
float HistoryWeight;

float3 GridSize;
uint MissedHistorySamplesCount;

uint3 GridSizeInt;
float PhaseG;

float2 Dummy0;
float VolumetricFogMaxDistance;
float InverseSquaredLightDistanceBiasScale;

float4 FogParameters;

float4x4 PrevWorldToClip;

float4 FrameJitterOffsets[8];

LightData DirectionalLight;
SkyLightData SkyLight;
DDGIData DDGI;
META_CB_END

META_CB_BEGIN(2, PerLight)
float2 SliceToDepth;
int MinZ;
float LocalLightScatteringIntensity;

float4 ViewSpaceBoundingSphere;
float4x4 ViewToVolumeClip;

LightData LocalLight;
META_CB_END

// The Henyey-Greenstein phase function
// [Henyey and Greenstein 1941, https://www.astro.umd.edu/~jph/HG_note.pdf]
float HenyeyGreensteinPhase(float g, float cosTheta)
{
	return (1 - g * g) / (4 * PI * pow(1 + g * g + 2 * g * cosTheta, 1.5f));
}

float GetPhase(float g, float cosTheta)
{
	return HenyeyGreensteinPhase(g, cosTheta);
}

float GetSliceDepth(float zSlice)
{
	return (zSlice / GridSize.z) * VolumetricFogMaxDistance;
}

float3 GetCellPositionWS(uint3 gridCoordinate, float3 cellOffset, out float sceneDepth)
{
	float2 volumeUV = (gridCoordinate.xy + cellOffset.xy) / GridSize.xy;
	sceneDepth = GetSliceDepth(gridCoordinate.z + cellOffset.z) / GBuffer.ViewFar;
	float deviceDepth = LinearZ2DeviceDepth(GBuffer, sceneDepth);
	return GetWorldPos(GBuffer, volumeUV, deviceDepth);
}

float3 GetCellPositionWS(uint3 gridCoordinate, float3 cellOffset)
{
	float temp;
	return GetCellPositionWS(gridCoordinate, cellOffset, temp);
}

float3 GetVolumeUV(float3 worldPosition, float4x4 worldToClip)
{
	float4 ndcPosition = mul(float4(worldPosition, 1), worldToClip);
	ndcPosition.xy /= ndcPosition.w;
	return float3(ndcPosition.xy * float2(0.5f, -0.5f) + 0.5f, ndcPosition.w / VolumetricFogMaxDistance);
}

// Vertex shader that writes to a range of slices of a volume texture
META_VS(true, FEATURE_LEVEL_SM5)
META_FLAG(VertexToGeometryShader)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
Quad_VS2GS VS_WriteToSlice(float2 TexCoord : TEXCOORD0, uint LayerIndex : SV_InstanceID)
{
	Quad_VS2GS output;

	uint slice = LayerIndex + MinZ;
	float depth = (slice / SliceToDepth.x) * SliceToDepth.y;
	float depthOffset = abs(depth - ViewSpaceBoundingSphere.z);

	if (depthOffset < ViewSpaceBoundingSphere.w)
	{
		float radius = sqrt(ViewSpaceBoundingSphere.w * ViewSpaceBoundingSphere.w - depthOffset * depthOffset);
		float3 positionVS = float3(ViewSpaceBoundingSphere.xy + (TexCoord * 2 - 1) * radius, depth);
		output.Vertex.Position = mul(float4(positionVS, 1), ViewToVolumeClip);
	}
	else
	{
		output.Vertex.Position = float4(0, 0, 0, 0);
	}

	output.Vertex.TexCoord = TexCoord;
	output.LayerIndex = slice;

	return output;
}

// Geometry shader that writes to a range of slices of a volume texture
META_GS(true, FEATURE_LEVEL_SM5)
[maxvertexcount(3)]
void GS_WriteToSlice(triangle Quad_VS2GS input[3], inout TriangleStream<Quad_GS2PS> stream)
{
	Quad_GS2PS vertex;

	vertex.Vertex = input[0].Vertex;
	vertex.LayerIndex = input[0].LayerIndex;
	stream.Append(vertex);

	vertex.Vertex = input[1].Vertex;
	vertex.LayerIndex = input[1].LayerIndex;
	stream.Append(vertex);

	vertex.Vertex = input[2].Vertex;
	vertex.LayerIndex = input[2].LayerIndex;
	stream.Append(vertex);
}

#if USE_SHADOW
Texture2D<float> ShadowMap : register(t0);
Buffer<float4> ShadowsBuffer : register(t1);
#endif

META_PS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(USE_SHADOW=0)
META_PERMUTATION_1(USE_SHADOW=1)
float4 PS_InjectLight(Quad_GS2PS input) : SV_Target0
{
	uint3 gridCoordinate = uint3(input.Vertex.Position.xy, input.LayerIndex);

	// Prevent from shading locations outside the volume
	if (!all(gridCoordinate < GridSizeInt))
		return 0;

	float3 historyUV = GetVolumeUV(GetCellPositionWS(gridCoordinate, 0.5f), PrevWorldToClip);
	float historyAlpha = HistoryWeight;
	FLATTEN
	if (any(historyUV < 0) || any(historyUV > 1))
		historyAlpha = 0;
	uint samplesCount = historyAlpha < 0.001f ? MissedHistorySamplesCount : 1;

	float NoL = 0;
	bool isSpotLight = LocalLight.SpotAngles.x > -2.0f;
	float4 scattering = 0;
	for (uint sampleIndex = 0; sampleIndex < samplesCount; sampleIndex++)
	{
		float3 cellOffset = FrameJitterOffsets[sampleIndex].xyz;
		//float cellOffset = 0.5f;

		float3 positionWS = GetCellPositionWS(gridCoordinate, cellOffset);
		float3 cameraVector = normalize(positionWS - GBuffer.ViewPos);
		float cellRadius = length(positionWS - GetCellPositionWS(gridCoordinate + uint3(1, 1, 1), cellOffset));
		float distanceBias = max(cellRadius * InverseSquaredLightDistanceBiasScale, 1);
		float3 toLight = LocalLight.Position - positionWS;
		float distanceSqr = dot(toLight, toLight);
		float3 L = toLight * rsqrt(distanceSqr);

		// Calculate the light attenuation
		float attenuation = 1;
		GetRadialLightAttenuation(LocalLight, isSpotLight, float3(0, 0, 1), distanceSqr, distanceBias * distanceBias, toLight, L, NoL, attenuation);

		// Peek the shadow
		float shadow = 1.0f;
#if USE_SHADOW
		if (attenuation > 0)
		{
            if (isSpotLight)
	        {
                shadow = SampleSpotLightShadow(LocalLight, ShadowsBuffer, ShadowMap, positionWS).SurfaceShadow;
	        }
	        else
	        {
                shadow = SamplePointLightShadow(LocalLight, ShadowsBuffer, ShadowMap, positionWS).SurfaceShadow;
	        }
		}
#endif

		scattering.rgb += LocalLight.Color * (GetPhase(PhaseG, dot(L, -cameraVector)) * attenuation * shadow * LocalLightScatteringIntensity);
	}

	scattering.rgb /= (float)samplesCount;
	return scattering;
}

#if defined(_CS_Initialize)

RWTexture3D<float4> RWVBufferA : register(u0);
RWTexture3D<float4> RWVBufferB : register(u1);

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(4, 4, 4)]
void CS_Initialize(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 gridCoordinate = DispatchThreadId;

	float voxelOffset = 0.5f;
	float3 positionWS = GetCellPositionWS(gridCoordinate, voxelOffset);

	// Unpack the fog parameters (packing done in C++ ExponentialHeightFog::GetVolumetricFogOptions)
	float fogDensity = FogParameters.x;
	float fogHeight = FogParameters.y;
	float fogHeightFalloff = FogParameters.z;

	// Calculate the global fog density that matches the exponential height fog density
	float globalDensity = fogDensity * exp2(-fogHeightFalloff * (positionWS.y - fogHeight));
	float extinction = max(0.0f, globalDensity * GlobalExtinctionScale * 0.24f);

	float3 scattering = GlobalAlbedo * extinction;
	float absorption = max(0.0f, extinction - Luminance(scattering));

	if (all(gridCoordinate < GridSizeInt))
	{
		RWVBufferA[gridCoordinate] = float4(scattering, absorption);
		RWVBufferB[gridCoordinate] = float4(GlobalEmissive, 0);
	}
}

#elif defined(_CS_LightScattering)

RWTexture3D<float4> RWLightScattering : register(u0);

Texture3D<float4> VBufferA : register(t0);
Texture3D<float4> VBufferB : register(t1);
Texture3D<float4> LightScatteringHistory : register(t2);
Texture3D<float4> LocalShadowedLightScattering : register(t3);
Texture2D<float> ShadowMap : register(t4);
Buffer<float4> ShadowsBuffer : register(t5);
#if USE_DDGI
Texture2D<snorm float4> ProbesData : register(t6);
Texture2D<float4> ProbesDistance : register(t7);
Texture2D<float4> ProbesIrradiance : register(t8);
#else
TextureCube SkyLightImage : register(t6);
#endif

META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(USE_DDGI=0)
META_PERMUTATION_1(USE_DDGI=1)
[numthreads(4, 4, 4)]
void CS_LightScattering(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 gridCoordinate = DispatchThreadId;
	float3 lightScattering = 0;
	uint samplesCount = 1;

	float3 historyUV = GetVolumeUV(GetCellPositionWS(gridCoordinate, 0.5f), PrevWorldToClip);
	float historyAlpha = HistoryWeight;
	FLATTEN
	if (any(historyUV < 0) || any(historyUV > 1))
		historyAlpha = 0;
	samplesCount = historyAlpha < 0.001f && all(gridCoordinate < GridSizeInt) ? MissedHistorySamplesCount : 1;

	for (uint sampleIndex = 0; sampleIndex < samplesCount; sampleIndex++)
	{
		float3 cellOffset = FrameJitterOffsets[sampleIndex].xyz;
		//float3 cellOffset = 0.5f;

		float sceneDepth;
		float3 positionWS = GetCellPositionWS(gridCoordinate, cellOffset, sceneDepth);
		float3 cameraVector = positionWS - GBuffer.ViewPos;
		float cameraVectorLength = length(cameraVector);
		float3 cameraVectorNormalized = cameraVector / cameraVectorLength;

		// Directional light
        {
            float shadow = SampleDirectionalLightShadow(DirectionalLight, ShadowsBuffer, ShadowMap, positionWS, cameraVectorLength).SurfaceShadow;
			lightScattering += DirectionalLight.Color * (8 * shadow * GetPhase(PhaseG, dot(DirectionalLight.Direction, cameraVectorNormalized)));
		}

#if USE_DDGI
        // Dynamic Diffuse Global Illumination
        float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesData, ProbesDistance, ProbesIrradiance, positionWS, cameraVectorNormalized, 0.0f, cellOffset.x);
        lightScattering += float4(irradiance, 1);
#else
		// Sky light
		if (SkyLight.VolumetricScatteringIntensity > 0)
		{
			float3 skyLighting = SkyLightImage.SampleLevel(SamplerLinearClamp, float3(0, 0, 0), 10000).rgb;
			skyLighting = skyLighting * SkyLight.MultiplyColor + SkyLight.AdditiveColor;
			lightScattering += skyLighting * SkyLight.VolumetricScatteringIntensity;
		}
#endif
	}
	lightScattering /= (float)samplesCount;

	// Apply scattering from the point and spot lights
	lightScattering += LocalShadowedLightScattering[gridCoordinate].rgb;

	float4 materialScatteringAndAbsorption = VBufferA[gridCoordinate];
	float extinction = materialScatteringAndAbsorption.w + Luminance(materialScatteringAndAbsorption.xyz);
	float3 materialEmissive = VBufferB[gridCoordinate].xyz;
	float4 scatteringAndExtinction = float4(lightScattering * materialScatteringAndAbsorption.xyz + materialEmissive, extinction);

	BRANCH
	if (historyAlpha > 0)
	{
		float4 historyScatteringAndExtinction = LightScatteringHistory.SampleLevel(SamplerLinearClamp, historyUV, 0);
		scatteringAndExtinction = lerp(scatteringAndExtinction, historyScatteringAndExtinction, historyAlpha);
	}
	
	if (all(gridCoordinate < GridSizeInt))
	{
		scatteringAndExtinction = isnan(scatteringAndExtinction) || isinf(scatteringAndExtinction) ? 0 : scatteringAndExtinction;
		RWLightScattering[gridCoordinate] = max(scatteringAndExtinction, 0);
	}
}

#elif defined(_CS_FinalIntegration)

RWTexture3D<float4> RWIntegratedLightScattering : register(u0);

Texture3D<float4> LightScattering : register(t0);

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(8, 8, 1)]
void CS_FinalIntegration(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 gridCoordinate = DispatchThreadId;
	float4 acc = float4(0, 0, 0, 1);
	float3 prevPositionWS = GBuffer.ViewPos;

	for (uint layerIndex = 0; layerIndex < GridSizeInt.z; layerIndex++)
	{
		uint3 coords = uint3(gridCoordinate.xy, layerIndex);
		float4 scatteringExtinction = LightScattering[coords];
		float3 positionWS = GetCellPositionWS(coords, 0.5f);
		
		// Ref: "Physically Based and Unified Volumetric Rendering in Frostbite"
		float transmittance = exp(-scatteringExtinction.w * length(positionWS - prevPositionWS));
		float3 scatteringIntegratedOverSlice = (scatteringExtinction.rgb - scatteringExtinction.rgb * transmittance) / max(scatteringExtinction.w, 0.00001f);
		acc.rgb += scatteringIntegratedOverSlice * acc.a;
		acc.a *= transmittance;
	
#if DEBUG_VOXELS
		RWIntegratedLightScattering[coords] = float4(scatteringExtinction.rgb, 1.0f);
#elif DEBUG_VOXEL_WS_POS
		RWIntegratedLightScattering[coords] = float4(positionWS.rgb, 1.0f);
#else
		RWIntegratedLightScattering[coords] = acc;
#endif

		prevPositionWS = positionWS;
	}
}

#endif
