// Copyright (c) Wojciech Figat. All rights reserved.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"

// Motion blur implementation based on:
// Jimenez, 2014, http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
// Chapman, 2013, http://john-chapman-graphics.blogspot.com/2013/01/per-object-motion-blur.html
// McGuire et. al., 2012, "A reconstruction filter for plausible motion blur"

META_CB_BEGIN(0, Data)
GBufferData GBuffer;
float4x4 CurrentVP;
float4x4 PreviousVP;
float4 TemporalAAJitter;
float VelocityScale;
float Dummy0;
int MaxBlurSamples;
uint VariableTileLoopCount;
float2 Input0SizeInv;
float2 Input2SizeInv;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Input0 : register(t0);
Texture2D Input1 : register(t1);
Texture2D Input2 : register(t2);
Texture2D Input3 : register(t3);

// Pixel shader for camera motion vectors
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CameraMotionVectors(Quad_VS2PS input) : SV_Target
{
	// Get the pixel world space position
	float deviceDepth = SAMPLE_RT(Input0, input.TexCoord).r;
	GBufferData gBufferData = GetGBufferData();
	float4 worldPos = float4(GetWorldPos(gBufferData, input.TexCoord, deviceDepth), 1);

	float4 prevClipPos = mul(worldPos, PreviousVP);
	float4 curClipPos = mul(worldPos, CurrentVP);
	float2 prevHPos = prevClipPos.xy / prevClipPos.w;
	float2 curHPos = curClipPos.xy / curClipPos.w;

	// Revert temporal jitter offset
	prevHPos -= TemporalAAJitter.zw;
	curHPos -= TemporalAAJitter.xy;

	// Clip Space -> UV Space
	float2 vPosPrev = prevHPos.xy * 0.5f + 0.5f;
	float2 vPosCur = curHPos.xy * 0.5f + 0.5f;
	vPosPrev.y = 1.0 - vPosPrev.y;
	vPosCur.y = 1.0 - vPosCur.y;

	return float4(vPosCur - vPosPrev, 0, 1);
}

// Calculates the color for the a motion vector debugging
float4 MotionVectorToColor(float2 v)
{
    float angle = atan2(v.y, v.x);
    float hue = (angle * (1.0f / PI) + 1.0f) * 0.5f;
    return saturate(float4(abs(hue * 6.0f - 3.0f) - 1.0f, 2.0f - abs(hue * 6.0f - 4.0f), 2.0f - abs(hue * 6.0f - 2.0f), length(v)));
}

// Pixel shader for motion vectors debug view
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MotionVectorsDebug(Quad_VS2PS input) : SV_Target
{
	float4 c = SAMPLE_RT(Input0, input.TexCoord);
    float2 v = SAMPLE_RT(Input1, input.TexCoord).xy * 20.0f;
    float4 vC = MotionVectorToColor(v);
    return float4(lerp(c.rgb, vC.rgb, vC.a * 0.6f), c.a);
}

// Returns the longer velocity vector
float2 maxV(float2 a, float2 b)
{
	// Use squared length for branch
	return dot(a, a) > dot(b, b) ? a : b;
}

// Pixel shader for motion vectors downscale with maximum velocity extraction (2x2 kernel)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax(Quad_VS2PS input) : SV_Target
{
	// Reference: [McGuire 2012] (2.3 Filter Passes)
	float4 offset = Input0SizeInv.xyxy * float4(-1, -1, 1, 1);
	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + offset.xy).xy;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + offset.xw).xy;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + offset.zy).xy;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + offset.zw).xy;
	return float4(maxV(maxV(maxV(v1, v2), v3), v4), 0, 0);
}

// Pixel shader for motion vectors downscale with maximum velocity extraction (NxN kernel)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMaxVariable(Quad_VS2PS input) : SV_Target
{
	// Reference: [McGuire 2012] (2.3 Filter Passes)
	float2 result = float2(0, 0);
	LOOP
	for (uint x = 0; x < VariableTileLoopCount; x++)
	{
		LOOP
		for (uint y = 0; y < VariableTileLoopCount; y++)
		{
			float2 v = SAMPLE_RT(Input0, input.TexCoord + Input0SizeInv * float2(x, y)).xy;
			result = maxV(result, v);
		}
	}
	return float4(result, 0, 0);
}

// Pixel shader for motion vectors tiles maximum neighbors velocities extraction (3x3 kernel)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_NeighborMax(Quad_VS2PS input) : SV_Target
{
	// Reference: [McGuire 2012] (2.3 Filter Passes)
	float2 result = float2(0, 0);
	UNROLL
	for (int x = -1; x <= 1; x++)
	{
		UNROLL
		for (int y = -1; y <= 1; y++)
		{
			float2 v = SAMPLE_RT(Input0, input.TexCoord + Input0SizeInv * float2(x, y)).xy;
			result = maxV(result, v);
		}
	}
	return float4(result, 0, 0);
}

float2 ClampVelocity(float2 v)
{
	// Prevent too big blur over the screen
	float velocityLimit = 0.2f;
	return clamp(v * VelocityScale, -velocityLimit, velocityLimit);
}

// [Jimenez, 2014]
float2 DepthCmp(float centerDepth, float sampleDepth, float depthScale)
{
	return saturate(0.5f + float2(depthScale, -depthScale) * (sampleDepth - centerDepth));
}

// [Jimenez, 2014]
float2 SpreadCmp(float offsetLen, float2 spreadLen, float pixelToSampleUnitsScale)
{
	return saturate(pixelToSampleUnitsScale * spreadLen - offsetLen + 1.0f);
	//return saturate(pixelToSampleUnitsScale * spreadLen - max(offsetLen - 1.0f, 0));
}

// [Jimenez, 2014]
float SampleWeight(float centerDepth, float sampleDepth, float offsetLen, float centerSpreadLen, float sampleSpreadLen, float pixelToSampleUnitsScale, float depthScale)
{
	float2 depthCmp = DepthCmp(centerDepth, sampleDepth, depthScale);
	float2 spreadCmp = SpreadCmp(offsetLen, float2(centerSpreadLen, sampleSpreadLen), pixelToSampleUnitsScale);
	return dot(depthCmp, spreadCmp);
}

// [Jimenez, 2014]
float FullscreenGradientNoise(float2 uv)
{
    uv = floor(uv * GBuffer.ScreenSize.xy);
    float f = dot(float2(0.06711056f, 0.00583715f), uv);
    return frac(52.9829189f * frac(f));
}

float2 NeighborMaxJitter(float2 uv)
{
	// Reduce max velocity tiles visibility by applying some jitter and noise to the uvs
	float rx, ry;
	float noise = FullscreenGradientNoise(uv + float2(2.0f, 0.0f)) * (PI * 2);
	sincos(noise, ry, rx);
	return float2(rx, ry) * Input2SizeInv * 0.25f;
}

// Pixel shader for motion blur rendering
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MotionBlur(Quad_VS2PS input) : SV_Target
{
	// Reference: [McGuire 2012, 2013], [Jimenez, 2014]

	// Sample pixel color
	float4 pixelColor = SAMPLE_RT(Input0, input.TexCoord);

	// Sample largest velocity in the neighborhood
	float2 neighborhoodVelocity = Input2.SampleLevel(SamplerLinearClamp, input.TexCoord + NeighborMaxJitter(input.TexCoord), 0).xy;
	neighborhoodVelocity = ClampVelocity(neighborhoodVelocity);
	float neighborhoodVelocityLength = length(neighborhoodVelocity);
	int neighborhoodVelocityPixelsLength = (int)length(neighborhoodVelocity * GBuffer.ScreenSize.xy);
	if (neighborhoodVelocityPixelsLength <= 1)
		return pixelColor;

	// Sample pixel velocity
	float2 pixelVelocity = Input1.SampleLevel(SamplerLinearClamp, input.TexCoord, 0).xy;
	pixelVelocity = ClampVelocity(pixelVelocity);
	float pixelVelocityLength = length(pixelVelocity);

	// Sample pixel depth
	GBufferData gBufferData = GetGBufferData();
	float pixelDepth = LinearizeZ(gBufferData, SAMPLE_RT(Input3, input.TexCoord).x);

	// Calculate noise to make it look better with less samples per pixel
	float noise = FullscreenGradientNoise(input.TexCoord);

	// Accumulate color using evenly placed filter taps along maximum neighborhood velocity direction
	float2 direction = neighborhoodVelocity;
	//float2 direction = pixelVelocity;
	uint sampleCount = MaxBlurSamples;
	float pixelToSampleUnitsScale = sampleCount * rsqrt(dot(direction, direction));
	float4 sum = 0;
	LOOP
	for (uint i = 0; i < sampleCount; i++)
	{
		float2 samplePos = float2(noise - 0.5f, 0.5f - noise) + ((float)i + 0.5f);
		float2 samplePosNormalized = samplePos / sampleCount;

		float2 sampleUV1 = input.TexCoord + samplePosNormalized.x * direction;
		float2 sampleUV2 = input.TexCoord - samplePosNormalized.y * direction;

		// TODO: use cheaper version if neighborhood min and max are almost equal (then calc min value too)
#if 0
		float weight1 = 1;
		float weight2 = 1;
#else
		float depth1 = LinearizeZ(gBufferData, SAMPLE_RT(Input3, sampleUV1).x);
		float2 velocity1 = Input1.SampleLevel(SamplerPointClamp, sampleUV1, 0).xy;
		velocity1 = ClampVelocity(velocity1);
		float velocityLength1 = length(velocity1);

		float depth2 = LinearizeZ(gBufferData, SAMPLE_RT(Input3, sampleUV2).x);
		float2 velocity2 = Input1.SampleLevel(SamplerPointClamp, sampleUV2, 0).xy;
		velocity2 = ClampVelocity(velocity2);
		float velocityLength2 = length(velocity2);

		float weight1 = SampleWeight(pixelDepth, depth1, samplePos.x, pixelVelocityLength, velocityLength1, pixelToSampleUnitsScale, 1);
		float weight2 = SampleWeight(pixelDepth, depth2, samplePos.x, pixelVelocityLength, velocityLength2, pixelToSampleUnitsScale, 1);

		bool2 mirror = bool2(depth1 > depth2, velocityLength2 > velocityLength1);
		weight1 = all(mirror) ? weight2 : weight1;
		weight2 = any(mirror) ? weight2 : weight1;
#endif

		sum += weight1 * float4(SAMPLE_RT(Input0, sampleUV1).rgb, 1);
		sum += weight2 * float4(SAMPLE_RT(Input0, sampleUV2).rgb, 1);
	}

	// Normalize result
	sum *= 0.5f / sampleCount;

	// Blend result with background
	return float4(sum.rgb + (1 - sum.w) * pixelColor.rgb, pixelColor.a);
}
