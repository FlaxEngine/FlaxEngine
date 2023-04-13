// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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

// Use AOT for Mono
#define USE_MONO_AOT 1
#define USE_MONO_AOT_MODE MONO_AOT_MODE_FULL

#endif
