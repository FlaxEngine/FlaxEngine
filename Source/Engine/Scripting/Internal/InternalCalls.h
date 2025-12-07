// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/Types.h"

#if defined(__clang__)
// Helper utility to override vtable entry with automatic restore
// See BindingsGenerator.Cpp.cs that generates virtual method wrappers for scripting to properly call overriden base method
struct FLAXENGINE_API VTableFunctionInjector
{
    void** VTableAddr;
    void* OriginalValue;

    VTableFunctionInjector(void* object, void* originalFunc, void* func)
    {
        void** vtable = *(void***)object;
        const int32 vtableIndex = GetVTableIndex(vtable, 200, originalFunc);
        VTableAddr = vtable + vtableIndex;
        OriginalValue = *VTableAddr;
        *VTableAddr = func;
    }

    ~VTableFunctionInjector()
    {
        *VTableAddr = OriginalValue;
    }
};
#elif defined(_MSC_VER)
#define MSVC_FUNC_EXPORT(name) __pragma(comment(linker, "/EXPORT:" #name "=" __FUNCDNAME__))
#endif

#if USE_CSHARP

#if USE_NETCORE
#define ADD_INTERNAL_CALL(fullName, method)
#define DEFINE_INTERNAL_CALL(returnType) extern "C" DLLEXPORT returnType
#else
extern "C" FLAXENGINE_API void mono_add_internal_call(const char* name, const void* method);
#define ADD_INTERNAL_CALL(fullName, method) mono_add_internal_call(fullName, (const void*)method)
#define DEFINE_INTERNAL_CALL(returnType) static returnType
#endif

#if BUILD_RELEASE && 0

// Using invalid handle will crash engine in Release build
#define INTERNAL_CALL_CHECK(obj)
#define INTERNAL_CALL_CHECK_EXP(expression)
#define INTERNAL_CALL_CHECK_RETURN(obj, defaultValue)
#define INTERNAL_CALL_CHECK_EXP_RETURN(expression, defaultValue)

#else

// Use additional checks in debug/development builds
#define INTERNAL_CALL_CHECK(obj) \
	if (obj == nullptr) \
	{ \
		DebugLog::ThrowNullReference(); \
		return; \
	}
#define INTERNAL_CALL_CHECK_EXP(expression) \
	if (expression) \
	{ \
		DebugLog::ThrowNullReference(); \
		return; \
	}
#define INTERNAL_CALL_CHECK_RETURN(obj, defaultValue) \
	if (obj == nullptr) \
	{ \
		DebugLog::ThrowNullReference(); \
		return defaultValue; \
	}
#define INTERNAL_CALL_CHECK_EXP_RETURN(expression, defaultValue) \
	if (expression) \
	{ \
		DebugLog::ThrowNullReference(); \
		return defaultValue; \
	}

#endif

#else

#define ADD_INTERNAL_CALL(fullName, method)
#define DEFINE_INTERNAL_CALL(returnType) static returnType
#define INTERNAL_CALL_CHECK(obj)
#define INTERNAL_CALL_CHECK_EXP(expression)
#define INTERNAL_CALL_CHECK_RETURN(obj, defaultValue)
#define INTERNAL_CALL_CHECK_EXP_RETURN(expression, defaultValue)

#endif

template<typename T>
T& InternalGetReference(T* obj)
{
    if (!obj)
        DebugLog::ThrowNullReference();
    return *obj;
}

#ifdef USE_MONO_AOT_COOP

// Cooperative Suspend - where threads suspend themselves when the runtime requests it.
// https://www.mono-project.com/docs/advanced/runtime/docs/coop-suspend/
typedef struct _MonoStackData {
    void* stackpointer;
    const char* function_name;
} MonoStackData;
#if BUILD_DEBUG
#define MONO_STACKDATA(x) MonoStackData x = { &x, __func__ }
#else
#define MONO_STACKDATA(x) MonoStackData x = { &x, NULL }
#endif
#define MONO_THREAD_INFO_TYPE struct MonoThreadInfo
DLLIMPORT extern "C" MONO_THREAD_INFO_TYPE* mono_thread_info_attach(void);
DLLIMPORT extern "C" void* mono_threads_enter_gc_safe_region_with_info(MONO_THREAD_INFO_TYPE* info, MonoStackData* stackdata);
DLLIMPORT extern "C" void mono_threads_exit_gc_safe_region_internal(void* cookie, MonoStackData* stackdata);
#ifndef _MONO_UTILS_FORWARD_
typedef struct _MonoDomain MonoDomain;
DLLIMPORT extern "C" MonoDomain* mono_domain_get(void);
#endif
#define MONO_ENTER_GC_SAFE	\
	do {	\
		MONO_STACKDATA(__gc_safe_dummy); \
		void* __gc_safe_cookie = mono_threads_enter_gc_safe_region_internal(&__gc_safe_dummy)
#define MONO_EXIT_GC_SAFE	\
		mono_threads_exit_gc_safe_region_internal(__gc_safe_cookie, &__gc_safe_dummy);	\
	} while (0)
#define MONO_ENTER_GC_SAFE_WITH_INFO(info)	\
	do {	\
		MONO_STACKDATA(__gc_safe_dummy); \
		void* __gc_safe_cookie = mono_threads_enter_gc_safe_region_with_info((info), &__gc_safe_dummy)
#define MONO_EXIT_GC_SAFE_WITH_INFO	MONO_EXIT_GC_SAFE
#define MONO_THREAD_INFO_GET(info) if (!info && mono_domain_get()) info = mono_thread_info_attach()

#else

#define MONO_THREAD_INFO_TYPE void
#define MONO_ENTER_GC_SAFE
#define MONO_EXIT_GC_SAFE
#define MONO_ENTER_GC_SAFE_WITH_INFO(info)
#define MONO_EXIT_GC_SAFE_WITH_INFO
#define MONO_THREAD_INFO_GET(info)
#define mono_thread_info_attach() nullptr

#endif
