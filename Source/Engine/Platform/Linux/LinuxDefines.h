// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "../Unix/UnixDefines.h"

// Platform description
#if defined(_LINUX64)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#else
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH_X86 1
#define PLATFORM_ARCH ArchitectureType::x86
#endif
#define PLATFORM_TYPE PlatformType::Linux
#define PLATFORM_DESKTOP 1
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_HAS_HEADLESS_MODE 1
#define PLATFORM_DEBUG_BREAK __asm__ volatile("int $0x03")

#endif
