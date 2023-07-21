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

#ifndef PX_OMNI_PVD_H
#define PX_OMNI_PVD_H

#include "PxPhysXConfig.h"

class OmniPvdWriter;
class OmniPvdFileWriteStream;

#if !PX_DOXYGEN
namespace physx
{
#endif

class PxFoundation;

class PxOmniPvd
{
public:
	virtual ~PxOmniPvd()
	{
	}
	/**
	\brief Gets an instance of the OmniPvd writer

	\return OmniPvdWriter instance on succes, NULL otherwise.
	*/
	virtual OmniPvdWriter* getWriter() = 0;
	
	/**
	\brief Gets an instance to the OmniPvd file write stream
	
	\return OmniPvdFileWriteStream instance on succes, NULL otherwise.
	*/
	virtual OmniPvdFileWriteStream* getFileWriteStream() = 0;
	
	/**
	\brief Starts the OmniPvd sampling

	\return True if sampling started correctly, false if not.
	*/
	virtual bool startSampling() = 0;

	/**
	\brief Releases the PxOmniPvd object

	*/
	virtual void release() = 0;
};
#if !PX_DOXYGEN
} // namespace physx
#endif
/**
\brief Creates an instance of the OmniPvd object

Creates an instance of the OmniPvd class. There may be only one instance of this class per process. Calling this method after an instance
has been created already will return the same instance over and over.

\param foundation Foundation instance (see PxFoundation)

\return PxOmniPvd instance on succes, NULL otherwise.

*/
PX_C_EXPORT PX_PHYSX_CORE_API physx::PxOmniPvd* PX_CALL_CONV PxCreateOmniPvd(physx::PxFoundation& foundation);


#endif
