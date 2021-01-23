// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Win32/Win32Network.h"
#elif PLATFORM_UWP
#include "Win32/Win32Network.h" // place holder
#elif PLATFORM_LINUX
#include "Base/NetworkBase.h" // place holder 
#elif PLATFORM_PS4
#include "Base/NetworkBase.h" // place holder
#elif PLATFORM_XBOX_SCARLETT
#include "Win32/Win32Network.h" // it's probably isn't working
#elif PLATFORM_ANDROID
#include "Base/NetworkBase.h" // place holder
#else
#error Missing Network implementation!
#endif

#include "Types.h"
