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

#ifndef PX_ALLOCATOR_H
#define PX_ALLOCATOR_H

#include "foundation/PxAllocatorCallback.h"
#include "foundation/PxAssert.h"
#include "foundation/PxFoundation.h"
#include "foundation/Px.h"

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4577)
#endif

#if PX_WINDOWS_FAMILY
	#include <exception>
#if(_MSC_VER >= 1923)
	#include <typeinfo>
#else
	#include <typeinfo.h>
#endif
#endif
#if(PX_APPLE_FAMILY)
	#include <typeinfo>
#endif

#include <new>

#if PX_VC
#pragma warning(pop)
#endif


// PT: the rules are simple:
// - PX_ALLOC/PX_ALLOCATE/PX_FREE is similar to malloc/free. Use that for POD/anything that doesn't need ctor/dtor.
// - PX_NEW/PX_DELETE is similar to new/delete. Use that for anything that needs a ctor/dtor.
// - Everything goes through the user allocator.
// - Inherit from PxUserAllocated to PX_NEW something. Do it even on small classes, it's free.
// - You cannot PX_NEW a POD. Use PX_ALLOC.

#define PX_ALLOC(n, name) physx::PxAllocator().allocate(n, __FILE__, __LINE__)

// PT: use this one to reduce the amount of visible reinterpret_cast
#define PX_ALLOCATE(type, count, name)	reinterpret_cast<type*>(PX_ALLOC(count*sizeof(type), name))

#define PX_FREE(x)							\
	if(x)									\
	{										\
		physx::PxAllocator().deallocate(x);	\
		x = NULL;							\
	}

#define PX_FREE_THIS	physx::PxAllocator().deallocate(this)

#define PX_NEW(T)				new (physx::PxReflectionAllocator<T>(), __FILE__, __LINE__) T
#define PX_PLACEMENT_NEW(p, T)	new (p) T
#define PX_DELETE_THIS			delete this
#define PX_DELETE(x)			if(x)	{ delete x;		x = NULL;	}
#define PX_DELETE_ARRAY(x)		if(x)	{ delete []x;	x = NULL;	}
#define PX_RELEASE(x)			if(x)	{ x->release(); x = NULL;	}

#if !PX_DOXYGEN
namespace physx
{
#endif
	/**
	Allocator used to access the global PxAllocatorCallback instance without providing additional information.
	*/
	class PxAllocator
	{
	public:
		PX_FORCE_INLINE	PxAllocator(const char* = NULL){}

		PX_FORCE_INLINE	void*	allocate(size_t size, const char* file, int line)
		{
			return size ? PxGetBroadcastAllocator()->allocate(size, "", file, line) : NULL;
		}

		PX_FORCE_INLINE	void	deallocate(void* ptr)
		{
			if(ptr)
				PxGetBroadcastAllocator()->deallocate(ptr);
		}
	};

	/*
	* Bootstrap allocator using malloc/free.
	* Don't use unless your objects get allocated before foundation is initialized.
	*/
	class PxRawAllocator
	{
	public:
		PxRawAllocator(const char* = 0)	{}

		PX_FORCE_INLINE	void*	allocate(size_t size, const char*, int)
		{
			// malloc returns valid pointer for size==0, no need to check
			return ::malloc(size);
		}

		PX_FORCE_INLINE	void	deallocate(void* ptr)
		{
			// free(0) is guaranteed to have no side effect, no need to check
			::free(ptr);
		}
	};

	/*
	\brief	Virtual allocator callback used to provide run-time defined allocators to foundation types like Array or Bitmap.
	This is used by VirtualAllocator
	*/
	class PxVirtualAllocatorCallback
	{
	public:
		PxVirtualAllocatorCallback()			{}
		virtual ~PxVirtualAllocatorCallback()	{}

		virtual void*	allocate(const size_t size, const int group, const char* file, const int line) = 0;
		virtual void	deallocate(void* ptr) = 0;
	};

	/*
	\brief Virtual allocator to be used by foundation types to provide run-time defined allocators.
	Due to the fact that Array extends its allocator, rather than contains a reference/pointer to it, the VirtualAllocator
	must
	be a concrete type containing a pointer to a virtual callback. The callback may not be available at instantiation time,
	therefore
	methods are provided to set the callback later.
	*/
	class PxVirtualAllocator
	{
	public:
		PxVirtualAllocator(PxVirtualAllocatorCallback* callback = NULL, const int group = 0) : mCallback(callback), mGroup(group)	{}

		PX_FORCE_INLINE	void* allocate(const size_t size, const char* file, const int line)
		{
			PX_ASSERT(mCallback);
			if (size)
				return mCallback->allocate(size, mGroup, file, line);
			return NULL;
		}

		PX_FORCE_INLINE	void deallocate(void* ptr)
		{
			PX_ASSERT(mCallback);
			if (ptr)
				mCallback->deallocate(ptr);
		}

	private:
		PxVirtualAllocatorCallback* mCallback;
		const int mGroup;
		PxVirtualAllocator& operator=(const PxVirtualAllocator&);
	};

	/**
	Allocator used to access the global PxAllocatorCallback instance using a static name derived from T.
	*/
	template <typename T>
	class PxReflectionAllocator
	{
		static const char* getName()
		{
			if (!PxGetFoundation().getReportAllocationNames())
				return "<allocation names disabled>";
#if PX_GCC_FAMILY
			return __PRETTY_FUNCTION__;
#else
			// name() calls malloc(), raw_name() wouldn't
			return typeid(T).name();
#endif
		}

	public:
		PxReflectionAllocator(const PxEMPTY)	{}
		PxReflectionAllocator(const char* = 0)	{}

		inline PxReflectionAllocator(const PxReflectionAllocator&)	{}

		PX_FORCE_INLINE	void*	allocate(size_t size, const char* filename, int line)
		{
			return size ? PxGetBroadcastAllocator()->allocate(size, getName(), filename, line) : NULL;
		}

		PX_FORCE_INLINE	void	deallocate(void* ptr)
		{
			if (ptr)
				PxGetBroadcastAllocator()->deallocate(ptr);
		}
	};

	template <typename T>
	struct PxAllocatorTraits
	{
		typedef PxReflectionAllocator<T> Type;
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif

