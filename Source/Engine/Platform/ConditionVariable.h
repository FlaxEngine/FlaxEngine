// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32ConditionVariable.h"
#elif PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_PS4 || PLATFORM_PS5 || PLATFORM_MAC || PLATFORM_IOS
#include "Unix/UnixConditionVariable.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchConditionVariable.h"
#else
#error Missing Condition Variable implementation!
#endif

#include "Types.h"
