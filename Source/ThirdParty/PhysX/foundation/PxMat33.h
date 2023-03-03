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

#ifndef PX_MAT33_H
#define PX_MAT33_H
/** \addtogroup foundation
@{
*/

#include "foundation/PxVec3.h"
#include "foundation/PxQuat.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
/*!
\brief 3x3 matrix class

Some clarifications, as there have been much confusion about matrix formats etc in the past.

Short:
- Matrix have base vectors in columns (vectors are column matrices, 3x1 matrices).
- Matrix is physically stored in column major format
- Matrices are concaternated from left

Long:
Given three base vectors a, b and c the matrix is stored as

|a.x b.x c.x|
|a.y b.y c.y|
|a.z b.z c.z|

Vectors are treated as columns, so the vector v is

|x|
|y|
|z|

And matrices are applied _before_ the vector (pre-multiplication)
v' = M*v

|x'|   |a.x b.x c.x|   |x|   |a.x*x + b.x*y + c.x*z|
|y'| = |a.y b.y c.y| * |y| = |a.y*x + b.y*y + c.y*z|
|z'|   |a.z b.z c.z|   |z|   |a.z*x + b.z*y + c.z*z|


Physical storage and indexing:
To be compatible with popular 3d rendering APIs (read D3d and OpenGL)
the physical indexing is

|0 3 6|
|1 4 7|
|2 5 8|

index = column*3 + row

which in C++ translates to M[column][row]

The mathematical indexing is M_row,column and this is what is used for _-notation
so _12 is 1st row, second column and operator(row, column)!
*/

template<class Type>
class PxMat33T
{
	public:
	//! Default constructor
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat33T()
	{
	}

	//! identity constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat33T(PxIDENTITY) :
		column0(Type(1.0), Type(0.0), Type(0.0)),
		column1(Type(0.0), Type(1.0), Type(0.0)),
		column2(Type(0.0), Type(0.0), Type(1.0))
	{
	}

	//! zero constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat33T(PxZERO) :
		column0(Type(0.0)),
		column1(Type(0.0)),
		column2(Type(0.0))
	{
	}

	//! Construct from three base vectors
	PX_CUDA_CALLABLE PxMat33T(const PxVec3T<Type>& col0, const PxVec3T<Type>& col1, const PxVec3T<Type>& col2) :
		column0(col0),
		column1(col1),
		column2(col2)
	{
	}

	//! constructor from a scalar, which generates a multiple of the identity matrix
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat33T(Type r) :
		column0(r, Type(0.0), Type(0.0)),
		column1(Type(0.0), r, Type(0.0)),
		column2(Type(0.0), Type(0.0), r)
	{
	}

	//! Construct from Type[9]
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat33T(Type values[]) :
		column0(values[0], values[1], values[2]),
		column1(values[3], values[4], values[5]),
		column2(values[6], values[7], values[8])
	{
	}

	//! Construct from a quaternion
	explicit PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat33T(const PxQuatT<Type>& q)
	{
		// PT: TODO: PX-566
		const Type x = q.x;
		const Type y = q.y;
		const Type z = q.z;
		const Type w = q.w;

		const Type x2 = x + x;
		const Type y2 = y + y;
		const Type z2 = z + z;

		const Type xx = x2 * x;
		const Type yy = y2 * y;
		const Type zz = z2 * z;

		const Type xy = x2 * y;
		const Type xz = x2 * z;
		const Type xw = x2 * w;

		const Type yz = y2 * z;
		const Type yw = y2 * w;
		const Type zw = z2 * w;

		column0 = PxVec3T<Type>(Type(1.0) - yy - zz, xy + zw, xz - yw);
		column1 = PxVec3T<Type>(xy - zw, Type(1.0) - xx - zz, yz + xw);
		column2 = PxVec3T<Type>(xz + yw, yz - xw, Type(1.0) - xx - yy);
	}

	//! Copy constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat33T(const PxMat33T& other) :
		column0(other.column0),
		column1(other.column1),
		column2(other.column2)
	{
	}

	//! Assignment operator
	PX_CUDA_CALLABLE PX_FORCE_INLINE PxMat33T& operator=(const PxMat33T& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		column2 = other.column2;
		return *this;
	}

	//! Construct from diagonal, off-diagonals are zero.
	PX_CUDA_CALLABLE PX_INLINE static const PxMat33T createDiagonal(const PxVec3T<Type>& d)
	{
		return PxMat33T(PxVec3T<Type>(d.x, Type(0.0), Type(0.0)),
						PxVec3T<Type>(Type(0.0), d.y, Type(0.0)),
						PxVec3T<Type>(Type(0.0), Type(0.0), d.z));
	}

	//! Computes the outer product of two vectors
	PX_CUDA_CALLABLE PX_INLINE static const PxMat33T outer(const PxVec3T<Type>& a, const PxVec3T<Type>& b)
	{
		return PxMat33T(a * b.x, a * b.y, a * b.z);
	}

	/**
	\brief returns true if the two matrices are exactly equal
	*/
	PX_CUDA_CALLABLE PX_INLINE bool operator==(const PxMat33T& m) const
	{
		return column0 == m.column0 && column1 == m.column1 && column2 == m.column2;
	}

	//! Get transposed matrix
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxMat33T getTranspose() const
	{
		const PxVec3T<Type> v0(column0.x, column1.x, column2.x);
		const PxVec3T<Type> v1(column0.y, column1.y, column2.y);
		const PxVec3T<Type> v2(column0.z, column1.z, column2.z);

		return PxMat33T(v0, v1, v2);
	}

	//! Get the real inverse
	PX_CUDA_CALLABLE PX_INLINE const PxMat33T getInverse() const
	{
		const Type det = getDeterminant();
		PxMat33T inverse;

		if(det != Type(0.0))
		{
			const Type invDet = Type(1.0) / det;

			inverse.column0.x = invDet * (column1.y * column2.z - column2.y * column1.z);
			inverse.column0.y = invDet * -(column0.y * column2.z - column2.y * column0.z);
			inverse.column0.z = invDet * (column0.y * column1.z - column0.z * column1.y);

			inverse.column1.x = invDet * -(column1.x * column2.z - column1.z * column2.x);
			inverse.column1.y = invDet * (column0.x * column2.z - column0.z * column2.x);
			inverse.column1.z = invDet * -(column0.x * column1.z - column0.z * column1.x);

			inverse.column2.x = invDet * (column1.x * column2.y - column1.y * column2.x);
			inverse.column2.y = invDet * -(column0.x * column2.y - column0.y * column2.x);
			inverse.column2.z = invDet * (column0.x * column1.y - column1.x * column0.y);

			return inverse;
		}
		else
		{
			return PxMat33T(PxIdentity);
		}
	}

	//! Get determinant
	PX_CUDA_CALLABLE PX_INLINE Type getDeterminant() const
	{
		return column0.dot(column1.cross(column2));
	}

	//! Unary minus
	PX_CUDA_CALLABLE PX_INLINE const PxMat33T operator-() const
	{
		return PxMat33T(-column0, -column1, -column2);
	}

	//! Add
	PX_CUDA_CALLABLE PX_INLINE const PxMat33T operator+(const PxMat33T& other) const
	{
		return PxMat33T(column0 + other.column0, column1 + other.column1, column2 + other.column2);
	}

	//! Subtract
	PX_CUDA_CALLABLE PX_INLINE const PxMat33T operator-(const PxMat33T& other) const
	{
		return PxMat33T(column0 - other.column0, column1 - other.column1, column2 - other.column2);
	}

	//! Scalar multiplication
	PX_CUDA_CALLABLE PX_INLINE const PxMat33T operator*(Type scalar) const
	{
		return PxMat33T(column0 * scalar, column1 * scalar, column2 * scalar);
	}

	template<class Type2>
	PX_CUDA_CALLABLE PX_INLINE friend PxMat33T<Type2> operator*(Type2, const PxMat33T<Type2>&);

	//! Matrix vector multiplication (returns 'this->transform(vec)')
	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> operator*(const PxVec3T<Type>& vec) const
	{
		return transform(vec);
	}

	// a <op>= b operators

	//! Matrix multiplication
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxMat33T operator*(const PxMat33T& other) const
	{
		// Rows from this <dot> columns from other
		// column0 = transform(other.column0) etc
		return PxMat33T(transform(other.column0),
						transform(other.column1),
						transform(other.column2));
	}

	//! Equals-add
	PX_CUDA_CALLABLE PX_INLINE PxMat33T& operator+=(const PxMat33T& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		column2 += other.column2;
		return *this;
	}

	//! Equals-sub
	PX_CUDA_CALLABLE PX_INLINE PxMat33T& operator-=(const PxMat33T& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		column2 -= other.column2;
		return *this;
	}

	//! Equals scalar multiplication
	PX_CUDA_CALLABLE PX_INLINE PxMat33T& operator*=(Type scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		column2 *= scalar;
		return *this;
	}

	//! Equals matrix multiplication
	PX_CUDA_CALLABLE PX_INLINE PxMat33T& operator*=(const PxMat33T& other)
	{
		*this = *this * other;
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
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxVec3T<Type> transform(const PxVec3T<Type>& other) const
	{
		return column0 * other.x + column1 * other.y + column2 * other.z;
	}

	//! Transform vector by matrix transpose, v' = M^t*v
	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> transformTranspose(const PxVec3T<Type>& other) const
	{
		return PxVec3T<Type>(column0.dot(other), column1.dot(other), column2.dot(other));
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const Type* front() const
	{
		return &column0.x;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec3T<Type>& operator[](PxU32 num)
	{
		return (&column0)[num];
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxVec3T<Type>& operator[](PxU32 num) const
	{
		return (&column0)[num];
	}

	// Data, see above for format!

	PxVec3T<Type>	column0, column1, column2; // the three base vectors
};

template<class Type>
PX_CUDA_CALLABLE PX_INLINE PxMat33T<Type> operator*(Type scalar, const PxMat33T<Type>& m)
{
	return PxMat33T<Type>(scalar * m.column0, scalar * m.column1, scalar * m.column2);
}

// implementation from PxQuat.h
template<class Type>
PX_CUDA_CALLABLE PX_INLINE PxQuatT<Type>::PxQuatT(const PxMat33T<Type>& m)
{
	if(m.column2.z < Type(0))
	{
		if(m.column0.x > m.column1.y)
		{
			const Type t = Type(1.0) + m.column0.x - m.column1.y - m.column2.z;
			*this = PxQuatT<Type>(t, m.column0.y + m.column1.x, m.column2.x + m.column0.z, m.column1.z - m.column2.y) * (Type(0.5) / PxSqrt(t));
		}
		else
		{
			const Type t = Type(1.0) - m.column0.x + m.column1.y - m.column2.z;
			*this = PxQuatT<Type>(m.column0.y + m.column1.x, t, m.column1.z + m.column2.y, m.column2.x - m.column0.z) * (Type(0.5) / PxSqrt(t));
		}
	}
	else
	{
		if(m.column0.x < -m.column1.y)
		{
			const Type t = Type(1.0) - m.column0.x - m.column1.y + m.column2.z;
			*this = PxQuatT<Type>(m.column2.x + m.column0.z, m.column1.z + m.column2.y, t, m.column0.y - m.column1.x) * (Type(0.5) / PxSqrt(t));
		}
		else
		{
			const Type t = Type(1.0) + m.column0.x + m.column1.y + m.column2.z;
			*this = PxQuatT<Type>(m.column1.z - m.column2.y, m.column2.x - m.column0.z, m.column0.y - m.column1.x, t) * (Type(0.5) / PxSqrt(t));
		}
	}
}

typedef PxMat33T<float>		PxMat33;
typedef PxMat33T<double>	PxMat33d;

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

