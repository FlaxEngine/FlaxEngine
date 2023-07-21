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

#ifndef PX_THREAD_H
#define PX_THREAD_H

#include "foundation/PxUserAllocated.h"

// todo: these need to go somewhere else
// PT: looks like this is still used on some platforms

#if PX_WINDOWS_FAMILY
#define PxSpinLockPause() __asm pause
#elif PX_LINUX || PX_ANDROID || PX_PS4 || PX_PS5 || PX_APPLE_FAMILY || PX_SWITCH
#define PxSpinLockPause() asm("nop")
#else
#error "Platform not supported!"
#endif

#if !PX_DOXYGEN
namespace physx
{
#endif

struct PxThreadPriority
{
	enum Enum
	{
		eHIGH         = 0,	//!< High priority
		eABOVE_NORMAL = 1,	//!< Above Normal priority
		eNORMAL       = 2,	//!< Normal/default priority
		eBELOW_NORMAL = 3,	//!< Below Normal priority
		eLOW          = 4,	//!< Low priority.
		eFORCE_DWORD  = 0xffFFffFF
	};
};

class PxRunnable
{
  public:
			PxRunnable()	{}
	virtual ~PxRunnable()	{}
	virtual void execute()	{}
};

class PX_FOUNDATION_API PxThreadImpl
{
  public:
	typedef size_t Id; // space for a pointer or an integer
	typedef void* (*ExecuteFn)(void*);

	static PxU32 getDefaultStackSize();
	static Id getId();

	/**
	Construct (but do not start) the thread object. The OS thread object will not be created
	until start() is called. Executes in the context
	of the spawning thread.
	*/

	PxThreadImpl();

	/**
	Construct and start the the thread, passing the given arg to the given fn. (pthread style)
	*/

	PxThreadImpl(ExecuteFn fn, void* arg, const char* name);

	/**
	Deallocate all resources associated with the thread. Should be called in the
	context of the spawning thread.
	*/

	~PxThreadImpl();

	/**
	Create the OS thread and start it running. Called in the context of the spawning thread.
	If an affinity mask has previously been set then it will be applied after the
	thread has been created.
	*/

	void start(PxU32 stackSize, PxRunnable* r);

	/**
	Violently kill the current thread. Blunt instrument, not recommended since
	it can leave all kinds of things unreleased (stack, memory, mutexes...) Should
	be called in the context of the spawning thread.
	*/

	void kill();

	/**
	Stop the thread. Signals the spawned thread that it should stop, so the
	thread should check regularly
	*/

	void signalQuit();

	/**
	Wait for a thread to stop. Should be called in the context of the spawning
	thread. Returns false if the thread has not been started.
	*/

	bool waitForQuit();

	/**
	check whether the thread is signalled to quit. Called in the context of the
	spawned thread.
	*/

	bool quitIsSignalled();

	/**
	Cleanly shut down this thread. Called in the context of the spawned thread.
	*/
	void quit();

	/**
	Change the affinity mask for this thread. The mask is a platform
	specific value.

	On Windows, Linux, PS4, PS5, and Switch platforms, each set mask bit represents
	the index of a logical processor that the OS may schedule thread execution on.
	Bits outside the range of valid logical processors may be ignored or cause
	the function to return an error.

	On Apple platforms, this function has no effect.

	If the thread has not yet been started then the mask is stored
	and applied when the thread is started.

	If the thread has already been started then this method	returns the
	previous affinity mask on success, otherwise it returns zero.
	*/
	PxU32 setAffinityMask(PxU32 mask);

	static PxThreadPriority::Enum getPriority(Id threadId);

	/** Set thread priority. */
	void setPriority(PxThreadPriority::Enum prio);

	/** set the thread's name */
	void setName(const char* name);

	/** Put the current thread to sleep for the given number of milliseconds */
	static void sleep(PxU32 ms);

	/** Yield the current thread's slot on the CPU */
	static void yield();

	/** Inform the processor that we're in a busy wait to give it a chance to do something clever.
	yield() yields the thread, while yieldProcessor() aims to yield the processor */
	static void yieldProcessor();

	/** Return the number of physical cores (does not include hyper-threaded cores), returns 0 on failure */
	static PxU32 getNbPhysicalCores();

	/**
	Size of this class.
	*/
	static PxU32 getSize();
};

/**
Thread abstraction API
*/
template <typename Alloc = PxReflectionAllocator<PxThreadImpl> >
class PxThreadT : protected Alloc, public PxUserAllocated, public PxRunnable
{
  public:
	typedef PxThreadImpl::Id Id; // space for a pointer or an integer

	/**
	Construct (but do not start) the thread object. Executes in the context
	of the spawning thread
	*/
	PxThreadT(const Alloc& alloc = Alloc()) : Alloc(alloc)
	{
		mImpl = reinterpret_cast<PxThreadImpl*>(Alloc::allocate(PxThreadImpl::getSize(), __FILE__, __LINE__));
		PX_PLACEMENT_NEW(mImpl, PxThreadImpl)();
	}

	/**
	Construct and start the the thread, passing the given arg to the given fn. (pthread style)
	*/
	PxThreadT(PxThreadImpl::ExecuteFn fn, void* arg, const char* name, const Alloc& alloc = Alloc()) : Alloc(alloc)
	{
		mImpl = reinterpret_cast<PxThreadImpl*>(Alloc::allocate(PxThreadImpl::getSize(), __FILE__, __LINE__));
		PX_PLACEMENT_NEW(mImpl, PxThreadImpl)(fn, arg, name);
	}

	/**
	Deallocate all resources associated with the thread. Should be called in the
	context of the spawning thread.
	*/
	virtual ~PxThreadT()
	{
		mImpl->~PxThreadImpl();
		Alloc::deallocate(mImpl);
	}

	/**
	start the thread running. Called in the context of the spawning thread.
	*/

	void start(PxU32 stackSize = PxThreadImpl::getDefaultStackSize())
	{
		mImpl->start(stackSize, this);
	}

	/**
	Violently kill the current thread. Blunt instrument, not recommended since
	it can leave all kinds of things unreleased (stack, memory, mutexes...) Should
	be called in the context of the spawning thread.
	*/

	void kill()
	{
		mImpl->kill();
	}

	/**
	The virtual execute() method is the user defined function that will
	run in the new thread. Called in the context of the spawned thread.
	*/

	virtual void execute(void)
	{
	}

	/**
	stop the thread. Signals the spawned thread that it should stop, so the
	thread should check regularly
	*/

	void signalQuit()
	{
		mImpl->signalQuit();
	}

	/**
	Wait for a thread to stop. Should be called in the context of the spawning
	thread. Returns false if the thread has not been started.
	*/

	bool waitForQuit()
	{
		return mImpl->waitForQuit();
	}

	/**
	check whether the thread is signalled to quit. Called in the context of the
	spawned thread.
	*/

	bool quitIsSignalled()
	{
		return mImpl->quitIsSignalled();
	}

	/**
	Cleanly shut down this thread. Called in the context of the spawned thread.
	*/
	void quit()
	{
		mImpl->quit();
	}

	PxU32 setAffinityMask(PxU32 mask)
	{
		return mImpl->setAffinityMask(mask);
	}

	static PxThreadPriority::Enum getPriority(PxThreadImpl::Id threadId)
	{
		return PxThreadImpl::getPriority(threadId);
	}

	/** Set thread priority. */
	void setPriority(PxThreadPriority::Enum prio)
	{
		mImpl->setPriority(prio);
	}

	/** set the thread's name */
	void setName(const char* name)
	{
		mImpl->setName(name);
	}

	/** Put the current thread to sleep for the given number of milliseconds */
	static void sleep(PxU32 ms)
	{
		PxThreadImpl::sleep(ms);
	}

	/** Yield the current thread's slot on the CPU */
	static void yield()
	{
		PxThreadImpl::yield();
	}

	/** Inform the processor that we're in a busy wait to give it a chance to do something clever 
		yield() yields the thread, while yieldProcessor() aims to yield the processor */
	static void yieldProcesor()
	{
		PxThreadImpl::yieldProcessor();
	}

	static PxU32 getDefaultStackSize()
	{
		return PxThreadImpl::getDefaultStackSize();
	}

	static PxThreadImpl::Id getId()
	{
		return PxThreadImpl::getId();
	}

	static PxU32 getNbPhysicalCores()
	{
		return PxThreadImpl::getNbPhysicalCores();
	}

  private:
	class PxThreadImpl* mImpl;
};

typedef PxThreadT<> PxThread;

PX_FOUNDATION_API PxU32 PxTlsAlloc();
PX_FOUNDATION_API void PxTlsFree(PxU32 index);
PX_FOUNDATION_API void* PxTlsGet(PxU32 index);
PX_FOUNDATION_API size_t PxTlsGetValue(PxU32 index);
PX_FOUNDATION_API PxU32 PxTlsSet(PxU32 index, void* value);
PX_FOUNDATION_API PxU32 PxTlsSetValue(PxU32 index, size_t value);


#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

