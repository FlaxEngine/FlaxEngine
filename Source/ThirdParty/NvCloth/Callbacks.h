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

/** \file Callbacks.h
	\brief All functions to initialize and use user provided callbacks are declared in this header.
	Initialize the callbacks with InitializeNvCloth(...) before using any other NvCloth API.
	The other functions defined in this header are used to access the functionality provided by the callbacks, and are mostly for internal use.
  */

#pragma once
#include <foundation/PxPreprocessor.h>
#include <foundation/PxProfiler.h>
#include <foundation/PxAllocatorCallback.h>
#ifndef NV_CLOTH_IMPORT
#define NV_CLOTH_IMPORT PX_DLL_IMPORT
#endif

#define NV_CLOTH_DLL_ID 0x2

#define NV_CLOTH_LINKAGE PX_C_EXPORT NV_CLOTH_IMPORT
#define NV_CLOTH_CALL_CONV PX_CALL_CONV
#define NV_CLOTH_API(ret_type) NV_CLOTH_LINKAGE ret_type NV_CLOTH_CALL_CONV



/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
	class PxAllocatorCallback;
	class PxErrorCallback;
	class PxProfilerCallback;
	class PxAssertHandler;

/** \brief Initialize the library by passing in callback functions.
	This needs to be called before using any other part of the library.
	@param allocatorCallback Callback interface for memory allocations. Needs to return 16 byte aligned memory.
	@param errorCallback Callback interface for debug/warning/error messages.
	@param assertHandler Callback interface for asserts.
	@param profilerCallback Optional callback interface for performance information.
	@param autoDllIDCheck Leave as default parameter. This is used to check header and dll version compatibility.
	*/
NV_CLOTH_API(void)
	InitializeNvCloth(physx::PxAllocatorCallback* allocatorCallback, physx::PxErrorCallback* errorCallback, 
						nv::cloth::PxAssertHandler* assertHandler, physx::PxProfilerCallback* profilerCallback,
						int autoDllIDCheck = NV_CLOTH_DLL_ID);
}
}

//Allocator
NV_CLOTH_API(physx::PxAllocatorCallback*) GetNvClothAllocator(); //Only use internally

namespace nv
{
namespace cloth
{

/* Base class to handle assert failures */
class PxAssertHandler
{
public:
	virtual ~PxAssertHandler()
	{
	}
	virtual void operator()(const char* exp, const char* file, int line, bool& ignore) = 0;
};


//Logging
void LogErrorFn  (const char* fileName, int lineNumber, const char* msg, ...);
void LogInvalidParameterFn  (const char* fileName, int lineNumber, const char* msg, ...);
void LogWarningFn(const char* fileName, int lineNumber, const char* msg, ...);
void LogInfoFn   (const char* fileName, int lineNumber, const char* msg, ...);
///arguments: NV_CLOTH_LOG_ERROR("format %s %s\n","additional","arguments");
#define NV_CLOTH_LOG_ERROR(...) nv::cloth::LogErrorFn(__FILE__,__LINE__,__VA_ARGS__)
#define NV_CLOTH_LOG_INVALID_PARAMETER(...) nv::cloth::LogInvalidParameterFn(__FILE__,__LINE__,__VA_ARGS__)
#define NV_CLOTH_LOG_WARNING(...) nv::cloth::LogWarningFn(__FILE__,__LINE__,__VA_ARGS__)
#define NV_CLOTH_LOG_INFO(...) nv::cloth::LogInfoFn(__FILE__,__LINE__,__VA_ARGS__)

//ASSERT
NV_CLOTH_API(nv::cloth::PxAssertHandler*) GetNvClothAssertHandler(); //This function needs to be exposed to properly inline asserts outside this dll
#if !PX_ENABLE_ASSERTS
#if PX_VC
#define NV_CLOTH_ASSERT(exp) __noop
#define NV_CLOTH_ASSERT_WITH_MESSAGE(message, exp) __noop
#else
#define NV_CLOTH_ASSERT(exp) ((void)0)
#define NV_CLOTH_ASSERT_WITH_MESSAGE(message, exp) ((void)0)
#endif
#else
#if PX_VC
#define PX_CODE_ANALYSIS_ASSUME(exp)                                                                                   \
	__analysis_assume(!!(exp)) // This macro will be used to get rid of analysis warning messages if a NV_CLOTH_ASSERT is used
																 // to "guard" illegal mem access, for example.
#else
#define PX_CODE_ANALYSIS_ASSUME(exp)
#endif
#define NV_CLOTH_ASSERT(exp)                                                                                           \
	{                                                                                                                  \
		static bool _ignore = false;                                                                                   \
		((void)((!!(exp)) || (!_ignore && ((*nv::cloth::GetNvClothAssertHandler())(#exp, __FILE__, __LINE__, _ignore), false))));    \
		PX_CODE_ANALYSIS_ASSUME(exp);                                                                                  \
	}
#define NV_CLOTH_ASSERT_WITH_MESSAGE(message, exp)                                                                             \
	{                                                                                                                    \
		static bool _ignore = false;                                                                                     \
		((void)((!!(exp)) || (!_ignore && ((*nv::cloth::GetNvClothAssertHandler())(message, __FILE__, __LINE__, _ignore), false)))); \
		PX_CODE_ANALYSIS_ASSUME(exp);                                                                                    \
	}
#endif

//Profiler
physx::PxProfilerCallback* GetNvClothProfiler(); //Only use internally
#if PX_DEBUG || PX_CHECKED || PX_PROFILE
class NvClothProfileScoped
{
  public:
	PX_FORCE_INLINE NvClothProfileScoped(const char* eventName, bool detached, uint64_t contextId,
	                                const char* fileName, int lineno, physx::PxProfilerCallback* callback)
	: mCallback(callback)
	{
		PX_UNUSED(fileName);
		PX_UNUSED(lineno);
		mProfilerData = NULL; //nullptr doesn't work here on mac
		if (mCallback)
		{
			mEventName = eventName;
			mDetached = detached;
			mContextId = contextId;
			mProfilerData = mCallback->zoneStart(mEventName, mDetached, mContextId);
		}
	}
	~NvClothProfileScoped(void)
	{
		if (mCallback)
		{
			mCallback->zoneEnd(mProfilerData, mEventName, mDetached, mContextId);
		}
	}
	physx::PxProfilerCallback* mCallback;
	const char* mEventName;
	bool mDetached;
	uint64_t mContextId;
	void* mProfilerData;
};

#define NV_CLOTH_PROFILE_ZONE(x, y)                                                                                          \
	nv::cloth::NvClothProfileScoped PX_CONCAT(_scoped, __LINE__)(x, false, y, __FILE__, __LINE__, nv::cloth::GetNvClothProfiler())
#define NV_CLOTH_PROFILE_START_CROSSTHREAD(x, y)                                                                             \
	(GetNvClothProfiler()!=nullptr?																					       \
	GetNvClothProfiler()->zoneStart(x, true, y):nullptr)
#define NV_CLOTH_PROFILE_STOP_CROSSTHREAD(profilerData, x, y)                                                                              \
	if (GetNvClothProfiler())                                                                                           \
	GetNvClothProfiler()->zoneEnd(profilerData, x, true, y)
#else

#define NV_CLOTH_PROFILE_ZONE(x, y)
#define NV_CLOTH_PROFILE_START_CROSSTHREAD(x, y) nullptr
#define NV_CLOTH_PROFILE_STOP_CROSSTHREAD(profilerData, x, y)

#endif

}
}
