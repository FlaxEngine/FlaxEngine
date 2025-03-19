// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_SDL
#include "SDL/SDLWindow.h"
#elif PLATFORM_WINDOWS
#include "Windows/WindowsWindow.h"
#elif PLATFORM_UWP
#include "UWP/UWPWindow.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxWindow.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4Window.h"
#elif PLATFORM_XBOX_ONE
#include "GDK/GDKWindow.h"
#elif PLATFORM_XBOX_SCARLETT
#include "GDK/GDKWindow.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidWindow.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchWindow.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5Window.h"
#elif PLATFORM_MAC
#include "Mac/MacWindow.h"
#elif PLATFORM_IOS
#include "iOS/iOSWindow.h"
#else
#error Missing Window implementation!
#endif

#include "Types.h"
