#ifndef PX_FOUNDATION_PX_WEB_INTRINSICS_H
#define PX_FOUNDATION_PX_WEB_INTRINSICS_H

#include "foundation/Px.h"
#include "foundation/PxSharedAssert.h"

#if !PX_WEB
	#error "This file should only be included by Web builds!!"
#endif

#include <math.h>
#include <float.h>

#include "nn/cstd/cstd_CMath.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

namespace intrinsics
{
	//! \brief platform-specific absolute value
	PX_CUDA_CALLABLE PX_FORCE_INLINE float abs(float a)						{	return ::fabsf(a);	}

	//! \brief platform-specific select float
	PX_CUDA_CALLABLE PX_FORCE_INLINE float fsel(float a, float b, float c)	{	return (a >= 0.0f) ? b : c;	}

	//! \brief platform-specific sign
	PX_CUDA_CALLABLE PX_FORCE_INLINE float sign(float a)					{	return (a >= 0.0f) ? 1.0f : -1.0f; }

	//! \brief platform-specific reciprocal
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recip(float a)					{	return 1.0f/a;			}

	//! \brief platform-specific reciprocal estimate
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recipFast(float a)				{	return 1.0f/a;			}

	//! \brief platform-specific square root
	PX_CUDA_CALLABLE PX_FORCE_INLINE float sqrt(float a)					{	return ::sqrtf(a);	}

	//! \brief platform-specific reciprocal square root
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recipSqrt(float a)				{   return 1.0f/::sqrtf(a); }

	//! \brief platform-specific reciprocal square root estimate
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recipSqrtFast(float a)			{	return 1.0f/::sqrtf(a); }

	//! \brief platform-specific sine
	PX_CUDA_CALLABLE PX_FORCE_INLINE float sin(float a)						{   return ::sinf(a); }

	//! \brief platform-specific cosine
	PX_CUDA_CALLABLE PX_FORCE_INLINE float cos(float a)						{   return ::cosf(a); }

	//! \brief platform-specific minimum
	PX_CUDA_CALLABLE PX_FORCE_INLINE float selectMin(float a, float b)		{	return a<b ? a : b;	}

	//! \brief platform-specific maximum
	PX_CUDA_CALLABLE PX_FORCE_INLINE float selectMax(float a, float b)		{	return a>b ? a : b; }

	//! \brief platform-specific finiteness check
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isFinite(float a)
	{
#ifdef __CUDACC__
		return isfinite(a) ? true : false;
#else
		return !nn::cstd::IsNan(a) && !nn::cstd::IsInf(a);
#endif
	}

	//! \brief platform-specific finiteness check
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isFinite(double a)
	{
#ifdef __CUDACC__
		return isfinite(a) ? true : false;
#else
		return !nn::cstd::IsNan(a) && !nn::cstd::IsInf(a);
#endif
	}

	/*!
	Sets \c count bytes starting at \c dst to zero.
	*/
	PX_FORCE_INLINE void* memZero(void* PX_RESTRICT dest, uint32_t count)
	{
		return memset(dest, 0, count);
	}

	/*!
	Sets \c count bytes starting at \c dst to \c c.
	*/
	PX_FORCE_INLINE void* memSet(void* PX_RESTRICT dest, int32_t c, uint32_t count)
	{
		return memset(dest, c, count);
	}

	/*!
	Copies \c count bytes from \c src to \c dst. User memMove if regions overlap.
	*/
	PX_FORCE_INLINE void* memCopy(void* PX_RESTRICT dest, const void* PX_RESTRICT src, uint32_t count)
	{
		return memcpy(dest, src, count);
	}

	/*!
	Copies \c count bytes from \c src to \c dst. Supports overlapping regions.
	*/
	PX_FORCE_INLINE void* memMove(void* PX_RESTRICT dest, const void* PX_RESTRICT src, uint32_t count)
	{
		return memmove(dest, src, count);
	}

	/*!
	Set 128B to zero starting at \c dst+offset. Must be aligned.
	*/
	PX_FORCE_INLINE void memZero128(void* PX_RESTRICT dest, uint32_t offset = 0)
	{
		PX_SHARED_ASSERT(((size_t(dest)+offset) & 0x7f) == 0);
		memSet((char* PX_RESTRICT)dest+offset, 0, 128);
	}

} // namespace intrinsics
#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
