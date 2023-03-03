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

#ifndef PX_MATH_UTILS_H
#define PX_MATH_UTILS_H

/** \addtogroup common
  @{
*/

#include "foundation/PxFoundationConfig.h"
#include "foundation/Px.h"
#include "foundation/PxVec4.h"
#include "foundation/PxAssert.h"
#include "foundation/PxPlane.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief finds the shortest rotation between two vectors.

\param[in] from the vector to start from
\param[in] target the vector to rotate to
\return a rotation about an axis normal to the two vectors which takes one to the other via the shortest path
*/
PX_FOUNDATION_API PxQuat PxShortestRotation(const PxVec3& from, const PxVec3& target);

/* \brief diagonalizes a 3x3 symmetric matrix y

The returned matrix satisfies M = R * D * R', where R is the rotation matrix for the output quaternion, R' its
transpose, and D the diagonal matrix

If the matrix is not symmetric, the result is undefined.

\param[in] m the matrix to diagonalize
\param[out] axes a quaternion rotation which diagonalizes the matrix
\return the vector diagonal of the diagonalized matrix.
*/
PX_FOUNDATION_API PxVec3 PxDiagonalize(const PxMat33& m, PxQuat& axes);

/** \brief creates a transform from the endpoints of a segment, suitable for an actor transform for a PxCapsuleGeometry

\param[in] p0 one end of major axis of the capsule
\param[in] p1 the other end of the axis of the capsule
\param[out] halfHeight the halfHeight of the capsule. This parameter is optional.
\return A PxTransform which will transform the vector (1,0,0) to the capsule axis shrunk by the halfHeight
*/
PX_FOUNDATION_API PxTransform PxTransformFromSegment(const PxVec3& p0, const PxVec3& p1, PxReal* halfHeight = NULL);

/** \brief creates a transform from a plane equation, suitable for an actor transform for a PxPlaneGeometry

\param[in] plane the desired plane equation
\return a PxTransform which will transform the plane PxPlane(1,0,0,0) to the specified plane
*/
PX_FOUNDATION_API PxTransform PxTransformFromPlaneEquation(const PxPlane& plane);

/** \brief creates a plane equation from a transform, such as the actor transform for a PxPlaneGeometry

\param[in] pose		the transform
\return the plane
*/
PX_INLINE PxPlane PxPlaneEquationFromTransform(const PxTransform& pose)
{
	return PxPlane(1.0f, 0.0f, 0.0f, 0.0f).transform(pose);
}

/**
\brief Spherical linear interpolation of two quaternions. 
\param[in] t is the interpolation parameter in range (0, 1)
\param[in] left is the start of the interpolation
\param[in] right is the end of the interpolation
\return Returns left when t=0, right when t=1 and a linear interpolation of left and right when 0 < t < 1.
Returns angle between -PI and PI in radians
*/
PX_FOUNDATION_API PxQuat PxSlerp(const PxReal t, const PxQuat& left, const PxQuat& right);

/**
\brief integrate transform.
\param[in] curTrans The current transform
\param[in] linvel Linear velocity
\param[in] angvel Angular velocity
\param[in] timeStep The time-step for integration
\param[out] result The integrated transform
*/
PX_FOUNDATION_API void PxIntegrateTransform(const PxTransform& curTrans, const PxVec3& linvel, const PxVec3& angvel,
	PxReal timeStep, PxTransform& result);

//!	\brief Compute the exponent of a PxVec3
PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuat PxExp(const PxVec3& v)
{
	const PxReal m = v.magnitudeSquared();
	return m < 1e-24f ? PxQuat(PxIdentity) : PxQuat(PxSqrt(m), v * PxRecipSqrt(m));
}

/**
\brief computes a oriented bounding box around the scaled basis.
\param basis Input = skewed basis, Output = (normalized) orthogonal basis.
\return Bounding box extent.
*/
PX_FOUNDATION_API PxVec3 PxOptimizeBoundingBox(PxMat33& basis);

/**
\brief return Returns the log of a PxQuat
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3 PxLog(const PxQuat& q)
{
	const PxReal s = q.getImaginaryPart().magnitude();
	if (s < 1e-12f)
		return PxVec3(0.0f);
	// force the half-angle to have magnitude <= pi/2
	PxReal halfAngle = q.w < 0 ? PxAtan2(-s, -q.w) : PxAtan2(s, q.w);
	PX_ASSERT(halfAngle >= -PxPi / 2 && halfAngle <= PxPi / 2);

	return q.getImaginaryPart().getNormalized() * 2.f * halfAngle;
}

/**
\brief return Returns 0 if v.x is largest element of v, 1 if v.y is largest element, 2 if v.z is largest element.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxU32 PxLargestAxis(const PxVec3& v)
{
	PxU32 m = PxU32(v.y > v.x ? 1 : 0);
	return v.z > v[m] ? 2 : m;
}

/**
\brief Compute tan(theta/2) given sin(theta) and cos(theta) as inputs.
\param[in] sin has value sin(theta)
\param[in] cos has value cos(theta)
\return Returns tan(theta/2)
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxReal PxTanHalf(PxReal sin, PxReal cos)
{
	// PT: avoids divide by zero for singularity. We return sqrt(FLT_MAX) instead of FLT_MAX
	// to make sure the calling code doesn't generate INF values when manipulating the returned value
	// (some joints multiply it by 4, etc).
	if (cos == -1.0f)
		return sin < 0.0f ? -sqrtf(FLT_MAX) : sqrtf(FLT_MAX);

	// PT: half-angle formula: tan(a/2) = sin(a)/(1+cos(a))
	return sin / (1.0f + cos);
}

/**
\brief Compute the closest point on an 2d ellipse to a given 2d point.
\param[in] point is a 2d point in the y-z plane represented by (point.y, point.z) 
\param[in] radii are the radii of the ellipse (radii.y and radii.z) in the y-z plane.
\return Returns the 2d position on the surface of the ellipse that is closest to point.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3 PxEllipseClamp(const PxVec3& point, const PxVec3& radii)
{
	// lagrange multiplier method with Newton/Halley hybrid root-finder.
	// see http://www.geometrictools.com/Documentation/DistancePointToEllipse2.pdf
	// for proof of Newton step robustness and initial estimate.
	// Halley converges much faster but sometimes overshoots - when that happens we take
	// a newton step instead

	// converges in 1-2 iterations where D&C works well, and it's good with 4 iterations
	// with any ellipse that isn't completely crazy

	const PxU32 MAX_ITERATIONS = 20;
	const PxReal convergenceThreshold = 1e-4f;

	// iteration requires first quadrant but we recover generality later

	PxVec3 q(0, PxAbs(point.y), PxAbs(point.z));
	const PxReal tinyEps = 1e-6f; // very close to minor axis is numerically problematic but trivial
	if (radii.y >= radii.z)
	{
		if (q.z < tinyEps)
			return PxVec3(0, point.y > 0 ? radii.y : -radii.y, 0);
	}
	else
	{
		if (q.y < tinyEps)
			return PxVec3(0, 0, point.z > 0 ? radii.z : -radii.z);
	}

	PxVec3 denom, e2 = radii.multiply(radii), eq = radii.multiply(q);

	// we can use any initial guess which is > maximum(-e.y^2,-e.z^2) and for which f(t) is > 0.
	// this guess works well near the axes, but is weak along the diagonals.

	PxReal t = PxMax(eq.y - e2.y, eq.z - e2.z);

	for (PxU32 i = 0; i < MAX_ITERATIONS; i++)
	{
		denom = PxVec3(0, 1 / (t + e2.y), 1 / (t + e2.z));
		PxVec3 denom2 = eq.multiply(denom);

		PxVec3 fv = denom2.multiply(denom2);
		PxReal f = fv.y + fv.z - 1;

		// although in exact arithmetic we are guaranteed f>0, we can get here
		// on the first iteration via catastrophic cancellation if the point is
		// very close to the origin. In that case we just behave as if f=0

		if (f < convergenceThreshold)
			return e2.multiply(point).multiply(denom);

		PxReal df = fv.dot(denom) * -2.0f;
		t = t - f / df;
	}

	// we didn't converge, so clamp what we have
	PxVec3 r = e2.multiply(point).multiply(denom);
	return r * PxRecipSqrt(PxSqr(r.y / radii.y) + PxSqr(r.z / radii.z));
}

/**
\brief Compute from an input quaternion q a pair of quaternions (swing, twist) such that 
q = swing * twist
with the caveats that swing.x = twist.y = twist.z = 0.
\param[in] q is the quaternion to be decomposed into swing and twist quaternions.
\param[out] swing is the swing component of the quaternion decomposition.
\param[out] twist is the twist component of the quaternion decomposition.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE void PxSeparateSwingTwist(const PxQuat& q, PxQuat& swing, PxQuat& twist)
{
	twist = q.x != 0.0f ? PxQuat(q.x, 0, 0, q.w).getNormalized() : PxQuat(PxIdentity);
	swing = q * twist.getConjugate();
}

/**
\brief Compute the angle between two non-unit vectors
\param[in] v0 is one of the non-unit vectors
\param[in] v1 is the other of the two non-unit vectors
\return Returns the angle (in radians) between the two vector v0 and v1.
*/
PX_CUDA_CALLABLE PX_FORCE_INLINE PxF32 PxComputeAngle(const PxVec3& v0, const PxVec3& v1)
{
	const PxF32 cos = v0.dot(v1);                 // |v0|*|v1|*Cos(Angle)
	const PxF32 sin = (v0.cross(v1)).magnitude(); // |v0|*|v1|*Sin(Angle)
	return PxAtan2(sin, cos);
}

/**
\brief Compute two normalized vectors (right and up) that are perpendicular to an input normalized vector (dir).
\param[in] dir is a normalized vector that is used to compute the perpendicular vectors.
\param[out] right is the first of the two vectors perpendicular to dir
\param[out] up is the second of the two vectors perpendicular to dir
*/
PX_INLINE void PxComputeBasisVectors(const PxVec3& dir, PxVec3& right, PxVec3& up)
{
	// Derive two remaining vectors
	if (PxAbs(dir.y) <= 0.9999f)
	{
		right = PxVec3(dir.z, 0.0f, -dir.x);
		right.normalize();

		// PT: normalize not needed for 'up' because dir & right are unit vectors,
		// and by construction the angle between them is 90 degrees (i.e. sin(angle)=1)
		up = PxVec3(dir.y * right.z, dir.z * right.x - dir.x * right.z, -dir.y * right.x);
	}
	else
	{
		right = PxVec3(1.0f, 0.0f, 0.0f);

		up = PxVec3(0.0f, dir.z, -dir.y);
		up.normalize();
	}
}

/**
\brief Compute three normalized vectors (dir, right and up) that are parallel to (dir) and perpendicular to (right, up) the 
normalized direction vector (p1 - p0)/||p1 - p0||.
\param[in] p0 is used to compute the normalized vector dir = (p1 - p0)/||p1 - p0||.
\param[in] p1 is used to compute the normalized vector dir = (p1 - p0)/||p1 - p0||.
\param[out] dir is the normalized vector (p1 - p0)/||p1 - p0||.
\param[out] right is the first of the two normalized vectors perpendicular to dir
\param[out] up is the second of the two normalized vectors perpendicular to dir
*/
PX_INLINE void PxComputeBasisVectors(const PxVec3& p0, const PxVec3& p1, PxVec3& dir, PxVec3& right, PxVec3& up)
{
	// Compute the new direction vector
	dir = p1 - p0;
	dir.normalize();

	// Derive two remaining vectors
	PxComputeBasisVectors(dir, right, up);
}


/**
\brief Compute (i+1)%3
*/
PX_INLINE PxU32 PxGetNextIndex3(PxU32 i)
{
	return (i + 1 + (i >> 1)) & 3;
}

PX_INLINE PX_CUDA_CALLABLE void computeBarycentric(const PxVec3& a, const PxVec3& b, const PxVec3& c, const PxVec3& d, const PxVec3& p, PxVec4& bary)
{
	const PxVec3 ba = b - a;
	const PxVec3 ca = c - a;
	const PxVec3 da = d - a;
	const PxVec3 pa = p - a;

	const PxReal detBcd = ba.dot(ca.cross(da));
	const PxReal detPcd = pa.dot(ca.cross(da));

	bary.y = detPcd / detBcd;

	const PxReal detBpd = ba.dot(pa.cross(da));
	bary.z = detBpd / detBcd;

	const PxReal detBcp = ba.dot(ca.cross(pa));

	bary.w = detBcp / detBcd;
	bary.x = 1 - bary.y - bary.z - bary.w;
}

PX_INLINE PX_CUDA_CALLABLE void computeBarycentric(const PxVec3& a, const PxVec3& b, const PxVec3& c, const PxVec3& p, PxVec4& bary)
{
	const PxVec3 v0 = b - a;
	const PxVec3 v1 = c - a;
	const PxVec3 v2 = p - a;

	const float d00 = v0.dot(v0);
	const float d01 = v0.dot(v1);
	const float d11 = v1.dot(v1);
	const float d20 = v2.dot(v0);
	const float d21 = v2.dot(v1);

	const float denom = d00 * d11 - d01 * d01;
	const float v = (d11 * d20 - d01 * d21) / denom;
	const float w = (d00 * d21 - d01 * d20) / denom;
	const float u = 1.f - v - w;

	bary.x = u; bary.y = v; bary.z = w;
	bary.w = 0.f;
}


// lerp
struct Interpolation
{
	PX_INLINE PX_CUDA_CALLABLE static float PxLerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	PX_INLINE PX_CUDA_CALLABLE static PxReal PxBiLerp(
		const PxReal f00,
		const PxReal f10,
		const PxReal f01,
		const PxReal f11,
		const PxReal tx, const PxReal ty)
	{
		return PxLerp(
			PxLerp(f00, f10, tx),
			PxLerp(f01, f11, tx),
			ty);
	}

	PX_INLINE PX_CUDA_CALLABLE static PxReal PxTriLerp(
		const PxReal f000,
		const PxReal f100,
		const PxReal f010,
		const PxReal f110,
		const PxReal f001,
		const PxReal f101,
		const PxReal f011,
		const PxReal f111,
		const PxReal tx,
		const PxReal ty,
		const PxReal tz)
	{
		return PxLerp(
			PxBiLerp(f000, f100, f010, f110, tx, ty),
			PxBiLerp(f001, f101, f011, f111, tx, ty),
			tz);
	}

	PX_INLINE PX_CUDA_CALLABLE static PxU32 PxSDFIdx(PxU32 i, PxU32 j, PxU32 k, PxU32 nbX, PxU32 nbY)
	{
		return i + j * nbX + k * nbX*nbY;
	}

	PX_INLINE PX_CUDA_CALLABLE static PxReal PxSDFSampleImpl(const PxReal* PX_RESTRICT sdf, const PxVec3& localPos, const PxVec3& sdfBoxLower,
		const PxVec3& sdfBoxHigher, const PxReal sdfDx, const PxReal invSdfDx, const PxU32 dimX, const PxU32 dimY, const PxU32 dimZ, PxReal tolerance)
	{
		PxVec3 clampedGridPt = localPos.maximum(sdfBoxLower).minimum(sdfBoxHigher);

		const PxVec3 diff = (localPos - clampedGridPt);

		if (diff.magnitudeSquared() > tolerance*tolerance)
			return PX_MAX_F32;

		PxVec3 f = (clampedGridPt - sdfBoxLower) * invSdfDx;

		PxU32 i = PxU32(f.x);
		PxU32 j = PxU32(f.y);
		PxU32 k = PxU32(f.z);

		f -= PxVec3(PxReal(i), PxReal(j), PxReal(k));

		if (i >= (dimX - 1))
		{
			i = dimX - 2;
			clampedGridPt.x -= f.x * sdfDx;
			f.x = 1.f;
		}
		if (j >= (dimY - 1))
		{
			j = dimY - 2;
			clampedGridPt.y -= f.y * sdfDx;
			f.y = 1.f;
		}
		if (k >= (dimZ - 1))
		{
			k = dimZ - 2;
			clampedGridPt.z -= f.z * sdfDx;
			f.z = 1.f;
		}

		const PxReal s000 = sdf[Interpolation::PxSDFIdx(i, j, k, dimX, dimY)];
		const PxReal s100 = sdf[Interpolation::PxSDFIdx(i + 1, j, k, dimX, dimY)];
		const PxReal s010 = sdf[Interpolation::PxSDFIdx(i, j + 1, k, dimX, dimY)];
		const PxReal s110 = sdf[Interpolation::PxSDFIdx(i + 1, j + 1, k, dimX, dimY)];
		const PxReal s001 = sdf[Interpolation::PxSDFIdx(i, j, k + 1, dimX, dimY)];
		const PxReal s101 = sdf[Interpolation::PxSDFIdx(i + 1, j, k + 1, dimX, dimY)];
		const PxReal s011 = sdf[Interpolation::PxSDFIdx(i, j + 1, k + 1, dimX, dimY)];
		const PxReal s111 = sdf[Interpolation::PxSDFIdx(i + 1, j + 1, k + 1, dimX, dimY)];

		PxReal dist = PxTriLerp(
			s000,
			s100,
			s010,
			s110,
			s001,
			s101,
			s011,
			s111,
			f.x, f.y, f.z);

		dist += diff.magnitude();

		return dist;
	}

};

PX_INLINE PX_CUDA_CALLABLE PxReal PxSdfSample(const PxReal* PX_RESTRICT sdf, const PxVec3& localPos, const PxVec3& sdfBoxLower,
	const PxVec3& sdfBoxHigher, const PxReal sdfDx, const PxReal invSdfDx, const PxU32 dimX, const PxU32 dimY, const PxU32 dimZ, PxVec3& gradient, PxReal tolerance = PX_MAX_F32)
{

	PxReal dist = Interpolation::PxSDFSampleImpl(sdf, localPos, sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance);

	if (dist < tolerance)
	{
		
		PxVec3 grad;
		grad.x = Interpolation::PxSDFSampleImpl(sdf, localPos + PxVec3(sdfDx, 0.f, 0.f), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance) -
			Interpolation::PxSDFSampleImpl(sdf, localPos - PxVec3(sdfDx, 0.f, 0.f), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance);
		grad.y = Interpolation::PxSDFSampleImpl(sdf, localPos + PxVec3(0.f, sdfDx, 0.f), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance) -
			Interpolation::PxSDFSampleImpl(sdf, localPos - PxVec3(0.f, sdfDx, 0.f), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance);
		grad.z = Interpolation::PxSDFSampleImpl(sdf, localPos + PxVec3(0.f, 0.f, sdfDx), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance) -
			Interpolation::PxSDFSampleImpl(sdf, localPos - PxVec3(0.f, 0.f, sdfDx), sdfBoxLower, sdfBoxHigher, sdfDx, invSdfDx, dimX, dimY, dimZ, tolerance);

		gradient = grad;
		
	}

	return dist;
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
