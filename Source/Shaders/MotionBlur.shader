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

// Converts a motion vector into RGBA color.
float4 VectorToColor(float2 mv)
{
    float phi = atan2(mv.x, mv.y);
    float hue = (phi / PI + 1) * 0.5;

    float r = abs(hue * 6 - 3) - 1;
    float g = 2 - abs(hue * 6 - 2);
    float b = 2 - abs(hue * 6 - 4);
    float a = length(mv);

    return saturate(float4(r, g, b, a));
}

// Pixel shader for motion vectors debug view
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MotionVectorsDebug(Quad_VS2PS input) : SV_Target
{
	float4 src = SAMPLE_RT(Input0, input.TexCoord);

    float2 mv = SAMPLE_RT(Input1, input.TexCoord).rg * (DebugAmplitude * 5.0f);
    float4 mc = VectorToColor(mv);

    float3 rgb = mc.rgb;

    float src_ratio = saturate(2 - DebugBlend * 2);
    float mc_ratio = saturate(DebugBlend * 2);
    rgb = lerp(src.rgb * src_ratio, rgb, mc.a * mc_ratio);

    return float4(rgb, src.a);
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
    float inv_aspect = GBuffer.ScreenSize.y * GBuffer.ScreenSize.z;

    // Vertex IDs
    uint arrow_id = VertexId / 6;
    uint point_id = VertexId - arrow_id * 6;

    // Column/Row number of the arrow
    uint row = arrow_id / DebugColumnCount;
    uint col = arrow_id - row * DebugColumnCount;

    // Texture coordinate of the reference point
    float2 uv = float2((col + 0.5) / DebugColumnCount, (row + 0.5) / DebugRowCount);

    // Retrieve the motion vector
    float2 mv = SAMPLE_RT(Input1, uv).rg * DebugAmplitude;

    // Arrow color
    float4 color = VectorToColor(mv);

    // Arrow vertex position parameter (0 = origin, 1 = head)
    float arrow_l = point_id > 0;

    // Rotation matrix for the arrow head
    float2 head_dir = normalize(mv * float2(aspect, 1));
    float2x2 head_rot = float2x2(head_dir.y, head_dir.x, -head_dir.x, head_dir.y);

    // Offset for arrow head vertices
    float head_x = point_id == 3 ? -1 : (point_id == 5 ? 1 : 0);
    head_x *= arrow_l * 0.3 * saturate(length(mv) * DebugRowCount);

    float2 head_offs = float2(head_x, -abs(head_x));
    head_offs = mul(head_rot, head_offs) * float2(inv_aspect, 1);

    // Vertex position in the clip space
    float2 vp = mv * arrow_l + head_offs * 2 / DebugRowCount + uv * 2 - 1;

    // Convert to the screen coordinates
    float2 scoord = (vp + 1) * 0.5 * GBuffer.ScreenSize.xy;

    // Snap to a pixel-perfect position.
    scoord = round(scoord);

    // Bring back to the clip space
    vp = (scoord + 0.5) * GBuffer.ScreenSize.zw * 2 - 1;
    vp.y *= -1;

    // Color tweaks
    color.rgb = lerp(color.rgb, 1, 0.5);
    color.a = DebugBlend;

    // Output
    ArrowVaryings output;
    output.Position = float4(vp, 0, 1);
    output.ScreenUV = scoord;
    output.Color = color;
    return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DebugArrow(ArrowVaryings input) : SV_Target
{
    // Pseudo anti-aliasing
    float aa = length(frac(input.ScreenUV) - 0.5) / 0.707;
    aa *= (aa * (aa * 0.305306011 + 0.682171111) + 0.012522878); // gamma
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
	v *= (VelocityScale * 0.5) * GBuffer.ScreenSize.xy;

	// Clamp the vector with the maximum blur radius
	v /= max(1.0, length(v) * RcpMaxBlurRadius);

	// Sample the depth of the pixel
	float depth = SAMPLE_RT(Input1, input.TexCoord).r;
	GBufferData gBufferData = GetGBufferData();
	depth = LinearizeZ(gBufferData, depth);

	// Pack into 10/10/10/2 format
	return float4((v * RcpMaxBlurRadius + 1.0) * 0.5, depth, 0.0);
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

	v1 = (v1 * 2.0 - 1.0) * MaxBlurRadius;
	v2 = (v2 * 2.0 - 1.0) * MaxBlurRadius;
	v3 = (v3 * 2.0 - 1.0) * MaxBlurRadius;
	v4 = (v4 * 2.0 - 1.0) * MaxBlurRadius;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0, 0.0);
}

// Pixel Shader for TileMax filter (2 pixel width)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax2(Quad_VS2PS input) : SV_Target
{
	float4 d = TexelSize2.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + d.zw).rg;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0, 0.0);
}

// Pixel Shader for TileMax filter (2 pixel width)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TileMax4(Quad_VS2PS input) : SV_Target
{
	float4 d = TexelSize4.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = SAMPLE_RT(Input0, input.TexCoord + d.xy).rg;
	float2 v2 = SAMPLE_RT(Input0, input.TexCoord + d.zy).rg;
	float2 v3 = SAMPLE_RT(Input0, input.TexCoord + d.xw).rg;
	float2 v4 = SAMPLE_RT(Input0, input.TexCoord + d.zw).rg;

	return float4(MaxV(MaxV(MaxV(v1, v2), v3), v4), 0.0, 0.0);
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

	return float4(MaxV(va, MaxV(vb, vc)) * (1.0 / cw), 0.0, 0.0);
}

// Interleaved gradient function from Jimenez 2014
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float GradientNoise(float2 uv)
{
    uv = floor(uv * GBuffer.ScreenSize.xy);
    float f = dot(float2(0.06711056, 0.00583715), uv);
    return frac(52.9829189 * frac(f));
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
	sincos(GradientNoise(uv + float2(2.0, 0.0)) * (2.0f * PI), ry, rx);
	return float2(rx, ry) * TexelSizeNM.xy * 0.25;
}

// Velocity sampling function
float3 SampleVelocity(float2 uv)
{
	float3 v = SAMPLE_RT(Input1, uv).xyz;
	return float3((v.xy * 2.0 - 1.0) * MaxBlurRadius, v.z);
}

// Pixel Shader for reconstruction filter (applies the motion blur to the frame)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Reconstruction(Quad_VS2PS input) : SV_Target
{
	// Color sample at the center point
	const float4 c_p = SAMPLE_RT(Input0, input.TexCoord);

	// Velocity/Depth sample at the center point
	const float3 vd_p = SampleVelocity(input.TexCoord);
	const float l_v_p = max(length(vd_p.xy), 0.5);
	const float rcp_d_p = 1.0 / vd_p.z;

	// NeighborMax vector sample at the center point
	const float2 v_max = SAMPLE_RT(Input2, input.TexCoord + JitterTile(input.TexCoord)).xy;
	const float l_v_max = length(v_max);
	const float rcp_l_v_max = 1.0 / l_v_max;

	// Escape early if the NeighborMax vector is small enough
	if (l_v_max < 2.0)
		return c_p;

	// Use V_p as a secondary sampling direction except when it's too small
	// compared to V_max. This vector is rescaled to be the length of V_max.
	const float2 v_alt = (l_v_p * 2.0 > l_v_max) ? vd_p.xy * (l_v_max / l_v_p) : v_max;

	// Determine the sample count.
	const float sc = floor(min(LoopCount, l_v_max * 0.5));

	// Loop variables (starts from the outermost sample)
	const float dt = 1.0 / sc;
	const float t_offs = (GradientNoise(input.TexCoord) - 0.5) * dt;
	float t = 1.0 - dt * 0.5;
	float count = 0.0;

	// Background velocity
	// This is used for tracking the maximum velocity in the background layer
	float l_v_bg = max(l_v_p, 1.0);

	// Color accumlation
	float4 acc = 0.0;
	LOOP
	while (t > dt * 0.25)
	{
		// Sampling direction (switched per every two samples)
		const float2 v_s = Interval(count, 4.0) ? v_alt : v_max;

		// Sample position (inverted per every sample)
		const float t_s = (Interval(count, 2.0) ? -t : t) + t_offs;

		// Distance to the sample position
		const float l_t = l_v_max * abs(t_s);

		// UVs for the sample position
		const float2 uv0 = input.TexCoord + v_s * t_s * GBuffer.ScreenSize.zw;
		//const float2 uv1 = input.TexCoord + v_s * t_s * MotionVectorsTexelSize.xy;
		const float2 uv1 = uv0;

		// Color sample
		const float3 c = SAMPLE_RT(Input0, uv0).rgb;

		// Velocity/Depth sample
		const float3 vd = SampleVelocity(uv1);

		// Background/Foreground separation
		const float fg = saturate((vd_p.z - vd.z) * 20.0 * rcp_d_p);

		// Length of the velocity vector
		const float l_v = lerp(l_v_bg, length(vd.xy), fg);

		// Sample weight
		// (Distance test) * (Spreading out by motion) * (Triangular window)
		const float w = saturate(l_v - l_t) / l_v * (1.2 - t);

		// Color accumulation
		acc += float4(c, 1.0) * w;

		// Update the background velocity.
		l_v_bg = max(l_v_bg, l_v);

		// Advance to the next sample.
		t = Interval(count, 2.0) ? t - dt : t;
		count += 1.0;
	}

	// Add the center sample
	acc += float4(c_p.rgb, 1.0) * (1.2 / (l_v_bg * sc * 2.0));

	return float4(acc.rgb / acc.a, c_p.a);
}
