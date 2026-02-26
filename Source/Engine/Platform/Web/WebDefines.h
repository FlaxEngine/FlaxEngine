// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "../Unix/UnixDefines.h"

// Platform description
#define PLATFORM_TYPE PlatformType::Web
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH ArchitectureType::x86
#define PLATFORM_CACHE_LINE_SIZE 64
#define PLATFORM_DEBUG_BREAK 
#define PLATFORM_OUT_OF_MEMORY_BUFFER_SIZE 0
#define WEB_CANVAS_ID "#canvas"

// Configure graphics
#define GPU_ALLOW_TESSELLATION_SHADERS 0
#define GPU_ALLOW_GEOMETRY_SHADERS 0
//#define GPU_ALLOW_PROFILE_EVENTS 0
#define GPU_AUTO_PROFILE_EVENTS (!BUILD_RELEASE)
#define GPU_ENABLE_PRELOADING_RESOURCES 0 // Don't preload things unless needed

// Threading is optional
#ifdef __EMSCRIPTEN_PTHREADS__
#define PLATFORM_THREADS_LIMIT 4
#else
#define PLATFORM_THREADS_LIMIT 1
#define GPU_ENABLE_ASYNC_RESOURCES_CREATION 0
#endif

#endif
