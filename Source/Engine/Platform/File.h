// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32File.h"
#elif PLATFORM_UWP
#include "Win32/Win32File.h"
#elif PLATFORM_LINUX
#include "Unix/UnixFile.h"
#elif PLATFORM_PS4
#include "Unix/UnixFile.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32File.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidFile.h"
#else
#error Missing File implementation!
#endif

#include "Types.h"
