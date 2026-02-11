// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

// Those defines must match the C++
#define THREADGROUP_SIZE_X 16
#define THREADGROUP_SIZE_Y 16
#define HISTOGRAM_SIZE 64

META_CB_BEGIN(0, Data)

uint2 InputSize;
float HistogramMul;
float HistogramAdd;

META_CB_END

#ifdef _CS_ClearHistogram

RWStructuredBuffer<uint> HistogramBuffer : register(u0);

// Clears the histogram
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREADGROUP_SIZE_X, 1, 1)]
void CS_ClearHistogram(uint dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId < HISTOGRAM_SIZE)
		HistogramBuffer[dispatchThreadId] = 0u;
}

#endif

#ifdef _CS_GenerateHistogram

Texture2D<float4> Input : register(t0);

RWStructuredBuffer<uint> HistogramBuffer : register(u0);

float ComputeHistogramPositionFromLuminance(float luminance)
{
	return log2(luminance) * HistogramMul + HistogramAdd;
}

groupshared uint SharedHistogram[HISTOGRAM_SIZE];

// Generates the histogram
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(THREADGROUP_SIZE_X, THREADGROUP_SIZE_Y, 1)]
void CS_GenerateHistogram(uint3 dispatchThreadId : SV_DispatchThreadID, uint3 groupThreadId : SV_GroupThreadID)
{
	const uint threadId = groupThreadId.y * THREADGROUP_SIZE_X + groupThreadId.x;

	// Clear the histogram
	if (threadId < HISTOGRAM_SIZE)
		SharedHistogram[threadId] = 0u;

	GroupMemoryBarrierWithGroupSync();

	// Gather local group histogram
	if (dispatchThreadId.x < InputSize.x && dispatchThreadId.y < InputSize.y)
	{
#if 1
        // Screen vignette to put more weight into samples in the center of the image
        float2 uv = (float2)dispatchThreadId.xy / float2(InputSize.x, InputSize.y);
        float2 center = abs(uv - float2(0.5f, 0.5f));
        float scale = saturate(1 - dot(center, center));
        uint weight = (uint)(scale * scale * 64.0f);
#else
		uint weight = 1u;
#endif
		float3 color = Input[dispatchThreadId.xy].xyz;
		float luminance = Luminance(color);
		float logLuminance = ComputeHistogramPositionFromLuminance(luminance);
		uint idx = (uint)(saturate(logLuminance) * (HISTOGRAM_SIZE - 1u));
		InterlockedAdd(SharedHistogram[idx], weight);
	}

	GroupMemoryBarrierWithGroupSync();

	// Merge everything
	if (threadId < HISTOGRAM_SIZE)
		InterlockedAdd(HistogramBuffer[threadId], SharedHistogram[threadId]);
}

#endif
