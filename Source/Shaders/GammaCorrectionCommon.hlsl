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

half3 LinearTo709Branchless(half3 linearColor) 
{
	linearColor = max(6.10352e-5, linearColor);
	return min(linearColor * 4.5, pow(max(linearColor, 0.018), 0.45) * 1.099 - 0.099);
}

half3 LinearToSrgbBranchless(half3 linearColor) 
{
	linearColor = max(6.10352e-5, linearColor);
	return min(linearColor * 12.92, pow(max(linearColor, 0.00313067), 1.0/2.4) * 1.055 - 0.055);
}

half LinearToSrgbBranchingChannel(half linearColor) 
{
	if (linearColor < 0.00313067)
		return linearColor * 12.92;
	return pow(linearColor, (1.0/2.4)) * 1.055 - 0.055;
}

half3 LinearToSrgbBranching(half3 linearColor) 
{
	return half3(
		LinearToSrgbBranchingChannel(linearColor.r),
		LinearToSrgbBranchingChannel(linearColor.g),
		LinearToSrgbBranchingChannel(linearColor.b));
}

half3 LinearToSrgb(half3 linearColor) 
{
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM4
	// Branching is faster than branchless on PC
	return LinearToSrgbBranching(linearColor);
#else
	// Use branchless on mobile
	return LinearToSrgbBranchless(linearColor);
#endif
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
	float3 logColor = log2(linearColor) / linearRange - log2(linearGrey) / linearRange + exposureGrey / 1023.0;	// scalar: 3log2 3mad
	//float3 logColor = (log2(linearColor) - log2(linearGrey)) / linearRange + exposureGrey / 1023.0;
	//float3 logColor = log2(linearColor / linearGrey) / linearRange + exposureGrey / 1023.0;
	//float3 logColor = (0.432699 * log10(0.5 * linearColor + 0.037584) + 0.616596) + 0.03; // SLog
	//float3 logColor = (300 * log10(linearColor * (1 - 0.0108) + 0.0108) + 685) / 1023; // Cineon
	logColor = saturate(logColor);

	return logColor;
}

// Alexa LogC converters (El 1000)
// See http://www.vocas.nl/webfm_send/964
// Max range is ~58.85666

struct ParamsLogC
{
    float cut;
    float a, b, c, d, e, f;
};

static const ParamsLogC LogC =
{
    0.011361, // cut
    5.555556, // a
    0.047996, // b
    0.244161, // c
    0.386036, // d
    5.301883, // e
    0.092819  // f
};

float3 LinearToLogC(float3 linearColor)
{
    return LogC.c * log10(LogC.a * linearColor + LogC.b) + LogC.d;
}

float3 LogCToLinear(float3 logcColor)
{
    return (pow(10.0, (logcColor - LogC.d) / LogC.c) - LogC.b) / LogC.a;
}

// Dolby PQ transforms
float3 ST2084ToLinear(float3 pq)
{
	const float m1 = 0.1593017578125; // = 2610. / 4096. * .25;
	const float m2 = 78.84375; // = 2523. / 4096. *  128;
	const float c1 = 0.8359375; // = 2392. / 4096. * 32 - 2413./4096.*32 + 1;
	const float c2 = 18.8515625; // = 2413. / 4096. * 32;
	const float c3 = 18.6875; // = 2392. / 4096. * 32;
	const float C = 10000.;

	float3 Np = pow(pq, 1.0 / m2);
	float3 L = Np - c1;
	L = max(0.0, L);
	L = L / (c2 - c3 * Np);
	L = pow(L, 1.0 / m1);
	float3 P = L * C;

	return P;
}

float3 LinearToST2084(float3 lin)
{
	const float m1 = 0.1593017578125; // = 2610. / 4096. * .25;
	const float m2 = 78.84375; // = 2523. / 4096. *  128;
	const float c1 = 0.8359375; // = 2392. / 4096. * 32 - 2413./4096.*32 + 1;
	const float c2 = 18.8515625; // = 2413. / 4096. * 32;
	const float c3 = 18.6875; // = 2392. / 4096. * 32;
	const float C = 10000.0;

	float3 L = lin / C;
	float3 Lm = pow(L, m1);
	float3 N1 = (c1 + c2 * Lm);
	float3 N2 = (1.0 + c3 * Lm);
	float3 N = N1 * rcp(N2);
	float3 P = pow(N, m2);

	return P;
}

#endif
