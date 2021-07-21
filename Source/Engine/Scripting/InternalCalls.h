// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Debug/DebugLog.h"
#include "Engine/Core/Log.h"
#include "ScriptingType.h"

extern "C" FLAXENGINE_API void mono_add_internal_call(const char* name, const void* method);
#define ADD_INTERNAL_CALL(fullName, method) mono_add_internal_call(fullName, (const void*)method)

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
