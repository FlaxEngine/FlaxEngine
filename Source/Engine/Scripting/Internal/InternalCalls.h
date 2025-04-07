// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/Types.h"

#if defined(__clang__)
// Helper utility to override vtable entry with automatic restore
// See BindingsGenerator.Cpp.cs that generates virtuall method wrappers for scripting to properly call overriden base method
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
