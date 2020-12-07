// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsWindow.h"
#elif PLATFORM_UWP
#include "UWP/UWPWindow.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxWindow.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Window.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettWindow.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidWindow.h"
#else
#error Missing Window implementation!
#endif

#include "Types.h"
