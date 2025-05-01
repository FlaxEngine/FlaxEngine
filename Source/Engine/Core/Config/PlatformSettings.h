// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "PlatformSettingsBase.h"

// Settings defined per-platform, available at runtime on that platform or at design time in editor
#if PLATFORM_WINDOWS
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#endif
#if PLATFORM_UWP
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#endif
#if PLATFORM_LINUX
#include "Engine/Platform/Linux/LinuxPlatformSettings.h"
#endif
#if PLATFORM_PS4
#include "Platforms/PS4/Engine/Platform/PS4PlatformSettings.h"
#endif
#if PLATFORM_XBOX_ONE
#include "Platforms/XboxOne/Engine/Platform/XboxOnePlatformSettings.h"
#endif
#if PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettPlatformSettings.h"
#endif
#if PLATFORM_ANDROID
#include "Engine/Platform/Android/AndroidPlatformSettings.h"
#endif
#if PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchPlatformSettings.h"
#endif
#if PLATFORM_PS5
#include "Platforms/PS5/Engine/Platform/PS5PlatformSettings.h"
#endif
#if PLATFORM_MAC
#include "Engine/Platform/Mac/MacPlatformSettings.h"
#endif
#if PLATFORM_IOS
#include "Engine/Platform/iOS/iOSPlatformSettings.h"
#endif
