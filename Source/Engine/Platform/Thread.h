// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32Thread.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxThread.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Thread.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5Thread.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidThread.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchThread.h"
#elif PLATFORM_MAC || PLATFORM_IOS
#include "Apple/AppleThread.h"
#else
#error Missing Thread implementation!
#endif

#include "Types.h"
