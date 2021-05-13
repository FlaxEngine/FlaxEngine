// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#if PLATFORM_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Engine/Platform/XboxScarlettPlatformSettings.h"
#endif
#if PLATFORM_ANDROID
#include "Engine/Platform/Android/AndroidPlatformSettings.h"
#endif
#if PLATFORM_SWITCH
#include "Platforms/Switch/Engine/Platform/SwitchPlatformSettings.h"
#endif
