// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "../Win32/Win32Defines.h"

// Platform description
#if PLATFORM_XBOX_ONE
#define PLATFORM_TYPE PlatformType::XboxOne
#else
#define PLATFORM_TYPE PlatformType::UWP
#endif

// Use AOT for Mono
#define USE_MONO_AOT 1
#define USE_MONO_AOT_MODE MONO_AOT_MODE_INTERP // TODO: support Full AOT mode instead of interpreter

#endif
