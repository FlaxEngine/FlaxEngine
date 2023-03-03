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

#ifndef PX_BASIC_TEMPLATES_H
#define PX_BASIC_TEMPLATES_H

#include "foundation/PxPreprocessor.h"

#if !PX_DOXYGEN
namespace physx
{
#endif
	template <typename A>
	struct PxEqual
	{
		bool operator()(const A& a, const A& b) const
		{
			return a == b;
		}
	};

	template <typename A>
	struct PxLess
	{
		bool operator()(const A& a, const A& b) const
		{
			return a < b;
		}
	};

	template <typename A>
	struct PxGreater
	{
		bool operator()(const A& a, const A& b) const
		{
			return a > b;
		}
	};

	template <class F, class S>
	class PxPair
	{
	public:
		F first;
		S second;
		PX_CUDA_CALLABLE PxPair() : first(F()), second(S())
		{
		}
		PX_CUDA_CALLABLE PxPair(const F& f, const S& s) : first(f), second(s)
		{
		}
		PX_CUDA_CALLABLE PxPair(const PxPair& p) : first(p.first), second(p.second)
		{
		}
		// CN - fix for /.../PxBasicTemplates.h(61) : warning C4512: 'physx::PxPair<F,S>' : assignment operator could
		// not be generated
		PX_CUDA_CALLABLE PxPair& operator=(const PxPair& p)
		{
			first = p.first;
			second = p.second;
			return *this;
		}
		PX_CUDA_CALLABLE bool operator==(const PxPair& p) const
		{
			return first == p.first && second == p.second;
		}
		PX_CUDA_CALLABLE bool operator<(const PxPair& p) const
		{
			if (first < p.first)
				return true;
			else
				return !(p.first < first) && (second < p.second);
		}
	};

	template <unsigned int A>
	struct PxLogTwo
	{
		static const unsigned int value = PxLogTwo<(A >> 1)>::value + 1;
	};
	template <>
	struct PxLogTwo<1>
	{
		static const unsigned int value = 0;
	};

	template <typename T>
	struct PxUnConst
	{
		typedef T Type;
	};
	template <typename T>
	struct PxUnConst<const T>
	{
		typedef T Type;
	};

	template <typename T>
	T PxPointerOffset(void* p, ptrdiff_t offset)
	{
		return reinterpret_cast<T>(reinterpret_cast<char*>(p) + offset);
	}
	template <typename T>
	T PxPointerOffset(const void* p, ptrdiff_t offset)
	{
		return reinterpret_cast<T>(reinterpret_cast<const char*>(p) + offset);
	}

	template <class T>
	PX_CUDA_CALLABLE PX_INLINE void PxSwap(T& x, T& y)
	{
		const T tmp = x;
		x = y;
		y = tmp;
	}

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

