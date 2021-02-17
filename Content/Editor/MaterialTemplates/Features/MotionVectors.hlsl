// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

@0// Motion Vectors: Defines
@1// Motion Vectors: Includes
@2// Motion Vectors: Constants
@3// Motion Vectors: Resources
@4// Motion Vectors: Utilities
@5// Motion Vectors: Shaders

// Pixel Shader function for Motion Vectors Pass
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MotionVectors(PixelInput input) : SV_Target0
{
#if USE_DITHERED_LOD_TRANSITION
	// LOD masking
	ClipLODTransition(input);
#endif

#if MATERIAL_MASKED
	// Perform per pixel clipping if material requries it
	MaterialInput materialInput = GetMaterialInput(input);
	Material material = GetMaterialPS(materialInput);
	clip(material.Mask - MATERIAL_MASK_THRESHOLD);
#endif

	// Calculate this and previosu frame pixel locations in clip space
	float4 prevClipPos = mul(float4(input.Geometry.PrevWorldPosition, 1), PrevViewProjectionMatrix);
	float4 curClipPos = mul(float4(input.Geometry.WorldPosition, 1), ViewProjectionMatrix);
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

	// Calculate per-pixel motion vector
	return float4(vPosCur - vPosPrev, 0, 1);
}
