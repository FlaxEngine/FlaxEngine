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

#ifndef PX_VEC2_H
#define PX_VEC2_H

/** \addtogroup foundation
@{
*/

#include "foundation/PxMath.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief 2 Element vector class.

This is a 2-dimensional vector class with public data members.
*/
template<class Type>
class PxVec2T
{
  public:
	/**
	\brief default constructor leaves data uninitialized.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T()
	{
	}

	/**
	\brief zero constructor.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T(PxZERO) : x(Type(0.0)), y(Type(0.0))
	{
	}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T(Type a) : x(a), y(a)
	{
	}

	/**
	\brief Initializes from 2 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T(Type nx, Type ny) : x(nx), y(ny)
	{
	}

	/**
	\brief Copy ctor.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T(const PxVec2T& v) : x(v.x), y(v.y)
	{
	}

	// Operators

	/**
	\brief Assignment operator
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T& operator=(const PxVec2T& p)
	{
		x = p.x;
		y = p.y;
		return *this;
	}

	/**
	\brief element access
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type& operator[](unsigned int index)
	{
		PX_ASSERT(index <= 1);
		return reinterpret_cast<Type*>(this)[index];
	}

	/**
	\brief element access
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE const Type& operator[](unsigned int index) const
	{
		PX_ASSERT(index <= 1);
		return reinterpret_cast<const Type*>(this)[index];
	}

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator==(const PxVec2T& v) const
	{
		return x == v.x && y == v.y;
	}

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator!=(const PxVec2T& v) const
	{
		return x != v.x || y != v.y;
	}

	/**
	\brief tests for exact zero vector
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isZero() const
	{
		return x == Type(0.0) && y == Type(0.0);
	}

	/**
	\brief returns true if all 2 elems of the vector are finite (not NAN or INF, etc.)
	*/
	PX_CUDA_CALLABLE PX_INLINE bool isFinite() const
	{
		return PxIsFinite(x) && PxIsFinite(y);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isNormalized() const
	{
		const Type unitTolerance = Type(1e-4);
		return isFinite() && PxAbs(magnitude() - Type(1.0)) < unitTolerance;
	}

	/**
	\brief returns the squared magnitude

	Avoids calling PxSqrt()!
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type magnitudeSquared() const
	{
		return x * x + y * y;
	}

	/**
	\brief returns the magnitude
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type magnitude() const
	{
		return PxSqrt(magnitudeSquared());
	}

	/**
	\brief negation
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T operator-() const
	{
		return PxVec2T(-x, -y);
	}

	/**
	\brief vector addition
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T operator+(const PxVec2T& v) const
	{
		return PxVec2T(x + v.x, y + v.y);
	}

	/**
	\brief vector difference
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T operator-(const PxVec2T& v) const
	{
		return PxVec2T(x - v.x, y - v.y);
	}

	/**
	\brief scalar post-multiplication
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T operator*(Type f) const
	{
		return PxVec2T(x * f, y * f);
	}

	/**
	\brief scalar division
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T operator/(Type f) const
	{
		f = Type(1.0) / f;
		return PxVec2T(x * f, y * f);
	}

	/**
	\brief vector addition
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T& operator+=(const PxVec2T& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	/**
	\brief vector difference
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T& operator-=(const PxVec2T& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T& operator*=(Type f)
	{
		x *= f;
		y *= f;
		return *this;
	}

	/**
	\brief scalar division
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T& operator/=(Type f)
	{
		f = Type(1.0) / f;
		x *= f;
		y *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type dot(const PxVec2T& v) const
	{
		return x * v.x + y * v.y;
	}

	/** returns a unit vector */
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T getNormalized() const
	{
		const Type m = magnitudeSquared();
		return m > Type(0.0) ? *this * PxRecipSqrt(m) : PxVec2T(Type(0));
	}

	/**
	\brief normalizes the vector in place
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type normalize()
	{
		const Type m = magnitude();
		if(m > Type(0.0))
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T multiply(const PxVec2T& a) const
	{
		return PxVec2T(x * a.x, y * a.y);
	}

	/**
	\brief element-wise minimum
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T minimum(const PxVec2T& v) const
	{
		return PxVec2T(PxMin(x, v.x), PxMin(y, v.y));
	}

	/**
	\brief returns MIN(x, y);
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type minElement() const
	{
		return PxMin(x, y);
	}

	/**
	\brief element-wise maximum
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec2T maximum(const PxVec2T& v) const
	{
		return PxVec2T(PxMax(x, v.x), PxMax(y, v.y));
	}

	/**
	\brief returns MAX(x, y);
	*/
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type maxElement() const
	{
		return PxMax(x, y);
	}

	Type	x, y;
};

template<class Type>
PX_CUDA_CALLABLE static PX_FORCE_INLINE PxVec2T<Type> operator*(Type f, const PxVec2T<Type>& v)
{
	return PxVec2T<Type>(f * v.x, f * v.y);
}

typedef PxVec2T<float>	PxVec2;
typedef PxVec2T<double>	PxVec2d;

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

