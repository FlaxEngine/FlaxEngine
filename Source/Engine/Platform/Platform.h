// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

// Link types and defines
#include "Types.h"
#include "Defines.h"

#if PLATFORM_WINDOWS
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

//[ToDo] move it on to correct place ?
namespace preprocesors
{
    //converts whatever is value as a "value" but pares any value to string at compile time
    #define TOSTRING(s) STRINGIFY(s)
    //converts whatever is value as a "value"
    #define STRINGIFY(value) #value
    #define FILE_LINE __FILE__ ":" TOSTRING(__LINE__)
}

#if ENABLE_ASSERTION
// Performs hard assertion of the expression. Crashes the engine and inserts debugger break in case of expression fail.
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
#define ASSERT(expression) ((void)0)
#endif
#if ENABLE_ASSERTION_LOW_LAYERS
#define ASSERT_LOW_LAYER(x) ASSERT(x)
#else
#define ASSERT_LOW_LAYER(x)
#endif

// Performs soft check of the expression. Logs the expression fail to log and returns the function call.
#define CHECK(expression) \
    if (!(expression)) \
    { \
        Platform::CheckFailed(#expression, __FILE__, __LINE__); \
        return; \
    }
#define CHECK_RETURN(expression, returnValue) \
    if (!(expression)) \
    { \
        Platform::CheckFailed(#expression, __FILE__, __LINE__); \
        return returnValue; \
    }


#if ENABLE_ASSERTION && USE_EDITOR
//Performs soft assertion of the expression. crashes the engine if is not used with editor, inserts debugger break in case of expression fail, logs the massage to the file.
//[opcional returncode if funcion needs to return somfing]
#define SOFT_ASSERT(expression,Massage,returncode)                                                          \
if (expression)                                                                                             \
{                                                                                                           \
    if (Platform::IsDebuggerPresent())                                                                      \
    {                                                                                                       \
        PLATFORM_DEBUG_BREAK;                                                                               \
    }                                                                                                       \
    LOG_STR(Fatal,                                                                                          \
    TEXT(                                                                                                   \
    "\n[Soft Assert Triggered]"                                                                             \
    "\n" Massage                                                                                            \
    "\nReport a bug at https://github.com/FlaxEngine/FlaxEngine/issues if this is engine issue"             \
    "\nInfo:"                                                                                               \
    "\n" FILE_LINE));                                                                                       \
    returncode                                                                                              \
}
#else
#if ENABLE_ASSERTION
#define SOFT_ASSERT(expression,Massage,returncode) ASSERT(!(expression))
#else
#define SOFT_ASSERT(expression,Massage,returncode)
#endif
#endif
