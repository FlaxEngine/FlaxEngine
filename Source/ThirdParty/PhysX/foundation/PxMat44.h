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

#ifndef PX_MAT44_H
#define PX_MAT44_H
/** \addtogroup foundation
@{
*/

#include "foundation/PxQuat.h"
#include "foundation/PxVec4.h"
#include "foundation/PxMat33.h"
#include "foundation/PxTransform.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/*!
\brief 4x4 matrix class

This class is layout-compatible with D3D and OpenGL matrices. More notes on layout are given in the PxMat33

@see PxMat33 PxTransform
*/

template<class Type>
class PxMat44T
{
	public:
	//! Default constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat44T()
	{
	}

	//! identity constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat44T(PxIDENTITY) :
		column0(Type(1.0), Type(0.0), Type(0.0), Type(0.0)),
		column1(Type(0.0), Type(1.0), Type(0.0), Type(0.0)),
		column2(Type(0.0), Type(0.0), Type(1.0), Type(0.0)),
		column3(Type(0.0), Type(0.0), Type(0.0), Type(1.0))
	{
	}

	//! zero constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat44T(PxZERO) : column0(PxZero), column1(PxZero), column2(PxZero), column3(PxZero)
	{
	}

	//! Construct from four 4-vectors
	PX_CUDA_CALLABLE PxMat44T(const PxVec4T<Type>& col0, const PxVec4T<Type>& col1, const PxVec4T<Type>& col2, const PxVec4T<Type>& col3) :
		column0(col0),
		column1(col1),
		column2(col2),
		column3(col3)
	{
	}

	//! constructor that generates a multiple of the identity matrix
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat44T(Type r) :
		column0(r, Type(0.0), Type(0.0), Type(0.0)),
		column1(Type(0.0), r, Type(0.0), Type(0.0)),
		column2(Type(0.0), Type(0.0), r, Type(0.0)),
		column3(Type(0.0), Type(0.0), Type(0.0), r)
	{
	}

	//! Construct from three base vectors and a translation
	PX_CUDA_CALLABLE PxMat44T(const PxVec3T<Type>& col0, const PxVec3T<Type>& col1, const PxVec3T<Type>& col2, const PxVec3T<Type>& col3) :
		column0(col0, Type(0.0)),
		column1(col1, Type(0.0)),
		column2(col2, Type(0.0)),
		column3(col3, Type(1.0))
	{
	}

	//! Construct from Type[16]
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat44T(Type values[]) :
		column0(values[0], values[1], values[2], values[3]),
		column1(values[4], values[5], values[6], values[7]),
		column2(values[8], values[9], values[10], values[11]),
		column3(values[12], values[13], values[14], values[15])
	{
	}

	//! Construct from a quaternion
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat44T(const PxQuatT<Type>& q)
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

		column0 = PxVec4T<Type>(Type(1.0) - yy - zz, xy + zw, xz - yw, Type(0.0));
		column1 = PxVec4T<Type>(xy - zw, Type(1.0) - xx - zz, yz + xw, Type(0.0));
		column2 = PxVec4T<Type>(xz + yw, yz - xw, Type(1.0) - xx - yy, Type(0.0));
		column3 = PxVec4T<Type>(Type(0.0), Type(0.0), Type(0.0), Type(1.0));
	}

	//! Construct from a diagonal vector
	explicit PX_CUDA_CALLABLE PX_INLINE PxMat44T(const PxVec4T<Type>& diagonal) :
		column0(diagonal.x, Type(0.0), Type(0.0), Type(0.0)),
		column1(Type(0.0), diagonal.y, Type(0.0), Type(0.0)),
		column2(Type(0.0), Type(0.0), diagonal.z, Type(0.0)),
		column3(Type(0.0), Type(0.0), Type(0.0), diagonal.w)
	{
	}

	//! Construct from Mat33 and a translation
	PX_CUDA_CALLABLE PxMat44T(const PxMat33T<Type>& axes, const PxVec3T<Type>& position) :
		column0(axes.column0, Type(0.0)),
		column1(axes.column1, Type(0.0)),
		column2(axes.column2, Type(0.0)),
		column3(position, Type(1.0))
	{
	}

	PX_CUDA_CALLABLE PxMat44T(const PxTransform& t)
	{
		*this = PxMat44T(PxMat33T<Type>(t.q), t.p);
	}

	/**
	\brief returns true if the two matrices are exactly equal
	*/
	PX_CUDA_CALLABLE PX_INLINE bool operator==(const PxMat44T& m) const
	{
		return column0 == m.column0 && column1 == m.column1 && column2 == m.column2 && column3 == m.column3;
	}

	//! Copy constructor
	PX_CUDA_CALLABLE PX_INLINE PxMat44T(const PxMat44T& other) :
		column0(other.column0),
		column1(other.column1),
		column2(other.column2),
		column3(other.column3)
	{
	}

	//! Assignment operator
	PX_CUDA_CALLABLE PX_INLINE PxMat44T& operator=(const PxMat44T& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		column2 = other.column2;
		column3 = other.column3;
		return *this;
	}

	//! Get transposed matrix
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T getTranspose() const
	{
		return PxMat44T(
		    PxVec4T<Type>(column0.x, column1.x, column2.x, column3.x), PxVec4T<Type>(column0.y, column1.y, column2.y, column3.y),
		    PxVec4T<Type>(column0.z, column1.z, column2.z, column3.z), PxVec4T<Type>(column0.w, column1.w, column2.w, column3.w));
	}

	//! Unary minus
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T operator-() const
	{
		return PxMat44T(-column0, -column1, -column2, -column3);
	}

	//! Add
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T operator+(const PxMat44T& other) const
	{
		return PxMat44T(column0 + other.column0, column1 + other.column1, column2 + other.column2, column3 + other.column3);
	}

	//! Subtract
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T operator-(const PxMat44T& other) const
	{
		return PxMat44T(column0 - other.column0, column1 - other.column1, column2 - other.column2, column3 - other.column3);
	}

	//! Scalar multiplication
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T operator*(Type scalar) const
	{
		return PxMat44T(column0 * scalar, column1 * scalar, column2 * scalar, column3 * scalar);
	}

	template<class Type2>
	friend PxMat44T<Type2> operator*(Type2, const PxMat44T<Type2>&);

	//! Matrix multiplication
	PX_CUDA_CALLABLE PX_INLINE const PxMat44T operator*(const PxMat44T& other) const
	{
		// Rows from this <dot> columns from other
		// column0 = transform(other.column0) etc
		return PxMat44T(transform(other.column0), transform(other.column1), transform(other.column2), transform(other.column3));
	}

	// a <op>= b operators

	//! Equals-add
	PX_CUDA_CALLABLE PX_INLINE PxMat44T& operator+=(const PxMat44T& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		column2 += other.column2;
		column3 += other.column3;
		return *this;
	}

	//! Equals-sub
	PX_CUDA_CALLABLE PX_INLINE PxMat44T& operator-=(const PxMat44T& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		column2 -= other.column2;
		column3 -= other.column3;
		return *this;
	}

	//! Equals scalar multiplication
	PX_CUDA_CALLABLE PX_INLINE PxMat44T& operator*=(Type scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		column2 *= scalar;
		column3 *= scalar;
		return *this;
	}

	//! Equals matrix multiplication
	PX_CUDA_CALLABLE PX_INLINE PxMat44T& operator*=(const PxMat44T& other)
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

	//! Transform vector by matrix, equal to v' = M*v
	PX_CUDA_CALLABLE PX_INLINE const PxVec4T<Type> transform(const PxVec4T<Type>& other) const
	{
		return column0 * other.x + column1 * other.y + column2 * other.z + column3 * other.w;
	}

	//! Transform vector by matrix, equal to v' = M*v
	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> transform(const PxVec3T<Type>& other) const
	{
		return transform(PxVec4T<Type>(other, Type(1.0))).getXYZ();
	}

	//! Rotate vector by matrix, equal to v' = M*v
	PX_CUDA_CALLABLE PX_INLINE const PxVec4T<Type> rotate(const PxVec4T<Type>& other) const
	{
		return column0 * other.x + column1 * other.y + column2 * other.z; // + column3*0;
	}

	//! Rotate vector by matrix, equal to v' = M*v
	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> rotate(const PxVec3T<Type>& other) const
	{
		return rotate(PxVec4T<Type>(other, Type(1.0))).getXYZ();
	}

	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> getBasis(PxU32 num) const
	{
		PX_ASSERT(num < 3);
		return (&column0)[num].getXYZ();
	}

	PX_CUDA_CALLABLE PX_INLINE const PxVec3T<Type> getPosition() const
	{
		return column3.getXYZ();
	}

	PX_CUDA_CALLABLE PX_INLINE void setPosition(const PxVec3T<Type>& position)
	{
		column3.x = position.x;
		column3.y = position.y;
		column3.z = position.z;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const Type* front() const
	{
		return &column0.x;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE PxVec4T<Type>& operator[](PxU32 num)
	{
		return (&column0)[num];
	}
	PX_CUDA_CALLABLE PX_FORCE_INLINE const PxVec4T<Type>& operator[](PxU32 num) const
	{
		return (&column0)[num];
	}

	PX_CUDA_CALLABLE PX_INLINE void scale(const PxVec4T<Type>& p)
	{
		column0 *= p.x;
		column1 *= p.y;
		column2 *= p.z;
		column3 *= p.w;
	}

	PX_CUDA_CALLABLE PX_INLINE const PxMat44T inverseRT(void) const
	{
		const PxVec3T<Type> r0(column0.x, column1.x, column2.x);
		const PxVec3T<Type> r1(column0.y, column1.y, column2.y);
		const PxVec3T<Type> r2(column0.z, column1.z, column2.z);

		return PxMat44T(r0, r1, r2, -(r0 * column3.x + r1 * column3.y + r2 * column3.z));
	}

	PX_CUDA_CALLABLE PX_INLINE bool isFinite() const
	{
		return column0.isFinite() && column1.isFinite() && column2.isFinite() && column3.isFinite();
	}

	// Data, see above for format!

	PxVec4T<Type>	column0, column1, column2, column3; // the four base vectors
};

// implementation from PxTransform.h
template<class Type>
PX_CUDA_CALLABLE PX_FORCE_INLINE PxTransformT<Type>::PxTransformT(const PxMat44T<Type>& m)
{
	const PxVec3T<Type> column0(m.column0.x, m.column0.y, m.column0.z);
	const PxVec3T<Type> column1(m.column1.x, m.column1.y, m.column1.z);
	const PxVec3T<Type> column2(m.column2.x, m.column2.y, m.column2.z);

	q = PxQuatT<Type>(PxMat33T<Type>(column0, column1, column2));
	p = PxVec3T<Type>(m.column3.x, m.column3.y, m.column3.z);
}

typedef PxMat44T<float>		PxMat44;
typedef PxMat44T<double>	PxMat44d;

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

