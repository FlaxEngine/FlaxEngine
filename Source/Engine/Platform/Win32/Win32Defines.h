// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

// Platform description
#define PLATFORM_DESKTOP 1
#if defined(WIN64)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#else
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH_X86 1
#define PLATFORM_ARCH ArchitectureType::x86
#endif
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_LINE_TERMINATOR "\r\n"
#define PLATFORM_DEBUG_BREAK __debugbreak()

#endif
