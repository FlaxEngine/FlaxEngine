// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// The platform the game is running.
/// </summary>
API_ENUM() enum class PlatformType
{
    /// <summary>
    /// Running on Windows.
    /// </summary>
    Windows = 1,

    /// <summary>
    /// Running on Xbox One.
    /// </summary>
    XboxOne = 2,

    /// <summary>
    /// Running Windows Store App (Universal Windows Platform).
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"UWP\")")
    UWP = 3,

    /// <summary>
    /// Running on Linux system.
    /// </summary>
    Linux = 4,

    /// <summary>
    /// Running on PlayStation 4.
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"PS4\")")
    PS4 = 5,

    /// <summary>
    /// Running on Xbox Series X.
    /// </summary>
    XboxScarlett = 6,

    /// <summary>
    /// Running on Android.
    /// </summary>
    Android = 7,

    /// <summary>
    /// Running on Switch.
    /// </summary>
    Switch = 8,

    /// <summary>
    /// Running on PlayStation 5.
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"PS5\")")
    PS5 = 9,

    /// <summary>
    /// Running on Mac.
    /// </summary>
    Mac = 10,

    /// <summary>
    /// Running on iPhone.
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"iOS\")")
    iOS = 11,
};

/// <summary>
/// The platform architecture types.
/// </summary>
API_ENUM() enum class ArchitectureType
{
    /// <summary>
    /// Anything or not important.
    /// </summary>
    AnyCPU = 0,

    /// <summary>
    /// The x86 32-bit.
    /// </summary>
    x86 = 1,

    /// <summary>
    /// The x86 64-bit.
    /// </summary>
    x64 = 2,

    /// <summary>
    /// The ARM 32-bit.
    /// </summary>
    ARM = 3,

    /// <summary>
    /// The ARM 64-bit.
    /// </summary>
    ARM64 = 4,
};

// Add missing platforms definitions
#if !defined(PLATFORM_WINDOWS)
#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_UWP)
#define PLATFORM_UWP 0
#endif
#if !defined(PLATFORM_WIN32)
#define PLATFORM_WIN32 0
#endif
#if !defined(PLATFORM_XBOX_ONE)
#define PLATFORM_XBOX_ONE 0
#endif
#if !defined(PLATFORM_UNIX)
#define PLATFORM_UNIX 0
#endif
#if !defined(PLATFORM_LINUX)
#define PLATFORM_LINUX 0
#endif
#if !defined(PLATFORM_PS4)
#define PLATFORM_PS4 0
#endif
#if !defined(PLATFORM_PS5)
#define PLATFORM_PS5 0
#endif
#if !defined(PLATFORM_XBOX_SCARLETT)
#define PLATFORM_XBOX_SCARLETT 0
#endif
#if !defined(PLATFORM_ANDROID)
#define PLATFORM_ANDROID 0
#endif
#if !defined(PLATFORM_MAC)
#define PLATFORM_MAC 0
#endif
#if !defined(PLATFORM_IOS)
#define PLATFORM_IOS 0
#endif
#if !defined(PLATFORM_SWITCH)
#define PLATFORM_SWITCH 0
#endif

#if PLATFORM_WINDOWS
#include "Windows/WindowsDefines.h"
#elif PLATFORM_UWP
#include "UWP/UWPDefines.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxDefines.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Defines.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5Defines.h"
#elif PLATFORM_XBOX_ONE
#include "Platforms/XboxOne/Engine/Platform/XboxOneDefines.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettDefines.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidDefines.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchDefines.h"
#elif PLATFORM_MAC
#include "Mac/MacDefines.h"
#elif PLATFORM_IOS
#include "iOS/iOSDefines.h"
#else
#error Missing Defines implementation!
#endif

// Create default definitions if any missing
#ifndef PLATFORM_64BITS
#define PLATFORM_64BITS 0
#endif
#ifndef PLATFORM_DESKTOP
#define PLATFORM_DESKTOP 0
#endif
#ifndef PLATFORM_ARCH_X64
#define PLATFORM_ARCH_X64 0
#endif
#ifndef PLATFORM_ARCH_X86
#define PLATFORM_ARCH_X86 0
#endif
#ifndef PLATFORM_ARCH_ARM
#define PLATFORM_ARCH_ARM 0
#endif
#ifndef PLATFORM_ARCH_ARM64
#define PLATFORM_ARCH_ARM64 0
#endif
#ifndef PLATFORM_TEXT_IS_CHAR16
#define PLATFORM_TEXT_IS_CHAR16 0
#endif
#ifndef PLATFORM_DEBUG_BREAK
#define PLATFORM_DEBUG_BREAK
#endif
#ifndef PLATFORM_LINE_TERMINATOR
#define PLATFORM_LINE_TERMINATOR "\n"
#endif
#ifndef PLATFORM_THREADS_LIMIT
#define PLATFORM_THREADS_LIMIT 64
#endif
#define PLATFORM_32BITS (!PLATFORM_64BITS)

// Platform family defines
#define PLATFORM_WINDOWS_FAMILY (PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT)
#define PLATFORM_MICROSOFT_FAMILY (PLATFORM_WINDOWS_FAMILY)
#define PLATFORM_APPLE_FAMILY (PLATFORM_MAC || PLATFORM_IOS)
#define PLATFORM_UNIX_FAMILY (PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_PS4 || PLATFORM_PS5 || PLATFORM_APPLE_FAMILY)

// SIMD defines
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#define PLATFORM_SIMD_SSE2 1
#if defined(__SSE3__)
#define PLATFORM_SIMD_SSE3 1
#endif
#if defined(__SSE4__)
#define PLATFORM_SIMD_SSE4 1
#endif
#if defined(__SSE4_1__)
#define PLATFORM_SIMD_SSE4_1 1
#endif
#if defined(__SSE4_2__)
#define PLATFORM_SIMD_SSE4_2 1
#endif
#endif
#if defined(_M_ARM) || defined(__ARM_NEON__) || defined(__ARM_NEON)
#define PLATFORM_SIMD_NEON 1
#endif
#if defined(_M_PPC) || defined(__CELLOS_LV2__)
#define PLATFORM_SIMD_VMX 1
#endif

// Unicode text macro
#if !defined(TEXT)
#if PLATFORM_TEXT_IS_CHAR16
#define _TEXT(x) u ## x
#else
#define _TEXT(x) L ## x
#endif
#define TEXT(x) _TEXT(x)
#endif

// Static array size
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
