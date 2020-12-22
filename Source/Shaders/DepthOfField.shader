// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

// Implementation based on:
//	Bokeh Sample by MJP
//  http://mynameismjp.wordpress.com/
//  Licensed under Microsoft Public License (Ms-PL)

#include "./Flax/Common.hlsl"

// This must match C++ defines
#define DOF_MAX_SAMPLE_RADIUS 10
#define DOF_GRID_SIZE 450
#define DOF_APRON_SIZE DOF_MAX_SAMPLE_RADIUS
#define DOF_THREAD_GROUP_SIZE (DOF_GRID_SIZE + (DOF_APRON_SIZE * 2))

// Some configuration
#define USE_CS_HALF_PIXEL_OFFSET 1
#define USE_CS_LINEAR_SAMPLING 0

META_CB_BEGIN(0, DofData)

float2 ProjectionAB;
float BokehDepthCullThreshold;
float BokehDepthCutoff;

float4 DOFDepths;

float MaxBokehSize;
float BokehBrightnessThreshold;
float BokehBlurThreshold;
float BokehFalloff;

float2 BokehTargetSize;
float2 DOFTargetSize;

float2 InputSize;
float DepthLimit;
float BlurStrength;

float3 Dummy;
float BokehBrightness;
META_CB_END

static const float2 Offsets[4] =
{
	float2(-1, 1),
	float2(1, 1),
	float2(-1, -1),
	float2(1, -1)
};

static const float2 TexCoords[4] =
{
	float2(0, 0),
	float2(1, 0),
	float2(0, 1),
	float2(1, 1)
};

struct BokehVSOutput
{
	float2 Position : POSITION;
	float2 Size : SIZE;
	float3 Color : COLOR;
	float Depth : DEPTH;
};

struct BokehGSOutput
{
	float4 PositionCS : SV_Position;
	float2 TexCoord : TEXCOORD;
	float3 Color : COLOR;
	float Depth : DEPTH;
};

struct BokehPSInput
{
	float4 PositionSS : SV_Position;
	float2 TexCoord : TEXCOORD;
	float3 Color : COLOR;
	float Depth : DEPTH;
};

// Structure used for outputing bokeh points to an AppendStructuredBuffer
struct BokehPoint
{
	float3 Position;
	float Blur;
	float3 Color;
};

Texture2D Input0 : register(t0);
Texture2D Input1 : register(t1);

// Converts z-buffer depth to linear view-space depth
float LinearDepth(in float zBufferDepth)
{
	return ProjectionAB.y / (zBufferDepth - ProjectionAB.x);
}

// Computes the depth of field blur factor
float BlurFactor(float depth)
{
    float f0 = 1.0f - saturate((depth - DOFDepths.x) / max(DOFDepths.y - DOFDepths.x, 0.01f));
    float f1 = saturate((depth - DOFDepths.z) / max(DOFDepths.w - DOFDepths.z, 0.01f));
    float blur = saturate(f0 + f1);
	float fade = 1 - saturate((depth - DepthLimit) * 100);
    return blur * fade * BlurStrength;
}

// Depth of Field depth blur generation (outputs linear depth + blur factor to R16G16 target)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DofDepthBlurGeneration(Quad_VS2PS input) : SV_Target
{
	float depth = LinearDepth(Input0.SampleLevel(SamplerPointClamp, input.TexCoord, 0).r);
	float blur = BlurFactor(depth);
	return float4(depth, blur, 1.0f, 1.0f);
}

#if defined(_CS_DepthOfFieldH) || defined(_CS_DepthOfFieldV)

RWTexture2D<float4> OutputTexture : register(u0);

struct DOFSample
{
	float3 Color;
	float Depth;
	float Blur;
};

// Shared memory for actial depth of field pass
groupshared DOFSample Samples[DOF_THREAD_GROUP_SIZE];

// Performs the horizontal pass for the DOF blur
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(DOF_THREAD_GROUP_SIZE, 1, 1)]
void CS_DepthOfFieldH(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	// These positions are relative to the "grid", AKA the horizontal group of pixels that this thread group is writing to
	const int gridStartX = groupID.x * DOF_GRID_SIZE;
	const int gridX = groupThreadID.x - DOF_APRON_SIZE;

	// These positions are relative to the pixel coordinates
	const int sampleX = gridStartX + gridX;
	const int sampleY = groupID.y;

	uint2 textureSize;
	Input0.GetDimensions(textureSize.x, textureSize.y);

	const int2 samplePos = int2(sampleX, sampleY);

	// Sample the textures
#if USE_CS_HALF_PIXEL_OFFSET
	float2 sampleCoord = saturate(((float2)samplePos + 0.5f) / float2(textureSize));
#else
	float2 sampleCoord = saturate(samplePos / float2(textureSize));
#endif
#if USE_CS_LINEAR_SAMPLING
	float3 color = Input0.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xyz;
	float2 depthBlur = Input1.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xy;
#else
	float3 color = Input0.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xyz;
	float2 depthBlur = Input1.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xy;
#endif
	float depth = depthBlur.x;
	float blur = depthBlur.y;
	float cocSize = blur * DOF_MAX_SAMPLE_RADIUS;

	// Store in shared memory
	Samples[groupThreadID.x].Color = color;
	Samples[groupThreadID.x].Depth = depth;
	Samples[groupThreadID.x].Blur = blur;

	GroupMemoryBarrierWithGroupSync();

	// Don't continue for threads in the apron, and threads outside the render target size
	if (gridX >= 0 && gridX < DOF_GRID_SIZE && sampleX < textureSize.x)
	{
		BRANCH
		if (cocSize > 0.0f)
		{
			float3 outputColor = 0.0f;
			float totalContribution = 0.0f;

			// Gather sample taps inside the radius
			for (int x = -DOF_MAX_SAMPLE_RADIUS; x <= DOF_MAX_SAMPLE_RADIUS; x++)
			{
				// Grab the sample from shared memory
				int groupTapX = groupThreadID.x + x;
				DOFSample tap = Samples[groupTapX];

				// Reject the sample if it's outside the CoC radius
				float cocWeight = saturate(cocSize + 1.0f - abs(float(x)));

				// Reject foreground samples, unless they're blurred as well
				float depthWeight = tap.Depth >= depth;
				float blurWeight = tap.Blur;
				float tapWeight = cocWeight * saturate(depthWeight + blurWeight);

				outputColor += tap.Color * tapWeight;
				totalContribution += tapWeight;
			}

			// Write out the result
			outputColor /= totalContribution;
			OutputTexture[samplePos] = float4(max(outputColor, 0), 1.0f);
		}
		else
		{
			OutputTexture[samplePos] = float4(color, 1.0f);
		}
	}
}

// Performs the vertical DOF pass
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, DOF_THREAD_GROUP_SIZE, 1)]
void CS_DepthOfFieldV(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	// These positions are relative to the "grid", AKA the vertical group of pixels that this thread group is writing to
	const int gridStartY = groupID.y * DOF_GRID_SIZE;
	const int gridY = groupThreadID.y - DOF_APRON_SIZE;

	// These positions are relative to the pixel coordinates
	const int sampleX = groupID.x;
	const int sampleY = gridStartY + gridY;

	uint2 textureSize;
	Input0.GetDimensions(textureSize.x, textureSize.y);

	const int2 samplePos = int2(sampleX, sampleY);

	// Sample the textures
#if USE_CS_HALF_PIXEL_OFFSET
	float2 sampleCoord = saturate(((float2)samplePos + 0.5f) / float2(textureSize));
#else
	float2 sampleCoord = saturate(samplePos / float2(textureSize));
#endif
#if USE_CS_LINEAR_SAMPLING
	float3 color = Input0.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xyz;
	float2 depthBlur = Input1.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xy;
#else
	float3 color = Input0.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xyz;
	float2 depthBlur = Input1.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xy;
#endif
	float depth = depthBlur.x;
	float blur = depthBlur.y;
	float cocSize = blur * DOF_MAX_SAMPLE_RADIUS;

	// Store in shared memory
	Samples[groupThreadID.y].Color = color;
	Samples[groupThreadID.y].Depth = depth;
	Samples[groupThreadID.y].Blur = blur;

	GroupMemoryBarrierWithGroupSync();

	// Don't continue for threads in the apron, and threads outside the render target size
	if (gridY >= 0 && gridY < DOF_GRID_SIZE && sampleY < textureSize.y)
	{
		BRANCH
		if (cocSize > 0.0f)
		{
			float3 outputColor = 0.0f;
			float totalContribution = 0.0f;

			// Gather sample taps inside the radius
			for (int y = -DOF_MAX_SAMPLE_RADIUS; y <= DOF_MAX_SAMPLE_RADIUS; y++)
			{
				// Grab the sample from shared memory
				int groupTapY = groupThreadID.y + y;
				DOFSample tap = Samples[groupTapY];

				// Reject the sample if it's outside the CoC radius
				float cocWeight = saturate(cocSize + 1.0f - abs(float(y)));

				// Reject foreground samples, unless they're blurred as well
				float depthWeight = tap.Depth >= depth;
				float blurWeight = tap.Blur;
				float tapWeight = cocWeight * saturate(depthWeight + blurWeight);

				outputColor += tap.Color * tapWeight;
				totalContribution += tapWeight;
			}

			// Write out the result
			outputColor /= totalContribution;
			OutputTexture[samplePos] = float4(max(outputColor, 0), 1.0f);
		}
		else
		{
			OutputTexture[samplePos] = float4(color, 1.0f);
		}
	}
}

#elif defined(_CS_CoCSpreadH) || defined(_CS_CoCSpreadV)

struct CoCSample
{
	float Depth;
	float Blur;
};

RWTexture2D<float2> OutputTexture : register(u0);

groupshared CoCSample Samples[DOF_THREAD_GROUP_SIZE];

// Performs the horizontal CoC spread
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(DOF_THREAD_GROUP_SIZE, 1, 1)]
void CS_CoCSpreadH(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	// These positions are relative to the "grid", AKA the horizontal group of pixels that this thread group is writing to
	const int gridStartX = groupID.x * DOF_GRID_SIZE;
	const int gridX = groupThreadID.x - DOF_APRON_SIZE;

	// These positions are relative to the pixel coordinates
	const int sampleX = gridStartX + gridX;
	const int sampleY = groupID.y;

	uint2 textureSize;
	Input0.GetDimensions(textureSize.x, textureSize.y);

	const int2 samplePos = int2(sampleX, sampleY);

	// Sample the textures
#if USE_CS_HALF_PIXEL_OFFSET
	float2 sampleCoord = saturate(((float2)samplePos + 0.5f) / float2(textureSize));
#else
	float2 sampleCoord = saturate(samplePos / float2(textureSize));
#endif
#if USE_CS_LINEAR_SAMPLING
	float2 depthBlur = Input0.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xy;
#else
	float2 depthBlur = Input0.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xy;
#endif

	float depth = depthBlur.x;
	float blur = depthBlur.y;
	float cocSize = blur * DOF_MAX_SAMPLE_RADIUS;

	// Store in shared memory
	Samples[groupThreadID.x].Depth = depth;
	Samples[groupThreadID.x].Blur = blur;

	GroupMemoryBarrierWithGroupSync();

	// Don't continue for threads in the apron, and threads outside the render target size
	if (gridX >= 0 && gridX < DOF_GRID_SIZE && sampleX < textureSize.x)
	{
		float outputBlur = 0.0f;
		float totalContribution = 0.0f;

		// Gather sample taps inside the radius
		for (int x = -DOF_MAX_SAMPLE_RADIUS; x <= DOF_MAX_SAMPLE_RADIUS; x++)
		{
			// Grab the sample from shared memory
			int groupTapX = groupThreadID.x + x;
			CoCSample tap = Samples[groupTapX];

			// Only accept samples if they're from the foreground, and have a higher blur amount
			float depthWeight = tap.Depth <= depth;
			float blurWeight = saturate(tap.Blur - blur);
			float tapWeight = depthWeight * blurWeight;

			// If it's the center tap, set the weight to 1 so and don't reject it
			float centerWeight = x == 0 ? 1.0 : 0.0f;
			tapWeight = saturate(tapWeight + centerWeight);

			outputBlur += tap.Blur * tapWeight;
			totalContribution += tapWeight;
		}

		// Write out the result
		OutputTexture[samplePos] = float2(depth, outputBlur / totalContribution);
	}
}

// Performs the vertical CoC spread
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, DOF_THREAD_GROUP_SIZE, 1)]
void CS_CoCSpreadV(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	// These positions are relative to the "grid", AKA the vertical group of pixels that this thread group is writing to
	const int gridStartY = groupID.y * DOF_GRID_SIZE;
	const int gridY = groupThreadID.y - DOF_APRON_SIZE;

	// These positions are relative to the pixel coordinates
	const int sampleX = groupID.x;
	const int sampleY = gridStartY + gridY;

	uint2 textureSize;
	Input0.GetDimensions(textureSize.x, textureSize.y);

	const int2 samplePos = int2(sampleX, sampleY);

	// Sample the textures
#if USE_CS_HALF_PIXEL_OFFSET
	float2 sampleCoord = saturate(((float2)samplePos + 0.5f) / float2(textureSize));
#else
	float2 sampleCoord = saturate(samplePos / float2(textureSize));
#endif
#if USE_CS_LINEAR_SAMPLING
	float2 depthBlur = Input0.SampleLevel(SamplerLinearClamp, sampleCoord, 0.0f).xy;
#else
	float2 depthBlur = Input0.SampleLevel(SamplerPointClamp, sampleCoord, 0.0f).xy;
#endif
	float depth = depthBlur.x;
	float blur = depthBlur.y;
	float cocSize = blur * DOF_MAX_SAMPLE_RADIUS;

	// Store in shared memory
	Samples[groupThreadID.y].Depth = depth;
	Samples[groupThreadID.y].Blur = blur;

	GroupMemoryBarrierWithGroupSync();

	// Don't continue for threads in the apron, and threads outside the render target size
	if (gridY >= 0 && gridY < DOF_GRID_SIZE && sampleY < textureSize.y)
	{
		float outputBlur = 0.0f;
		float totalContribution = 0.0f;

		// Gather sample taps inside the radius
		for (int y = -DOF_MAX_SAMPLE_RADIUS; y <= DOF_MAX_SAMPLE_RADIUS; y++)
		{
			// Grab the sample from shared memory
			int groupTapY = groupThreadID.y + y;
			CoCSample tap = Samples[groupTapY];

			// Only accept samples if they're from the foreground, and have a higher blur amount
			float depthWeight = tap.Depth <= depth;
			float blurWeight = saturate(tap.Blur - blur);
			float tapWeight = depthWeight * blurWeight;

			// If it's the center tap, set the weight to 1 and don't reject it
			float centerWeight = y == 0 ? 1.0 : 0.0f;
			tapWeight = saturate(tapWeight + centerWeight);

			outputBlur += tap.Blur * tapWeight;
			totalContribution += tapWeight;
		}

		// Write out the result
		OutputTexture[samplePos] = float2(depth, outputBlur / totalContribution);
	}
}

#elif defined(_PS_GenerateBokeh)

// Here we output bokeh points for DoF
AppendStructuredBuffer<BokehPoint> OutputBokehPointBuffer : register(u1);

// Extracts pixels for bokeh point generation
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_GenerateBokeh(Quad_VS2PS input) : SV_Target
{
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
	
	float2 centerCoord = input.TexCoord;

	// Start with center sample color
	float3 centerColor = Input0.Sample(SamplerPointClamp, centerCoord).rgb;
	float3 colorSum = centerColor;
	float totalContribution = 1.0f;

	// Sample the depth and blur at the center
	float2 centerDepthBlur = Input1.Sample(SamplerPointClamp, centerCoord).xy;
	float centerDepth = centerDepthBlur.x;
	float centerBlur = centerDepthBlur.y;

	// Calculate the average of the current 5x5 block of texels by taking 9 bilinear samples
	const uint NumSamples = 9;
	static const float2 SamplePoints[NumSamples] =
	{
		float2(-1.5f, -1.5f), float2(0.5f, -1.5f), float2(1.5f, -1.5f),
		float2(-1.5f, 0.5f), float2(0.5f, 0.5f), float2(1.5f, 0.5f),
		float2(-1.5f, 1.5f), float2(0.5f, 1.5f), float2(1.5f, 1.5f)
	};
	float3 averageColor = 0.0f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 sampleCoord = centerCoord + SamplePoints[i] / InputSize;
		float3 color = Input0.Sample(SamplerPointClamp, sampleCoord).rgb;
		averageColor += color;
	}
	averageColor /= NumSamples;

	// Calculate the difference between the current texel and the average
	float averageBrightness = dot(averageColor, 1.0f);
	float centerBrightness = dot(centerColor, 1.0f);
	float brightnessDiff = max(centerBrightness - averageBrightness, 0.0f);

	// If the texel is brighter than its neighbors and the blur amount is above our threshold, then push the pixel position/color/blur amount into our AppendStructuredBuffer containing bokeh points
	BRANCH
	if (brightnessDiff >= BokehBrightnessThreshold && centerBlur > BokehBlurThreshold)
	{
		BokehPoint bPoint;
		bPoint.Position = float3(input.Position.xy, centerDepth);
		bPoint.Blur = centerBlur;
		bPoint.Color = centerColor;

		OutputBokehPointBuffer.Append(bPoint);

		centerColor = 0.0f;
	}

	return float4(centerColor, 1.0f);
	
#else

	return float4(0, 0, 0, 1.0f);

#endif
}

#elif defined(_PS_DoNotGenerateBokeh)

// Extracts pixels for bokeh point generation
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_DoNotGenerateBokeh(Quad_VS2PS input) : SV_Target
{
	float2 centerCoord = input.TexCoord;

	// Start with center sample color
	float3 centerColor = Input0.Sample(SamplerPointClamp, centerCoord).rgb;
	float3 colorSum = centerColor;
	float totalContribution = 1.0f;

	// Sample the depth and blur at the center
	float2 centerDepthBlur = Input1.Sample(SamplerPointClamp, centerCoord).xy;
	float centerDepth = centerDepthBlur.x;
	float centerBlur = centerDepthBlur.y;

	// Calculate the average of the current 5x5 block of texels by taking 9 bilinear samples
	const uint NumSamples = 9;
	static const float2 SamplePoints[NumSamples] =
	{
		float2(-1.5f, -1.5f), float2(0.5f, -1.5f), float2(1.5f, -1.5f),
		float2(-1.5f, 0.5f), float2(0.5f, 0.5f), float2(1.5f, 0.5f),
		float2(-1.5f, 1.5f), float2(0.5f, 1.5f), float2(1.5f, 1.5f)
	};
	float3 averageColor = 0.0f;
	for (uint i = 0; i < NumSamples; i++)
	{
		float2 sampleCoord = centerCoord + SamplePoints[i] / InputSize;
		float3 color = Input0.Sample(SamplerPointClamp, sampleCoord).rgb;
		averageColor += color;
	}
	averageColor /= NumSamples;

	// Calculate the difference between the current texel and the average
	float averageBrightness = dot(averageColor, 1.0f);
	float centerBrightness = dot(centerColor, 1.0f);
	float brightnessDiff = max(centerBrightness - averageBrightness, 0.0f);

	// If the texel is brighter than its neighbors and the blur amount is above our threshold
	BRANCH
	if (brightnessDiff >= BokehBrightnessThreshold && centerBlur > BokehBlurThreshold)
	{
		centerColor = 0.0f;
	}

	return float4(centerColor, 1.0f);
}

#else

StructuredBuffer<BokehPoint> BokehPointBuffer : register(t2);

// Vertex Shader, positions and scales the bokeh point
META_VS(true, FEATURE_LEVEL_SM5)
META_FLAG(VertexToGeometryShader)
BokehVSOutput VS_Bokeh(in uint vertexID : SV_VertexID)
{
	BokehPoint bPoint = BokehPointBuffer[vertexID];
	BokehVSOutput output;

	// Position the vertex in normalized device coordinate space [-1, 1]
	output.Position.xy = bPoint.Position.xy;
	output.Position.xy /= DOFTargetSize;
	output.Position.xy = output.Position.xy * 2.0f - 1.0f;
	output.Position.y *= -1.0f;

	// Scale the size based on the CoC size, and max bokeh sprite size
	output.Size = bPoint.Blur * (1.0f / DOFTargetSize) * MaxBokehSize;

	output.Depth = bPoint.Position.z;

	// Scale the color to conserve energy, by comparing new area vs. old area
	float cocRadius = bPoint.Blur * MaxBokehSize * 0.45f;
	float cocArea = cocRadius * cocRadius * 3.14159f;
	float falloff = pow(saturate(1.0f / cocArea), BokehFalloff);
	output.Color = bPoint.Color * (falloff * BokehBrightness);

	return output;
}

// Geometry Shader, expands a vertex into a quad with two triangles
META_GS(true, FEATURE_LEVEL_SM5)
[maxvertexcount(4)]
void GS_Bokeh(point BokehVSOutput input[1], inout TriangleStream<BokehGSOutput> stream)
{
	BokehGSOutput output;

	// Emit 4 new verts, and 2 new triangles
	UNROLL
	for (int i = 0; i < 4; i++)
	{
		output.PositionCS = float4(input[0].Position.xy, 1.0f, 1.0f);
		output.PositionCS.xy += Offsets[i] * input[0].Size;
		output.TexCoord = TexCoords[i];
		output.Color = input[0].Color;
		output.Depth = input[0].Depth;
		
		stream.Append(output);
	}
	stream.RestartStrip();
}

// Pixel Shader, applies the bokeh shape texture
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_Bokeh(in BokehPSInput input) : SV_Target0
{
	// Sample the bokeh texture
	float textureSample = Input0.Sample(SamplerLinearClamp, input.TexCoord).x;
	//float textureSample = 1;

	float2 screenTexCoord = input.PositionSS.xy / BokehTargetSize;
	float2 depthBlurSample = Input1.Sample(SamplerLinearClamp, screenTexCoord).xy;

	// Reject pixels based on a depth test, but only if the occluder isn't blurred
	float attenutation = saturate(depthBlurSample.x - input.Depth + BokehDepthCutoff);
	attenutation = saturate(attenutation + depthBlurSample.y);

	return float4(input.Color * textureSample * attenutation, 1.0f);
}

// Composites the bokeh results with the previous DOF results
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_BokehComposite(in Quad_VS2PS input) : SV_Target
{
    float4 bokehSample = Input0.Sample(SamplerLinearClamp, input.TexCoord);
	float4 dofSample = Input1.Sample(SamplerPointClamp, input.TexCoord);

	float3 composite = bokehSample.rgb + dofSample.rgb;

	return float4(composite, 1.0f);
}

#endif
