// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PX_MATH_H
#define PX_MATH_H

/** \addtogroup foundation
@{
*/

#include "foundation/PxPreprocessor.h"


#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4985) // 'symbol name': attributes not present on previous declaration
#endif
#include <math.h>
#if PX_VC
#pragma warning(pop)
#endif

#if (PX_LINUX_FAMILY && !PX_ARM_FAMILY)
// Force linking against nothing newer than glibc v2.17 to remain compatible with platforms with older glibc versions
__asm__(".symver expf,expf@GLIBC_2.2.5");
__asm__(".symver powf,powf@GLIBC_2.2.5");
#endif

#include <float.h>
#include "foundation/PxMathIntrinsics.h"
#include "foundation/PxAssert.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

// constants
static const float PxPi = float(3.141592653589793);
static const float PxHalfPi = float(1.57079632679489661923);
static const float PxTwoPi = float(6.28318530717958647692);
static const float PxInvPi = float(0.31830988618379067154);
static const float PxInvTwoPi = float(0.15915494309189533577);
static const float PxPiDivTwo = float(1.57079632679489661923);
static const float PxPiDivFour = float(0.78539816339744830962);
static const float PxSqrt2 = float(1.4142135623730951);
static const float PxInvSqrt2 = float(0.7071067811865476);

/**
\brief The return value is the greater of the two specified values.
*/
template <class T>
PX_CUDA_CALLABLE PX_FORCE_INLINE T PxMax(T a, T b)
{
	return a < b ? b : a;
}

//! overload for float to use fsel on xbox
template <>
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxMax(float a, float b)
{
	return intrinsics::selectMax(a, b);
}

/**
\brief The return value is the lesser of the two specified values.
*/
template <class T>
PX_CUDA_CALLABLE PX_FORCE_INLINE T PxMin(T a, T b)
{
	return a < b ? a : b;
}

template <>
//! overload for float to use fsel on xbox
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxMin(float a, float b)
{
	return intrinsics::selectMin(a, b);
}

/*
Many of these are just implemented as PX_CUDA_CALLABLE PX_FORCE_INLINE calls to the C lib right now,
but later we could replace some of them with some approximations or more
clever stuff.
*/

/**
\brief abs returns the absolute value of its argument.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxAbs(float a)
{
	return intrinsics::abs(a);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE bool PxEquals(float a, float b, float eps)
{
	return (PxAbs(a - b) < eps);
}

/**
\brief abs returns the absolute value of its argument.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxAbs(double a)
{
	return ::fabs(a);
}

/**
\brief abs returns the absolute value of its argument.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE int32_t PxAbs(int32_t a)
{
	return ::abs(a);
}

/**
\brief Clamps v to the range [hi,lo]
*/
template <class T>
PX_CUDA_CALLABLE PX_FORCE_INLINE T PxClamp(T v, T lo, T hi)
{
	PX_ASSERT(lo <= hi);
	return PxMin(hi, PxMax(lo, v));
}

//!	\brief Square root.
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxSqrt(float a)
{
	return intrinsics::sqrt(a);
}

//!	\brief Square root.
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxSqrt(double a)
{
	return ::sqrt(a);
}

//!	\brief reciprocal square root.
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxRecipSqrt(float a)
{
	return intrinsics::recipSqrt(a);
}

//!	\brief reciprocal square root.
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxRecipSqrt(double a)
{
	return 1 / ::sqrt(a);
}

//!	\brief square of the argument
PX_CUDA_CALLABLE PX_FORCE_INLINE PxF32 PxSqr(const PxF32 a)
{
	return a * a;
}

//! trigonometry -- all angles are in radians.

//!	\brief Sine of an angle ( <b>Unit:</b> Radians )
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxSin(float a)
{
	return intrinsics::sin(a);
}

//!	\brief Sine of an angle ( <b>Unit:</b> Radians )
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxSin(double a)
{
	return ::sin(a);
}

//!	\brief Cosine of an angle (<b>Unit:</b> Radians)
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxCos(float a)
{
	return intrinsics::cos(a);
}

//!	\brief Cosine of an angle (<b>Unit:</b> Radians)
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxCos(double a)
{
	return ::cos(a);
}

//! \brief compute sine and cosine at the same time
PX_CUDA_CALLABLE PX_FORCE_INLINE void PxSinCos(const PxF32 a, PxF32& sin, PxF32& cos)
{
#if defined(__CUDACC__) && __CUDA_ARCH__ >= 350
	__sincosf(a, &sin, &cos);
#else
	sin = PxSin(a);
	cos = PxCos(a);
#endif
}

//! \brief compute sine and cosine at the same time
PX_CUDA_CALLABLE PX_FORCE_INLINE void PxSinCos(const double a, double& sin, double& cos)
{
	sin = PxSin(a);
	cos = PxCos(a);
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxTan(float a)
{
	return ::tanf(a);
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxTan(double a)
{
	return ::tan(a);
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxAsin(float f)
{
	return ::asinf(PxClamp(f, -1.0f, 1.0f));
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxAsin(double f)
{
	return ::asin(PxClamp(f, -1.0, 1.0));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxAcos(float f)
{
	return ::acosf(PxClamp(f, -1.0f, 1.0f));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxAcos(double f)
{
	return ::acos(PxClamp(f, -1.0, 1.0));
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxAtan(float a)
{
	return ::atanf(a);
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxAtan(double a)
{
	return ::atan(a);
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE float PxAtan2(float x, float y)
{
	return ::atan2f(x, y);
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE double PxAtan2(double x, double y)
{
	return ::atan2(x, y);
}

/**
\brief Converts degrees to radians.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxF32 PxDegToRad(const PxF32 a)
{
	return 0.01745329251994329547f * a;
}

//!	\brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
PX_CUDA_CALLABLE PX_FORCE_INLINE bool PxIsFinite(float f)
{
	return intrinsics::isFinite(f);
}

//!	\brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
PX_CUDA_CALLABLE PX_FORCE_INLINE bool PxIsFinite(double f)
{
	return intrinsics::isFinite(f);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxFloor(float a)
{
	return ::floorf(a);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxExp(float a)
{
	return ::expf(a);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxCeil(float a)
{
	return ::ceilf(a);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxSign(float a)
{
	return physx::intrinsics::sign(a);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxSign2(float a, float eps = FLT_EPSILON)
{
	return (a < -eps) ? -1.0f : (a > eps) ? 1.0f : 0.0f;
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxPow(float x, float y)
{
	return ::powf(x, y);
}

PX_CUDA_CALLABLE PX_FORCE_INLINE float PxLog(float x)
{
	return ::logf(x);
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

