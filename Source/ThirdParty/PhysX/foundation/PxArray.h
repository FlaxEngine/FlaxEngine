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

#ifndef PX_ARRAY_H
#define PX_ARRAY_H

#include "foundation/PxAssert.h"
#include "foundation/PxMathIntrinsics.h"
#include "foundation/PxAllocator.h"
#include "foundation/PxBasicTemplates.h"
#include "foundation/PxMemory.h"

namespace physx
{

/*!
An array is a sequential container.

Implementation note
* entries between 0 and size are valid objects
* we use inheritance to build this because the array is included inline in a lot
  of objects and we want the allocator to take no space if it's not stateful, which
  aggregation doesn't allow. Also, we want the metadata at the front for the inline
  case where the allocator contains some inline storage space
*/
template <class T, class Alloc = typename PxAllocatorTraits<T>::Type>
class PxArray : protected Alloc
{
  public:
	typedef T* Iterator;
	typedef const T* ConstIterator;

	explicit PxArray(const PxEMPTY v) : Alloc(v)
	{
		if(mData)
			mCapacity |= PX_SIGN_BITMASK;
	}

	/*!
	Default array constructor. Initialize an empty array
	*/
	PX_INLINE explicit PxArray(const Alloc& alloc = Alloc()) : Alloc(alloc), mData(0), mSize(0), mCapacity(0)
	{
	}

	/*!
	Initialize array with given capacity
	*/
	PX_INLINE explicit PxArray(uint32_t size, const T& a = T(), const Alloc& alloc = Alloc())
	: Alloc(alloc), mData(0), mSize(0), mCapacity(0)
	{
		resize(size, a);
	}

	/*!
	Copy-constructor. Copy all entries from other array
	*/
	template <class A>
	PX_INLINE explicit PxArray(const PxArray<T, A>& other, const Alloc& alloc = Alloc())
	: Alloc(alloc)
	{
		copy(other);
	}

	// This is necessary else the basic default copy constructor is used in the case of both arrays being of the same
	// template instance
	// The C++ standard clearly states that a template constructor is never a copy constructor [2]. In other words,
	// the presence of a template constructor does not suppress the implicit declaration of the copy constructor.
	// Also never make a copy constructor explicit, or copy-initialization* will no longer work. This is because
	// 'binding an rvalue to a const reference requires an accessible copy constructor' (http://gcc.gnu.org/bugs/)
	// *http://stackoverflow.com/questions/1051379/is-there-a-difference-in-c-between-copy-initialization-and-assignment-initializ
	PX_INLINE PxArray(const PxArray& other, const Alloc& alloc = Alloc()) : Alloc(alloc)
	{
		copy(other);
	}

	/*!
	Initialize array with given length
	*/
	PX_INLINE explicit PxArray(const T* first, const T* last, const Alloc& alloc = Alloc())
	: Alloc(alloc), mSize(last < first ? 0 : uint32_t(last - first)), mCapacity(mSize)
	{
		mData = allocate(mSize);
		copy(mData, mData + mSize, first);
	}

	/*!
	Destructor
	*/
	PX_INLINE ~PxArray()
	{
		destroy(mData, mData + mSize);

		if(capacity() && !isInUserMemory())
			deallocate(mData);
	}

	/*!
	Assignment operator. Copy content (deep-copy)
	*/
	template <class A>
	PX_INLINE PxArray& operator=(const PxArray<T, A>& rhs)
	{
		if(&rhs == this)
			return *this;

		clear();
		reserve(rhs.mSize);
		copy(mData, mData + rhs.mSize, rhs.mData);

		mSize = rhs.mSize;
		return *this;
	}

	PX_INLINE PxArray& operator=(const PxArray& t) // Needs to be declared, see comment at copy-constructor
	{
		return operator=<Alloc>(t);
	}

	/*!
	Array indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	PX_FORCE_INLINE const T& operator[](uint32_t i) const
	{
		PX_ASSERT(i < mSize);
		return mData[i];
	}

	/*!
	Array indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	PX_FORCE_INLINE T& operator[](uint32_t i)
	{
		PX_ASSERT(i < mSize);
		return mData[i];
	}

	/*!
	Returns a pointer to the initial element of the array.
	\return
	a pointer to the initial element of the array.
	*/
	PX_FORCE_INLINE ConstIterator begin() const
	{
		return mData;
	}

	PX_FORCE_INLINE Iterator begin()
	{
		return mData;
	}

	/*!
	Returns an iterator beyond the last element of the array. Do not dereference.
	\return
	a pointer to the element beyond the last element of the array.
	*/

	PX_FORCE_INLINE ConstIterator end() const
	{
		return mData + mSize;
	}

	PX_FORCE_INLINE Iterator end()
	{
		return mData + mSize;
	}

	/*!
	Returns a reference to the first element of the array. Undefined if the array is empty.
	\return a reference to the first element of the array
	*/

	PX_FORCE_INLINE const T& front() const
	{
		PX_ASSERT(mSize);
		return mData[0];
	}

	PX_FORCE_INLINE T& front()
	{
		PX_ASSERT(mSize);
		return mData[0];
	}

	/*!
	Returns a reference to the last element of the array. Undefined if the array is empty
	\return a reference to the last element of the array
	*/

	PX_FORCE_INLINE const T& back() const
	{
		PX_ASSERT(mSize);
		return mData[mSize - 1];
	}

	PX_FORCE_INLINE T& back()
	{
		PX_ASSERT(mSize);
		return mData[mSize - 1];
	}

	/*!
	Returns the number of entries in the array. This can, and probably will,
	differ from the array capacity.
	\return
	The number of of entries in the array.
	*/
	PX_FORCE_INLINE uint32_t size() const
	{
		return mSize;
	}

	/*!
	Clears the array.
	*/
	PX_INLINE void clear()
	{
		destroy(mData, mData + mSize);
		mSize = 0;
	}

	/*!
	Returns whether the array is empty (i.e. whether its size is 0).
	\return
	true if the array is empty
	*/
	PX_FORCE_INLINE bool empty() const
	{
		return mSize == 0;
	}

	/*!
	Finds the first occurrence of an element in the array.
	\param a
	The element to find.
	*/

	PX_INLINE Iterator find(const T& a)
	{
		uint32_t index;
		for(index = 0; index < mSize && mData[index] != a; index++)
			;
		return mData + index;
	}

	PX_INLINE ConstIterator find(const T& a) const
	{
		uint32_t index;
		for(index = 0; index < mSize && mData[index] != a; index++)
			;
		return mData + index;
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Adds one element to the end of the array. Operation is O(1).
	\param a
	The element that will be added to this array.
	*/
	/////////////////////////////////////////////////////////////////////////

	PX_FORCE_INLINE T& pushBack(const T& a)
	{
		if(capacity() <= mSize)
			return growAndPushBack(a);

		PX_PLACEMENT_NEW(reinterpret_cast<void*>(mData + mSize), T)(a);

		return mData[mSize++];
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Returns the element at the end of the array. Only legal if the array is non-empty.
	*/
	/////////////////////////////////////////////////////////////////////////
	PX_INLINE T popBack()
	{
		PX_ASSERT(mSize);
		T t = mData[mSize - 1];

		mData[--mSize].~T();

		return t;
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Construct one element at the end of the array. Operation is O(1).
	*/
	/////////////////////////////////////////////////////////////////////////
	PX_INLINE T& insert()
	{
		if(capacity() <= mSize)
			grow(capacityIncrement());

		T* ptr = mData + mSize++;
		PX_PLACEMENT_NEW(ptr, T); // not 'T()' because PODs should not get default-initialized.
		return *ptr;
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Subtracts the element on position i from the array and replace it with
	the last element.
	Operation is O(1)
	\param i
	The position of the element that will be subtracted from this array.
	*/
	/////////////////////////////////////////////////////////////////////////
	PX_INLINE void replaceWithLast(uint32_t i)
	{
		PX_ASSERT(i < mSize);
		mData[i] = mData[--mSize];

		mData[mSize].~T();
	}

	PX_INLINE void replaceWithLast(Iterator i)
	{
		replaceWithLast(static_cast<uint32_t>(i - mData));
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Replaces the first occurrence of the element a with the last element
	Operation is O(n)
	\param a
	The position of the element that will be subtracted from this array.
	\return true if the element has been removed.
	*/
	/////////////////////////////////////////////////////////////////////////

	PX_INLINE bool findAndReplaceWithLast(const T& a)
	{
		uint32_t index = 0;
		while(index < mSize && mData[index] != a)
			++index;
		if(index == mSize)
			return false;
		replaceWithLast(index);
		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Subtracts the element on position i from the array. Shift the entire
	array one step.
	Operation is O(n)
	\param i
	The position of the element that will be subtracted from this array.
	*/
	/////////////////////////////////////////////////////////////////////////
	PX_INLINE void remove(uint32_t i)
	{
		PX_ASSERT(i < mSize);

		T* it = mData + i;
		it->~T();
		while (++i < mSize)
		{								
			PX_PLACEMENT_NEW(it, T(mData[i]));
			++it;
			it->~T();
		} 
		--mSize;
	}

	/////////////////////////////////////////////////////////////////////////
	/*!
	Removes a range from the array.  Shifts the array so order is maintained.
	Operation is O(n)
	\param begin
	The starting position of the element that will be subtracted from this array.
	\param count
	The number of elments that will be subtracted from this array.
	*/
	/////////////////////////////////////////////////////////////////////////
	PX_INLINE void removeRange(uint32_t begin, uint32_t count)
	{
		PX_ASSERT(begin < mSize);
		PX_ASSERT((begin + count) <= mSize);

		for(uint32_t i = 0; i < count; i++)
			mData[begin + i].~T(); // call the destructor on the ones being removed first.

		T* dest = &mData[begin];                       // location we are copying the tail end objects to
		T* src = &mData[begin + count];                // start of tail objects
		uint32_t move_count = mSize - (begin + count); // compute remainder that needs to be copied down

		for(uint32_t i = 0; i < move_count; i++)
		{
			PX_PLACEMENT_NEW(dest, T(*src));	// copy the old one to the new location
			src->~T();							// call the destructor on the old location
			dest++;
			src++;
		}
		mSize -= count;
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Resize array
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_NOINLINE void resize(const uint32_t size, const T& a = T());

	PX_NOINLINE void resizeUninitialized(const uint32_t size);

	//////////////////////////////////////////////////////////////////////////
	/*!
	Resize array such that only as much memory is allocated to hold the
	existing elements
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void shrink()
	{
		recreate(mSize);
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Deletes all array elements and frees memory.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void reset()
	{
		resize(0);
		shrink();
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Resets or clears the array depending on occupancy.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void resetOrClear()
	{
		const PxU32 c = capacity();
		const PxU32 s = size();
		if(s>=c/2)
			clear();
		else
			reset();
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Ensure that the array has at least size capacity.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void reserve(const uint32_t capacity)
	{
		if(capacity > this->capacity())
			grow(capacity);
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Query the capacity(allocated mem) for the array.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_FORCE_INLINE uint32_t capacity() const
	{
		return mCapacity & ~PX_SIGN_BITMASK;
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Unsafe function to force the size of the array
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_FORCE_INLINE void forceSize_Unsafe(uint32_t size)
	{
		PX_ASSERT(size <= mCapacity);
		mSize = size;
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Swap contents of an array without allocating temporary storage
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void swap(PxArray<T, Alloc>& other)
	{
		PxSwap(mData, other.mData);
		PxSwap(mSize, other.mSize);
		PxSwap(mCapacity, other.mCapacity);
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Assign a range of values to this vector (resizes to length of range)
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void assign(const T* first, const T* last)
	{
		resizeUninitialized(uint32_t(last - first));
		copy(begin(), end(), first);
	}

	// We need one bit to mark arrays that have been deserialized from a user-provided memory block.
	// For alignment & memory saving purpose we store that bit in the rarely used capacity member.
	PX_FORCE_INLINE uint32_t isInUserMemory() const
	{
		return mCapacity & PX_SIGN_BITMASK;
	}

	/// return reference to allocator
	PX_INLINE Alloc& getAllocator()
	{
		return *this;
	}

  protected:
	// constructor for where we don't own the memory
	PxArray(T* memory, uint32_t size, uint32_t capacity, const Alloc& alloc = Alloc())
	: Alloc(alloc), mData(memory), mSize(size), mCapacity(capacity | PX_SIGN_BITMASK)
	{
	}

	template <class A>
	PX_NOINLINE void copy(const PxArray<T, A>& other);

	PX_INLINE T* allocate(uint32_t size)
	{
		if(size > 0)
		{
			T* p = reinterpret_cast<T*>(Alloc::allocate(sizeof(T) * size, __FILE__, __LINE__));
			PxMarkSerializedMemory(p, sizeof(T) * size);
			return p;
		}
		return 0;
	}

	PX_INLINE void deallocate(void* mem)
	{
		Alloc::deallocate(mem);
	}

	static PX_INLINE void create(T* first, T* last, const T& a)
	{
		for(; first < last; ++first)
			::PX_PLACEMENT_NEW(first, T(a));
	}

	static PX_INLINE void copy(T* first, T* last, const T* src)
	{
		if(last <= first)
			return;

		for(; first < last; ++first, ++src)
			::PX_PLACEMENT_NEW(first, T(*src));
	}

	static PX_INLINE void destroy(T* first, T* last)
	{
		for(; first < last; ++first)
			first->~T();
	}

	/*!
	Called when pushBack() needs to grow the array.
	\param a The element that will be added to this array.
	*/
	PX_NOINLINE T& growAndPushBack(const T& a);

	/*!
	Resizes the available memory for the array.

	\param capacity
	The number of entries that the set should be able to hold.
	*/
	PX_INLINE void grow(uint32_t capacity)
	{
		PX_ASSERT(this->capacity() < capacity);
		recreate(capacity);
	}

	/*!
	Creates a new memory block, copies all entries to the new block and destroys old entries.

	\param capacity
	The number of entries that the set should be able to hold.
	*/
	PX_NOINLINE void recreate(uint32_t capacity);

	// The idea here is to prevent accidental bugs with pushBack or insert. Unfortunately
	// it interacts badly with InlineArrays with smaller inline allocations.
	// TODO(dsequeira): policy template arg, this is exactly what they're for.
	PX_INLINE uint32_t capacityIncrement() const
	{
		const uint32_t capacity = this->capacity();
		return capacity == 0 ? 1 : capacity * 2;
	}

	T* mData;
	uint32_t mSize;
	uint32_t mCapacity;
};

template <class T, class Alloc>
PX_NOINLINE void PxArray<T, Alloc>::resize(const uint32_t size, const T& a)
{
	reserve(size);
	create(mData + mSize, mData + size, a);
	destroy(mData + size, mData + mSize);
	mSize = size;
}

template <class T, class Alloc>
template <class A>
PX_NOINLINE void PxArray<T, Alloc>::copy(const PxArray<T, A>& other)
{
	if(!other.empty())
	{
		mData = allocate(mSize = mCapacity = other.size());
		copy(mData, mData + mSize, other.begin());
	}
	else
	{
		mData = NULL;
		mSize = 0;
		mCapacity = 0;
	}

	// mData = allocate(other.mSize);
	// mSize = other.mSize;
	// mCapacity = other.mSize;
	// copy(mData, mData + mSize, other.mData);
}

template <class T, class Alloc>
PX_NOINLINE void PxArray<T, Alloc>::resizeUninitialized(const uint32_t size)
{
	reserve(size);
	mSize = size;
}

template <class T, class Alloc>
PX_NOINLINE T& PxArray<T, Alloc>::growAndPushBack(const T& a)
{
	uint32_t capacity = capacityIncrement();

	T* newData = allocate(capacity);
	PX_ASSERT((!capacity) || (newData && (newData != mData)));
	copy(newData, newData + mSize, mData);

	// inserting element before destroying old array
	// avoids referencing destroyed object when duplicating array element.
	PX_PLACEMENT_NEW(reinterpret_cast<void*>(newData + mSize), T)(a);

	destroy(mData, mData + mSize);
	if(!isInUserMemory())
		deallocate(mData);

	mData = newData;
	mCapacity = capacity;

	return mData[mSize++];
}

template <class T, class Alloc>
PX_NOINLINE void PxArray<T, Alloc>::recreate(uint32_t capacity)
{
	T* newData = allocate(capacity);
	PX_ASSERT((!capacity) || (newData && (newData != mData)));

	copy(newData, newData + mSize, mData);
	destroy(mData, mData + mSize);
	if(!isInUserMemory())
		deallocate(mData);

	mData = newData;
	mCapacity = capacity;
}

template <class T, class Alloc>
PX_INLINE void swap(PxArray<T, Alloc>& x, PxArray<T, Alloc>& y)
{
	x.swap(y);
}

} // namespace physx

#endif

