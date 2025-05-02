// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float2 InputSizeInv;
float2 Padding;
float SharpeningAmount;
float EdgeSharpening;
float MinEdgeThreshold;
float OverBlurLimit;
META_CB_END

Texture2D Input : register(t0);

// Pixel Shader for Contrast Adaptive Sharpening (CAS) filter. Based on AMD FidelityFX implementation.
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CAS(Quad_VS2PS input) : SV_Target0
{
    // Sample the color texture
    float4 color = Input.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);

    // Sample neighboring pixels
    float3 blurred = color.rgb;
    float3 edges = 0.0;
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float2 uv = float2(x, y) * InputSizeInv + input.TexCoord;
            float3 neighbor = Input.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
            blurred += neighbor;
            edges += abs(neighbor - color.rgb);
        }
    }
    blurred /= 25.0;
    edges /= 25.0;

    // Sharpen based on edge detection
    float edgeAmount = saturate((dot(edges, edges) - MinEdgeThreshold) / (0.001 + dot(edges, edges)));
    float sharpen = (1.0 - edgeAmount) * SharpeningAmount + edgeAmount * EdgeSharpening;
    float3 sharpened = color.rgb + (color.rgb - blurred) * sharpen;

    // Limit sharpening to avoid over-blurring
    sharpened = lerp(color.rgb, sharpened, saturate(OverBlurLimit / (OverBlurLimit + dot(abs(sharpened - color.rgb), float3(1.0, 1.0, 1.0)))));
    return float4(sharpened, color.a);
}
