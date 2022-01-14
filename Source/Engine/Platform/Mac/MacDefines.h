// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Unix/UnixDefines.h"

// Platform description
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#define PLATFORM_TYPE PlatformType::Mac
#define PLATFORM_DESKTOP 1
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_HAS_HEADLESS_MODE 1
#define PLATFORM_DEBUG_BREAK __builtin_trap()

#endif
