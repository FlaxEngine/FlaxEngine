// Copyright (c) Wojciech Figat. All rights reserved.

// Adaptive Screen Space Ambient Occlusion
// https://github.com/GameTechDev/ASSAO
// 
// Copyright (c) 2016, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of 
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/Gather.hlsl"
#include "./Flax/GBuffer.hlsl"

#define SSAO_DEPTH_MIP_LEVELS 4 // <- must match C++ define

// Progressive poisson-like pattern; x, y are in [-1, 1] range, .z is length(float2(x,y)), .w is log2(z)
#define INTELSSAO_MAIN_DISK_SAMPLE_COUNT (32)
static const float4 g_samplePatternMain[INTELSSAO_MAIN_DISK_SAMPLE_COUNT] =
{
    float4( 0.78488064,  0.56661671,  1.500000, -0.126083),    float4( 0.26022232, -0.29575172,  1.500000, -1.064030),    float4( 0.10459357,  0.08372527,  1.110000, -2.730563),    float4(-0.68286800,  0.04963045,  1.090000, -0.498827),
    float4(-0.13570161, -0.64190155,  1.250000, -0.532765),    float4(-0.26193795, -0.08205118,  0.670000, -1.783245),    float4(-0.61177456,  0.66664219,  0.710000, -0.044234),    float4( 0.43675563,  0.25119025,  0.610000, -1.167283),
    float4( 0.07884444,  0.86618668,  0.640000, -0.459002),    float4(-0.12790935, -0.29869005,  0.600000, -1.729424),    float4(-0.04031125,  0.02413622,  0.600000, -4.792042),    float4( 0.16201244, -0.52851415,  0.790000, -1.067055),
    float4(-0.70991218,  0.47301072,  0.640000, -0.335236),    float4( 0.03277707, -0.22349690,  0.600000, -1.982384),    float4( 0.68921727,  0.36800742,  0.630000, -0.266718),    float4( 0.29251814,  0.37775412,  0.610000, -1.422520),
    float4(-0.12224089,  0.96582592,  0.600000, -0.426142),    float4( 0.11071457, -0.16131058,  0.600000, -2.165947),    float4( 0.46562141, -0.59747696,  0.600000, -0.189760),    float4(-0.51548797,  0.11804193,  0.600000, -1.246800),
    float4( 0.89141309, -0.42090443,  0.600000,  0.028192),    float4(-0.32402530, -0.01591529,  0.600000, -1.543018),    float4( 0.60771245,  0.41635221,  0.600000, -0.605411),    float4( 0.02379565, -0.08239821,  0.600000, -3.809046),
    float4( 0.48951152, -0.23657045,  0.600000, -1.189011),    float4(-0.17611565, -0.81696892,  0.600000, -0.513724),    float4(-0.33930185, -0.20732205,  0.600000, -1.698047),    float4(-0.91974425,  0.05403209,  0.600000,  0.062246),
    float4(-0.15064627, -0.14949332,  0.600000, -1.896062),    float4( 0.53180975, -0.35210401,  0.600000, -0.758838),    float4( 0.41487166,  0.81442589,  0.600000, -0.505648),    float4(-0.24106961, -0.32721516,  0.600000, -1.665244)
};

// These values can be changed with no changes required elsewhere;
// The actual number of texture samples is two times this value (each "tap" has two symmetrical depth texture samples)
static const uint g_numTaps[4] = { 3, 5, 8, 12 };

//
// Optional parts that can be enabled for a required quality preset level and above (0 == Low, 1 == Medium, 2 == High, 3 == Highest)
// Each has its own cost. To disable just set to 5 or above.
//
// (experimental) tilts the disk (although only half of the samples!) towards surface normal; this helps with effect uniformity between objects but reduces effect distance and has other side-effects
#define SSAO_TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET                      (99)        // to disable simply set to 99 or similar
#define SSAO_TILT_SAMPLES_AMOUNT                                        (0.4)
//
#define SSAO_HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET                 (1)         // to disable simply set to 99 or similar
#define SSAO_HALOING_REDUCTION_AMOUNT                                   (0.6)       // values from 0.0 - 1.0, 1.0 means max weighting (will cause artifacts, 0.8 is more reasonable)
//
#define SSAO_NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET                (2)         // to disable simply set to 99 or similar
#define SSAO_NORMAL_BASED_EDGES_DOT_THRESHOLD                           (0.5)       // use 0-0.1 for super-sharp normal-based edges
//
#define SSAO_DETAIL_AO_ENABLE_AT_QUALITY_PRESET                         (1)         // whether to use DetailAOStrength; to disable simply set to 99 or similar
//
#define SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET                        (99)         // !!warning!! the MIP generation on the C++ side will be enabled on quality preset 2 regardless of this value, so if changing here, change the C++ side too
#define SSAO_DEPTH_MIPS_GLOBAL_OFFSET                                   (-4.3)      // best noise/quality/performance tradeoff, found empirically
//
// !!warning!! the edge handling is hard-coded to 'disabled' on quality level 0, and enabled above, on the C++ side; while toggling it here will work for 
// testing purposes, it will not yield performance gains (or correct results)
#define SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET                 (1)     
//
#define SSAO_REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET  (99)        // 99 means disabled; only helpful if artifacts at the edges caused by lack of out of screen depth data are not acceptable with the depth sampler in either clamp or mirror modes
//

// Constant Buffer

META_CB_BEGIN(0, Data)

GBufferData GBuffer;

float2 ViewportPixelSize;      // .zw == 1.0 / ViewportSize.xy
float2 HalfViewportPixelSize;  // .zw == 1.0 / ViewportHalfSize.xy

int2 PerPassFullResCoordOffset;
int PassIndex;
float EffectMaxDistance;

float2 Viewport2xPixelSize;
float2 Viewport2xPixelSize_x_025; // Viewport2xPixelSize * 0.25 (for fusing add+mul into mad)

float EffectRadius;              // world (viewspace) maximum size of the shadow
float EffectShadowStrength;      // global strength of the effect (0 - 5)
float EffectShadowPow;
float EffectNearFadeMul;

float EffectFadeOutMul;                 // fade out from distance (ex. 25)
float EffectFadeOutAdd;                 // fade out to distance   (ex. 100)
float EffectHorizonAngleThreshold;      // limit errors on slopes and caused by insufficient geometry tessellation (0.05 to 0.5)
float EffectSamplingRadiusNearLimitRec; // if viewspace pixel closer than this, don't enlarge shadow sampling radius anymore (makes no sense to grow beyond some distance, not enough samples to cover everything, so just limit the shadow growth; could be SSAOSettingsFadeOutFrom * 0.1 or less)

float DepthPrecisionOffsetMod;
float NegRecEffectRadius;               // -1.0 / EffectRadius
float InvSharpness;
float DetailAOStrength;

float4 PatternRotScaleMatrices[5];

float4x4 ViewMatrix;

META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

// Shader Resources

Texture2D<float> g_DepthSource               : register(t0); // corresponds to SSAO_TEXTURE_SLOT0
Texture2D        g_NormalmapSource           : register(t1); // corresponds to SSAO_TEXTURE_SLOT1

Texture2D<float> g_ViewspaceDepthSource      : register(t0); // corresponds to SSAO_TEXTURE_SLOT0
Texture2D<float> g_ViewspaceDepthSource1     : register(t1); // corresponds to SSAO_TEXTURE_SLOT1
Texture2D<float> g_ViewspaceDepthSource2     : register(t2); // corresponds to SSAO_TEXTURE_SLOT2
Texture2D<float> g_ViewspaceDepthSource3     : register(t3); // corresponds to SSAO_TEXTURE_SLOT3

Texture2D        g_BlurInput                 : register(t0); // corresponds to SSAO_TEXTURE_SLOT0

Texture2DArray   g_FinalSSAO                 : register(t0); // corresponds to SSAO_TEXTURE_SLOT0

// Packing/unpacking for edges; 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother transitions
float PackEdges(float4 edgesLRTB)
{
	//int4 edgesLRTBi = int4(saturate(edgesLRTB) * 3.0 + 0.5);
	//return ((edgesLRTBi.x << 6) + (edgesLRTBi.y << 4) + (edgesLRTBi.z << 2) + (edgesLRTBi.w << 0)) / 255.0;

	// Optimized, should be same as above
	edgesLRTB = round(saturate(edgesLRTB) * 3.05);
	return dot(edgesLRTB, float4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
}

float4 UnpackEdges(float _packedVal)
{
	uint packedVal = (uint)(_packedVal * 255.5);
	float4 edgesLRTB;
	
	// There's really no need for mask (as it's an 8 bit input) but maybe in future it will be needed
#define SSAO_UNPACK_EDGES_WITH_MASK 0
#if SSAO_UNPACK_EDGES_WITH_MASK
	edgesLRTB.x = float((packedVal >> 6) & 0x03) / 3.0;
	edgesLRTB.y = float((packedVal >> 4) & 0x03) / 3.0;
	edgesLRTB.z = float((packedVal >> 2) & 0x03) / 3.0;
	edgesLRTB.w = float((packedVal >> 0) & 0x03) / 3.0;
#else
	edgesLRTB.x = float(packedVal >> 6) * 0.33;
	edgesLRTB.y = float(packedVal >> 4) * 0.33;
	edgesLRTB.z = float(packedVal >> 2) * 0.33;
	edgesLRTB.w = float(packedVal >> 0) * 0.33;
#endif

	return saturate(edgesLRTB + InvSharpness);
}

// From [0, width], [0, height] to [-1, 1], [-1, 1]
float2 ScreenSpaceToClipSpacePositionXY(float2 screenPos)
{
	return screenPos * Viewport2xPixelSize.xy - float2(1.0f, 1.0f);
}

float3 NDCToViewspace(float2 pos, float viewspaceDepth)
{
	GBufferData gBufferData = GetGBufferData();
	float deviceDepth = LinearZ2DeviceDepth(gBufferData, viewspaceDepth / gBufferData.ViewFar);
	float4 clipPos = float4(pos * float2(2.0, -2.0) + float2(-1.0, 1.0), deviceDepth, 1.0);
	float4 viewPos = mul(clipPos, gBufferData.InvProjectionMatrix);
	return viewPos.xyz / viewPos.w;
}

// Calculate effect radius and fit our screen sampling pattern inside it
void CalculateRadiusParameters(const float pixCenterLength, const float2 pixelDirRBViewspaceSizeAtCenterZ, out float pixLookupRadiusMod, out float effectRadius, out float falloffCalcMulSq)
{
	effectRadius = EffectRadius;
	
	// Leaving this out for performance reasons: use something similar if radius needs to scale based on distance
	//effectRadius *= pow(pixCenterLength, RadiusDistanceScalingFunctionPow);
	
	// When too close, on-screen sampling disk will grow beyond screen size; limit this to avoid closeup temporal artifacts
	const float tooCloseLimitMod = saturate(pixCenterLength * EffectSamplingRadiusNearLimitRec) * 0.8 + 0.2;
	
	effectRadius *= tooCloseLimitMod;
	
	// Value 0.85 is to reduce the radius to allow for more samples on a slope to still stay within influence
	pixLookupRadiusMod = (0.85 * effectRadius) / pixelDirRBViewspaceSizeAtCenterZ.x;
	
	// Used to calculate falloff (both for AO samples and per-sample weights)
	falloffCalcMulSq = -1.0f / (effectRadius * effectRadius);
}

float4 CalculateEdges(const float centerZ, const float leftZ, const float rightZ, const float topZ, const float bottomZ)
{
	// Slope-sensitive depth-based edge detection
	float4 edgesLRTB = float4(leftZ, rightZ, topZ, bottomZ) - centerZ;
	float4 edgesLRTBSlopeAdjusted = edgesLRTB + edgesLRTB.yxwz;
	edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
	return saturate((1.3 - edgesLRTB / (centerZ * 0.040)));
	
	// Cheaper version but has artifacts
	//edgesLRTB = abs(float4(leftZ, rightZ, topZ, bottomZ) - centerZ);
	//return saturate((1.3 - edgesLRTB / (pixZ * 0.06 + 0.1)));
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_PrepareDepths(in float4 inPos : SV_POSITION, out float out0 : SV_Target0, out float out1 : SV_Target1, out float out2 : SV_Target2, out float out3 : SV_Target3)
{
#if CAN_USE_GATHER
	float2 gatherUV = inPos.xy * Viewport2xPixelSize;
	float4 depths = g_DepthSource.GatherRed(SamplerPointClamp, gatherUV);
	float c = depths.x;
	float d = depths.y;
	float b = depths.z;
	float a = depths.w;
#else
	int3 baseCoord = int3(int2(inPos.xy) * 2, 0);
	float c = g_DepthSource.Load(baseCoord, int2(0, 1)).x;
	float d = g_DepthSource.Load(baseCoord, int2(1, 1)).x;
	float b = g_DepthSource.Load(baseCoord, int2(1, 0)).x;
	float a = g_DepthSource.Load(baseCoord, int2(0, 0)).x;
#endif

	GBufferData gBufferData = GetGBufferData();
	out0 = LinearizeZ(gBufferData, a);
	out1 = LinearizeZ(gBufferData, b);
	out2 = LinearizeZ(gBufferData, c);
	out3 = LinearizeZ(gBufferData, d);
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_PrepareDepthsHalf(in float4 inPos : SV_POSITION, out float out0 : SV_Target0, out float out1 : SV_Target1)
{
	int3 baseCoord = int3(int2(inPos.xy) * 2, 0);
	float a = g_DepthSource.Load(baseCoord, int2(0, 0)).x;
	float d = g_DepthSource.Load(baseCoord, int2(1, 1)).x;

	GBufferData gBufferData = GetGBufferData();
	out0 = LinearizeZ(gBufferData, a);
	out1 = LinearizeZ(gBufferData, d);
}

void PrepareDepthMip(const float4 inPos, int mipLevel, out float outD0, out float outD1, out float outD2, out float outD3)
{
	int2 baseCoords = int2(inPos.xy) * 2;
	
	float4 depthsArr[4];
	float depthsOutArr[4];
	
	// How to Gather a specific mip level?
	depthsArr[0].x = g_ViewspaceDepthSource[baseCoords + int2(0, 0)].x;
	depthsArr[0].y = g_ViewspaceDepthSource[baseCoords + int2(1, 0)].x;
	depthsArr[0].z = g_ViewspaceDepthSource[baseCoords + int2(0, 1)].x;
	depthsArr[0].w = g_ViewspaceDepthSource[baseCoords + int2(1, 1)].x;
	depthsArr[1].x = g_ViewspaceDepthSource1[baseCoords + int2(0, 0)].x;
	depthsArr[1].y = g_ViewspaceDepthSource1[baseCoords + int2(1, 0)].x;
	depthsArr[1].z = g_ViewspaceDepthSource1[baseCoords + int2(0, 1)].x;
	depthsArr[1].w = g_ViewspaceDepthSource1[baseCoords + int2(1, 1)].x;
	depthsArr[2].x = g_ViewspaceDepthSource2[baseCoords + int2(0, 0)].x;
	depthsArr[2].y = g_ViewspaceDepthSource2[baseCoords + int2(1, 0)].x;
	depthsArr[2].z = g_ViewspaceDepthSource2[baseCoords + int2(0, 1)].x;
	depthsArr[2].w = g_ViewspaceDepthSource2[baseCoords + int2(1, 1)].x;
	depthsArr[3].x = g_ViewspaceDepthSource3[baseCoords + int2(0, 0)].x;
	depthsArr[3].y = g_ViewspaceDepthSource3[baseCoords + int2(1, 0)].x;
	depthsArr[3].z = g_ViewspaceDepthSource3[baseCoords + int2(0, 1)].x;
	depthsArr[3].w = g_ViewspaceDepthSource3[baseCoords + int2(1, 1)].x;
	
	const uint2 SVPosui = uint2(inPos.xy);
	//const uint pseudoRandomA = (SVPosui.x) + 2 * (SVPosui.y);
	
	float dummyUnused1;
	float dummyUnused2;
	float falloffCalcMulSq, falloffCalcAdd;
	
	for (int i = 0; i < 4; i++)
	{
		float4 depths = depthsArr[i];
		
		float closest = min(min(depths.x, depths.y), min(depths.z, depths.w));
		
		CalculateRadiusParameters(abs(closest), 1.0, dummyUnused1, dummyUnused2, falloffCalcMulSq);
		
		float4 dists = depths - closest.xxxx;
		
		float4 weights = saturate(dists * dists * falloffCalcMulSq + 1.0);
		
		float smartAvg = dot(weights, depths) / dot(weights, float4(1.0, 1.0, 1.0, 1.0));
		
		//const uint pseudoRandomIndex = (pseudoRandomA + i) % 4;
		
		//depthsOutArr[i] = closest;
		//depthsOutArr[i] = depths[pseudoRandomIndex];
		depthsOutArr[i] = smartAvg;
	}
	
	outD0 = depthsOutArr[0];
	outD1 = depthsOutArr[1];
	outD2 = depthsOutArr[2];
	outD3 = depthsOutArr[3];
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_PrepareDepthMip1(in float4 inPos : SV_POSITION, out float out0 : SV_Target0, out float out1 : SV_Target1, out float out2 : SV_Target2, out float out3 : SV_Target3)
{
	PrepareDepthMip(inPos, 1, out0, out1, out2, out3);
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_PrepareDepthMip2(in float4 inPos : SV_POSITION, out float out0 : SV_Target0, out float out1 : SV_Target1, out float out2 : SV_Target2, out float out3 : SV_Target3)
{
	PrepareDepthMip(inPos, 2, out0, out1, out2, out3);
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_PrepareDepthMip3(in float4 inPos : SV_POSITION, out float out0 : SV_Target0, out float out1 : SV_Target1, out float out2 : SV_Target2, out float out3 : SV_Target3)
{
	PrepareDepthMip(inPos, 3, out0, out1, out2, out3);
}

// AO generation

float3 LoadNormal(int2 pos)
{
	float3 normalEncoded = g_NormalmapSource.Load(int3(pos, 0)).xyz;
	float3 normalWS = DecodeNormal(normalEncoded);
	float3 normalVS = mul(normalWS, (float3x3)ViewMatrix);
	return normalVS;
}

float3 LoadNormal(int2 pos, int2 offset)
{
	float3 normalEncoded = g_NormalmapSource.Load(int3(pos, 0), offset).xyz;
	float3 normalWS = DecodeNormal(normalEncoded);
	float3 normalVS = mul(normalWS, (float3x3)ViewMatrix);
	return normalVS;
}

float CalculatePixelObscurance(float3 pixelNormal, float3 hitDelta, float falloffCalcMulSq)
{
	// All vectors are in viewspace
	
	float lengthSq = dot(hitDelta, hitDelta);
	float NdotD = dot(pixelNormal, hitDelta) / sqrt(lengthSq);
	float falloffMult = max(0.0, lengthSq * falloffCalcMulSq + 1.0);
	return max(0, NdotD - EffectHorizonAngleThreshold) * falloffMult;
}

void SSAOTapInner(const int qualityLevel, inout float obscuranceSum, inout float weightSum, const float2 samplingUV, const float mipLevel, const float3 pixCenterPos, const float3 negViewspaceDir, float3 pixelNormal, const float falloffCalcMulSq, const float weightMod, const int dbgTapIndex)
{
	// Get depth at sample
	float viewspaceSampleZ = g_ViewspaceDepthSource.SampleLevel(SamplerPointClamp, samplingUV.xy, mipLevel).x; 
	
	// Convert to viewspace
	float3 hitPos = NDCToViewspace(samplingUV.xy, viewspaceSampleZ).xyz;
	float3 hitDelta = hitPos - pixCenterPos;
	
	float obscurance = CalculatePixelObscurance(pixelNormal, hitDelta, falloffCalcMulSq);
	float weight = 1.0;
	
	if (qualityLevel >= SSAO_HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET)
	{
		//float reduct = max(0, dot(hitDelta, negViewspaceDir));
		float reduct = max(0, -hitDelta.z); // Cheaper, less correct version
		reduct = saturate(reduct * NegRecEffectRadius + 2.0); // saturate(2.0 - reduct / EffectRadius);
		weight = SSAO_HALOING_REDUCTION_AMOUNT * reduct + (1.0 - SSAO_HALOING_REDUCTION_AMOUNT);
	}
	
	weight *= weightMod;
	obscuranceSum += obscurance * weight;
	weightSum += weight;
}

void SSAOTap(const int qualityLevel, inout float obscuranceSum, inout float weightSum, const int tapIndex, const float2x2 rotScale, const float3 pixCenterPos, const float3 negViewspaceDir, float3 pixelNormal, const float2 normalizedScreenPos, const float mipOffset, const float falloffCalcMulSq, float weightMod, float2 normXY, float normXYLength)
{
	float2 sampleOffset;
	float samplePow2Len;
	
	// Patterns
	{
		float4 newSample = g_samplePatternMain[tapIndex];
		sampleOffset = mul(rotScale, newSample.xy);
		samplePow2Len = newSample.w;                      // Precalculated, same as: samplePow2Len = log2(length(newSample.xy));
		weightMod *= newSample.z;
	}
	
	// Snap to pixel center (more correct obscurance math, avoids artifacts)
	sampleOffset = round(sampleOffset);
	
	// Calculate MIP based on the sample distance from the centre, similar to as described 
	// in http://graphics.cs.williams.edu/papers/SAOHPG12/.
	float mipLevel = (qualityLevel < SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? (0) : (samplePow2Len + mipOffset);
	
	float2 samplingUV = sampleOffset * Viewport2xPixelSize + normalizedScreenPos;
	
	SSAOTapInner(qualityLevel, obscuranceSum, weightSum, samplingUV, mipLevel, pixCenterPos, negViewspaceDir, pixelNormal, falloffCalcMulSq, weightMod, tapIndex * 2);
	
	// For the second tap, just use the mirrored offset
	float2 sampleOffsetMirroredUV = -sampleOffset;
	
	// Tilt the second set of samples so that the disk is effectively rotated by the normal
	// Effective at removing one set of artifacts, but too expensive for lower quality settings
	if (qualityLevel >= SSAO_TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET)
	{
		float dotNorm = dot(sampleOffsetMirroredUV, normXY);
		sampleOffsetMirroredUV -= dotNorm * normXYLength * normXY;
		sampleOffsetMirroredUV = round(sampleOffsetMirroredUV);
	}
	
	// Snap to pixel center (more correct obscurance math, avoids artifacts)
	float2 samplingMirroredUV = sampleOffsetMirroredUV * Viewport2xPixelSize + normalizedScreenPos;
	
	SSAOTapInner(qualityLevel, obscuranceSum, weightSum, samplingMirroredUV, mipLevel, pixCenterPos, negViewspaceDir, pixelNormal, falloffCalcMulSq, weightMod, tapIndex * 2 + 1);
}

// This function is designed to only work with half/half depth at the moment - there's a couple of hardcoded paths that expect pixel/texel size, so it will not work for full res
void GenerateSSAOShadowsInternal(out float outShadowTerm, out float4 outEdges, out float outWeight, const float2 SVPos/*, const float2 normalizedScreenPos*/, int qualityLevel)
{
	float2 SVPosRounded = trunc(SVPos);
	uint2 SVPosui = uint2(SVPosRounded); // same as uint2(SVPos)
	
	const int numberOfTaps = g_numTaps[qualityLevel];
	float pixZ, pixLZ, pixTZ, pixRZ, pixBZ;

	float4 valuesUL = TextureGatherRed(g_ViewspaceDepthSource, SamplerPointWrap, SVPosRounded * HalfViewportPixelSize);
	float4 valuesBR = TextureGatherRed(g_ViewspaceDepthSource, SamplerPointWrap, SVPosRounded * HalfViewportPixelSize, int2(1, 1));
	
	// Get this pixel's viewspace depth
	pixZ = valuesUL.y;
	
	// Skip too far pixels
	if (pixZ > EffectMaxDistance)
	{
		outShadowTerm = 1;
		outEdges = 1;
		outWeight = 0;
		return;
	}
	
	// Get left right top bottom neighbouring pixels for edge detection (gets compiled out on qualityLevel == 0)
	pixLZ = valuesUL.x;
	pixTZ = valuesUL.z;
	pixRZ = valuesBR.z;
	pixBZ = valuesBR.x;
	
	float2 normalizedScreenPos = SVPosRounded * Viewport2xPixelSize + Viewport2xPixelSize_x_025;
	float3 pixCenterPos = NDCToViewspace(normalizedScreenPos, pixZ); // g
	
	// Load this pixel's viewspace normal
	uint2 fullResCoord = SVPosui * 2 + PerPassFullResCoordOffset.xy;
	float3 pixelNormal = LoadNormal(fullResCoord);
	
	float2 pixelDirRBViewspaceSizeAtCenterZ = NDCToViewspace(normalizedScreenPos.xy + ViewportPixelSize.xy, pixCenterPos.z).xy - pixCenterPos.xy;
	
	float pixLookupRadiusMod;
	float falloffCalcMulSq;
	
	// Calculate effect radius and fit our screen sampling pattern inside it
	float effectViewspaceRadius;
	CalculateRadiusParameters(length(pixCenterPos), pixelDirRBViewspaceSizeAtCenterZ, pixLookupRadiusMod, effectViewspaceRadius, falloffCalcMulSq);
	
	// Calculate samples rotation/scaling
	float2x2 rotScale;
	{
		// Reduce effect radius near the screen edges slightly; ideally, one would render a larger depth buffer (5% on each side) instead
		if (qualityLevel >= SSAO_REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET)
		{
			float nearScreenBorder = min(min(normalizedScreenPos.x, 1.0 - normalizedScreenPos.x), min(normalizedScreenPos.y, 1.0 - normalizedScreenPos.y));
			nearScreenBorder = saturate(10.0 * nearScreenBorder + 0.6);
			pixLookupRadiusMod *= nearScreenBorder;
		}
		
		// Load & update pseudo-random rotation matrix
		uint pseudoRandomIndex = uint(SVPosRounded.y * 2 + SVPosRounded.x) % 5;
		float4 rs = PatternRotScaleMatrices[pseudoRandomIndex];
		rotScale = float2x2(rs.x * pixLookupRadiusMod, rs.y * pixLookupRadiusMod, rs.z * pixLookupRadiusMod, rs.w * pixLookupRadiusMod);
	}
	
	// The main obscurance & sample weight storage
	float obscuranceSum = 0.0;
	float weightSum = 0.0;
	
	// Edge mask for between this and left/right/top/bottom neighbour pixels - not used in quality level 0 so initialize to "no edge" (1 is no edge, 0 is edge)
	float4 edgesLRTB = float4(1.0, 1.0, 1.0, 1.0);
	
	// Move center pixel slightly towards camera to avoid imprecision artifacts due to using of 16bit depth buffer; a lot smaller offsets needed when using 32bit floats
	pixCenterPos *= DepthPrecisionOffsetMod;
	
	if (qualityLevel >= SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET)
	{
		edgesLRTB = CalculateEdges(pixZ, pixLZ, pixRZ, pixTZ, pixBZ);
	}
	
	// Adds a more high definition sharp effect, which gets blurred out (reuses left/right/top/bottom samples that we used for edge detection)
	if (qualityLevel >= SSAO_DETAIL_AO_ENABLE_AT_QUALITY_PRESET)
	{
		// Approximate neighbouring pixels positions (actually just deltas or "positions - pixCenterPos")
		float3 viewspaceDirZNormalized = float3(pixCenterPos.xy / pixCenterPos.zz, 1.0);
		float3 pixLDelta = float3(-pixelDirRBViewspaceSizeAtCenterZ.x, 0.0, 0.0) + viewspaceDirZNormalized * (pixLZ - pixCenterPos.z); // very close approximation of: float3 pixLPos  = NDCToViewspace(normalizedScreenPos + float2(-HalfViewportPixelSize.x, 0.0), pixLZ).xyz - pixCenterPos.xyz;
		float3 pixRDelta = float3(+pixelDirRBViewspaceSizeAtCenterZ.x, 0.0, 0.0) + viewspaceDirZNormalized * (pixRZ - pixCenterPos.z); // very close approximation of: float3 pixRPos  = NDCToViewspace(normalizedScreenPos + float2(+HalfViewportPixelSize.x, 0.0), pixRZ).xyz - pixCenterPos.xyz;
		float3 pixTDelta = float3(0.0, -pixelDirRBViewspaceSizeAtCenterZ.y, 0.0) + viewspaceDirZNormalized * (pixTZ - pixCenterPos.z); // very close approximation of: float3 pixTPos  = NDCToViewspace(normalizedScreenPos + float2(0.0, -HalfViewportPixelSize.y), pixTZ).xyz - pixCenterPos.xyz;
		float3 pixBDelta = float3(0.0, +pixelDirRBViewspaceSizeAtCenterZ.y, 0.0) + viewspaceDirZNormalized * (pixBZ - pixCenterPos.z); // very close approximation of: float3 pixBPos  = NDCToViewspace(normalizedScreenPos + float2(0.0, +HalfViewportPixelSize.y), pixBZ).xyz - pixCenterPos.xyz;
		
		// This is to avoid various artifacts
		const float rangeReductionConst = 4.0f;                         
		const float modifiedFalloffCalcMulSq = rangeReductionConst * falloffCalcMulSq;
		
		float4 additionalObscurance;
		additionalObscurance.x = CalculatePixelObscurance(pixelNormal, pixLDelta, modifiedFalloffCalcMulSq);
		additionalObscurance.y = CalculatePixelObscurance(pixelNormal, pixRDelta, modifiedFalloffCalcMulSq);
		additionalObscurance.z = CalculatePixelObscurance(pixelNormal, pixTDelta, modifiedFalloffCalcMulSq);
		additionalObscurance.w = CalculatePixelObscurance(pixelNormal, pixBDelta, modifiedFalloffCalcMulSq);

		obscuranceSum += DetailAOStrength * dot(additionalObscurance, edgesLRTB);
	}
	
	// Sharp normals also create edges - but this adds to the cost as well
	if (qualityLevel >= SSAO_NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET)
	{
		float3 neighbourNormalL = LoadNormal(fullResCoord, int2(-2, 0));
		float3 neighbourNormalR = LoadNormal(fullResCoord, int2(2, 0));
		float3 neighbourNormalT = LoadNormal(fullResCoord, int2(0, -2));
		float3 neighbourNormalB = LoadNormal(fullResCoord, int2(0, 2));
		
		const float dotThreshold = SSAO_NORMAL_BASED_EDGES_DOT_THRESHOLD;
		
		float4 normalEdgesLRTB;
		normalEdgesLRTB.x = saturate((dot(pixelNormal, neighbourNormalL) + dotThreshold));
		normalEdgesLRTB.y = saturate((dot(pixelNormal, neighbourNormalR) + dotThreshold));
		normalEdgesLRTB.z = saturate((dot(pixelNormal, neighbourNormalT) + dotThreshold));
		normalEdgesLRTB.w = saturate((dot(pixelNormal, neighbourNormalB) + dotThreshold));
		
		edgesLRTB *= normalEdgesLRTB;
	}
	
	const float globalMipOffset = SSAO_DEPTH_MIPS_GLOBAL_OFFSET;
	float mipOffset = (qualityLevel < SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? (0) : (log2(pixLookupRadiusMod) + globalMipOffset);
	
	// Used to tilt the second set of samples so that the disk is effectively rotated by the normal
	// effective at removing one set of artifacts, but too expensive for lower quality settings
	float2 normXY = float2(pixelNormal.x, pixelNormal.y);
	float normXYLength = length(normXY);
	normXY /= float2(normXYLength, -normXYLength);
	normXYLength *= SSAO_TILT_SAMPLES_AMOUNT;
	
	const float3 negViewspaceDir = -normalize(pixCenterPos);
	
	// UNROLL // <- doesn't seem to help on any platform, although the compilers seem to unroll anyway if const number of tap used!
	for (int i = 0; i < numberOfTaps; i++)
	{
		SSAOTap(qualityLevel, obscuranceSum, weightSum, i, rotScale, pixCenterPos, negViewspaceDir, pixelNormal, normalizedScreenPos, mipOffset, falloffCalcMulSq, 1.0, normXY, normXYLength);
	}
	
	// Calculate weighted average
	float obscurance = obscuranceSum / weightSum;
	
	// Calculate fadeout (1 close, gradient, 0 far)
	float fadeOut = 1 - saturate(pixCenterPos.z * EffectFadeOutMul + EffectFadeOutAdd);
	
	// Calculate near cam fade (pixels near camera has some artifacts)
	fadeOut *= saturate(pixZ * EffectNearFadeMul + (-10.0f / 24.0f));
	
	// Reduce the SSAO shadowing if we're on the edge to remove artifacts on edges (we don't care for the lower quality one)
	if (qualityLevel >= SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET)
	{
		//float edgeCount = dot(1.0-edgesLRTB, float4(1.0, 1.0, 1.0, 1.0));
		
		// when there's more than 2 opposite edges, start fading out the occlusion to reduce aliasing artifacts
		float edgeFadeoutFactor = saturate((1.0 - edgesLRTB.x - edgesLRTB.y) * 0.35) + saturate((1.0 - edgesLRTB.z - edgesLRTB.w) * 0.35);
		
		// (experimental) if you want to reduce the effect next to any edge
		//edgeFadeoutFactor += 0.1 * saturate(dot(1 - edgesLRTB, float4(1, 1, 1, 1)));
		
		fadeOut *= saturate(1.0 - edgeFadeoutFactor);
	}
	
	// Same as a bove, but a lot more conservative version
	//fadeOut *= saturate(dot(edgesLRTB, float4(0.9, 0.9, 0.9, 0.9)) - 2.6);
	
	// Strength
	obscurance *= EffectShadowStrength;
	
	// Clamp
	obscurance = min(obscurance, 0.98f);
	
	// Apply fadeout
	obscurance *= fadeOut;
	
	// Conceptually switch to occlusion with the meaning being visibility (grows with visibility, occlusion == 1 implies full visibility), to be in line with what is more commonly used.
	float occlusion = 1.0 - obscurance;
	
	// Modify the gradient
	// Note: this cannot be moved to a later pass because of loss of precision after storing in the render target
	occlusion = pow(saturate(occlusion), EffectShadowPow);
	
	// Set outputs
	outShadowTerm = occlusion;    // Our final 'occlusion' term (0 means fully occluded, 1 means fully lit)
	outEdges = edgesLRTB;    // These are used to prevent blurring across edges, 1 means no edge, 0 means edge, 0.5 means half way there, etc.
	outWeight = weightSum;
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_GenerateQ0(in float4 inPos : SV_POSITION/*, in float2 inUV : TEXCOORD0*/, out float2 out0 : SV_Target0)
{
	float outShadowTerm;
	float outWeight;
	float4 outEdges;
	GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy/*, inUV*/, 0);
	out0.x = outShadowTerm;
	out0.y = PackEdges(float4(1, 1, 1, 1)); // No edges in low quality
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_GenerateQ1(in float4 inPos : SV_POSITION/*, in float2 inUV : TEXCOORD0*/, out float2 out0 : SV_Target0)
{
	float outShadowTerm;
	float outWeight;
	float4 outEdges;
	GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy/*, inUV*/, 1);
	out0.x = outShadowTerm;
	out0.y = PackEdges(outEdges);
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_GenerateQ2(in float4 inPos : SV_POSITION/*, in float2 inUV : TEXCOORD0*/, out float2 out0 : SV_Target0)
{
	float outShadowTerm;
	float outWeight;
	float4 outEdges;
	GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy/*, inUV*/, 2);
	out0.x = outShadowTerm;
	out0.y = PackEdges(outEdges);
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS_GenerateQ3(in float4 inPos : SV_POSITION/*, in float2 inUV : TEXCOORD0*/, out float2 out0 : SV_Target0)
{
	float outShadowTerm;
	float outWeight;
	float4 outEdges;
	GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy/*, inUV*/, 3);
	out0.x = outShadowTerm;
	out0.y = PackEdges(outEdges);
}

// Pixel shader that does smart blurring (to avoid bleeding)

void AddSample(float ssaoValue, float edgeValue, inout float sum, inout float sumWeight)
{
	float weight = edgeValue;
	sum += (weight * ssaoValue);
	sumWeight += weight;
}

float2 SampleBlurredWide(float4 inPos, float2 coord)
{
	float2 vC = g_BlurInput.SampleLevel(SamplerPointWrap, coord, 0.0, int2(0, 0)).xy;
	float2 vL = g_BlurInput.SampleLevel(SamplerPointWrap, coord, 0.0, int2(-2, 0)).xy;
	float2 vT = g_BlurInput.SampleLevel(SamplerPointWrap, coord, 0.0, int2(0, -2)).xy;
	float2 vR = g_BlurInput.SampleLevel(SamplerPointWrap, coord, 0.0, int2(2, 0)).xy;
	float2 vB = g_BlurInput.SampleLevel(SamplerPointWrap, coord, 0.0, int2(0, 2)).xy;

	float packedEdges = vC.y;
	float4 edgesLRTB = UnpackEdges(packedEdges);
	edgesLRTB.x *= UnpackEdges(vL.y).y;
	edgesLRTB.z *= UnpackEdges(vT.y).w;
	edgesLRTB.y *= UnpackEdges(vR.y).x;
	edgesLRTB.w *= UnpackEdges(vB.y).z;
	
	float ssaoValue = vC.x;
	float ssaoValueL = vL.x;
	float ssaoValueT = vT.x;
	float ssaoValueR = vR.x;
	float ssaoValueB = vB.x;
	
	float sumWeight = 0.8f;
	float sum = ssaoValue * sumWeight;
	
	AddSample(ssaoValueL, edgesLRTB.x, sum, sumWeight);
	AddSample(ssaoValueR, edgesLRTB.y, sum, sumWeight);
	AddSample(ssaoValueT, edgesLRTB.z, sum, sumWeight);
	AddSample(ssaoValueB, edgesLRTB.w, sum, sumWeight);
	
	float ssaoAvg = sum / sumWeight;
	
	ssaoValue = ssaoAvg; //min(ssaoValue, ssaoAvg) * 0.2 + ssaoAvg * 0.8;
	
	return float2(ssaoValue, packedEdges);
}

float2 SampleBlurred(float4 inPos, float2 coord)
{
	float packedEdges = g_BlurInput.Load(int3(inPos.xy, 0)).y;
	float4 edgesLRTB = UnpackEdges(packedEdges);
	
	float4 valuesUL = TextureGatherRed(g_BlurInput, SamplerPointWrap, coord - HalfViewportPixelSize * 0.5);
	float4 valuesBR = TextureGatherRed(g_BlurInput, SamplerPointWrap, coord + HalfViewportPixelSize * 0.5);
	
	float ssaoValue = valuesUL.y;
	float ssaoValueL = valuesUL.x;
	float ssaoValueT = valuesUL.z;
	float ssaoValueR = valuesBR.z;
	float ssaoValueB = valuesBR.x;
	
	float sumWeight = 0.5f;
	float sum = ssaoValue * sumWeight;
	
	AddSample(ssaoValueL, edgesLRTB.x, sum, sumWeight);
	AddSample(ssaoValueR, edgesLRTB.y, sum, sumWeight);
	AddSample(ssaoValueT, edgesLRTB.z, sum, sumWeight);
	AddSample(ssaoValueB, edgesLRTB.w, sum, sumWeight);
	
	float ssaoAvg = sum / sumWeight;
	
	ssaoValue = ssaoAvg; //min(ssaoValue, ssaoAvg) * 0.2 + ssaoAvg * 0.8;
	
	return float2(ssaoValue, packedEdges);
}

// Edge-sensitive blur
META_PS(true, FEATURE_LEVEL_ES2)
float2 PS_SmartBlur(in float4 inPos : SV_POSITION, in float2 inUV : TEXCOORD0) : SV_Target
{
	return SampleBlurred(inPos, inUV);
}

// Edge-sensitive blur (wider kernel)
META_PS(true, FEATURE_LEVEL_ES2)
float2 PS_SmartBlurWide(in float4 inPos : SV_POSITION, in float2 inUV : TEXCOORD0) : SV_Target
{
	return SampleBlurredWide(inPos, inUV);
}

// Edge-ignorant blur in x and y directions, 9 pixels touched (for the lowest quality level 0)
META_PS(true, FEATURE_LEVEL_ES2)
float2 PS_NonSmartBlur(in float4 inPos : SV_POSITION, in float2 inUV : TEXCOORD0) : SV_Target
{
	float2 halfPixel = HalfViewportPixelSize * 0.5f;
	
	float2 centre = g_BlurInput.SampleLevel(SamplerLinearClamp, inUV, 0.0).xy;
	
	float4 vals;
	vals.x = g_BlurInput.SampleLevel(SamplerLinearClamp, inUV + float2(-halfPixel.x * 3, -halfPixel.y), 0.0).x;
	vals.y = g_BlurInput.SampleLevel(SamplerLinearClamp, inUV + float2(+halfPixel.x, -halfPixel.y * 3), 0.0).x;
	vals.z = g_BlurInput.SampleLevel(SamplerLinearClamp, inUV + float2(-halfPixel.x, +halfPixel.y * 3), 0.0).x;
	vals.w = g_BlurInput.SampleLevel(SamplerLinearClamp, inUV + float2(+halfPixel.x * 3, +halfPixel.y), 0.0).x;
	
	return float2(dot(vals, 0.2.xxxx) + centre.x * 0.2, centre.y);
}

// Edge-ignorant blur & apply (for the lowest quality level 0)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Apply(in float4 inPos : SV_POSITION, in float2 inUV : TEXCOORD0) : SV_Target
{
	float a = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 0), 0.0).x;
	float b = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 1), 0.0).x;
	float c = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 2), 0.0).x;
	float d = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 3), 0.0).x;
	float avg = (a + b + c + d) * 0.25;
	
	// This must match GBuffer layout (material AO is in GBuffer0.a)
	return float4(1.0, 1.0, 1.0, avg);
}

// Edge-ignorant blur & apply, skipping half pixels in checkerboard pattern
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_ApplyHalf(in float4 inPos : SV_POSITION, in float2 inUV : TEXCOORD0) : SV_Target
{
	float a = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 0), 0.0).x;
	float d = g_FinalSSAO.SampleLevel(SamplerLinearClamp, float3(inUV.xy, 3), 0.0).x;
	float avg = (a + d) * 0.5;
	
	// This must match GBuffer layout (material AO is in GBuffer0.a)
	return float4(1.0, 1.0, 1.0, avg);
}
