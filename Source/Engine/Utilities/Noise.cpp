// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license.
// https://github.com/stegu/webgl-noise

#include "Noise.h"
#include "Engine/Core/Math/Vector4.h"

namespace
{
    FORCE_INLINE Float2 Mod289(const Float2& x)
    {
        return x - Float2::Floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    FORCE_INLINE Float3 Mod289(const Float3& x)
    {
        return x - Float3::Floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    FORCE_INLINE Float4 Mod289(const Float4& x)
    {
        return x - Float4::Floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    FORCE_INLINE Float3 Mod7(const Float3& x)
    {
        return x - Float3::Floor(x * (1.0f / 7.0f)) * 7.0f;
    }

    FORCE_INLINE Float3 Permute(const Float3& x)
    {
        return Mod289((x * 34.0f + 1.0f) * x);
    }

    FORCE_INLINE Float4 Permute(const Float4& x)
    {
        return Mod289((x * 34.0f + 1.0f) * x);
    }

    FORCE_INLINE Float4 TaylorInvSqrt(const Float4& r)
    {
        return 1.79284291400159f - 0.85373472095314f * r;
    }

    FORCE_INLINE Float2 PerlinNoiseFade(const Float2& t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    float rand2dTo1d(const Float2& value, const Float2& dotDir)
    {
        // https://www.ronja-tutorials.com/post/024-white-noise/
        const Float2 smallValue(Math::Sin(value.X), Math::Sin(value.Y));
        const float random = Float2::Dot(smallValue, dotDir);
        return Math::Frac(Math::Sin(random) * 143758.5453f);
    }

    float rand2dTo1d(const Float2& value)
    {
        // https://www.ronja-tutorials.com/post/024-white-noise/
        return rand2dTo1d(value, Float2(12.9898f, 78.233f));
    }

    Float2 rand2dTo2d(const Float2& value)
    {
        // https://www.ronja-tutorials.com/post/024-white-noise/
        return Float2(
            rand2dTo1d(value, Float2(12.989f, 78.233f)),
            rand2dTo1d(value, Float2(39.346f, 11.135f))
        );
    }

    float PerlinNoiseImpl(const Float2& p, const Float2* rep)
    {
        Float4 Pxy(p.X, p.Y, p.X, p.Y);
        Float4 Pi = Float4::Floor(Pxy) + Float4(0.0, 0.0, 1.0, 1.0);
        Float4 Pf = Float4::Frac(Pxy) - Float4(0.0, 0.0, 1.0, 1.0);
        if (rep)
        {
            Float4 Repxy(rep->X, rep->Y, rep->X, rep->Y);
            Pi = Float4::Mod(Pi, Repxy);
        }
        Pi = Mod289(Pi);
        Float4 ix(Pi.X, Pi.Z, Pi.X, Pi.Z);
        Float4 iy(Pi.Y, Pi.Y, Pi.W, Pi.W);
        Float4 fx(Pf.X, Pf.Z, Pf.X, Pf.Z);
        Float4 fy(Pf.Y, Pf.Y, Pf.W, Pf.W);

        Float4 i = Permute(Permute(ix) + iy);

        Float4 gx = Float4::Frac(i * (1.0 / 41.0)) * 2.0 - 1.0;
        Float4 gy = Float4::Abs(gx) - 0.5;
        Float4 tx = Float4::Floor(gx + 0.5);
        gx = gx - tx;

        Float2 g00(gx.X, gy.X);
        Float2 g10(gx.Y, gy.Y);
        Float2 g01(gx.Z, gy.Z);
        Float2 g11(gx.W, gy.W);

        Float4 norm = TaylorInvSqrt(Float4(Float2::Dot(g00, g00), Float2::Dot(g01, g01), Float2::Dot(g10, g10), Float2::Dot(g11, g11)));
        g00 *= norm.X;
        g01 *= norm.Y;
        g10 *= norm.Z;
        g11 *= norm.W;

        float n00 = Float2::Dot(g00, Float2(fx.X, fy.X));
        float n10 = Float2::Dot(g10, Float2(fx.Y, fy.Y));
        float n01 = Float2::Dot(g01, Float2(fx.Z, fy.Z));
        float n11 = Float2::Dot(g11, Float2(fx.W, fy.W));

        Float2 fade_xy = PerlinNoiseFade(Float2(Pf));
        Float2 n_x = Float2::Lerp(Float2(n00, n01), Float2(n10, n11), fade_xy.X);
        float n_xy = Math::Lerp(n_x.X, n_x.Y, fade_xy.Y);
        return Math::Saturate(n_xy * 2.136f + 0.5f); // Rescale to [0;1]
    }
}

float Noise::PerlinNoise(const Float2& p)
{
    return PerlinNoiseImpl(p, nullptr);
}

float Noise::PerlinNoise(const Float2& p, const Float2& rep)
{
    return PerlinNoiseImpl(p, &rep);
}

float Noise::SimplexNoise(const Float2& p)
{
    Float4 C(0.211324865405187f, // (3.0-math.sqrt(3.0))/6.0
             0.366025403784439f, // 0.5*(math.sqrt(3.0)-1.0)
             -0.577350269189626f, // -1.0 + 2.0 * C.x
             0.024390243902439f); // 1.0 / 41.0

    // First corner
    Float2 i = Float2::Floor(p + Float2::Dot(p, Float2(C.Y, C.Y)));
    Float2 x0 = p - i + Float2::Dot(i, Float2(C.X, C.X));

    // Other corners
    Float2 i1 = x0.X > x0.Y ? Float2(1.0f, 0.0f) : Float2(0.0f, 1.0f);
    Float4 x12 = Float4(x0.X, x0.Y, x0.X, x0.Y) + Float4(C.X, C.X, C.Z, C.Z);
    x12.X -= i1.X;
    x12.Y -= i1.Y;

    // Permutations
    i = Mod289(i);
    Float3 perm = Permute(Permute(i.Y + Float3(0.0f, i1.Y, 1.0f)) + i.X + Float3(0.0f, i1.X, 1.0f));
    Float2 x12xy(x12.X, x12.Y);
    Float2 x12zw(x12.Z, x12.W);
    Float3 m = Float3::Max(0.5f - Float3(Float2::Dot(x0, x0), Float2::Dot(x12xy, x12xy), Float2::Dot(x12zw, x12zw)), 0.0f);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    Float3 x = 2.0f * Float3::Frac(perm * C.W) - 1.0f;
    Float3 h = Float3::Abs(x) - 0.5f;
    Float3 ox = Float3::Floor(x + 0.5f);
    Float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversemath.sqrt( a0*a0 + h*h );
    m *= 1.79284291400159f - 0.85373472095314f * (a0 * a0 + h * h);

    // Compute final noise value at P
    float gx = a0.X * x0.X + h.X * x0.Y;
    Float2 gyz = Float2(a0.Y, a0.Z) * Float2(x12.X, x12.Z) + Float2(h.Y, h.Z) * Float2(x12.Y, x12.W);
    Float3 g(gx, gyz.X, gyz.Y);
    return Math::Saturate(Float3::Dot(m, g) * 71.428f + 0.5f); // Rescale to [0;1]
}

Float2 Noise::WorleyNoise(const Float2& p)
{
    const float K = 0.142857142857f; // 1/7
    const float Ko = 0.428571428571f; // 3/7
    const float jitter = 1.0f; // Less gives more regular pattern
    Float2 Pi = Mod289(Float2::Floor(p));
    Float2 Pf = Float2::Frac(p);
    Float3 oi = Float3(-1.0f, 0.0f, 1.0f);
    Float3 of = Float3(-0.5f, 0.5f, 1.5f);
    Float3 px = Permute(Pi.X + oi);
    Float3 pp = Permute(px.X + Pi.Y + oi); // p11, p12, p13
    Float3 ox = Float3::Frac(pp * K) - Ko;
    Float3 oy = Mod7(Float3::Floor(pp * K)) * K - Ko;
    Float3 dx = Pf.X + 0.5f + jitter * ox;
    Float3 dy = Pf.Y - of + jitter * oy;
    Float3 d1 = dx * dx + dy * dy; // d11, d12 and d13, squared
    pp = Permute(px.Y + Pi.Y + oi); // p21, p22, p23
    ox = Float3::Frac(pp * K) - Ko;
    oy = Mod7(Float3::Floor(pp * K)) * K - Ko;
    dx = Pf.X - 0.5f + jitter * ox;
    dy = Pf.Y - of + jitter * oy;
    Float3 d2 = dx * dx + dy * dy; // d21, d22 and d23, squared
    pp = Permute(px.Z + Pi.Y + oi); // p31, p32, p33
    ox = Float3::Frac(pp * K) - Ko;
    oy = Mod7(Float3::Floor(pp * K)) * K - Ko;
    dx = Pf.X - 1.5f + jitter * ox;
    dy = Pf.Y - of + jitter * oy;
    Float3 d3 = dx * dx + dy * dy; // d31, d32 and d33, squared
    Float3 d1a = Float3::Min(d1, d2); // Sort out the two smallest distances (F1, F2)
    d2 = Float3::Max(d1, d2); // Swap to keep candidates for F2
    d2 = Float3::Min(d2, d3); // neither F1 nor F2 are now in d3
    d1 = Float3::Min(d1a, d2); // F1 is now in d1
    d2 = Float3::Max(d1a, d2); // Swap to keep candidates for F2
    d1.X = d1.X < d1.Y ? d1.X : d1.Y; // Swap if smaller
    d1.Y = d1.X < d1.Y ? d1.Y : d1.X;
    d1.X = d1.X < d1.Z ? d1.X : d1.Z; // F1 is in d1.x
    d1.Z = d1.X < d1.Z ? d1.Z : d1.X;
    d1.Y = Math::Min(d1.Y, d2.Y); // F2 is now not in d2.yz
    d1.Z = Math::Min(d1.Z, d2.Z);
    d1.Y = Math::Min(d1.Y, d1.Z); // nor in  d1.z
    d1.Y = Math::Min(d1.Y, d2.X); // F2 is in d1.y, we're done.
    return Float2(Math::Saturate(Math::Sqrt(d1.X)), Math::Saturate(Math::Sqrt(d1.Y)));
}

Float3 Noise::VoronoiNoise(const Float2& p)
{
    // Reference: https://www.ronja-tutorials.com/post/028-voronoi-noise/
    const Float2 baseCell = Float2::Floor(p);

    // first pass to find the closest cell
    float minDistToCell = 10.0;
    Float2 toClosestCell;
    Float2 closestCell;
    for (int32 x1 = -1; x1 <= 1; x1++)
    {
        for (int32 y1 = -1; y1 <= 1; y1++)
        {
            Float2 cell = baseCell + Float2((float)x1, (float)y1);
            Float2 cellPosition = cell + rand2dTo2d(cell);
            Float2 toCell = cellPosition - p;
            const float distToCell = toCell.Length();
            if (distToCell < minDistToCell)
            {
                minDistToCell = distToCell;
                closestCell = cell;
                toClosestCell = toCell;
            }
        }
    }

    // second pass to find the distance to the closest edge
    float minEdgeDistance = 10.0;
    for (int32 x2 = -1; x2 <= 1; x2++)
    {
        for (int32 y2 = -1; y2 <= 1; y2++)
        {
            Float2 cell = baseCell + Float2((float)x2, (float)y2);
            Float2 cellPosition = cell + rand2dTo2d(cell);
            Float2 toCell = cellPosition - p;
            const Float2 diffToClosestCell = Float2::Abs(closestCell - cell);
            if (diffToClosestCell.X + diffToClosestCell.Y >= 0.1f)
            {
                Float2 toCenter = (toClosestCell + toCell) * 0.5;
                Float2 cellDifference = Float2::Normalize(toCell - toClosestCell);
                minEdgeDistance = Math::Min(minEdgeDistance, Float2::Dot(toCenter, cellDifference));
            }
        }
    }

    float random = rand2dTo1d(closestCell);
    return Float3(Math::Saturate(minDistToCell), Math::Saturate(random), Math::Saturate(minEdgeDistance));
}

float Noise::CustomNoise(const Float3& p)
{
    Float3 a = Float3::Floor(p);
    Float3 d = p - a;
    d = d * d * (3.0f - 2.0f * d);

    Float4 b(a.X, a.X + 1.0f, a.Y, a.Y + 1.0f);
    Float4 k1 = Permute(Float4(b.X, b.Y, b.X, b.Y));
    Float4 k2 = Permute(Float4(k1.X + b.Z, k1.Y + b.Z, k1.X + b.W, k1.Y + b.W));

    Float4 c = k2 + Float4(a.Z);
    Float4 k3 = Permute(c);
    Float4 k4 = Permute(c + 1.0f);

    Float4 o1 = Float4::Frac(k3 * (1.0f / 41.0f));
    Float4 o2 = Float4::Frac(k4 * (1.0f / 41.0f));

    Float4 o3 = o2 * d.Z + o1 * (1.0f - d.Z);
    Float2 o4 = Float2(o3.Y, o3.W) * d.X + Float2(o3.X, o3.Z) * (1.0f - d.X);

    return o4.Y * d.Y + o4.X * (1.0f - d.Y);
}

Float3 Noise::CustomNoise3D(const Float3& p)
{
    const float o = CustomNoise(p);
    const float a = CustomNoise(p + Float3(0.0001f, 0.0f, 0.0f));
    const float b = CustomNoise(p + Float3(0.0f, 0.0001f, 0.0f));
    const float c = CustomNoise(p + Float3(0.0f, 0.0f, 0.0001f));

    const Float3 grad(o - a, o - b, o - c);
    const Float3 other = Float3::Abs(Float3(grad.Z, grad.X, grad.Y));
    return Float3::Normalize(Float3::Cross(grad, other));
}

Float3 Noise::CustomNoise3D(const Float3& p, int32 octaves, float roughness)
{
    float weight = 0.0f;
    Float3 noise = Float3::Zero;
    float scale = 1.0f;
    for (int32 i = 0; i < octaves; i++)
    {
        const float curWeight = Math::Pow(1.0f - (float)i / (float)octaves, Math::Lerp(2.0f, 0.2f, roughness));
        noise += CustomNoise3D(p * scale) * curWeight;
        weight += curWeight;
        scale *= 1.72531f;
    }
    return noise / Math::Max(weight, ZeroTolerance);
}
