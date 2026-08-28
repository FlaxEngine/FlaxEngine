// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, OcclusionCullingData)
float4x4 ViewProjectionMatrix;
META_CB_END

// Vertex Shader function for Hardware Occlusion Culling queries bounds projection
META_VS(true, FEATURE_LEVEL_ES2)
float4 VS_HardwareOcclusionCulling(float3 Position : POSITION0) : SV_Position
{
    return mul(float4(Position, 1), ViewProjectionMatrix);
}
