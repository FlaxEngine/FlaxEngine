// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __MONTE_CARLO__
#define __MONTE_CARLO__

float3 TangentToWorld(float3 vec, float3 tangentZ)
{
    float3 upVector = abs(tangentZ.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangentX = normalize(cross(upVector, tangentZ));
    float3 tangentY = cross(tangentZ, tangentX);
    return tangentX * vec.x + tangentY * vec.y + tangentZ * vec.z;
}

uint ReverseBits32(uint bits)
{
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
    return reversebits(bits);
#else
	bits = ( bits << 16) | ( bits >> 16);
	bits = ( (bits & 0x00ff00ff) << 8 ) | ( (bits & 0xff00ff00) >> 8 );
	bits = ( (bits & 0x0f0f0f0f) << 4 ) | ( (bits & 0xf0f0f0f0) >> 4 );
	bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
	bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
	return bits;
#endif
}

float2 Hammersley(uint index, uint numSamples, uint2 random)
{
    float e1 = frac((float)index / numSamples + float(random.x & 0xffff) / (1 << 16));
    float e2 = float(ReverseBits32(index) ^ random.y) * 2.3283064365386963e-10;
    return float2(e1, e2);
}

float4 UniformSampleSphere(float2 e)
{
    float phi = 2 * PI * e.x;
    float cosTheta = 1 - 2 * e.y;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    float pdf = 1.0 / (4 * PI);
    return float4(h, pdf);
}

float4 UniformSampleHemisphere(float2 e)
{
    float phi = 2 * PI * e.x;
    float cosTheta = e.y;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    float pdf = 1.0 / (2 * PI);
    return float4(h, pdf);
}

float4 CosineSampleHemisphere(float2 e)
{
    float phi = 2 * PI * e.x;
    float cosTheta = sqrt(e.y);
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    float pdf = cosTheta / PI;
    return float4(h, pdf);
}

float4 UniformSampleCone(float2 e, float cosThetaMax)
{
    float phi = 2 * PI * e.x;
    float cosTheta = lerp(cosThetaMax, 1, e.y);
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 l;
    l.x = sinTheta * cos(phi);
    l.y = sinTheta * sin(phi);
    l.z = cosTheta;

    float pdf = 1.0 / (2 * PI * (1 - cosThetaMax));
    return float4(l, pdf);
}

float4 ImportanceSampleBlinn(float2 e, float roughness)
{
    float m = roughness * roughness;
    float n = 2 / (m * m) - 2;

    float phi = 2 * PI * e.x;
    float cosTheta = ClampedPow(e.y, 1 / (n + 1));
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    float d = (n + 2) / (2 * PI) * ClampedPow(cosTheta, n);
    float pdf = d * cosTheta;
    return float4(h, pdf);
}

float4 ImportanceSampleGGX(float2 e, float roughness)
{
    float m = roughness * roughness;
    float m2 = m * m;

    float phi = 2 * PI * e.x;
    float cosTheta = sqrt((1 - e.y) / (1 + (m2 - 1) * e.y));
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    float r = (cosTheta * m2 - cosTheta) * cosTheta + 1;
    float d = m2 / (PI * r * r);
    float pdf = d * cosTheta;
    return float4(h, pdf);
}

// Multiple importance sampling power heuristic of two functions with a power of two. 
// [Veach 1997, "Robust Monte Carlo Methods for Light Transport Simulation"]
float MISWeight(uint number, float PDF, uint otherNumber, float otherPDF)
{
    float weight = number * PDF;
    float otherWeight = otherNumber * otherPDF;
    return weight * weight / (weight * weight + otherWeight * otherWeight);
}

#endif
