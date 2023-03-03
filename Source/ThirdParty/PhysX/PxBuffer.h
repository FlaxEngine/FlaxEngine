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

#ifndef PX_BUFFER_H
#define PX_BUFFER_H
/** \addtogroup physics
@{ */

#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4435)
#endif

class PxCudaContextManager;

/**
\brief Specifies memory space for a PxBuffer instance.

@see PxBuffer
*/
struct PxBufferType
{
	enum Enum
	{
		eHOST,
		eDEVICE
	};
};

/**
\brief Buffer for delayed bulk read and write operations supporting host and GPU device memory spaces.

@see PxPhysics::createBuffer(), PxParticleSystem
*/
class PxBuffer
{
public:

	/**
	\brief Deletes the buffer.

	Do not keep a reference to the deleted instance.
	Unfinished operations will be flushed and synchronized on.
	*/
	virtual		void					release() = 0;

	/**
	\brief Provides access to internal memory (either device or pinned host memory depending on PxBufferType).

	Unfinished operations will be flushed and synchronized on before returning.
	*/
	virtual		void*					map() = 0;

	/**
	\brief Releases access to internal memory (either device or pinned host memory depending on PxBufferType).

	\param[in] event Optional pointer to CUevent. Used to synchronize on application side work that needs to be completed before 
	buffer can be accessed again.
	*/
	virtual		void					unmap(void* event = NULL) = 0;

	/**
	\brief Buffer memory space type.
	
	@see PxBufferType
	*/
	virtual		PxBufferType::Enum		getBufferType() const = 0;

	/**
	\brief Size of buffer in bytes.
	*/
	virtual		PxU64					getByteSize() const = 0;

	/**
	\brief Get the associated PxCudaContextManager.

	@see PxCudaContextManager.
	*/
	virtual		PxCudaContextManager*	getCudaContextManager() const = 0;

	/**
	\brief Helper function to synchronize on all pending operations.

	@see PxCudaContextManager.
	*/
	PX_INLINE	void					sync() { map(); unmap(); }

	virtual		void					resize(PxU64 size) = 0;

protected:

	virtual								~PxBuffer() {}
	PX_INLINE							PxBuffer() {}
};

#if PX_VC
#pragma warning(pop)
#endif


#if !PX_DOXYGEN
} // namespace physx
#endif

  /** @} */
#endif
