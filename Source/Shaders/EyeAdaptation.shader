// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)

float MinBrightness;
float MaxBrightness;
float SpeedUp;
float SpeedDown;

float PreExposure; // exp2 done on CPU for PreExposure
float DeltaTime;
float HistogramMul;
float HistogramAdd;

float HistogramLowPercent;
float HistogramHighPercent;
float DropHistory;
float Dummy1;

META_CB_END

float AdaptLuminance(float currentLum, Texture2D previousLuminance)
{
	float previousLum = previousLuminance.Load(int3(0, 0, 0)).x;

	// Adapt the luminance using Pattanaik's technique
	float delta = currentLum - previousLum;
	float adaptionSpeed = delta > 0 ? SpeedUp : SpeedDown;
	float luminance = previousLum + delta * (1.0f - exp2(-DeltaTime * adaptionSpeed));
	luminance = lerp(luminance, currentLum, DropHistory);

	return clamp(luminance, MinBrightness, MaxBrightness);
}

#ifdef _PS_Manual

// Applies the exposure scale to the scene color (using multiply blending mode)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Manual(Quad_VS2PS input) : SV_Target
{
	return PreExposure.xxxx; 
}

#endif

#ifdef _PS_LuminanceMap

Texture2D SceneColor : register(t0);

// Creates the luminance map for the scene
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_LuminanceMap(Quad_VS2PS input) : SV_Target
{
	// Sample the input
	float3 color = SceneColor.Sample(SamplerLinearClamp, input.TexCoord).rgb;

	// Calculate the luminance
	float luminance = Luminance(color);

	// Clamp to avoid artifacts from exceeding fp16
	return clamp(luminance, 1e-4, 60000.0f).xxxx;
}

#endif

#ifdef _PS_BlendLuminance

Texture2D CurrentLuminance : register(t0);
Texture2D PreviousLuminance : register(t1);

// Slowly adjusts the scene luminance based on the previous scene luminance
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_BlendLuminance(Quad_VS2PS input) : SV_Target
{
	float currentLum = CurrentLuminance.Load(int3(0, 0, 0)).x;
	return AdaptLuminance(currentLum, PreviousLuminance).xxxx;
}

#endif

#ifdef _PS_ApplyLuminance

Texture2D AverageLuminance : register(t0);

// Applies the luminance to the scene color (using multiply blending mode)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_ApplyLuminance(Quad_VS2PS input) : SV_Target
{
	float averageLuminance = AverageLuminance.Load(int3(0, 0, 0)).x;
	float exposure = 1.0f / averageLuminance;
	return float4((PreExposure * exposure).xxx, 1);
}

#endif

#ifdef _PS_Histogram

// This define must match the C++
#define HISTOGRAM_SIZE 64

StructuredBuffer<uint> HistogramBuffer : register(t0);
Texture2D PreviousLuminance : register(t1);

float ComputeLuminanceFromHistogramPosition(float histogramPosition)
{
	return exp2((histogramPosition - HistogramAdd) / HistogramMul);
}

float GetAverageLuminance()
{
	// Find maximum and sum of all histogram values
	float maxValue = 0.0f;
	float totalSum = 0.0f;
	UNROLL
	for (uint i = 0; i < HISTOGRAM_SIZE; i++)
	{
		float value = (float)HistogramBuffer[i];
		maxValue = max(maxValue, value);
		totalSum += value;
	}

	// Compute average luminance within the given percentage of the histogram
	float2 fractionSum = float2(HistogramLowPercent, HistogramHighPercent) * totalSum;
	float2 runningSum = 0;
	UNROLL
	for (uint i = 0; i < HISTOGRAM_SIZE; i++)
	{
		float value = (float)HistogramBuffer[i];

		// Remove values at lower end
		float tmp = min(value, fractionSum.x);
		value -= tmp;
		fractionSum -= tmp;

		// Remove values at upper end
		value = min(value, fractionSum.y);
		fractionSum.y -= value;

		// Accumulate luminance at the histogram position
		float luminance = ComputeLuminanceFromHistogramPosition(i / (float)HISTOGRAM_SIZE);
		runningSum += float2(luminance, 1) * value;
	}

	return runningSum.x / max(runningSum.y, 0.0001f);
}

// Samples evaluates the scene color luminance based on the histogram
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Histogram(Quad_VS2PS input) : SV_Target
{
	float currentLum = GetAverageLuminance();
	return AdaptLuminance(currentLum, PreviousLuminance).xxxx;
}

#endif
