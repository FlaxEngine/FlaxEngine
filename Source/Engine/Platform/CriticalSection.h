// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32CriticalSection.h"
#elif PLATFORM_UWP
#include "Win32/Win32CriticalSection.h"
#elif PLATFORM_LINUX
#include "Unix/UnixCriticalSection.h"
#elif PLATFORM_PS4
#include "Unix/UnixCriticalSection.h"
#elif PLATFORM_XBOX_ONE
#include "Win32/Win32CriticalSection.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32CriticalSection.h"
#elif PLATFORM_ANDROID
#include "Unix/UnixCriticalSection.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchCriticalSection.h"
#else
#error Missing Critical Section implementation!
#endif

#include "Types.h"
