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

#ifndef PX_QUAT_H
#define PX_QUAT_H

/** \addtogroup foundation
@{
*/

#include "foundation/PxVec3.h"
#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief This is a quaternion class. For more information on quaternion mathematics
consult a mathematics source on complex numbers.
*/

template<class Type>
class PxQuatT
{
  public:

	/**
	\brief Default constructor, does not do any initialization.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT()
	{
	}

	//! identity constructor
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT(PxIDENTITY) : x(Type(0.0)), y(Type(0.0)), z(Type(0.0)), w(Type(1.0))
	{
	}

	/**
	\brief Constructor from a scalar: sets the real part w to the scalar value, and the imaginary parts (x,y,z) to zero
	*/
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT(Type r) : x(Type(0.0)), y(Type(0.0)), z(Type(0.0)), w(r)
	{
	}

	/**
	\brief Constructor. Take note of the order of the elements!
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT(Type nx, Type ny, Type nz, Type nw) : x(nx), y(ny), z(nz), w(nw)
	{
	}

	/**
	\brief Creates from angle-axis representation.

	Axis must be normalized!

	Angle is in radians!

	<b>Unit:</b> Radians
	*/
	PX_CUDA_CALLABLE PX_INLINE PxQuatT(Type angleRadians, const PxVec3T<Type>& unitAxis)
	{
		PX_ASSERT(PxAbs(Type(1.0) - unitAxis.magnitude()) < Type(1e-3));
		const Type a = angleRadians * Type(0.5);

		Type s;
		PxSinCos(a, s, w);
		x = unitAxis.x * s;
		y = unitAxis.y * s;
		z = unitAxis.z * s;
	}

	/**
	\brief Copy ctor.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT(const PxQuatT& v) : x(v.x), y(v.y), z(v.z), w(v.w)
	{
	}

	/**
	\brief Creates from orientation matrix.

	\param[in] m Rotation matrix to extract quaternion from.
	*/
	PX_CUDA_CALLABLE PX_INLINE explicit PxQuatT(const PxMat33T<Type>& m); /* defined in PxMat33.h */

	/**
	\brief returns true if quat is identity
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isIdentity() const
	{
		return x==Type(0.0) && y==Type(0.0) && z==Type(0.0) && w==Type(1.0);
	}

	/**
	\brief returns true if all elements are finite (not NAN or INF, etc.)
	*/
	PX_CUDA_CALLABLE bool isFinite() const
	{
		return PxIsFinite(x) && PxIsFinite(y) && PxIsFinite(z) && PxIsFinite(w);
	}

	/**
	\brief returns true if finite and magnitude is close to unit
	*/
	PX_CUDA_CALLABLE bool isUnit() const
	{
		const Type unitTolerance = Type(1e-3);
		return isFinite() && PxAbs(magnitude() - Type(1.0)) < unitTolerance;
	}

	/**
	\brief returns true if finite and magnitude is reasonably close to unit to allow for some accumulation of error vs
	isValid
	*/
	PX_CUDA_CALLABLE bool isSane() const
	{
		const Type unitTolerance = Type(1e-2);
		return isFinite() && PxAbs(magnitude() - Type(1.0)) < unitTolerance;
	}

	/**
	\brief returns true if the two quaternions are exactly equal
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator==(const PxQuatT& q) const
	{
		return x == q.x && y == q.y && z == q.z && w == q.w;
	}

	/**
	\brief converts this quaternion to angle-axis representation
	*/
	PX_CUDA_CALLABLE PX_INLINE void toRadiansAndUnitAxis(Type& angle, PxVec3T<Type>& axis) const
	{
		const Type quatEpsilon = Type(1.0e-8);
		const Type s2 = x * x + y * y + z * z;
		if(s2 < quatEpsilon * quatEpsilon) // can't extract a sensible axis
		{
			angle = Type(0.0);
			axis = PxVec3T<Type>(Type(1.0), Type(0.0), Type(0.0));
		}
		else
		{
			const Type s = PxRecipSqrt(s2);
			axis = PxVec3T<Type>(x, y, z) * s;
			angle = PxAbs(w) < quatEpsilon ? Type(PxPi) : PxAtan2(s2 * s, w) * Type(2.0);
		}
	}

	/**
	\brief Gets the angle between this quat and the identity quaternion.

	<b>Unit:</b> Radians
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type getAngle() const
	{
		return PxAcos(w) * Type(2.0);
	}

	/**
	\brief Gets the angle between this quat and the argument

	<b>Unit:</b> Radians
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type getAngle(const PxQuatT& q) const
	{
		return PxAcos(dot(q)) * Type(2.0);
	}

	/**
	\brief This is the squared 4D vector length, should be 1 for unit quaternions.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type magnitudeSquared() const
	{
		return x * x + y * y + z * z + w * w;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type dot(const PxQuatT& v) const
	{
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT getNormalized() const
	{
		const Type s = Type(1.0) / magnitude();
		return PxQuatT(x * s, y * s, z * s, w * s);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE Type magnitude() const
	{
		return PxSqrt(magnitudeSquared());
	}

	// modifiers:
	/**
	\brief maps to the closest unit quaternion.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type normalize() // convert this PxQuatT to a unit quaternion
	{
		const Type mag = magnitude();
		if(mag != Type(0.0))
		{
			const Type imag = Type(1.0) / mag;

			x *= imag;
			y *= imag;
			z *= imag;
			w *= imag;
		}
		return mag;
	}

	/*
	\brief returns the conjugate.

	\note for unit quaternions, this is the inverse.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT getConjugate() const
	{
		return PxQuatT(-x, -y, -z, w);
	}

	/*
	\brief returns imaginary part.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> getImaginaryPart() const
	{
		return PxVec3T<Type>(x, y, z);
	}

	/** brief computes rotation of x-axis */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> getBasisVector0() const
	{
		const Type x2 = x * Type(2.0);
		const Type w2 = w * Type(2.0);
		return PxVec3T<Type>((w * w2) - Type(1.0) + x * x2, (z * w2) + y * x2, (-y * w2) + z * x2);
	}

	/** brief computes rotation of y-axis */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> getBasisVector1() const
	{
		const Type y2 = y * Type(2.0);
		const Type w2 = w * Type(2.0);
		return PxVec3T<Type>((-z * w2) + x * y2, (w * w2) - Type(1.0) + y * y2, (x * w2) + z * y2);
	}

	/** brief computes rotation of z-axis */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> getBasisVector2() const
	{
		const Type z2 = z * Type(2.0);
		const Type w2 = w * Type(2.0);
		return PxVec3T<Type>((y * w2) + x * z2, (-x * w2) + y * z2, (w * w2) - Type(1.0) + z * z2);
	}

	/**
	rotates passed vec by this (assumed unitary)
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxVec3T<Type> rotate(const PxVec3T<Type>& v) const
	{
		const Type vx = Type(2.0) * v.x;
		const Type vy = Type(2.0) * v.y;
		const Type vz = Type(2.0) * v.z;
		const Type w2 = w * w - 0.5f;
		const Type dot2 = (x * vx + y * vy + z * vz);
		return PxVec3T<Type>((vx * w2 + (y * vz - z * vy) * w + x * dot2), (vy * w2 + (z * vx - x * vz) * w + y * dot2),
						     (vz * w2 + (x * vy - y * vx) * w + z * dot2));
	}

	/**
	inverse rotates passed vec by this (assumed unitary)
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxVec3T<Type> rotateInv(const PxVec3T<Type>& v) const
	{
		const Type vx = Type(2.0) * v.x;
		const Type vy = Type(2.0) * v.y;
		const Type vz = Type(2.0) * v.z;
		const Type w2 = w * w - 0.5f;
		const Type dot2 = (x * vx + y * vy + z * vz);
		return PxVec3T<Type>((vx * w2 - (y * vz - z * vy) * w + x * dot2), (vy * w2 - (z * vx - x * vz) * w + y * dot2),
						    (vz * w2 - (x * vy - y * vx) * w + z * dot2));
	}

	/**
	\brief Assignment operator
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT& operator=(const PxQuatT& p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
		w = p.w;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT& operator*=(const PxQuatT& q)
	{
		const Type tx = w * q.x + q.w * x + y * q.z - q.y * z;
		const Type ty = w * q.y + q.w * y + z * q.x - q.z * x;
		const Type tz = w * q.z + q.w * z + x * q.y - q.x * y;

		w = w * q.w - q.x * x - y * q.y - q.z * z;
		x = tx;
		y = ty;
		z = tz;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT& operator+=(const PxQuatT& q)
	{
		x += q.x;
		y += q.y;
		z += q.z;
		w += q.w;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT& operator-=(const PxQuatT& q)
	{
		x -= q.x;
		y -= q.y;
		z -= q.z;
		w -= q.w;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT& operator*=(const Type s)
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}

	/** quaternion multiplication */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT operator*(const PxQuatT& q) const
	{
		return PxQuatT(w * q.x + q.w * x + y * q.z - q.y * z, w * q.y + q.w * y + z * q.x - q.z * x,
		              w * q.z + q.w * z + x * q.y - q.x * y, w * q.w - x * q.x - y * q.y - z * q.z);
	}

	/** quaternion addition */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT operator+(const PxQuatT& q) const
	{
		return PxQuatT(x + q.x, y + q.y, z + q.z, w + q.w);
	}

	/** quaternion subtraction */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT operator-() const
	{
		return PxQuatT(-x, -y, -z, -w);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT operator-(const PxQuatT& q) const
	{
		return PxQuatT(x - q.x, y - q.y, z - q.z, w - q.w);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxQuatT operator*(Type r) const
	{
		return PxQuatT(x * r, y * r, z * r, w * r);
	}

	/** the quaternion elements */
	Type	x, y, z, w;
};

typedef PxQuatT<float>	PxQuat;
typedef PxQuatT<double>	PxQuatd;

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

