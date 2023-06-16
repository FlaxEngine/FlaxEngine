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

#ifndef PX_SLIST_H
#define PX_SLIST_H

#include "foundation/Px.h"
#include "foundation/PxAssert.h"
#include "foundation/PxAlignedMalloc.h"

#if PX_P64_FAMILY
	#define PX_SLIST_ALIGNMENT 16
#else
	#define PX_SLIST_ALIGNMENT 8
#endif

#if !PX_DOXYGEN
namespace physx
{
#endif
#if PX_VC
	#pragma warning(push)
	#pragma warning(disable : 4324) // Padding was added at the end of a structure because of a __declspec(align) value.
#endif

PX_ALIGN_PREFIX(PX_SLIST_ALIGNMENT)
class PxSListEntry
{
	friend struct PxSListImpl;

  public:
	PxSListEntry() : mNext(NULL)
	{
		PX_ASSERT((size_t(this) & (PX_SLIST_ALIGNMENT - 1)) == 0);
	}

	// Only use on elements returned by SList::flush()
	// because the operation is not atomic.
	PxSListEntry* next()
	{
		return mNext;
	}

  private:
	PxSListEntry* mNext;
}PX_ALIGN_SUFFIX(PX_SLIST_ALIGNMENT);

#if PX_VC
	#pragma warning(pop)
#endif

// template-less implementation
struct PX_FOUNDATION_API PxSListImpl
{
	PxSListImpl();
	~PxSListImpl();
	void push(PxSListEntry* entry);
	PxSListEntry* pop();
	PxSListEntry* flush();
	static uint32_t getSize();
};

template <typename Alloc = PxReflectionAllocator<PxSListImpl> >
class PxSListT : protected Alloc
{
  public:
	PxSListT(const Alloc& alloc = Alloc()) : Alloc(alloc)
	{
		mImpl = reinterpret_cast<PxSListImpl*>(Alloc::allocate(PxSListImpl::getSize(), __FILE__, __LINE__));
		PX_ASSERT((size_t(mImpl) & (PX_SLIST_ALIGNMENT - 1)) == 0);
		PX_PLACEMENT_NEW(mImpl, PxSListImpl)();
	}
	~PxSListT()
	{
		mImpl->~PxSListImpl();
		Alloc::deallocate(mImpl);
	}

	// pushes a new element to the list
	void push(PxSListEntry& entry)
	{
		mImpl->push(&entry);
	}

	// pops an element from the list
	PxSListEntry* pop()
	{
		return mImpl->pop();
	}

	// removes all items from list, returns pointer to first element
	PxSListEntry* flush()
	{
		return mImpl->flush();
	}

  private:
	PxSListImpl* mImpl;
};

typedef PxSListT<> PxSList;

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

