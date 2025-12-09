// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/GUICommon.hlsl"

#ifndef BLUR_V
#define BLUR_V 0
#endif

META_CB_BEGIN(0, Data)
float4x4 ViewProjection;
META_CB_END

#define MAX_SAMPLES 64

META_CB_BEGIN(1, BlurData)
float2 InvBufferSize;
uint SampleCount;
float Dummy0;
float4 Bounds;
float4 WeightAndOffsets[MAX_SAMPLES / 2];
META_CB_END

Texture2D Image : register(t0);

META_VS(true, FEATURE_LEVEL_ES2)
VS2PS VS(Render2DVertex input)
{
	VS2PS output;

	// Render2D::RenderingFeatures::VertexSnapping
	if ((int)input.CustomDataAndClipOrigin.y & 1)
		input.Position = (float2)(int2)input.Position;

	output.Position = mul(float4(input.Position, 0, 1), ViewProjection);
	output.Color = input.Color;
	output.TexCoord = input.TexCoord;
	output.ClipOriginAndPos = float4(input.CustomDataAndClipOrigin.zw, input.Position);
	output.ClipExtents = input.ClipExtents;
	output.CustomData = input.CustomDataAndClipOrigin.xy;
    output.CustomData2 = input.CustomData2;

	return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Image(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	return Image.Sample(SamplerLinearClamp, input.TexCoord) * input.Color;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Circle(VS2PS input) : SV_Target0
{
    PerformClipping(input);

    float2 displacement = input.TexCoord - float2(0.5, 0.5);

    float distance = length(displacement);

    float radius = input.CustomData2.x;

    float width = fwidth(distance);
    float opacity = saturate(1.0 - (distance - radius) / width);
   
    return float4(input.Color.rgb, input.Color.a * opacity);
}

// SDF for Box with 4 radii
// p: centered position
// b: half-size of box
// r: radii (TL, TR, BR, BL)
float sdRoundedBox(float2 p, float2 b, float4 r)
{
    // Select X side radii pair
    // If Right (x>0): use yz (TR, BR)
    // If Left  (x<0): use xw (TL, BL)
    float2 side = (p.x > 0.0) ? r.yz : r.xw;
    
    // Select Y specific radius from pair
    // If Bottom (y>0): use side.y
    // If Top    (y<0): use side.x
    float radius = (p.y > 0.0) ? side.y : side.x;

    float2 q = abs(p) - b + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_RoundedRect(VS2PS input) : SV_Target0
{
    PerformClipping(input);

    // Unpack custom data
    const float multiplier = 4096.0;

    // X: TL + TR * 4096
    float packedTop = input.CustomData2.x;
    float rTR = floor(packedTop / multiplier);
    float rTL = packedTop - rTR * multiplier;
    
    // Y: (BL + BR * 4096) * 2
    float packedBottom = floor(input.CustomData2.y * 0.5);
    float rBR = floor(packedBottom / multiplier);
    float rBL = packedBottom - rBR * multiplier;

    // Format: TL, TR, BR, BL
    float4 radii = float4(rTL, rTR, rBR, rBL);

    float2 rectSize = input.CustomData2.zw;
    float2 halfSize = rectSize * 0.5;

    float maxRadius = min(halfSize.x, halfSize.y);
    float4 clampedRadii = clamp(radii, 0.0, maxRadius);

    float2 p = (input.TexCoord * rectSize) - halfSize;
    float dist = sdRoundedBox(p, halfSize, clampedRadii);

    float aaf = fwidth(dist);
    float opacity = 1.0 - smoothstep(-aaf, aaf, dist);

    return float4(input.Color.rgb, input.Color.a * opacity);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_ImagePoint(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	return Image.Sample(SamplerPointClamp, input.TexCoord) * input.Color;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Color(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	return input.Color;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_LineAA(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	float lineWidth = input.CustomData.x; // Set per-vertex
	float filterWidthScale = 1; // Must match C++ code
	float gradient = input.TexCoord.x;
	float2 gradientDerivative = float2(abs(ddx(gradient)), abs(ddy(gradient)));
	float pixelSizeInUV = sqrt(dot(gradientDerivative, gradientDerivative));
	float halfLineWidthUV = 0.5f * pixelSizeInUV * lineWidth;
	float halfFilterWidthUV = filterWidthScale * pixelSizeInUV;
	float distanceToLineCenter = abs(0.5f - gradient);
	float lineCoverage = smoothstep(halfLineWidthUV + halfFilterWidthUV, halfLineWidthUV - halfFilterWidthUV, distanceToLineCenter);
	if (lineCoverage <= 0.0f)
		discard;
	input.Color.a *= lineCoverage;

	return input.Color;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Font(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	float4 color = input.Color;
	color.a *= Image.Sample(SamplerLinearClamp, input.TexCoord).r;
	return color;
}

float4 GetSample(float weight, float offset, float2 uv)
{
#if BLUR_V
	const float2 uvOffset = float2(offset * InvBufferSize.x, 0);
#else
	const float2 uvOffset = float2(0, offset * InvBufferSize.y);
#endif

	return Image.Sample(SamplerLinearClamp, uv + uvOffset) * weight
	     + Image.Sample(SamplerLinearClamp, uv - uvOffset) * weight;
}

// 1:-1 to 0:1
float2 ClipToUv(float2 clipPos)
{
	return clipPos * float2(0.5, -0.5) + float2(0.5, 0.5);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Downscale(Quad_VS2PS input) : SV_Target0
{
	float2 boundsPos = input.TexCoord * Bounds.zw + Bounds.xy;

	float4 clipPos = mul(float4(boundsPos, 0, 1), ViewProjection);
	clipPos.xy /= clipPos.w;

	float2 uvPos = ClipToUv(clipPos.xy);

	// TODO: use 4-tap box blur to reduce aliasing when downscaling

	return Image.Sample(SamplerLinearClamp, uvPos);
}

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(BLUR_V=0)
META_PERMUTATION_1(BLUR_V=1)
float4 PS_Blur(Quad_VS2PS input) : SV_Target0
{
	float2 uv = input.TexCoord;
	float4 result = Image.Sample(SamplerLinearClamp, uv) * WeightAndOffsets[0].x;

	{
		float weight = WeightAndOffsets[0].z;
		float offset = WeightAndOffsets[0].w;
		result += GetSample(weight, offset, uv);
	}

	for (uint i = 2; i < SampleCount; i += 2)
	{
		uint index = i / 2;
		{
			float weight = WeightAndOffsets[index].x;
			float offset = WeightAndOffsets[index].y;
			result += GetSample(weight, offset, uv);
		}

		{
			float weight = WeightAndOffsets[index].z;
			float offset = WeightAndOffsets[index].w;
			result += GetSample(weight, offset, uv);
		}
	}

	return float4(result.rgb, 1);
}
