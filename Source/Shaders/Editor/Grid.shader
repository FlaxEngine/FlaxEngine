// Implementation based on:
// "The Best Darn Grid Shader (Yet)", Medium, Oct 2023
// Ben Golus
// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8#3e73

#define USE_FORWARD true;

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjectionMatrix;
float4 GridColor;
float3 ViewPos;
float Far;
float3 ViewOrigin;
float GridSize;
META_CB_END

// Geometry data passed to the vertex shader
struct ModelInput
{
    float3 Position : POSITION;
};

// Interpolants passed from the vertex shader
struct VertexOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
};

// Interpolants passed to the pixel shader
struct PixelInput
{
    float4 Position : SV_Position;
    noperspective float2 TexCoord : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
};

// Vertex shader function for grid rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R16G16_FLOAT, 1, ALIGN, PER_VERTEX, 0, true)
VertexOutput VS_Grid(ModelInput input)
{
    VertexOutput output;
    output.WorldPosition = input.Position.xyz + ViewOrigin;
    float3 geoPosition = input.Position.xyz - float3(0, ViewOrigin.y, 0);
    output.Position = mul(float4(geoPosition, 1), ViewProjectionMatrix);
    return output;
}

float invLerp(float from, float to, float value)
{
    return (value - from) / (to - from);
}

float remap(float origFrom, float origTo, float targetFrom, float targetTo, float value)
{
    float rel = invLerp(origFrom, origTo, value);
    return lerp(targetFrom, targetTo, rel);
}

float ddLength(float a)
{
    return length(float2(ddx(a), ddy(a)));
}

float GetLine(float pos, float scale, float thickness)
{
    float lineWidth = thickness;
    float coord = (pos * 0.01) * scale;

    float2 uvDDXY = float2(ddx(coord), ddy(coord));

    float deriv = float(length(uvDDXY.xy));
    float drawWidth = clamp(lineWidth, deriv, 0.5);
    float lineAA = deriv * 1.5;
    float gridUV = abs(coord);
    float grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(lineWidth / drawWidth);
    grid2 = lerp(grid2, lineWidth, saturate(deriv * 2.0 - 1.0));

    float grid = lerp(grid2, 1.0, grid2);
    return grid; 
}

float GetGrid(float3 pos, float scale, float thickness)
{
    float lineWidth = thickness;
    float2 coord = (pos.xz * 0.01) * scale;

    float4 uvDDXY = float4(ddx(coord), ddy(coord));

    float2 deriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    float2 drawWidth = clamp(lineWidth, deriv, 0.5);
    float2 lineAA = deriv * 1.5;
    float2 gridUV = 1.0 - abs(frac(coord) * 2.0 - 1.0);
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(lineWidth / drawWidth);
    grid2 = lerp(grid2, lineWidth, saturate(deriv * 2.0 - 1.0));
    
    float grid = lerp(grid2.x, 1.0, grid2.y);
    return grid;
}

float4 GetColor(float3 pos, float scale)
{
    float dist = 1 - saturate(distance(float3(ViewPos.x, 0, ViewPos.z), pos) / GridSize);
    
    // Line width
    float g1LW = 0.01;
    // Major line Z
    float l1 = GetLine(pos.x, 1, g1LW * 2);
    // Major line X
    float l2 = GetLine(pos.z, 1, g1LW);
    
    // Main grid
    float g1 = GetGrid(pos, 1, g1LW * 0.8);
    float g2 = GetGrid(pos, 2, g1LW * 0.4);
    float g3 = GetGrid(pos, 0.1, g1LW * 2);

    float camFadeLarge = clamp(invLerp(2500, 4000, abs(ViewPos.y)),0, g3);
    g3 *= camFadeLarge;

    float g4 = GetGrid(pos, 10, g1LW);
    float camFadeTiny = clamp(invLerp(150, 100, abs(ViewPos.y)), 0, g4);
    g4 *= camFadeTiny;

    float grid = 0;
    grid = max(l1, l2);
    grid = max(grid, g1);
    grid = max(grid, g2);
    grid = max(grid, g3);
    grid = max(grid, g4);
    
    float4 color = grid * GridColor;
    color = lerp(color, float4(1,0,0,1), l2);
    color = lerp(color, float4(0,0,1,1), l1);
    color *= dist;
    color *= 1 - saturate(length(pos) - 600000); // Fade out when far from origin (60km)
    return color;
}

// Pixel shader function for grid rendering
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Grid(PixelInput input) : SV_Target
{ 
    float4 color = GetColor(input.WorldPosition, 1);
    return color;
}
