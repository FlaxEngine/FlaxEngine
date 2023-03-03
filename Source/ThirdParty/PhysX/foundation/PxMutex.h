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

#ifndef PX_MUTEX_H
#define PX_MUTEX_H

#include "foundation/PxAllocator.h"

/*
 * This <new> inclusion is a best known fix for gcc 4.4.1 error:
 * Creating object file for apex/src/PsAllocator.cpp ...
 * In file included from apex/include/PsFoundation.h:30,
 *                from apex/src/PsAllocator.cpp:26:
 * apex/include/PsMutex.h: In constructor  'physx::PxMutexT<Alloc>::MutexT(const Alloc&)':
 * apex/include/PsMutex.h:92: error: no matching function for call to 'operator new(unsigned int,
 * physx::PxMutexImpl*&)'
 * <built-in>:0: note: candidates are: void* operator new(unsigned int)
 */
#include <new>

#if !PX_DOXYGEN
namespace physx
{
#endif
class PX_FOUNDATION_API PxMutexImpl
{
  public:
	/**
	The constructor for Mutex creates a mutex. It is initially unlocked.
	*/
	PxMutexImpl();

	/**
	The destructor for Mutex deletes the mutex.
	*/
	~PxMutexImpl();

	/**
	Acquire (lock) the mutex. If the mutex is already locked
	by another thread, this method blocks until the mutex is
	unlocked.
	*/
	void lock();

	/**
	Acquire (lock) the mutex. If the mutex is already locked
	by another thread, this method returns false without blocking.
	*/
	bool trylock();

	/**
	Release (unlock) the mutex.
	*/
	void unlock();

	/**
	Size of this class.
	*/
	static uint32_t getSize();
};

template <typename Alloc = PxReflectionAllocator<PxMutexImpl> >
class PxMutexT : protected Alloc
{
	PX_NOCOPY(PxMutexT)
  public:
	class ScopedLock
	{
		PxMutexT<Alloc>& mMutex;
		PX_NOCOPY(ScopedLock)
	  public:
		PX_INLINE ScopedLock(PxMutexT<Alloc>& mutex) : mMutex(mutex)
		{
			mMutex.lock();
		}
		PX_INLINE ~ScopedLock()
		{
			mMutex.unlock();
		}
	};

	/**
	The constructor for Mutex creates a mutex. It is initially unlocked.
	*/
	PxMutexT(const Alloc& alloc = Alloc()) : Alloc(alloc)
	{
		mImpl = reinterpret_cast<PxMutexImpl*>(Alloc::allocate(PxMutexImpl::getSize(), __FILE__, __LINE__));
		PX_PLACEMENT_NEW(mImpl, PxMutexImpl)();
	}

	/**
	The destructor for Mutex deletes the mutex.
	*/
	~PxMutexT()
	{
		mImpl->~PxMutexImpl();
		Alloc::deallocate(mImpl);
	}

	/**
	Acquire (lock) the mutex. If the mutex is already locked
	by another thread, this method blocks until the mutex is
	unlocked.
	*/
	PX_FORCE_INLINE	void lock() const
	{
		mImpl->lock();
	}

	/**
	Acquire (lock) the mutex. If the mutex is already locked
	by another thread, this method returns false without blocking,
	returns true if lock is successfully acquired
	*/
	PX_FORCE_INLINE	bool trylock() const
	{
		return mImpl->trylock();
	}

	/**
	Release (unlock) the mutex, the calling thread must have
	previously called lock() or method will error
	*/
	PX_FORCE_INLINE	void unlock() const
	{
		mImpl->unlock();
	}

  private:
	PxMutexImpl* mImpl;
};

class PX_FOUNDATION_API PxReadWriteLock
{
	PX_NOCOPY(PxReadWriteLock)
  public:
	PxReadWriteLock();
	~PxReadWriteLock();

	// "takeLock" can only be false if the thread already holds the mutex, e.g. if it already acquired the write lock
	void lockReader(bool takeLock);
	void lockWriter();

	void unlockReader();
	void unlockWriter();

  private:
	class ReadWriteLockImpl* mImpl;
};

typedef PxMutexT<> PxMutex;

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

