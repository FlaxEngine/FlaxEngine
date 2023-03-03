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

#ifndef PX_MAT34_H
#define PX_MAT34_H
/** \addtogroup foundation
@{
*/

#include "foundation/PxTransform.h"
#include "foundation/PxMat33.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
/*!
Basic mathematical 3x4 matrix, implemented as a 3x3 rotation matrix and a translation

See PxMat33 for the format of the rotation matrix.
*/

template<class Type>
class PxMat34T
{
	public:
	//! Default constructor
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T()
	{
	}

	//! Construct from four base vectors
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(const PxVec3T<Type>& b0, const PxVec3T<Type>& b1, const PxVec3T<Type>& b2, const PxVec3T<Type>& b3)
		: m(b0, b1, b2), p(b3)
	{
	}

	//! Construct from Type[12]
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(Type values[]) :
		m(values), p(values[9], values[10], values[11])
	{		
	}

	//! Construct from a 3x3 matrix
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(const PxMat33T<Type>& other)
		: m(other), p(PxZero)
	{
	}

	//! Construct from a 3x3 matrix and a translation vector
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(const PxMat33T<Type>& other, const PxVec3T<Type>& t)
		: m(other), p(t)
	{
	}

	//! Construct from a PxTransformT<Type>
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(const PxTransformT<Type>& other)
		 : m(other.q), p(other.p)
	{
	}

	//! Copy constructor
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T(const PxMat34T& other) : m(other.m), p(other.p)
	{
	}

	//! Assignment operator
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxMat34T& operator=(const PxMat34T& other)
	{
		m = other.m;
		p = other.p;
		return *this;
	}

	//! Set to identity matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE void setIdentity()
	{
		m = PxMat33T<Type>(PxIdentity);
		p = PxVec3T<Type>(0);
	}
	
	// Simpler operators
	//! Equality operator
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator==(const PxMat34T& other) const
	{
		return m == other.m && p == other.p;
	}

	//! Inequality operator
	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator!=(const PxMat34T& other) const
	{
		return !operator==(other);
	}

	//! Unary minus
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator-() const
	{
		return PxMat34T(-m, -p);
	}

	//! Add
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator+(const PxMat34T& other) const
	{
		return PxMat34T(m + other.m, p + other.p);
	}

	//! Subtract
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator-(const PxMat34T& other) const
	{
		return PxMat34T(m - other.m, p - other.p);
	}

	//! Scalar multiplication
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator*(Type scalar) const
	{
		return PxMat34T(m*scalar, p*scalar);
	}

	//! Matrix multiplication
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator*(const PxMat34T& other) const
	{
		//Rows from this <dot> columns from other
		//base0 = rotate(other.m.column0) etc
		return PxMat34T(m*other.m, m*other.p + p);
	}

	//! Matrix multiplication, extend the second matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T operator*(const PxMat33T<Type>& other) const
	{
		//Rows from this <dot> columns from other
		//base0 = transform(other.m.column0) etc
		return PxMat34T(m*other, p);
	}

	template<class Type2>
	friend PxMat34T<Type2> operator*(const PxMat33T<Type2>& a, const PxMat34T<Type2>& b);
	
	// a <op>= b operators

	//! Equals-add
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T& operator+=(const PxMat34T& other)
	{
		m += other.m;
		p += other.p;
		return *this;
	}

	//! Equals-sub
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T& operator-=(const PxMat34T& other)
	{
		m -= other.m;
		p -= other.p;
		return *this;
	}

	//! Equals scalar multiplication
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T& operator*=(Type scalar)
	{
		m *= scalar;
		p *= scalar;
		return *this;
	}

	//! Element access, mathematical way!
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type operator()(PxU32 row, PxU32 col) const
	{
		return (*this)[col][row];
	}

	//! Element access, mathematical way!
	PX_CUDA_CALLABLE PX_FORCE_INLINE Type& operator()(PxU32 row, PxU32 col)
	{
		return (*this)[col][row];
	}

	// Transform etc
	
	//! Transform vector by matrix, equal to v' = M*v
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> rotate(const PxVec3T<Type>& other) const
	{
		return m*other;
	}

	//! Transform vector by transpose of matrix, equal to v' = M^t*v
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> rotateTranspose(const PxVec3T<Type>& other) const
	{
		return m.transformTranspose(other);
	}

	//! Transform point by matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> transform(const PxVec3T<Type>& other) const
	{
		return m*other + p;
	}

	//! Transform point by transposed matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type> transformTranspose(const PxVec3T<Type>& other) const
	{
		return m.transformTranspose(other - p);
	}

	//! Transform point by transposed matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T transformTranspose(const PxMat34T& other) const
	{
		return PxMat34T(m.transformTranspose(other.m.column0), 
						m.transformTranspose(other.m.column1), 
						m.transformTranspose(other.m.column2), 
						m.transformTranspose(other.p - p));
	}

	//! Invert matrix treating it as a rotation+translation matrix only
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat34T getInverseRT() const
	{
		return PxMat34T(m.getTranspose(), m.transformTranspose(-p));
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE		PxVec3T<Type>& operator[](PxU32 num)			{ return (&m.column0)[num];	}
	PX_CUDA_CALLABLE PX_FORCE_INLINE const	PxVec3T<Type>& operator[](PxU32 num)	const	{ return (&m.column0)[num];	}

	//Data, see above for format!

	PxMat33T<Type> m;
	PxVec3T<Type> p;
};

//! Multiply a*b, a is extended
template<class Type>
PX_INLINE PxMat34T<Type> operator*(const PxMat33T<Type>& a, const PxMat34T<Type>& b)
{
	return PxMat34T<Type>(a * b.m, a * b.p);
}

typedef PxMat34T<float>		PxMat34;
typedef PxMat34T<double>	PxMat34d;

//! A padded version of PxMat34, to safely load its data using SIMD
class PxMat34Padded : public PxMat34
{
	public:
		PX_FORCE_INLINE	PxMat34Padded(const PxMat34& src) : PxMat34(src)	{}
		PX_FORCE_INLINE	PxMat34Padded()										{}
		PX_FORCE_INLINE	~PxMat34Padded()									{}
		PxU32	padding;
};
PX_COMPILE_TIME_ASSERT(0==(sizeof(PxMat34Padded)==16));

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
