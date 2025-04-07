// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "../Unix/UnixDefines.h"

// Platform description
#define PLATFORM_TYPE PlatformType::iOS
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_ARM64 1
#define PLATFORM_ARCH ArchitectureType::ARM64
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_DEBUG_BREAK __builtin_trap()
#define PLATFORM_OUT_OF_MEMORY_BUFFER_SIZE (64ull * 1024) // 64 kB

// Use AOT for Mono
#define USE_MONO_AOT 1
#define USE_MONO_AOT_MODE MONO_AOT_MODE_FULL

#define GPU_ALLOW_TESSELLATION_SHADERS 0 // MoltenVK has artifacts when using tess so disable it
#define GPU_ALLOW_GEOMETRY_SHADERS 0 // Don't even try GS on mobile

#endif
