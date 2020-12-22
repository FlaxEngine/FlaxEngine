// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"

META_CB_BEGIN(0, Data)

GBufferData GBuffer;

// Camera Motion Vectors
float4x4 CurrentVP;
float4x4 PreviousVP;
float4 TemporalAAJitter;

// Motion Blur
float2 TileMaxOffs;
float VelocityScale;
int TileMaxLoop;
float MaxBlurRadius;
float RcpMaxBlurRadius;
float2 TexelSize1;
float2 TexelSize2;
float2 TexelSize4;
float2 TexelSizeV;
float2 TexelSizeNM;
float LoopCount;
float Dummy0;
float2 MotionVectorsTexelSize;

// Motion Vectors Debug Parameters
float DebugBlend;
float DebugAmplitude;
int DebugColumnCount;
int DebugRowCount;

META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Input0 : register(t0);
Texture2D Input1 : register(t1);
Texture2D Input2 : register(t2);

// Calculates the color for the a motion vector debugging
float4 VectorToColor(float2 motionVector)
{
    float phi = atan2(motionVector.x, motionVector.y);
    float hue = (phi / PI + 1) * 0.5;

    float r = abs(hue * 6 - 3) - 1;
    float g = 2 - abs(hue * 6 - 2);
    float b = 2 - abs(hue * 6 - 4);
    float a = length(motionVector);

    return saturate(float4(r, g, b, a));
}

// Pixel shader for motion vectors debug view
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MotionVectorsDebug(Quad_VS2PS input) : SV_Target
{
	float4 color = SAMPLE_RT(Input0, input.TexCoord);
    float2 motionVector = SAMPLE_RT(Input1, input.TexCoord).rg * (DebugAmplitude * 5.0f);
    float4 motionColor = VectorToColor(motionVector);

    float colorRation = saturate(2 - DebugBlend * 2);
    float motionColorRatio = saturate(DebugBlend * 2);
    color.rgb = lerp(color.rgb * colorRation, motionColor.rgb, motionColor.a * motionColorRatio);

    return color;
}

// Motion vector arrow data from VS to PS
struct ArrowVaryings
{
    float4 Position : SV_POSITION;
    float2 ScreenUV : TEXCOORD;
    float4 Color    : COLOR;
};

META_VS(true, FEATURE_LEVEL_ES2)
ArrowVaryings VS_DebugArrow(uint VertexId : SV_VertexID)
{
    // Screen aspect ratio
    float aspect = GBuffer.ScreenSize.x * GBuffer.ScreenSize.w;
    float aspectInv = GBuffer.ScreenSize.y * GBuffer.ScreenSize.z;

    // Vertex IDs
    uint arrowId = VertexId / 6;
    uint pointId = VertexId - arrowId * 6;

    // Column and row number of the arrow
    uint row = arrowId / DebugColumnCount;
    uint col = arrowId - row * DebugColumnCount;

    // Get the motion vector
    float2 uv = float2((col + 0.5) / DebugColumnCount, (row + 0.5) / DebugRowCount);
    float2 motionVector = SAMPLE_RT(Input1, uv).rg * DebugAmplitude;

    // Arrow color
    float4 color = VectorToColor(motionVector);

    // Arrow transformation
    float isEnd = pointId > 0;
    float2 direction = normalize(motionVector * float2(aspect, 1));
    float2x2 rotation = float2x2(direction.y, direction.x, -direction.x, direction.y);
    float offsetStart = pointId == 3 ? -1 : (pointId == 5 ? 1 : 0);
    offsetStart *= isEnd * 0.3f * saturate(length(motionVector) * DebugRowCount);
    float2 offset = float2(offsetStart, -abs(offsetStart));
    offset = mul(rotation, offset) * float2(aspectInv, 1);

    // Vertex position in the clip space
    float2 pos = motionVector * isEnd + offset * 2 / DebugRowCount + uv * 2.0f - 1.0f;

    // Convert to the screen coordinates
    float2 posSS = (pos + 1) * 0.5f * GBuffer.ScreenSize.xy;
    posSS = round(posSS);

    // Bring back to the clip space
    pos = (posSS + 0.5f) * GBuffer.ScreenSize.zw * 2.0f - 1.0f;
    pos.y *= -1;

    // Color tweaks
    color.rgb = lerp(color.rgb, 1, 0.5f);
    color.a = DebugBlend;

    // Output
    ArrowVaryings output;
    output.Position = float4(pos, 0, 1);
    output.ScreenUV = posSS;
    output.Color = color;
    return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DebugArrow(ArrowVaryings input) : SV_Target
{
    // Pseudo anti-aliasing
    float aa = length(frac(input.ScreenUV) - 0.5f) / 0.707f;
    aa *= (aa * (aa * 0.305306011f + 0.682171111f) + 0.012522878f);
    return float4(input.Color.rgb, input.Color.a * aa);
}

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

// Pixel Shader for velocity texture setup
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_VelocitySetup(Quad_VS2PS input) : SV_Target
{
	// Sample the motion vector
	float2 v = SAMPLE_RT(Input0, input.TexCoord).rg;

	// Apply the exposure time and convert to the pixel space
	v *= (VelocityScale * 0.5f) * GBuffer.ScreenSize.xy;

	// Clamp the vector with the maximum blur radius
	v /= max(1.0f, length(v) * RcpMaxBlurRadius);

	// Sample the depth of the pixel
	float depth = SAMPLE_RT(Input1, input.TexCoord).r;
	GBufferData gBufferData = GetGBufferData();
	depth = LinearizeZ(gBufferData, depth);

	// Pack into 10/10/10/2 format
	return float4((v * RcpMaxBlurRadius + 1.0f) * 0.5f, depth, 0.0f);
}

float2 MaxV(float2 v1, float2 v2)
{
	return dot(v1, v1) < dot(v2, v2) ? v2 : v1;
}

// Pixel Shader for TileMax filter (2 pixel width with normalization)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax1(Quad_VS2PS input) : SV_Target
{
	float4 d = TexelSize1.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + d.zw).rg;

	v1 = (v1 * 2.0f - 1.0f) * MaxBlurRadius;
	v2 = (v2 * 2.0f - 1.0f) * MaxBlurRadius;
	v3 = (v3 * 2.0f - 1.0f) * MaxBlurRadius;
	v4 = (v4 * 2.0f - 1.0f) * MaxBlurRadius;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0f, 0.0f);
}

// Pixel Shader for TileMax filter (2 pixel width)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax2(Quad_VS2PS input) : SV_Target
{
	float4 d = TexelSize2.xyxy * float4(-0.5f, -0.5f, 0.5f, 0.5f);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + d.zw).rg;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0f, 0.0f);
}

// Pixel Shader for TileMax filter (2 pixel width)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax4(Quad_VS2PS input) : SV_Target
{
	float4 d = TexelSize4.xyxy * float4(-0.5f, -0.5f, 0.5f, 0.5f);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + d.zw).rg;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0f, 0.0f);
}

// Pixel Shader for TileMax filter (variable width)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMaxV(Quad_VS2PS input) : SV_Target
{
	float2 uv0 = input.TexCoord + TexelSizeV.xy * TileMaxOffs.xy;

	float2 du = float2(TexelSizeV.x, 0.0);
	float2 dv = float2(0.0, TexelSizeV.y);

	float2 vo = 0.0;

	LOOP
	for (int x = 0; x < TileMaxLoop; x++)
	{
		LOOP
		for (int y = 0; y < TileMaxLoop; y++)
		{
			float2 uv = uv0 + du * x + dv * y;
			vo = MaxV(vo, SAMPLE_RT(Input0, uv).rg);
		}
	}

	return float4(vo, 0.0, 0.0);
}

// Pixel Shader for NeighborMax filter
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_NeighborMax(Quad_VS2PS input) : SV_Target
{
	// Center weight tweak
	const float cw = 1.01;

	float4 d = TexelSizeNM.xyxy * float4(1.0, 1.0, -1.0, 0.0);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord - d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord - d.wy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord - d.zy).rg;

	float2 v4 = SAMPLE_RT(Input0, input.TexCoord - d.xw).rg;
	float2 v5 = SAMPLE_RT(Input0, input.TexCoord).rg * cw;
	float2 v6 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;

	float2 v7 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v8 = SAMPLE_RT(Input0, input.TexCoord + d.wy).rg;
	float2 v9 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;

	float2 va = MaxV(v1, MaxV(v2, v3));
	float2 vb = MaxV(v4, MaxV(v5, v6));
	float2 vc = MaxV(v7, MaxV(v8, v9));

	return float4(MaxV(va, MaxV(vb, vc)) * (1.0f / cw), 0.0f, 0.0f);
}

// Interleaved gradient function from Jimenez 2014
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float GradientNoise(float2 uv)
{
    uv = floor(uv * GBuffer.ScreenSize.xy);
    float f = dot(float2(0.06711056f, 0.00583715f), uv);
    return frac(52.9829189f * frac(f));
}

// Returns true or false with a given interval
bool Interval(float phase, float interval)
{
	return frac(phase / interval) > 0.499;
}

// Jitter function for tile lookup
float2 JitterTile(float2 uv)
{
	float rx, ry;
	sincos(GradientNoise(uv + float2(2.0f, 0.0f)) * (2.0f * PI), ry, rx);
	return float2(rx, ry) * TexelSizeNM.xy * 0.25f;
}

// Velocity sampling function
float3 SampleVelocity(float2 uv)
{
	float3 v = SAMPLE_RT(Input1, uv).xyz;
	return float3((v.xy * 2.0f - 1.0f) * MaxBlurRadius, v.z);
}

// Pixel Shader for reconstruction filter (applies the motion blur to the frame)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Reconstruction(Quad_VS2PS input) : SV_Target
{
	// Sample at the current location
	const float4 color = SAMPLE_RT(Input0, input.TexCoord);
	const float3 velocity = SampleVelocity(input.TexCoord);
	const float velocityLen = max(length(velocity.xy), 0.5);
	const float depthInv = 1.0 / velocity.z;

	const float2 velocityMax = SAMPLE_RT(Input2, input.TexCoord + JitterTile(input.TexCoord)).xy;
	const float velocityMaxLength = length(velocityMax);
	if (velocityMaxLength < 2.0f)
		return color;
	const float2 velocityWeighted = (velocityLen * 2.0f > velocityMaxLength) ? velocity.xy * (velocityMaxLength / velocityLen) : velocityMax;

	// Calculate the amount of samples
	const float sc = floor(min(LoopCount, velocityMaxLength * 0.5f));

	// Accumlation loop
	float backgroudVelocity = max(velocityLen, 1.0f);
	const float dt = 1.0f / sc;
	const float offsetNoise = (GradientNoise(input.TexCoord) - 0.5f) * dt;
	float t = 1.0f - dt * 0.5f;
	float count = 0.0f;
	float4 sum = 0.0f;
	LOOP
	while (t > dt * 0.25)
	{
		// Sampling direction (switched per every two samples)
		const float2 sampleVelocity = Interval(count, 4.0) ? velocityWeighted : velocityMax;

		// Sample position (inverted per every sample)
		const float samplePosition = (Interval(count, 2.0) ? -t : t) + offsetNoise;

		// Calculate UVs for the sample position
		const float2 sampleUV = input.TexCoord + sampleVelocity * samplePosition * GBuffer.ScreenSize.zw;

		// Sample color and velocity with depth
		const float3 c = SAMPLE_RT(Input0, sampleUV).rgb;
		const float3 velocityDepth = SampleVelocity(sampleUV);

		// Length of the velocity vector
		const float foreground = saturate((velocity.z - velocityDepth.z) * 20.0f * depthInv);
		const float sampleVelocityLength = lerp(backgroudVelocity, length(velocityDepth.xy), foreground);

		// Apply color accumulation
		float weight = saturate(sampleVelocityLength - (velocityMaxLength * abs(samplePosition))) / sampleVelocityLength * (1.2f - t);
		sum += float4(c, 1.0) * weight;

		// Calculate the background velocity
		backgroudVelocity = max(backgroudVelocity, sampleVelocityLength);

		// Move to the next sample
		t = Interval(count, 2.0f) ? t - dt : t;
		count += 1.0f;
	}

	// Add the center sample
	sum += float4(color.rgb, 1.0f) * (1.2f / (backgroudVelocity * sc * 2.0f));

	return float4(sum.rgb / sum.a, color.a);
}
