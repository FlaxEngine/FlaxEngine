// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __QUATERNION__
#define __QUATERNION__

float4 QuaternionMultiply(float4 q1, float4 q2)
{
    return float4(q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz), q1.w * q2.w - dot(q1.xyz, q2.xyz));
}

float3 QuaternionRotate(float4 q, float3 v)
{
    float3 b = q.xyz;
    float b2 = dot(b, b);
    return (v * (q.w * q.w - b2) + b * (dot(v, b) * 2.f) + cross(b, v) * (q.w * 2.f));
}

float4 QuaternionFromAxisAngle(float3 axis, float angle)
{
    return float4(axis * sin(angle * 0.5f), cos(angle * 0.5f));
}

#endif
