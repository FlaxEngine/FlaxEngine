// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __MATH__
#define __MATH__

#define RadiansToDegrees (180.0f / PI)
#define DegreesToRadians (PI / 180.0f)

uint NextPow2(uint value)
{
    uint mask = (1 << firstbithigh(value)) - 1;
    return (value + mask) & ~mask;
}

float3 SafeNormalize(float3 v)
{
    return v / sqrt(max(dot(v, v), 0.01));
}

float3 ExtractLargestComponent(float3 v)
{
    float3 a = abs(v);
    if (a.x > a.y)
    {
        if (a.x > a.z)
        {
            return float3(v.x > 0 ? 1 : -1, 0, 0);
        }
    }
    else
    {
        if (a.y > a.z)
        {
            return float3(0, v.y > 0 ? 1 : -1, 0);
        }
    }
    return float3(0, 0, v.z > 0 ? 1 : -1);
}

float Square(float x)
{
    return x * x;
}

float2 Square(float2 x)
{
    return x * x;
}

float3 Square(float3 x)
{
    return x * x;
}

float4 Square(float4 x)
{
    return x * x;
}

float Min2(float2 x)
{
    return min(x.x, x.y);
}

float Min3(float3 x)
{
    return min(x.x, min(x.y, x.z));
}

float Min4(float4 x)
{
    return min(x.x, min(x.y, min(x.z, x.w)));
}

float Max2(float2 x)
{
    return max(x.x, x.y);
}

float Max3(float3 x)
{
    return max(x.x, max(x.y, x.z));
}

float Max4(float4 x)
{
    return max(x.x, max(x.y, max(x.z, x.w)));
}

float Pow2(float x)
{
    return x * x;
}

float2 Pow2(float2 x)
{
    return x * x;
}

float3 Pow2(float3 x)
{
    return x * x;
}

float4 Pow2(float4 x)
{
    return x * x;
}

float Pow3(float x)
{
    return x * x * x;
}

float2 Pow3(float2 x)
{
    return x * x * x;
}

float3 Pow3(float3 x)
{
    return x * x * x;
}

float4 Pow3(float4 x)
{
    return x * x * x;
}

float Pow4(float x)
{
    float xx = x * x;
    return xx * xx;
}

float2 Pow4(float2 x)
{
    float2 xx = x * x;
    return xx * xx;
}

float3 Pow4(float3 x)
{
    float3 xx = x * x;
    return xx * xx;
}

float4 Pow4(float4 x)
{
    float4 xx = x * x;
    return xx * xx;
}

float Pow5(float x)
{
    float xx = x * x;
    return xx * xx * x;
}

float2 Pow5(float2 x)
{
    float2 xx = x * x;
    return xx * xx * x;
}

float3 Pow5(float3 x)
{
    float3 xx = x * x;
    return xx * xx * x;
}

float4 Pow5(float4 x)
{
    float4 xx = x * x;
    return xx * xx * x;
}

float Pow6(float x)
{
    float xx = x * x;
    return xx * xx * xx;
}

float2 Pow6(float2 x)
{
    float2 xx = x * x;
    return xx * xx * xx;
}

float3 Pow6(float3 x)
{
    float3 xx = x * x;
    return xx * xx * xx;
}

float4 Pow6(float4 x)
{
    float4 xx = x * x;
    return xx * xx * xx;
}

float ClampedPow(float x, float y)
{
    return pow(max(abs(x), 0.000001f), y);
}

float2 ClampedPow(float2 x, float2 y)
{
    return pow(max(abs(x), float2(0.000001f, 0.000001f)), y);
}

float3 ClampedPow(float3 x, float3 y)
{
    return pow(max(abs(x), float3(0.000001f, 0.000001f, 0.000001f)), y);
}

float4 ClampedPow(float4 x, float4 y)
{
    return pow(max(abs(x), float4(0.000001f, 0.000001f, 0.000001f, 0.000001f)), y);
}

float4 FindQuatBetween(float3 from, float3 to)
{
    // http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
    float normAB = 1.0f;
    float w = normAB + dot(from, to);
    float4 result;
    if (w >= 1e-6f * normAB)
    {
        result = float4
        (
            from.y * to.z - from.z * to.y,
            from.z * to.x - from.x * to.z,
            from.x * to.y - from.y * to.x,
            w
        );
    }
    else
    {
        result = abs(from.x) > abs(from.y)
                     ? float4(-from.z, 0.f, from.x, 0.0f)
                     : float4(0.f, -from.z, from.y, 0.0f);
    }
    return normalize(result);
}

// Rotates position about the input axis by the given angle (in radians), and returns the delta to position
float3 RotateAboutAxis(float4 normalizedRotationAxisAndAngle, float3 positionOnAxis, float3 position)
{
    float3 pointOnAxis = positionOnAxis + normalizedRotationAxisAndAngle.xyz * dot(normalizedRotationAxisAndAngle.xyz, position - positionOnAxis);
    float3 axisU = position - pointOnAxis;
    float3 axisV = cross(normalizedRotationAxisAndAngle.xyz, axisU);
    float cosAngle, sinAngle;
    sincos(normalizedRotationAxisAndAngle.w, sinAngle, cosAngle);
    float3 rotation = axisU * cosAngle + axisV * sinAngle;
    return pointOnAxis + rotation - position;
}

#endif
