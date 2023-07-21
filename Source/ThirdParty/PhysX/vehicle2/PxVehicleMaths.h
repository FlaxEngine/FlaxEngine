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

#pragma once
/** \addtogroup vehicle2
  @{
*/

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxMemory.h"

#include "PxVehicleLimits.h"

#if !PX_DOXYGEN
namespace physx
{
namespace vehicle2
{
#endif

class PxVehicleVectorN
{
public:
	enum
	{
		eMAX_SIZE = PxVehicleLimits::eMAX_NB_WHEELS + 3
	};

	PxVehicleVectorN(const PxU32 size)
		: mSize(size)
	{
		PX_ASSERT(mSize <= PxVehicleVectorN::eMAX_SIZE);
		PxMemZero(mValues, sizeof(PxReal)*PxVehicleVectorN::eMAX_SIZE);
	}

	~PxVehicleVectorN()
	{
	}

	PxVehicleVectorN(const PxVehicleVectorN& src)
	{
		for (PxU32 i = 0; i < src.mSize; i++)
		{
			mValues[i] = src.mValues[i];
		}
		mSize = src.mSize;
	}

	PX_FORCE_INLINE PxVehicleVectorN& operator=(const PxVehicleVectorN& src)
	{
		for (PxU32 i = 0; i < src.mSize; i++)
		{
			mValues[i] = src.mValues[i];
		}
		mSize = src.mSize;
		return *this;
	}

	PX_FORCE_INLINE PxReal& operator[] (const PxU32 i)
	{
		PX_ASSERT(i < mSize);
		return (mValues[i]);
	}

	PX_FORCE_INLINE const PxReal& operator[] (const PxU32 i) const
	{
		//PX_ASSERT(i < mSize);
		return (mValues[i]);
	}

	PX_FORCE_INLINE PxU32 getSize() const { return mSize; }

private:

	PxReal mValues[PxVehicleVectorN::eMAX_SIZE];
	PxU32 mSize;
};

class PxVehicleMatrixNN
{
public:

	PxVehicleMatrixNN()
		: mSize(0)
	{
	}

	PxVehicleMatrixNN(const PxU32 size)
		: mSize(size)
	{
		PX_ASSERT(mSize <= PxVehicleVectorN::eMAX_SIZE);
		PxMemZero(mValues, sizeof(PxReal)*PxVehicleVectorN::eMAX_SIZE*PxVehicleVectorN::eMAX_SIZE);
	}

	PxVehicleMatrixNN(const PxVehicleMatrixNN& src)
	{
		for (PxU32 i = 0; i < src.mSize; i++)
		{
			for (PxU32 j = 0; j < src.mSize; j++)
			{
				mValues[i][j] = src.mValues[i][j];
			}
		}
		mSize = src.mSize;
	}

	~PxVehicleMatrixNN()
	{
	}

	PX_FORCE_INLINE PxVehicleMatrixNN& operator=(const PxVehicleMatrixNN& src)
	{
		for (PxU32 i = 0; i < src.mSize; i++)
		{
			for (PxU32 j = 0; j < src.mSize; j++)
			{
				mValues[i][j] = src.mValues[i][j];
			}
		}
		mSize = src.mSize;
		return *this;
	}

	PX_FORCE_INLINE PxReal get(const PxU32 i, const PxU32 j) const
	{
		PX_ASSERT(i < mSize);
		PX_ASSERT(j < mSize);
		return mValues[i][j];
	}

	PX_FORCE_INLINE void set(const PxU32 i, const PxU32 j, const PxReal val)
	{
		PX_ASSERT(i < mSize);
		PX_ASSERT(j < mSize);
		mValues[i][j] = val;
	}

	PX_FORCE_INLINE PxU32 getSize() const { return mSize; }

	PX_FORCE_INLINE void setSize(const PxU32 size)
	{
		PX_ASSERT(size <= PxVehicleVectorN::eMAX_SIZE);
		mSize = size;
	}

public:

	PxReal mValues[PxVehicleVectorN::eMAX_SIZE][PxVehicleVectorN::eMAX_SIZE];
	PxU32 mSize;
};


/*
	LUPQ decomposition

	Based upon "Outer Product LU with Complete Pivoting," from Matrix Computations (4th Edition), Golub and Van Loan

	Solve A*x = b using:

		MatrixNNLUSolver solver;
		solver.decomposeLU(A);
		solver.solve(b, x);
*/
class PxVehicleMatrixNNLUSolver
{
private:

	PxVehicleMatrixNN mLU;
	PxU32	mP[PxVehicleVectorN::eMAX_SIZE - 1];	// Row permutation
	PxU32	mQ[PxVehicleVectorN::eMAX_SIZE - 1];	// Column permutation
	PxReal	mDetM;

public:

	PxVehicleMatrixNNLUSolver() {}
	~PxVehicleMatrixNNLUSolver() {}

	PxReal getDet() const { return mDetM; }

	void decomposeLU(const PxVehicleMatrixNN& A);

	//Given a matrix A and a vector b find x that satisfies Ax = b, where the matrix A is the matrix that was passed to #decomposeLU.
	//Returns true if the lu decomposition indicates that the matrix has an inverse and x was successfully computed.
	//Returns false if the lu decomposition resulted in zero determinant ie the matrix has no inverse and no solution exists for x.
	//Returns false if the size of either b or x doesn't match the size of the matrix passed to #decomposeLU.
	//If false is returned then each relevant element of x is set to zero. 
	bool solve(const PxVehicleVectorN& b, PxVehicleVectorN& x) const;
};


class PxVehicleMatrixNGaussSeidelSolver
{
public:

	void solve(const PxU32 maxIterations, const PxReal tolerance, const PxVehicleMatrixNN& A, const PxVehicleVectorN& b, PxVehicleVectorN& result) const;
};

class PxVehicleMatrix33Solver
{
public:

	bool solve(const PxVehicleMatrixNN& A_, const PxVehicleVectorN& b_, PxVehicleVectorN& result) const;
};

#if !PX_DOXYGEN
} //namespace vehicle2
} //namespace physx
#endif

/** @} */


