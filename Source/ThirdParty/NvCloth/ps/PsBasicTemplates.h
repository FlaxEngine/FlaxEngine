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

#ifndef PSFOUNDATION_PSBASICTEMPLATES_H
#define PSFOUNDATION_PSBASICTEMPLATES_H

#include "Ps.h"

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
template <typename A>
struct Equal
{
	bool operator()(const A& a, const A& b) const
	{
		return a == b;
	}
};

template <typename A>
struct Less
{
	bool operator()(const A& a, const A& b) const
	{
		return a < b;
	}
};

template <typename A>
struct Greater
{
	bool operator()(const A& a, const A& b) const
	{
		return a > b;
	}
};

template <class F, class S>
class Pair
{
  public:
	F first;
	S second;
	Pair() : first(F()), second(S())
	{
	}
	Pair(const F& f, const S& s) : first(f), second(s)
	{
	}
	Pair(const Pair& p) : first(p.first), second(p.second)
	{
	}
	// CN - fix for /.../PsBasicTemplates.h(61) : warning C4512: 'nv::cloth::Pair<F,S>' : assignment operator could
	// not be generated
	Pair& operator=(const Pair& p)
	{
		first = p.first;
		second = p.second;
		return *this;
	}
	bool operator==(const Pair& p) const
	{
		return first == p.first && second == p.second;
	}
	bool operator<(const Pair& p) const
	{
		if(first < p.first)
			return true;
		else
			return !(p.first < first) && (second < p.second);
	}
};

template <unsigned int A>
struct LogTwo
{
	static const unsigned int value = LogTwo<(A >> 1)>::value + 1;
};
template <>
struct LogTwo<1>
{
	static const unsigned int value = 0;
};

template <typename T>
struct UnConst
{
	typedef T Type;
};
template <typename T>
struct UnConst<const T>
{
	typedef T Type;
};

template <typename T>
T pointerOffset(void* p, ptrdiff_t offset)
{
	return reinterpret_cast<T>(reinterpret_cast<char*>(p) + offset);
}
template <typename T>
T pointerOffset(const void* p, ptrdiff_t offset)
{
	return reinterpret_cast<T>(reinterpret_cast<const char*>(p) + offset);
}

template <class T>
PX_CUDA_CALLABLE PX_INLINE void swap(T& x, T& y)
{
	const T tmp = x;
	x = y;
	y = tmp;
}

} // namespace ps
} // namespace cloth
} // namespace nv

#endif // #ifndef PSFOUNDATION_PSBASICTEMPLATES_H
