// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsFileSystem.h"
#elif PLATFORM_UWP
#include "UWP/UWPFileSystem.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxFileSystem.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4FileSystem.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettFileSystem.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidFileSystem.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchFileSystem.h"
#else
#error Missing File System implementation!
#endif

#include "Types.h"
