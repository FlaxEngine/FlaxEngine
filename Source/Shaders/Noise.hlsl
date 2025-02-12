// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license.
// https://github.com/stegu/webgl-noise

#ifndef __NOISE__
#define __NOISE__

#include "./Flax/Common.hlsl"

float2 Mod289(float2 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 Mod289(float3 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 Mod289(float4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 Mod7(float3 x)
{
    return x - floor(x * (1.0f / 7.0f)) * 7.0f;
}

float2 Permute(float2 x)
{
    return Mod289(((x * 34.0) + 1.0) * x);
}

float3 Permute(float3 x)
{
    return Mod289(((x * 34.0) + 1.0) * x);
}

float4 Permute(float4 x)
{
    return Mod289(((x * 34.0) + 1.0) * x);
}

float4 TaylorInvSqrt(float4 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

float2 PerlinNoiseFade(float2 t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    // https://www.ronja-tutorials.com/post/024-white-noise/
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    return frac(sin(random) * 143758.5453);
}

float2 rand2dTo2d(float2 value)
{
    // https://www.ronja-tutorials.com/post/024-white-noise/
    return float2(
        rand2dTo1d(value, float2(12.989, 78.233)),
        rand2dTo1d(value, float2(39.346, 11.135))
    );
}

float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719))
{
    // https://www.ronja-tutorials.com/post/024-white-noise/
    float3 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    return frac(sin(random) * 143758.5453);
}

float3 rand3dTo3d(float3 value)
{
    // https://www.ronja-tutorials.com/post/024-white-noise/
    return float3(
        rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float3(39.346, 11.135, 83.155)),
        rand3dTo1d(value, float3(73.156, 52.235,  9.151))
    );
}

float PerlinNoiseImpl(float4 Pi, float4 Pf)
{
    Pi = Mod289(Pi);
    float4 ix = Pi.xzxz;
    float4 iy = Pi.yyww;
    float4 fx = Pf.xzxz;
    float4 fy = Pf.yyww;

    float4 i = Permute(Permute(ix) + iy);

    float4 gx = frac(i * (1.0 / 41.0)) * 2.0 - 1.0;
    float4 gy = abs(gx) - 0.5;
    float4 tx = floor(gx + 0.5);
    gx = gx - tx;

    float2 g00 = float2(gx.x, gy.x);
    float2 g10 = float2(gx.y, gy.y);
    float2 g01 = float2(gx.z, gy.z);
    float2 g11 = float2(gx.w, gy.w);

    float4 norm = TaylorInvSqrt(float4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;

    float n00 = dot(g00, float2(fx.x, fy.x));
    float n10 = dot(g10, float2(fx.y, fy.y));
    float n01 = dot(g01, float2(fx.z, fy.z));
    float n11 = dot(g11, float2(fx.w, fy.w));

    float2 fade_xy = PerlinNoiseFade(Pf.xy);
    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), fade_xy.x);
    float n_xy = lerp(n_x.x, n_x.y, fade_xy.y);
    return saturate(n_xy * 2.136f + 0.5f); // Rescale to [0;1]
}

// Classic Perlin noise
float PerlinNoise(float2 p)
{
    float4 Pi = floor(p.xyxy) + float4(0.0, 0.0, 1.0, 1.0);
    float4 Pf = frac(p.xyxy) - float4(0.0, 0.0, 1.0, 1.0);
    return PerlinNoiseImpl(Pi, Pf);
}

// Classic Perlin noise with periodic variant
float PerlinNoise(float2 p, float2 rep)
{
    float4 Pi = floor(p.xyxy) + float4(0.0, 0.0, 1.0, 1.0);
    float4 Pf = frac(p.xyxy) - float4(0.0, 0.0, 1.0, 1.0);
    Pi = fmod(Pi, rep.xyxy);
    return PerlinNoiseImpl(Pi, Pf);
}

// Simplex noise
float SimplexNoise(float2 p)
{
    float4 C = float4(0.211324865405187f, // (3.0-math.sqrt(3.0))/6.0
                      0.366025403784439f, // 0.5*(math.sqrt(3.0)-1.0)
                      -0.577350269189626f, // -1.0 + 2.0 * C.x
                      0.024390243902439f); // 1.0 / 41.0

    // First corner
    float2 i = floor(p + dot(p, C.yy));
    float2 x0 = p - i + dot(i, C.xx);

    // Other corners
    float2 i1 = (x0.x > x0.y) ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f);
    float4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;

    // Permutations
    i = Mod289(i);
    float3 perm = Permute(Permute(i.y + float3(0.0f, i1.y, 1.0f)) + i.x + float3(0.0f, i1.x, 1.0f));
    float3 m = max(0.5f - float3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0f);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    float3 x = 2.0f * frac(perm * C.www) - 1.0f;
    float3 h = abs(x) - 0.5f;
    float3 ox = floor(x + 0.5f);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversemath.sqrt( a0*a0 + h*h );
    m *= 1.79284291400159f - 0.85373472095314f * (a0 * a0 + h * h);

    // Compute final noise value at P
    float gx = a0.x * x0.x + h.x * x0.y;
    float2 gyz = a0.yz * x12.xz + h.yz * x12.yw;
    float3 g = float3(gx, gyz);
    return saturate(dot(m, g) * 71.428f + 0.5f); // Rescale to [0;1]
}

// Worley noise (cellar noise with standard 3x3 search window for F1 and F2 values)
float2 WorleyNoise(float2 p)
{
    const float K = 0.142857142857f; // 1/7
    const float Ko = 0.428571428571f; // 3/7
    const float jitter = 1.0f; // Less gives more regular pattern
    float2 Pi = Mod289(floor(p));
    float2 Pf = frac(p);
    float3 oi = float3(-1.0f, 0.0f, 1.0f);
    float3 of = float3(-0.5f, 0.5f, 1.5f);
    float3 px = Permute(Pi.x + oi);
    float3 pp = Permute(px.x + Pi.y + oi); // p11, p12, p13
    float3 ox = frac(pp * K) - Ko;
    float3 oy = Mod7(floor(pp * K)) * K - Ko;
    float3 dx = Pf.x + 0.5f + jitter * ox;
    float3 dy = Pf.y - of + jitter * oy;
    float3 d1 = dx * dx + dy * dy; // d11, d12 and d13, squared
    pp = Permute(px.y + Pi.y + oi); // p21, p22, p23
    ox = frac(pp * K) - Ko;
    oy = Mod7(floor(pp * K)) * K - Ko;
    dx = Pf.x - 0.5f + jitter * ox;
    dy = Pf.y - of + jitter * oy;
    float3 d2 = dx * dx + dy * dy; // d21, d22 and d23, squared
    pp = Permute(px.z + Pi.y + oi); // p31, p32, p33
    ox = frac(pp * K) - Ko;
    oy = Mod7(floor(pp * K)) * K - Ko;
    dx = Pf.x - 1.5f + jitter * ox;
    dy = Pf.y - of + jitter * oy;
    float3 d3 = dx * dx + dy * dy; // d31, d32 and d33, squared
    float3 d1a = min(d1, d2); // Sort out the two smallest distances (F1, F2)
    d2 = max(d1, d2); // Swap to keep candidates for F2
    d2 = min(d2, d3); // neither F1 nor F2 are now in d3
    d1 = min(d1a, d2); // F1 is now in d1
    d2 = max(d1a, d2); // Swap to keep candidates for F2
    d1.xy = (d1.x < d1.y) ? d1.xy : d1.yx; // Swap if smaller
    d1.xz = (d1.x < d1.z) ? d1.xz : d1.zx; // F1 is in d1.x
    d1.yz = min(d1.yz, d2.yz); // F2 is now not in d2.yz
    d1.y = min(d1.y, d1.z); // nor in  d1.z
    d1.y = min(d1.y, d2.x); // F2 is in d1.y, we're done.
    return saturate(sqrt(d1.xy));
}

// Voronoi noise (X=minDistToCell, Y=randomColor, Z=minEdgeDistance)
float3 VoronoiNoise(float2 p)
{
    // Reference: https://www.ronja-tutorials.com/post/028-voronoi-noise/
    float2 baseCell = floor(p);

    // first pass to find the closest cell
    float minDistToCell = 10;
    float2 toClosestCell;
    float2 closestCell;
    UNROLL
    for (int x1 = -1; x1 <= 1; x1++)
    {
        UNROLL
        for (int y1 = -1; y1 <= 1; y1++)
        {
            float2 cell = baseCell + float2(x1, y1);
            float2 cellPosition = cell + rand2dTo2d(cell);
            float2 toCell = cellPosition - p;
            float distToCell = length(toCell);
            if (distToCell < minDistToCell)
            {
                minDistToCell = distToCell;
                closestCell = cell;
                toClosestCell = toCell;
            }
        }
    }

    // second pass to find the distance to the closest edge
    float minEdgeDistance = 10;
    UNROLL
    for (int x2 = -1; x2 <= 1; x2++)
    {
        UNROLL
        for (int y2 = -1; y2 <= 1; y2++)
        {
            float2 cell = baseCell + float2(x2, y2);
            float2 cellPosition = cell + rand2dTo2d(cell);
            float2 toCell = cellPosition - p;
            float2 diffToClosestCell = abs(closestCell - cell);
            if (diffToClosestCell.x + diffToClosestCell.y >= 0.1)
            {
                float2 toCenter = (toClosestCell + toCell) * 0.5;
                float2 cellDifference = normalize(toCell - toClosestCell);
                minEdgeDistance = min(minEdgeDistance, dot(toCenter, cellDifference));
            }
        }
    }

    float random = rand2dTo1d(closestCell);
    return saturate(float3(minDistToCell, random, minEdgeDistance));
}

float CustomNoise(float3 p)
{
    float3 a = floor(p);
    float3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);
	
    float4 b = a.xxyy + float4(0.0, 1.0, 0.0, 1.0);
    float4 k1 = Permute(b.xyxy);
    float4 k2 = Permute(k1.xyxy + b.zzww);
	
    float4 c = k2 + a.zzzz;
    float4 k3 = Permute(c);
    float4 k4 = Permute(c + 1.0);
	
    float4 o1 = frac(k3 * (1.0 / 41.0));
    float4 o2 = frac(k4 * (1.0 / 41.0));
	
    float4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    float2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);
	
    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float3 CustomNoise3D(float3 p)
{
    float o = CustomNoise(p);
    float a = CustomNoise(p + float3(0.0001f, 0.0f, 0.0f));
    float b = CustomNoise(p + float3(0.0f, 0.0001f, 0.0f));
    float c = CustomNoise(p + float3(0.0f, 0.0f, 0.0001f));

    float3 grad = float3(o - a, o - b, o - c);
    float3 ret = cross(grad, abs(grad.zxy));
    if (length(ret) <= 0.0001f) return float3(0.0f, 0.0f, 0.0f);
    return normalize(ret);
}

float3 CustomNoise3D(float3 position, int octaves, float roughness)
{
	float weight = 0.0f;
	float3 noise = float3(0.0f, 0.0f, 0.0f);
	float scale = 1.0f;
    roughness = lerp(2.0f, 0.2f, roughness);
	for (int i = 0; i < octaves; i++)
	{
		float curWeight = pow((1.0f - ((float)i / (float)octaves)), roughness);
		noise += CustomNoise3D(position * scale) * curWeight;
		weight += curWeight;
		scale *= 1.72531f;
	}
	return noise / max(weight, 0.0001f);
}

#endif
