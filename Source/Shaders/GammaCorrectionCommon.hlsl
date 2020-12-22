// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __GAMMA_CORRECTION_COMMON__
#define __GAMMA_CORRECTION_COMMON__

#include "./Flax/Math.hlsl"

// Fast reversible tonemapper
// http://gpuopen.com/optimized-reversible-tonemapper-for-resolve/
float3 FastTonemap(float3 c)
{
	return c * rcp(max(max(c.r, c.g), c.b) + 1.0);
}

float4 FastTonemap(float4 c)
{
	return float4(FastTonemap(c.rgb), c.a);
}

float3 FastTonemap(float3 c, float w)
{
	return c * (w * rcp(max(max(c.r, c.g), c.b) + 1.0));
}

float4 FastTonemap(float4 c, float w)
{
	return float4(FastTonemap(c.rgb, w), c.a);
}

float3 FastTonemapInvert(float3 c)
{
	return c * rcp(1.0 - max(max(c.r, c.g), c.b));
}

float4 FastTonemapInvert(float4 c)
{
	return float4(FastTonemapInvert(c.rgb), c.a);
}

half LinearToSrgbChannel(half linearColor)
{
	if (linearColor < 0.00313067)
		return linearColor * 12.92;
	return pow(linearColor, (1.0 / 2.4)) * 1.055 - 0.055;
}

half3 LinearToSrgb(half3 linearColor)
{
	return half3(
		LinearToSrgbChannel(linearColor.r),
		LinearToSrgbChannel(linearColor.g),
		LinearToSrgbChannel(linearColor.b));
}

half3 sRGBToLinear(half3 color)
{
	color = max(6.10352e-5, color);
	return color > 0.04045 ? pow(color * (1.0 / 1.055) + 0.0521327, 2.4) : color * (1.0 / 12.92);
}

float3 LogToLinear(float3 logColor)
{
	const float linearRange = 14;
	const float linearGrey = 0.18;
	const float exposureGrey = 444;

	// Using stripped down, 'pure log', formula. Parameterized by grey points and dynamic range covered.
	float3 linearColor = exp2((logColor - exposureGrey / 1023.0) * linearRange) * linearGrey;
	//float3 linearColor = 2 * (pow(10.0, ((logColor - 0.616596 - 0.03) / 0.432699)) - 0.037584); // SLog
	//float3 linearColor = (pow( 10, (1023 * logColor - 685) / 300) - 0.0108) / (1 - 0.0108); // Cineon
	//linearColor = max(0, linearColor);

	return linearColor;
}

float3 LinearToLog(float3 linearColor)
{
	const float linearRange = 14;
	const float linearGrey = 0.18;
	const float exposureGrey = 444;

	// Using stripped down, 'pure log', formula. Parameterized by grey points and dynamic range covered.
	float3 logColor = log2(linearColor) / linearRange - log2(linearGrey) / linearRange + exposureGrey / 1023.0; // scalar: 3log2 3mad
	//float3 logColor = (log2(linearColor) - log2(linearGrey)) / linearRange + exposureGrey / 1023.0;
	//float3 logColor = log2(linearColor / linearGrey) / linearRange + exposureGrey / 1023.0;
	//float3 logColor = (0.432699 * log10(0.5 * linearColor + 0.037584) + 0.616596) + 0.03; // SLog
	//float3 logColor = (300 * log10(linearColor * (1 - 0.0108) + 0.0108) + 685) / 1023; // Cineon
	logColor = saturate(logColor);

	return logColor;
}

#endif
