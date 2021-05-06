// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "../Win32/Win32Defines.h"

// Platform description
#define PLATFORM_TYPE PlatformType::Windows
#define PLATFORM_HAS_HEADLESS_MODE 1

#if !USE_EDITOR
// Use AOT for Mono
#define USE_MONO_AOT 1
#define USE_MONO_AOT_MODE MONO_AOT_MODE_INTERP // TODO: support Full AOT mode instead of interpreter
#endif

#endif
