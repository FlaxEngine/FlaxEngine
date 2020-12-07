// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/GammaCorrectionCommon.hlsl"
#include "./Flax/ACES.hlsl"

#ifdef _PS_Lut3D
#define USE_VOLUME_LUT 1
#else
#define USE_VOLUME_LUT 0
#endif
static const float LUTSize = 32;

META_CB_BEGIN(0, Data)
float4 ColorSaturationShadows;
float4 ColorContrastShadows;
float4 ColorGammaShadows;
float4 ColorGainShadows;
float4 ColorOffsetShadows;

float4 ColorSaturationMidtones;
float4 ColorContrastMidtones;
float4 ColorGammaMidtones;
float4 ColorGainMidtones;
float4 ColorOffsetMidtones;

float4 ColorSaturationHighlights;
float4 ColorContrastHighlights;
float4 ColorGammaHighlights;
float4 ColorGainHighlights;
float4 ColorOffsetHighlights;

float ColorCorrectionShadowsMax;
float ColorCorrectionHighlightsMin;
float WhiteTemp;
float WhiteTint;

float3 Dummy;
float LutWeight;
META_CB_END

Texture2D LutTexture : register(t0);

// Accurate for 1000K < temp < 15000K
// [Krystek 1985, "An algorithm to calculate correlated colour temperature"]
float2 PlanckianLocusChromaticity(float temp)
{
	float u = ( 0.860117757f + 1.54118254e-4f * temp + 1.28641212e-7f * temp*temp ) / ( 1.0f + 8.42420235e-4f * temp + 7.08145163e-7f * temp*temp );
	float v = ( 0.317398726f + 4.22806245e-5f * temp + 4.20481691e-8f * temp*temp ) / ( 1.0f - 2.89741816e-5f * temp + 1.61456053e-7f * temp*temp );

	float x = 3*u / ( 2*u - 8*v + 4 );
	float y = 2*v / ( 2*u - 8*v + 4 );

	return float2(x,y);
}

// Accurate for 4000K < temp < 25000K
// in: correlated color temperature
// out: CIE 1931 chromaticity
float2 D_IlluminantChromaticity(float temp)
{
	// Correct for revision of Plank's law
	// This makes 6500 == D65
	temp *= 1.4388 / 1.438;

	float x =	temp <= 7000 ?
				0.244063 + ( 0.09911e3 + ( 2.9678e6 - 4.6070e9 / temp ) / temp ) / temp :
				0.237040 + ( 0.24748e3 + ( 1.9018e6 - 2.0064e9 / temp ) / temp ) / temp;
	
	float y = -3 * x*x + 2.87 * x - 0.275;

	return float2(x,y);
}

// Find closest color temperature to chromaticity
// [McCamy 1992, "Correlated color temperature as an explicit function of chromaticity coordinates"]
float CorrelatedColortemperature(float x, float y)
{
	float n = (x - 0.3320) / (0.1858 - y);
	return -449 * n*n*n + 3525 * n*n - 6823.3 * n + 5520.33;
}

float2 PlanckianIsothermal(float temp, float tint)
{
	float u = ( 0.860117757f + 1.54118254e-4f * temp + 1.28641212e-7f * temp*temp ) / ( 1.0f + 8.42420235e-4f * temp + 7.08145163e-7f * temp*temp );
	float v = ( 0.317398726f + 4.22806245e-5f * temp + 4.20481691e-8f * temp*temp ) / ( 1.0f - 2.89741816e-5f * temp + 1.61456053e-7f * temp*temp );

	float ud = ( -1.13758118e9f - 1.91615621e6f * temp - 1.53177f * temp*temp ) / Square( 1.41213984e6f + 1189.62f * temp + temp*temp );
	float vd = (  1.97471536e9f - 705674.0f * temp - 308.607f * temp*temp ) / Square( 6.19363586e6f - 179.456f * temp + temp*temp );

	float2 uvd = normalize( float2( u, v ) );

	// Correlated color temperature is meaningful within +/- 0.05
	u += -uvd.y * tint * 0.05;
	v +=  uvd.x * tint * 0.05;

	float x = 3*u / ( 2*u - 8*v + 4 );
	float y = 2*v / ( 2*u - 8*v + 4 );

	return float2(x,y);
}

float3 WhiteBalance(float3 linearColor)
{
	float2 srcWhiteDaylight = D_IlluminantChromaticity(WhiteTemp);
	float2 srcWhitePlankian = PlanckianLocusChromaticity(WhiteTemp);

	float2 srcWhite = WhiteTemp < 4000 ? srcWhitePlankian : srcWhiteDaylight;
	float2 d65White = float2(0.31270,  0.32900);

	// Offset along isotherm
	float2 isothermal = PlanckianIsothermal(WhiteTemp, WhiteTint) - srcWhitePlankian;
	srcWhite += isothermal;

	float3x3 whiteBalance = ChromaticAdaptation(srcWhite, d65White);
	whiteBalance = mul(XYZ_2_sRGB_MAT, mul(whiteBalance, sRGB_2_XYZ_MAT));

	return mul(whiteBalance, linearColor);
}

float3 ColorCorrect(float3 color, float luma, float4 saturation, float4 contrast, float4 gamma, float4 gain, float4 offset)
{
	color = max(0, lerp(luma.xxx, color, saturation.xyz * saturation.w));
	color = pow(color * (1.0 / 0.18), contrast.xyz * contrast.w) * 0.18;
	color = pow(color, 1.0 / (gamma.xyz * gamma.w));
	color = color * (gain.xyz * gain.w) + (offset.xyz + offset.w);
	return color;
}

float3 ColorCorrectAll(float3 color)
{
	float luma = dot(color, AP1_RGB2Y);

	// Shadow CC
	float3 ccColorShadows = ColorCorrect(color, luma,
		ColorSaturationShadows, 
		ColorContrastShadows, 
		ColorGammaShadows, 
		ColorGainShadows, 
		ColorOffsetShadows);
	float ccWeightShadows = 1 - smoothstep(0, ColorCorrectionShadowsMax, luma);

	// Highlight CC
	float3 ccColorHighlights = ColorCorrect(color, luma,
		ColorSaturationHighlights, 
		ColorContrastHighlights, 
		ColorGammaHighlights, 
		ColorGainHighlights, 
		ColorOffsetHighlights);
	float ccWeightHighlights = smoothstep(ColorCorrectionHighlightsMin, 1, luma);

	// Midtone CC
	float3 ccColorMidtones = ColorCorrect(color, luma,
		ColorSaturationMidtones, 
		ColorContrastMidtones, 
		ColorGammaMidtones, 
		ColorGainMidtones, 
		ColorOffsetMidtones);
	float ccWeightMidtones = 1 - ccWeightShadows - ccWeightHighlights;

	// Blend shadow, midtone and highlight CCs
	return ccColorShadows * ccWeightShadows + ccColorMidtones * ccWeightMidtones + ccColorHighlights * ccWeightHighlights;
}

float3 ColorGrade(float3 linearColor)
{
	// ACEScg working space
	const float3x3 sRGB_2_AP1 = mul(XYZ_2_AP1_MAT,  mul(D65_2_D60_CAT, sRGB_2_XYZ_MAT));
	const float3x3 AP1_2_sRGB = mul(XYZ_2_sRGB_MAT, mul(D60_2_D65_CAT, AP1_2_XYZ_MAT));
	float3 color = mul(sRGB_2_AP1, linearColor);

	// Nuke-style color correct
	color = ColorCorrectAll(color);

	// Convert back to linear color
	return mul(AP1_2_sRGB, color);
}

#ifdef TONE_MAPPING_MODE_NEUTRAL

// Neutral tonemapping (Hable/Hejl/Frostbite)
float3 NeutralCurve(float3 linearColor, float a, float b, float c, float d, float e, float f)
{
    return ((linearColor * (a * linearColor + c * b) + d * e) / (linearColor * (a * linearColor + b) + d * f)) - e / f;
}

float3 TonemapNeutral(float3 linearColor)
{
	// Neutral tonemapping
	const float a = 0.2;
	const float b = 0.29;
	const float c = 0.24;
	const float d = 0.272;
	const float e = 0.02;
	const float f = 0.3;
	const float whiteLevel = 5.3;
	const float whiteClip = 1.0;
	float3 whiteScale = (1.0).xxx / NeutralCurve(whiteLevel, a, b, c, d, e, f);
	linearColor = NeutralCurve(linearColor * whiteScale, a, b, c, d, e, f);
	linearColor *= whiteScale;
	return linearColor / whiteClip.xxx;
}

#endif

#ifdef TONE_MAPPING_MODE_ACES

float3 TonemapACES(float3 linearColor)
{
	// The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
	// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)

	// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
	static const float3x3 ACESInputMat =
	{
		{0.59719, 0.35458, 0.04823},
		{0.07600, 0.90834, 0.01566},
		{0.02840, 0.13383, 0.83777}
	};

	// ODT_SAT => XYZ => D60_2_D65 => sRGB
	static const float3x3 ACESOutputMat =
	{
		{ 1.60475, -0.53108, -0.07367},
		{-0.10208,  1.10813, -0.00605},
		{-0.00327, -0.07276,  1.07602}
	};

	float3 color = linearColor;
	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	float3 a = color * (color + 0.0245786f) - 0.000090537f;
	float3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
	color = a / b;

	color = mul(ACESOutputMat, color);
	return saturate(color);
}

#endif

// Perfoms the tonemapping on the input linear color
float3 Tonemap(float3 linearColor)
{
#if defined(TONE_MAPPING_MODE_NONE)
	return linearColor;
#elif defined(TONE_MAPPING_MODE_NEUTRAL)
	return TonemapNeutral(linearColor);
#elif defined(TONE_MAPPING_MODE_ACES)
	return TonemapACES(linearColor);
#else
	return float3(0, 0, 0);
#endif
}

float4 CombineLUTs(float2 uv, uint layerIndex)
{
	float3 encodedColor;
#if USE_VOLUME_LUT
	// Construct the neutral color from a 3d position volume texture	
	{
		uv = uv - float2(0.5f / LUTSize, 0.5f / LUTSize);
		encodedColor = float3(uv * LUTSize / (LUTSize - 1), layerIndex / (LUTSize - 1));
	}
#else
	// Construct the neutral color from a 2d position in 256x16
	{
		uv -= float2(0.49999f / (LUTSize * LUTSize), 0.49999f / LUTSize);

		float3 rgb;
		rgb.r = frac(uv.x * LUTSize);
		rgb.b = uv.x - rgb.r / LUTSize;
		rgb.g = uv.y;

		float scale = LUTSize / (LUTSize - 1);
		encodedColor = rgb * scale;
	}
#endif

	// Move from encoded LUT color space to linear color
	//float3 linearColor = encodedColor.rgb; // Default
	//float3 linearColor = LogCToLinear(encodedColor.rgb); // LogC
	float3 linearColor = LogToLinear(encodedColor.rgb) - LogToLinear(0); // Log

	// Apply white balance
	linearColor = WhiteBalance(linearColor);

	// Apply color grading
	linearColor = ColorGrade(linearColor);

	// Apply tone mapping
	float3 color = Tonemap(linearColor);

	// Apply LDR LUT color grading
	{
		float3 uvw = color * (15.0 / 16.0) + (0.5f / 16.0);
		float3 lutColor = SampleUnwrappedTexture3D(LutTexture, SamplerLinearClamp, uvw, 16).rgb;
		color = lerp(color, lutColor, LutWeight);
	}

	return float4(color, 1);
}

// Vertex shader that writes to a range of slices of a volume texture
META_VS(true, FEATURE_LEVEL_SM4)
META_FLAG(VertexToGeometryShader)
META_VS_IN_ELEMENT(POSITION, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
Quad_VS2GS VS_WriteToSlice(float2 Position : POSITION0, float2 TexCoord : TEXCOORD0, uint LayerIndex : SV_InstanceID)
{
	Quad_VS2GS output;

	output.Vertex.Position = float4(Position, 0, 1);
	output.Vertex.TexCoord = TexCoord;
	output.LayerIndex = LayerIndex;

	return output;
}

// Geometry shader that writes to a range of slices of a volume texture
META_GS(true, FEATURE_LEVEL_SM4)
[maxvertexcount(3)]
void GS_WriteToSlice(triangle Quad_VS2GS input[3], inout TriangleStream<Quad_GS2PS> OutStream)
{
	Quad_GS2PS vertex0;
	vertex0.Vertex = input[0].Vertex;
	vertex0.LayerIndex = input[0].LayerIndex;

	Quad_GS2PS vertex1;
	vertex1.Vertex = input[1].Vertex;
	vertex1.LayerIndex = input[1].LayerIndex;

	Quad_GS2PS vertex2;
	vertex2.Vertex = input[2].Vertex;
	vertex2.LayerIndex = input[2].LayerIndex;

	OutStream.Append(vertex0);
	OutStream.Append(vertex1);
	OutStream.Append(vertex2);
}

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(TONE_MAPPING_MODE_NONE=1)
META_PERMUTATION_1(TONE_MAPPING_MODE_NEUTRAL=1)
META_PERMUTATION_1(TONE_MAPPING_MODE_ACES=1)
float4 PS_Lut2D(Quad_VS2PS input) : SV_Target
{
	return CombineLUTs(input.TexCoord, 0);
}

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(TONE_MAPPING_MODE_NONE=1)
META_PERMUTATION_1(TONE_MAPPING_MODE_NEUTRAL=1)
META_PERMUTATION_1(TONE_MAPPING_MODE_ACES=1)
float4 PS_Lut3D(Quad_GS2PS input) : SV_Target
{
	return CombineLUTs(input.Vertex.TexCoord, input.LayerIndex);
}
