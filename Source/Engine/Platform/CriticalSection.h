// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32CriticalSection.h"
#elif PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_PS4 || PLATFORM_PS5 || PLATFORM_MAC || PLATFORM_IOS
#include "Unix/UnixCriticalSection.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchCriticalSection.h"
#else
#error Missing Critical Section implementation!
#endif

#include "Types.h"
