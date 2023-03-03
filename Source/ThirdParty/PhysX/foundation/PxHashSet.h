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

#ifndef PX_HASHSET_H
#define PX_HASHSET_H

#include "foundation/PxHashInternals.h"

// TODO: make this doxy-format

// This header defines two hash sets. Hash sets
// * support custom initial table sizes (rounded up internally to power-of-2)
// * support custom static allocator objects
// * auto-resize, based on a load factor (i.e. a 64-entry .75 load factor hash will resize
//                                        when the 49th element is inserted)
// * are based on open hashing
//
// Sets have STL-like copying semantics, and properly initialize and destruct copies of objects
//
// There are two forms of set: coalesced and uncoalesced. Coalesced sets keep the entries in the
// initial segment of an array, so are fast to iterate over; however deletion is approximately
// twice as expensive.
//
// HashSet<T>:
//		bool		insert(const T& k)						amortized O(1) (exponential resize policy)
// 		bool		contains(const T& k)	const;			O(1)
//		bool		erase(const T& k);						O(1)
//		uint32_t		size()					const;			constant
//		void		reserve(uint32_t size);					O(MAX(size, currentOccupancy))
//		void		clear();								O(currentOccupancy) (with zero constant for objects without
// destructors)
//      Iterator    getIterator();
//
// Use of iterators:
//
// for(HashSet::Iterator iter = test.getIterator(); !iter.done(); ++iter)
//			myFunction(*iter);
//
// CoalescedHashSet<T> does not support getIterator, but instead supports
// 		const Key *getEntries();
//
// insertion into a set already containing the element fails returning false, as does
// erasure of an element not in the set
//

#if !PX_DOXYGEN
namespace physx
{
#endif
template <class Key, class HashFn = PxHash<Key>, class Allocator = PxAllocator>
class PxHashSet : public physx::PxHashSetBase<Key, HashFn, Allocator, false>
{
  public:
	typedef physx::PxHashSetBase<Key, HashFn, Allocator, false> HashSetBase;
	typedef typename HashSetBase::Iterator Iterator;

	PxHashSet(uint32_t initialTableSize = 64, float loadFactor = 0.75f) : HashSetBase(initialTableSize, loadFactor)
	{
	}
	PxHashSet(uint32_t initialTableSize, float loadFactor, const Allocator& alloc)
	: HashSetBase(initialTableSize, loadFactor, alloc)
	{
	}
	PxHashSet(const Allocator& alloc) : HashSetBase(64, 0.75f, alloc)
	{
	}
	Iterator getIterator()
	{
		return Iterator(HashSetBase::mBase);
	}
};

template <class Key, class HashFn = PxHash<Key>, class Allocator = PxAllocator>
class PxCoalescedHashSet : public physx::PxHashSetBase<Key, HashFn, Allocator, true>
{
  public:
	typedef typename physx::PxHashSetBase<Key, HashFn, Allocator, true> HashSetBase;

	PxCoalescedHashSet(uint32_t initialTableSize = 64, float loadFactor = 0.75f)
	: HashSetBase(initialTableSize, loadFactor)
	{
	}

	PxCoalescedHashSet(uint32_t initialTableSize, float loadFactor, const Allocator& alloc)
	: HashSetBase(initialTableSize, loadFactor, alloc)
	{
	}
	PxCoalescedHashSet(const Allocator& alloc) : HashSetBase(64, 0.75f, alloc)
	{
	}

	const Key* getEntries() const
	{
		return HashSetBase::mBase.getEntries();
	}
};

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

