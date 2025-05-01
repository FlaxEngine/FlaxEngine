// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32
#include "Win32/Win32VulkanPlatform.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxVulkanPlatform.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidVulkanPlatform.h"
#elif PLATFORM_SWITCH
#include "Platforms/Switch/Engine/GraphicsDevice/Vulkan/SwitchVulkanPlatform.h"
#elif PLATFORM_MAC
#include "Mac/MacVulkanPlatform.h"
#elif PLATFORM_IOS
#include "iOS/iOSVulkanPlatform.h"
#endif
