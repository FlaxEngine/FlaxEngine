// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsFileSystem.h"
#elif PLATFORM_UWP
#include "UWP/UWPFileSystem.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxFileSystem.h"
#elif PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4FileSystem.h"
#elif PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5FileSystem.h"
#elif PLATFORM_XBOX_ONE
#include "Platforms/XboxOne/Engine/Platform/XboxOneFileSystem.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettFileSystem.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidFileSystem.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchFileSystem.h"
#elif PLATFORM_MAC
#include "Mac/MacFileSystem.h"
#elif PLATFORM_IOS
#include "iOS/iOSFileSystem.h"
#else
#error Missing File System implementation!
#endif

#include "Types.h"
