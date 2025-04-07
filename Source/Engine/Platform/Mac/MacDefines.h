// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Unix/UnixDefines.h"

// Platform description
#define PLATFORM_TYPE PlatformType::Mac
#if __aarch64__
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_ARM64 1
#define PLATFORM_ARCH ArchitectureType::ARM64
#else
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#endif
#define PLATFORM_DESKTOP 1
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_HAS_HEADLESS_MODE 1
#define PLATFORM_DEBUG_BREAK __builtin_trap()

// MoltenVK has artifacts when using tess so disable it
#define GPU_ALLOW_TESSELLATION_SHADERS 0

#endif
