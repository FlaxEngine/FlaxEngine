// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PSFOUNDATION_PSMATHUTILS_H
#define PSFOUNDATION_PSMATHUTILS_H

#include "foundation/PxPreprocessor.h"
#include "foundation/PxTransform.h"
#include "foundation/PxMat33.h"
#include "NvCloth/ps/Ps.h"
#include "NvCloth/ps/PsIntrinsics.h"
#include "NvCloth/Callbacks.h"

// General guideline is: if it's an abstract math function, it belongs here.
// If it's a math function where the inputs have specific semantics (e.g.
// separateSwingTwist) it doesn't.

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
/**
\brief sign returns the sign of its argument. The sign of zero is undefined.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 sign(const physx::PxF32 a)
{
	return physx::intrinsics::sign(a);
}

/**
\brief sign returns the sign of its argument. The sign of zero is undefined.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 sign(const physx::PxF64 a)
{
	return (a >= 0.0) ? 1.0 : -1.0;
}

/**
\brief sign returns the sign of its argument. The sign of zero is undefined.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxI32 sign(const physx::PxI32 a)
{
	return (a >= 0) ? 1 : -1;
}

/**
\brief Returns true if the two numbers are within eps of each other.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE bool equals(const physx::PxF32 a, const physx::PxF32 b, const physx::PxF32 eps)
{
	return (physx::PxAbs(a - b) < eps);
}

/**
\brief Returns true if the two numbers are within eps of each other.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE bool equals(const physx::PxF64 a, const physx::PxF64 b, const physx::PxF64 eps)
{
	return (physx::PxAbs(a - b) < eps);
}

/**
\brief The floor function returns a floating-point value representing the largest integer that is less than or equal to
x.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 floor(const physx::PxF32 a)
{
	return ::floorf(a);
}

/**
\brief The floor function returns a floating-point value representing the largest integer that is less than or equal to
x.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 floor(const physx::PxF64 a)
{
	return ::floor(a);
}

/**
\brief The ceil function returns a single value representing the smallest integer that is greater than or equal to x.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 ceil(const physx::PxF32 a)
{
	return ::ceilf(a);
}

/**
\brief The ceil function returns a double value representing the smallest integer that is greater than or equal to x.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 ceil(const physx::PxF64 a)
{
	return ::ceil(a);
}

/**
\brief mod returns the floating-point remainder of x / y.

If the value of y is 0.0, mod returns a quiet NaN.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 mod(const physx::PxF32 x, const physx::PxF32 y)
{
	return physx::PxF32(::fmodf(x, y));
}

/**
\brief mod returns the floating-point remainder of x / y.

If the value of y is 0.0, mod returns a quiet NaN.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 mod(const physx::PxF64 x, const physx::PxF64 y)
{
	return ::fmod(x, y);
}

/**
\brief Square.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 sqr(const physx::PxF32 a)
{
	return a * a;
}

/**
\brief Square.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 sqr(const physx::PxF64 a)
{
	return a * a;
}

/**
\brief Calculates x raised to the power of y.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 pow(const physx::PxF32 x, const physx::PxF32 y)
{
	return ::powf(x, y);
}

/**
\brief Calculates x raised to the power of y.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 pow(const physx::PxF64 x, const physx::PxF64 y)
{
	return ::pow(x, y);
}

/**
\brief Calculates e^n
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 exp(const physx::PxF32 a)
{
	return ::expf(a);
}
/**

\brief Calculates e^n
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 exp(const physx::PxF64 a)
{
	return ::exp(a);
}

/**
\brief Calculates 2^n
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 exp2(const physx::PxF32 a)
{
	return ::expf(a * 0.693147180559945309417f);
}
/**

\brief Calculates 2^n
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 exp2(const physx::PxF64 a)
{
	return ::exp(a * 0.693147180559945309417);
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 logE(const physx::PxF32 a)
{
	return ::logf(a);
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 logE(const physx::PxF64 a)
{
	return ::log(a);
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 log2(const physx::PxF32 a)
{
	return ::logf(a) / 0.693147180559945309417f;
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 log2(const physx::PxF64 a)
{
	return ::log(a) / 0.693147180559945309417;
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 log10(const physx::PxF32 a)
{
	return ::log10f(a);
}

/**
\brief Calculates logarithms.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 log10(const physx::PxF64 a)
{
	return ::log10(a);
}

/**
\brief Converts degrees to radians.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 degToRad(const physx::PxF32 a)
{
	return 0.01745329251994329547f * a;
}

/**
\brief Converts degrees to radians.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 degToRad(const physx::PxF64 a)
{
	return 0.01745329251994329547 * a;
}

/**
\brief Converts radians to degrees.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 radToDeg(const physx::PxF32 a)
{
	return 57.29577951308232286465f * a;
}

/**
\brief Converts radians to degrees.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF64 radToDeg(const physx::PxF64 a)
{
	return 57.29577951308232286465 * a;
}

//! \brief compute sine and cosine at the same time. There is a 'fsincos' on PC that we probably want to use here
PX_CUDA_CALLABLE PX_FORCE_INLINE void sincos(const physx::PxF32 radians, physx::PxF32& sin, physx::PxF32& cos)
{
	/* something like:
	_asm fld  Local
	_asm fsincos
	_asm fstp LocalCos
	_asm fstp LocalSin
	*/
	sin = physx::PxSin(radians);
	cos = physx::PxCos(radians);
}

/**
\brief uniform random number in [a,b]
*/
PX_FORCE_INLINE physx::PxI32 rand(const physx::PxI32 a, const physx::PxI32 b)
{
	return a + physx::PxI32(::rand() % (b - a + 1));
}

/**
\brief uniform random number in [a,b]
*/
PX_FORCE_INLINE physx::PxF32 rand(const physx::PxF32 a, const physx::PxF32 b)
{
	return a + (b - a) * ::rand() / (physx::PxF32)RAND_MAX;
}

//! \brief return angle between two vectors in radians
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxF32 angle(const physx::PxVec3& v0, const physx::PxVec3& v1)
{
	const physx::PxF32 cos = v0.dot(v1);                 // |v0|*|v1|*Cos(Angle)
	const physx::PxF32 sin = (v0.cross(v1)).magnitude(); // |v0|*|v1|*Sin(Angle)
	return physx::PxAtan2(sin, cos);
}

//! If possible use instead fsel on the dot product /*fsel(d.dot(p),onething,anotherthing);*/
//! Compares orientations (more readable, user-friendly function)
PX_CUDA_CALLABLE PX_FORCE_INLINE bool sameDirection(const physx::PxVec3& d, const physx::PxVec3& p)
{
	return d.dot(p) >= 0.0f;
}

//! Checks 2 values have different signs
PX_CUDA_CALLABLE PX_FORCE_INLINE IntBool differentSign(physx::PxReal f0, physx::PxReal f1)
{
#if !PX_EMSCRIPTEN
	union
	{
		physx::PxU32 u;
		physx::PxReal f;
	} u1, u2;
	u1.f = f0;
	u2.f = f1;
	return IntBool((u1.u ^ u2.u) & PX_SIGN_BITMASK);
#else
	// javascript floats are 64-bits...
	return IntBool( (f0*f1) < 0.0f );
#endif
}

PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxMat33 star(const physx::PxVec3& v)
{
	return physx::PxMat33(physx::PxVec3(0, v.z, -v.y), physx::PxVec3(-v.z, 0, v.x), physx::PxVec3(v.y, -v.x, 0));
}

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 log(const physx::PxQuat& q)
{
	const physx::PxReal s = q.getImaginaryPart().magnitude();
	if(s < 1e-12f)
		return physx::PxVec3(0.0f);
	// force the half-angle to have magnitude <= pi/2
	physx::PxReal halfAngle = q.w < 0 ? physx::PxAtan2(-s, -q.w) : physx::PxAtan2(s, q.w);
	NV_CLOTH_ASSERT(halfAngle >= -physx::PxPi / 2 && halfAngle <= physx::PxPi / 2);

	return q.getImaginaryPart().getNormalized() * 2.f * halfAngle;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxQuat exp(const physx::PxVec3& v)
{
	const physx::PxReal m = v.magnitudeSquared();
	return m < 1e-24f ? physx::PxQuat(physx::PxIdentity) : physx::PxQuat(physx::PxSqrt(m), v * physx::PxRecipSqrt(m));
}

// quat to rotate v0 t0 v1
PX_CUDA_CALLABLE PX_INLINE physx::PxQuat rotationArc(const physx::PxVec3& v0, const physx::PxVec3& v1)
{
	const physx::PxVec3 cross = v0.cross(v1);
	const physx::PxReal d = v0.dot(v1);
	if(d <= -0.99999f)
		return (physx::PxAbs(v0.x) < 0.1f ? physx::PxQuat(0.0f, v0.z, -v0.y, 0.0f) : physx::PxQuat(v0.y, -v0.x, 0.0, 0.0)).getNormalized();

	const physx::PxReal s = physx::PxSqrt((1 + d) * 2), r = 1 / s;

	return physx::PxQuat(cross.x * r, cross.y * r, cross.z * r, s * 0.5f).getNormalized();
}

/**
\brief returns largest axis
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxU32 largestAxis(const physx::PxVec3& v)
{
	physx::PxU32 m = physx::PxU32(v.y > v.x ? 1 : 0);
	return v.z > v[m] ? 2 : m;
}

/**
\brief returns indices for the largest axis and 2 other axii
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxU32 largestAxis(const physx::PxVec3& v, physx::PxU32& other1, physx::PxU32& other2)
{
	if(v.x >= physx::PxMax(v.y, v.z))
	{
		other1 = 1;
		other2 = 2;
		return 0;
	}
	else if(v.y >= v.z)
	{
		other1 = 0;
		other2 = 2;
		return 1;
	}
	else
	{
		other1 = 0;
		other2 = 1;
		return 2;
	}
}

/**
\brief returns axis with smallest absolute value
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxU32 closestAxis(const physx::PxVec3& v)
{
	physx::PxU32 m = physx::PxU32(physx::PxAbs(v.y) > physx::PxAbs(v.x) ? 1 : 0);
	return physx::PxAbs(v.z) > physx::PxAbs(v[m]) ? 2 : m;
}

PX_CUDA_CALLABLE PX_INLINE physx::PxU32 closestAxis(const physx::PxVec3& v, physx::PxU32& j, physx::PxU32& k)
{
	// find largest 2D plane projection
	const physx::PxF32 absPx = physx::PxAbs(v.x);
	const physx::PxF32 absNy = physx::PxAbs(v.y);
	const physx::PxF32 absNz = physx::PxAbs(v.z);

	physx::PxU32 m = 0; // x biggest axis
	j = 1;
	k = 2;
	if(absNy > absPx && absNy > absNz)
	{
		// y biggest
		j = 2;
		k = 0;
		m = 1;
	}
	else if(absNz > absPx)
	{
		// z biggest
		j = 0;
		k = 1;
		m = 2;
	}
	return m;
}

/*!
Extend an edge along its length by a factor
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE void makeFatEdge(physx::PxVec3& p0, physx::PxVec3& p1, physx::PxReal fatCoeff)
{
	physx::PxVec3 delta = p1 - p0;

	const physx::PxReal m = delta.magnitude();
	if(m > 0.0f)
	{
		delta *= fatCoeff / m;
		p0 -= delta;
		p1 += delta;
	}
}

//! Compute point as combination of barycentric coordinates
PX_CUDA_CALLABLE PX_FORCE_INLINE physx::PxVec3
computeBarycentricPoint(const physx::PxVec3& p0, const physx::PxVec3& p1, const physx::PxVec3& p2, physx::PxReal u, physx::PxReal v)
{
	// This seems to confuse the compiler...
	// return (1.0f - u - v)*p0 + u*p1 + v*p2;
	const physx::PxF32 w = 1.0f - u - v;
	return physx::PxVec3(w * p0.x + u * p1.x + v * p2.x, w * p0.y + u * p1.y + v * p2.y, w * p0.z + u * p1.z + v * p2.z);
}

// generates a pair of quaternions (swing, twist) such that in = swing * twist, with
// swing.x = 0
// twist.y = twist.z = 0, and twist is a unit quat
PX_FORCE_INLINE void separateSwingTwist(const physx::PxQuat& q, physx::PxQuat& swing, physx::PxQuat& twist)
{
	twist = q.x != 0.0f ? physx::PxQuat(q.x, 0, 0, q.w).getNormalized() : physx::PxQuat(physx::PxIdentity);
	swing = q * twist.getConjugate();
}

// generate two tangent vectors to a given normal
PX_FORCE_INLINE void normalToTangents(const physx::PxVec3& normal, physx::PxVec3& tangent0, physx::PxVec3& tangent1)
{
	tangent0 = physx::PxAbs(normal.x) < 0.70710678f ? physx::PxVec3(0, -normal.z, normal.y) : physx::PxVec3(-normal.y, normal.x, 0);
	tangent0.normalize();
	tangent1 = normal.cross(tangent0);
}

/**
\brief computes a oriented bounding box around the scaled basis.
\param basis Input = skewed basis, Output = (normalized) orthogonal basis.
\return Bounding box extent.
*/
NV_CLOTH_IMPORT physx::PxVec3 optimizeBoundingBox(physx::PxMat33& basis);

NV_CLOTH_IMPORT physx::PxQuat slerp(const physx::PxReal t, const physx::PxQuat& left, const physx::PxQuat& right);

PX_CUDA_CALLABLE PX_INLINE physx::PxVec3 ellipseClamp(const physx::PxVec3& point, const physx::PxVec3& radii)
{
	// This function need to be implemented in the header file because
	// it is included in a spu shader program.

	// finds the closest point on the ellipse to a given point

	// (p.y, p.z) is the input point
	// (e.y, e.z) are the radii of the ellipse

	// lagrange multiplier method with Newton/Halley hybrid root-finder.
	// see http://www.geometrictools.com/Documentation/DistancePointToEllipse2.pdf
	// for proof of Newton step robustness and initial estimate.
	// Halley converges much faster but sometimes overshoots - when that happens we take
	// a newton step instead

	// converges in 1-2 iterations where D&C works well, and it's good with 4 iterations
	// with any ellipse that isn't completely crazy

	const physx::PxU32 MAX_ITERATIONS = 20;
	const physx::PxReal convergenceThreshold = 1e-4f;

	// iteration requires first quadrant but we recover generality later

	physx::PxVec3 q(0, physx::PxAbs(point.y), physx::PxAbs(point.z));
	const physx::PxReal tinyEps = 1e-6f; // very close to minor axis is numerically problematic but trivial
	if(radii.y >= radii.z)
	{
		if(q.z < tinyEps)
			return physx::PxVec3(0, point.y > 0 ? radii.y : -radii.y, 0);
	}
	else
	{
		if(q.y < tinyEps)
			return physx::PxVec3(0, 0, point.z > 0 ? radii.z : -radii.z);
	}

	physx::PxVec3 denom, e2 = radii.multiply(radii), eq = radii.multiply(q);

	// we can use any initial guess which is > maximum(-e.y^2,-e.z^2) and for which f(t) is > 0.
	// this guess works well near the axes, but is weak along the diagonals.

	physx::PxReal t = physx::PxMax(eq.y - e2.y, eq.z - e2.z);

	for(physx::PxU32 i = 0; i < MAX_ITERATIONS; i++)
	{
		denom = physx::PxVec3(0, 1 / (t + e2.y), 1 / (t + e2.z));
		physx::PxVec3 denom2 = eq.multiply(denom);

		physx::PxVec3 fv = denom2.multiply(denom2);
		physx::PxReal f = fv.y + fv.z - 1;

		// although in exact arithmetic we are guaranteed f>0, we can get here
		// on the first iteration via catastrophic cancellation if the point is
		// very close to the origin. In that case we just behave as if f=0

		if(f < convergenceThreshold)
			return e2.multiply(point).multiply(denom);

		physx::PxReal df = fv.dot(denom) * -2.0f;
		t = t - f / df;
	}

	// we didn't converge, so clamp what we have
	physx::PxVec3 r = e2.multiply(point).multiply(denom);
	return r * physx::PxRecipSqrt(sqr(r.y / radii.y) + sqr(r.z / radii.z));
}

PX_CUDA_CALLABLE PX_INLINE physx::PxReal tanHalf(physx::PxReal sin, physx::PxReal cos)
{
	return sin / (1 + cos);
}

PX_INLINE physx::PxQuat quatFromTanQVector(const physx::PxVec3& v)
{
	physx::PxReal v2 = v.dot(v);
	if(v2 < 1e-12f)
		return physx::PxQuat(physx::PxIdentity);
	physx::PxReal d = 1 / (1 + v2);
	return physx::PxQuat(v.x * 2, v.y * 2, v.z * 2, 1 - v2) * d;
}

PX_FORCE_INLINE physx::PxVec3 cross100(const physx::PxVec3& b)
{
	return physx::PxVec3(0.0f, -b.z, b.y);
}
PX_FORCE_INLINE physx::PxVec3 cross010(const physx::PxVec3& b)
{
	return physx::PxVec3(b.z, 0.0f, -b.x);
}
PX_FORCE_INLINE physx::PxVec3 cross001(const physx::PxVec3& b)
{
	return physx::PxVec3(-b.y, b.x, 0.0f);
}

PX_INLINE void decomposeVector(physx::PxVec3& normalCompo, physx::PxVec3& tangentCompo, const physx::PxVec3& outwardDir,
                               const physx::PxVec3& outwardNormal)
{
	normalCompo = outwardNormal * (outwardDir.dot(outwardNormal));
	tangentCompo = outwardDir - normalCompo;
}

//! \brief Return (i+1)%3
// Avoid variable shift for XBox:
// PX_INLINE physx::PxU32 Ps::getNextIndex3(physx::PxU32 i)			{	return (1<<i) & 3;			}
PX_INLINE physx::PxU32 getNextIndex3(physx::PxU32 i)
{
	return (i + 1 + (i >> 1)) & 3;
}

PX_INLINE physx::PxMat33 rotFrom2Vectors(const physx::PxVec3& from, const physx::PxVec3& to)
{
	// See bottom of http://www.euclideanspace.com/maths/algebra/matrix/orthogonal/rotation/index.htm

	// Early exit if to = from
	if((from - to).magnitudeSquared() < 1e-4f)
		return physx::PxMat33(physx::PxIdentity);

	// Early exit if to = -from
	if((from + to).magnitudeSquared() < 1e-4f)
		return physx::PxMat33::createDiagonal(physx::PxVec3(1.0f, -1.0f, -1.0f));

	physx::PxVec3 n = from.cross(to);

	physx::PxReal C = from.dot(to), S = physx::PxSqrt(1 - C * C), CC = 1 - C;

	physx::PxReal xx = n.x * n.x, yy = n.y * n.y, zz = n.z * n.z, xy = n.x * n.y, yz = n.y * n.z, xz = n.x * n.z;

	physx::PxMat33 R;

	R(0, 0) = 1 + CC * (xx - 1);
	R(0, 1) = -n.z * S + CC * xy;
	R(0, 2) = n.y * S + CC * xz;

	R(1, 0) = n.z * S + CC * xy;
	R(1, 1) = 1 + CC * (yy - 1);
	R(1, 2) = -n.x * S + CC * yz;

	R(2, 0) = -n.y * S + CC * xz;
	R(2, 1) = n.x * S + CC * yz;
	R(2, 2) = 1 + CC * (zz - 1);

	return R;
}

NV_CLOTH_IMPORT void integrateTransform(const physx::PxTransform& curTrans, const physx::PxVec3& linvel, const physx::PxVec3& angvel,
                                          physx::PxReal timeStep, physx::PxTransform& result);

PX_INLINE void computeBasis(const physx::PxVec3& dir, physx::PxVec3& right, physx::PxVec3& up)
{
	// Derive two remaining vectors
	if(physx::PxAbs(dir.y) <= 0.9999f)
	{
		right = physx::PxVec3(dir.z, 0.0f, -dir.x);
		right.normalize();

		// PT: normalize not needed for 'up' because dir & right are unit vectors,
		// and by construction the angle between them is 90 degrees (i.e. sin(angle)=1)
		up = physx::PxVec3(dir.y * right.z, dir.z * right.x - dir.x * right.z, -dir.y * right.x);
	}
	else
	{
		right = physx::PxVec3(1.0f, 0.0f, 0.0f);

		up = physx::PxVec3(0.0f, dir.z, -dir.y);
		up.normalize();
	}
}

PX_INLINE void computeBasis(const physx::PxVec3& p0, const physx::PxVec3& p1, physx::PxVec3& dir, physx::PxVec3& right, physx::PxVec3& up)
{
	// Compute the new direction vector
	dir = p1 - p0;
	dir.normalize();

	// Derive two remaining vectors
	computeBasis(dir, right, up);
}

PX_FORCE_INLINE bool isAlmostZero(const physx::PxVec3& v)
{
	if(physx::PxAbs(v.x) > 1e-6f || physx::PxAbs(v.y) > 1e-6f || physx::PxAbs(v.z) > 1e-6f)
		return false;
	return true;
}
} // namespace ps
} // namespace cloth
} // namespace nv

#endif
