// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

// Platform description
#define PLATFORM_DESKTOP 1
#if defined(WIN64) && defined(_M_X64)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_X64 1
#define PLATFORM_ARCH ArchitectureType::x64
#elif defined(WIN64) && defined(_M_ARM64)
#define PLATFORM_64BITS 1
#define PLATFORM_ARCH_ARM64 1
#define PLATFORM_ARCH ArchitectureType::ARM64
#else
#define PLATFORM_64BITS 0
#define PLATFORM_ARCH_X86 1
#define PLATFORM_ARCH ArchitectureType::x86
#endif
#define PLATFORM_CACHE_LINE_SIZE 128
#define PLATFORM_LINE_TERMINATOR "\r\n"
#define PLATFORM_DEBUG_BREAK __debugbreak()

#endif
