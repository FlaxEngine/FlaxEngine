// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

// Build mode
#if BUILD_DEBUG
#define BUILD_DEVELOPMENT 0
#define BUILD_RELEASE 0
#elif BUILD_DEVELOPMENT
#define BUILD_DEBUG 0
#define BUILD_RELEASE 0
#elif BUILD_RELEASE
#define BUILD_DEBUG 0
#define BUILD_DEVELOPMENT 0
#else
#error "Invalid build mode configuration"
#endif

#include "Config.Gen.h"

// Disable undefined macros
#ifndef USE_EDITOR
#define USE_EDITOR 0
#endif
#ifndef OFFICIAL_BUILD
#define OFFICIAL_BUILD 0
#endif
#ifndef COMPILE_WITH_DEV_ENV
#define COMPILE_WITH_DEV_ENV 1
#endif

// Enable logging service (saving log to file, can be disabled using -nolog command line)
#define LOG_ENABLE 1

// Enable crash reporting service (stack trace and crash dump collecting)
#define CRASH_LOG_ENABLE (!BUILD_RELEASE)

// Enable/disable assertion
#define ENABLE_ASSERTION (!BUILD_RELEASE)

// Enable/disable assertion for Engine low layers
#define ENABLE_ASSERTION_LOW_LAYERS ENABLE_ASSERTION && (BUILD_DEBUG || FLAX_TESTS)

// Scripting API defines (see C++ scripting documentation for more info)
#define API_ENUM(...)
#define API_CLASS(...)
#define API_INTERFACE(...)
#define API_STRUCT(...)
#define API_FUNCTION(...)
#define API_PROPERTY(...)
#define API_FIELD(...)
#define API_EVENT(...)
#define API_PARAM(...)
#define API_TYPEDEF(...)
#define API_INJECT_CODE(...)
#define API_AUTO_SERIALIZATION(...) public: void Serialize(SerializeStream& stream, const void* otherObj) override; void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
#define DECLARE_SCRIPTING_TYPE_MINIMAL(type) public: friend class type##Internal; static struct ScriptingTypeInitializer TypeInitializer;
