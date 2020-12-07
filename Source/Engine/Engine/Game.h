// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if !USE_EDITOR

#if PLATFORM_WINDOWS
#include "Windows/WindowsGame.h"
#elif PLATFORM_UWP
#include "UWP/UWPGame.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxGame.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Engine/PS4Game.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Engine/XboxScarlettGame.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidGame.h"
#else
#error Missing Game implementation!
#endif

#endif
