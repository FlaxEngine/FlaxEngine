// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_UWP || PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT
#include "Win32/Win32File.h"
#elif PLATFORM_LINUX || PLATFORM_PS4 || PLATFORM_PS5
#include "Unix/UnixFile.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidFile.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchFile.h"
#elif PLATFORM_MAC
#include "Mac/MacFile.h"
#else
#error Missing File implementation!
#endif

#include "Types.h"
