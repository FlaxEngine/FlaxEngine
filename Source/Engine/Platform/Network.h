// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32Network.h"
#elif PLATFORM_UWP
#include "Win32/Win32Network.h"
#elif PLATFORM_LINUX

#elif PLATFORM_PS4

#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32Network.h" // it's probably isn't working
#elif PLATFORM_ANDROID

#else
#error Missing Network implementation!
#endif

#include "Types.h"
