// Source: https://github.com/GPUOpen-Effects/TressFX
// License: MIT

//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

//When building the SDF we want to find the lowest distance at each SDF cell. In order to allow multiple threads to write to the same
//cells, it is necessary to use atomics. However, there is no support for atomics with 32-bit floats so we convert the float into unsigned int
//and use atomic_min() / InterlockedMin() as a workaround.
//
//When used with atomic_min, both FloatFlip2() and FloatFlip3() will store the float with the lowest magnitude.
//The difference is that FloatFlip2() will preper negative values ( InterlockedMin( FloatFlip2(1.0), FloatFlip2(-1.0) ) == -1.0 ),
//while FloatFlip3() prefers positive values (  InterlockedMin( FloatFlip3(1.0), FloatFlip3(-1.0) ) == 1.0 ).
//Using FloatFlip3() seems to result in a SDF with higher quality compared to FloatFlip2().
uint FloatFlip2(float fl)
{
    uint f = asuint(fl);
    return (f << 1) | (f >> 31 ^ 0x00000001);		//Rotate sign bit to least significant and Flip sign bit so that (0 == negative)
}
uint IFloatFlip2(uint f2)
{
    return (f2 >> 1) | (f2 << 31 ^ 0x80000000);
}
uint FloatFlip3(float fl)
{
    uint f = asuint(fl);
    return (f << 1) | (f >> 31);		//Rotate sign bit to least significant
}
uint IFloatFlip3(uint f2)
{
    return (f2 >> 1) | (f2 << 31);
}

float DistancePointToEdge(float3 p, float3 x0, float3 x1, out float3 n)
{
    float3 x10 = x1 - x0;

    float t = dot(x1 - p, x10) / dot(x10, x10);
    t = max(0.0f, min(t, 1.0f));

    float3 a = p - (t*x0 + (1.0f - t)*x1);
    float d = length(a);
    n = a / (d + 1e-30f);

    return d;
}

// Check if p is in the positive or negative side of triangle (x0, x1, x2)
// Positive side is where the normal vector of triangle ( (x1-x0) x (x2-x0) ) is pointing to.
float SignedDistancePointToTriangle(float3 p, float3 x0, float3 x1, float3 x2)
{
    float d = 0;
    float3 x02 = x0 - x2;
    float l0 = length(x02) + 1e-30f;
    x02 = x02 / l0;
    float3 x12 = x1 - x2;
    float l1 = dot(x12, x02);
    x12 = x12 - l1*x02;
    float l2 = length(x12) + 1e-30f;
    x12 = x12 / l2;
    float3 px2 = p - x2;

    float b = dot(x12, px2) / l2;
    float a = (dot(x02, px2) - l1*b) / l0;
    float c = 1 - a - b;

    // normal vector of triangle. Don't need to normalize this yet.
    float3 nTri = cross((x1 - x0), (x2 - x0));
    float3 n;

    float tol = 1e-8f;

    if (a >= -tol && b >= -tol && c >= -tol)
    {
        n = p - (a*x0 + b*x1 + c*x2);
        d = length(n);

        float3 n1 = n / d;
        float3 n2 = nTri / (length(nTri) + 1e-30f);		// if d == 0

        n = (d > 0) ? n1 : n2;
    }
    else
    {
        float3 n_12;
        float3 n_02;
        d = DistancePointToEdge(p, x0, x1, n);

        float d12 = DistancePointToEdge(p, x1, x2, n_12);
        float d02 = DistancePointToEdge(p, x0, x2, n_02);

        d = min(d, d12);
        d = min(d, d02);

        n = (d == d12) ? n_12 : n;
        n = (d == d02) ? n_02 : n;
    }

    d = (dot(p - x0, nTri) < 0.f) ? -d : d;

    return d;
}
