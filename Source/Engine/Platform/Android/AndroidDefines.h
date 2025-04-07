// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "../Unix/UnixDefines.h"

// Platform description
#if defined(__arm__)
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH ArchitectureType::ARM
#define PLATFORM_ARCH_ARM 1
#define PLATFORM_DEBUG_BREAK __asm__("trap")
#elif defined(__aarch64__)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_ARM64 1
#define PLATFORM_ARCH ArchitectureType::ARM64
#define PLATFORM_DEBUG_BREAK __asm__(".inst 0xd4200000")
#elif defined(__i386__)
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH_X86 1
#define PLATFORM_ARCH ArchitectureType::x86
#define PLATFORM_DEBUG_BREAK __asm__("int $3")
#elif defined(__x86_64__)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#define PLATFORM_DEBUG_BREAK __asm__("int $3")
#else
#error "Unknown Android ABI."
#endif
#define PLATFORM_TYPE PlatformType::Android
#define PLATFORM_CACHE_LINE_SIZE 64
#define PLATFORM_OUT_OF_MEMORY_BUFFER_SIZE (64ull * 1024) // 64 kB

#define USE_MONO_AOT_MODE MONO_AOT_MODE_NONE

#define GPU_ALLOW_TESSELLATION_SHADERS 0 // Tess on mobile is not well supported
#define GPU_ALLOW_GEOMETRY_SHADERS 0 // Don't even try GS on mobile

#endif
