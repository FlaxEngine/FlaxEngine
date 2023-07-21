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

#ifndef PX_ERRORS_H
#define PX_ERRORS_H
/** \addtogroup foundation
@{
*/

#include "foundation/Px.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Error codes

These error codes are passed to #PxErrorCallback

@see PxErrorCallback
*/
struct PxErrorCode
{
	enum Enum
	{
		eNO_ERROR          = 0,

		//! \brief An informational message.
		eDEBUG_INFO        = 1,

		//! \brief a warning message for the user to help with debugging
		eDEBUG_WARNING     = 2,

		//! \brief method called with invalid parameter(s)
		eINVALID_PARAMETER = 4,

		//! \brief method was called at a time when an operation is not possible
		eINVALID_OPERATION = 8,

		//! \brief method failed to allocate some memory
		eOUT_OF_MEMORY     = 16,

		/** \brief The library failed for some reason.
	    Possibly you have passed invalid values like NaNs, which are not checked for.
	    */
		eINTERNAL_ERROR    = 32,

		//! \brief An unrecoverable error, execution should be halted and log output flushed
		eABORT             = 64,

		//! \brief The SDK has determined that an operation may result in poor performance.
		ePERF_WARNING      = 128,

		//! \brief A bit mask for including all errors
		eMASK_ALL          = -1
	};
};

#if PX_CHECKED
	#define PX_CHECK_MSG(exp, msg)					(!!(exp) || (PxGetFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, PX_FL, msg), 0) )
	#define PX_CHECK_AND_RETURN(exp, msg)			{ if(!(exp)) { PxGetFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, PX_FL, msg); return; } }
	#define PX_CHECK_AND_RETURN_NULL(exp, msg)		{ if(!(exp)) { PxGetFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, PX_FL, msg); return 0; } }
	#define PX_CHECK_AND_RETURN_VAL(exp, msg, r)	{ if(!(exp)) { PxGetFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, PX_FL, msg); return r; } }
#else
	#define PX_CHECK_MSG(exp, msg)
	#define PX_CHECK_AND_RETURN(exp, msg)
	#define PX_CHECK_AND_RETURN_NULL(exp, msg)
	#define PX_CHECK_AND_RETURN_VAL(exp, msg, r)
#endif

// shortcut macros:
// usage: PxGetFoundation().error(PX_WARN, "static friction %f is is lower than dynamic friction %d", sfr, dfr);
#define PX_WARN ::physx::PxErrorCode::eDEBUG_WARNING, PX_FL
#define PX_INFO ::physx::PxErrorCode::eDEBUG_INFO, PX_FL

#if PX_DEBUG || PX_CHECKED
	#define PX_WARN_ONCE(string)							\
		{													\
			static PxU32 timestamp = 0;						\
			const PxU32 ts = PxGetWarnOnceTimeStamp();		\
			if(timestamp != ts)								\
			{                                               \
				timestamp = ts;								\
				PxGetFoundation().error(PX_WARN, string);   \
			}                                               \
		}
	#define PX_WARN_ONCE_IF(condition, string)	\
		{                                       \
			if(condition)                       \
			{                                   \
				PX_WARN_ONCE(string)            \
			}                                   \
		}
#else
	#define PX_WARN_ONCE(string) ((void)0)
	#define PX_WARN_ONCE_IF(condition, string) ((void)0)
#endif

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif

