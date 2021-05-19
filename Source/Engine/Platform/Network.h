// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32Network.h"
#elif PLATFORM_UWP
#include "Win32/Win32Network.h"
#elif PLATFORM_LINUX
#include "Unix/UnixNetwork.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Network.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32Network.h"
#elif PLATFORM_ANDROID
#include "Unix/UnixNetwork.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchNetwork.h"
#else
#error Missing Network implementation!
#endif

#include "Types.h"
