// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32Thread.h"
#elif PLATFORM_UWP
#include "Win32/Win32Thread.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxThread.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Thread.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32Thread.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidThread.h"
#else
#error Missing Thread implementation!
#endif

#include "Types.h"
