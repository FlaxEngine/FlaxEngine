// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32ConditionVariable.h"
#elif PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_PS4 || PLATFORM_PS5
#include "Unix/UnixConditionVariable.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchConditionVariable.h"
#elif PLATFORM_MAC
#include "Mac/MacConditionVariable.h"
#else
#error Missing Condition Variable implementation!
#endif

#include "Types.h"
