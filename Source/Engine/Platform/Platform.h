// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

// Link types and defines
#include "Types.h"
#include "Defines.h"

#if PLATFORM_SDL
#include "SDL/SDLPlatform.h"
#elif PLATFORM_WINDOWS
#include "Windows/WindowsPlatform.h"
#elif PLATFORM_UWP
#include "UWP/UWPPlatform.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatform.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Platform.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5Platform.h"
#elif PLATFORM_XBOX_ONE
#include "Platforms/XboxOne/Engine/Platform/XboxOnePlatform.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettPlatform.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidPlatform.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchPlatform.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatform.h"
#elif PLATFORM_IOS
#include "iOS/iOSPlatform.h"
#else
#error Missing Platform implementation!
#endif

#if ENABLE_ASSERTION
// Performs a hard assertion of the expression. Crashes the engine and triggers a debugger break if the expression fails.
#define ASSERT(expression) \
    if (!(expression)) \
    { \
        if (Platform::IsDebuggerPresent()) \
        { \
            PLATFORM_DEBUG_BREAK; \
        } \
        Platform::Assert(#expression, __FILE__, __LINE__); \
    }
#else
// Performs a hard assertion of the expression. Crashes the engine and triggers a debugger break if the expression fails.
#define ASSERT(expression) ((void)0)
#endif
#if ENABLE_ASSERTION_LOW_LAYERS
// Performs a hard assertion of the expression. Crashes the engine and triggers a debugger break if the expression fails.
#define ASSERT_LOW_LAYER(x) ASSERT(x)
#else
// Performs a hard assertion of the expression. Crashes the engine and triggers a debugger break if the expression fails.
#define ASSERT_LOW_LAYER(x)
#endif

// Performs a soft check of the expression. Logs the expression failure and returns from the function call.
#define CHECK(expression) \
    if (!(expression)) \
    { \
        Platform::CheckFailed(#expression, __FILE__, __LINE__); \
        return; \
    }
// Performs a soft check of the expression. Logs the expression failure and returns from the function call using the given return value.
#define CHECK_RETURN(expression, returnValue) \
    if (!(expression)) \
    { \
        Platform::CheckFailed(#expression, __FILE__, __LINE__); \
        return returnValue; \
    }

#if ENABLE_ASSERTION
// Performs a soft check of the expression. Logs the expression failure and returns from the function call.
#define CHECK_DEBUG(expression) CHECK(expression)
// Performs a soft check of the expression. Logs the expression failure and returns from the function call using the given return value.
#define CHECK_RETURN_DEBUG(expression, returnValue) CHECK_RETURN(expression, returnValue)
#else
// Performs a soft check of the expression. Logs the expression failure and returns from the function call.
#define CHECK_DEBUG(expression) ((void)0)
// Performs a soft check of the expression. Logs the expression failure and returns from the function call using the given return value.
#define CHECK_RETURN_DEBUG(expression, returnValue) ((void)0)
#endif
