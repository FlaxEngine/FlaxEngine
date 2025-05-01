// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32File.h"
#elif PLATFORM_LINUX || PLATFORM_PS4 || PLATFORM_PS5 || PLATFORM_MAC
#include "Unix/UnixFile.h"
#elif PLATFORM_IOS
#include "iOS/iOSFile.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidFile.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchFile.h"
#else
#error Missing File implementation!
#endif

#include "Types.h"
