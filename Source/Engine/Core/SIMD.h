// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#if PLATFORM_SIMD_SSE2
#ifdef _WIN32
#include <xmmintrin.h>
#endif
#else
#include <math.h>
#endif

#if PLATFORM_SIMD_SSE2

// Vector of four floating point values stored in a vector register.
typedef __m128 SimdVector4;

namespace SIMD
{
    FORCE_INLINE SimdVector4 Load(float xyzw)
    {
        return _mm_set1_ps(xyzw);
    }

    FORCE_INLINE SimdVector4 Load(float x, float y, float z, float w)
    {
        return _mm_set_ps(w, z, y, x);
    }

    FORCE_INLINE SimdVector4 Load(const float* __restrict src)
    {
        return _mm_loadu_ps(src);
    }

    FORCE_INLINE SimdVector4 LoadAligned(const float* __restrict src)
    {
        ASSERT_LOW_LAYER(((uintptr)src & 15) == 0);
        return _mm_load_ps(src);
    }

    FORCE_INLINE SimdVector4 Splat(float value)
    {
        return _mm_set_ps1(value);
    }

    FORCE_INLINE void Store(float* __restrict dst, SimdVector4 src)
    {
        _mm_storeu_ps(dst, src);
    }

    FORCE_INLINE void StoreAligned(float* __restrict dst, SimdVector4 src)
    {
        ASSERT_LOW_LAYER(((uintptr)dst & 15) == 0);
        _mm_store_ps(dst, src);
    }

    FORCE_INLINE int MoveMask(SimdVector4 a)
    {
        return _mm_movemask_ps(a);
    }

    FORCE_INLINE SimdVector4 Add(SimdVector4 a, SimdVector4 b)
    {
        return _mm_add_ps(a, b);
    }

    FORCE_INLINE SimdVector4 Sub(SimdVector4 a, SimdVector4 b)
    {
        return _mm_sub_ps(a, b);
    }

    FORCE_INLINE SimdVector4 Mul(SimdVector4 a, SimdVector4 b)
    {
        return _mm_mul_ps(a, b);
    }

    FORCE_INLINE SimdVector4 Div(SimdVector4 a, SimdVector4 b)
    {
        return _mm_div_ps(a, b);
    }

    FORCE_INLINE SimdVector4 Rcp(SimdVector4 a)
    {
        return _mm_rcp_ps(a);
    }

    FORCE_INLINE SimdVector4 Sqrt(SimdVector4 a)
    {
        return _mm_sqrt_ps(a);
    }

    FORCE_INLINE SimdVector4 Rsqrt(SimdVector4 a)
    {
        return _mm_rsqrt_ps(a);
    }

    FORCE_INLINE SimdVector4 Min(SimdVector4 a, SimdVector4 b)
    {
        return _mm_min_ps(a, b);
    }

    FORCE_INLINE SimdVector4 Max(SimdVector4 a, SimdVector4 b)
    {
        return _mm_max_ps(a, b);
    }
}

#else

struct SimdVector4
{
	float X, Y, Z, W;
};

namespace SIMD
{
    FORCE_INLINE SimdVector4 Load(float xyzw)
    {
		return { xyzw, xyzw, xyzw, xyzw };
    }

    FORCE_INLINE SimdVector4 Load(float x, float y, float z, float w)
    {
		return { x, y, z, w };
    }

	FORCE_INLINE SimdVector4 Load(const float* __restrict src)
	{
		return *(const SimdVector4*)src;
	}

	FORCE_INLINE SimdVector4 LoadAligned(const float* __restrict src)
	{
		return *(const SimdVector4*)src;
	}

	FORCE_INLINE SimdVector4 Splat(float value)
	{
		return { value, value, value, value };
	}

    FORCE_INLINE void Store(float* __restrict dst, SimdVector4 src)
    {
		(*(SimdVector4*)dst) = src;
    }

    FORCE_INLINE void StoreAligned(float* __restrict dst, SimdVector4 src)
    {
		(*(SimdVector4*)dst) = src;
    }

	FORCE_INLINE int MoveMask(SimdVector4 a)
	{
		return (a.W < 0 ? (1 << 3) : 0) |
				(a.Z < 0 ? (1 << 2) : 0) |
				(a.Y < 0 ? (1 << 1) : 0) |
				(a.X < 0 ? 1 : 0);
	}

	FORCE_INLINE SimdVector4 Add(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X + b.X,
			a.Y + b.Y,
			a.Z + b.Z,
			a.W + b.W
		};
	}

	FORCE_INLINE SimdVector4 Sub(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X - b.X,
			a.Y - b.Y,
			a.Z - b.Z,
			a.W - b.W
		};
	}

	FORCE_INLINE SimdVector4 Mul(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X * b.X,
			a.Y * b.Y,
			a.Z * b.Z,
			a.W * b.W
		};
	}

	FORCE_INLINE SimdVector4 Div(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X / b.X,
			a.Y / b.Y,
			a.Z / b.Z,
			a.W / b.W
		};
	}

	FORCE_INLINE SimdVector4 Rcp(SimdVector4 a)
	{
		return
		{
			1 / a.X,
			1 / a.Y,
			1 / a.Z,
			1 / a.W
		};
	}

	FORCE_INLINE SimdVector4 Sqrt(SimdVector4 a)
	{
		return
		{
			(float)sqrt(a.X),
			(float)sqrt(a.Y),
			(float)sqrt(a.Z),
			(float)sqrt(a.W)
		};
	}

	FORCE_INLINE SimdVector4 Rsqrt(SimdVector4 a)
	{
		return
		{
			1 / (float)sqrt(a.X),
			1 / (float)sqrt(a.Y),
			1 / (float)sqrt(a.Z),
			1 / (float)sqrt(a.W)
		};
	}

	FORCE_INLINE SimdVector4 Min(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X < b.X ? a.X : b.X,
			a.Y < b.Y ? a.Y : b.Y,
			a.Z < b.Z ? a.Z : b.Z,
			a.W < b.W ? a.W : b.W
		};
	}

	FORCE_INLINE SimdVector4 Max(SimdVector4 a, SimdVector4 b)
	{
		return
		{
			a.X > b.X ? a.X : b.X,
			a.Y > b.Y ? a.Y : b.Y,
			a.Z > b.Z ? a.Z : b.Z,
			a.W > b.W ? a.W : b.W
		};
	}
}

#endif
