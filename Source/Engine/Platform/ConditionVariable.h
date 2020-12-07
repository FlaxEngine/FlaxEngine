// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32ConditionVariable.h"
#elif PLATFORM_UWP
#include "Win32/Win32ConditionVariable.h"
#elif PLATFORM_LINUX
#include "Unix/UnixConditionVariable.h"
#elif PLATFORM_PS4
#include "Unix/UnixConditionVariable.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32ConditionVariable.h"
#elif PLATFORM_ANDROID
#include "Unix/UnixConditionVariable.h"
#else
#error Missing Condition Variable implementation!
#endif

#include "Types.h"
