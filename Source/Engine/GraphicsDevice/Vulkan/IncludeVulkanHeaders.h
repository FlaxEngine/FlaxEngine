// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

#if GRAPHICS_API_VULKAN

#include "Engine/Core/Math/Math.h"

#if PLATFORM_WIN32
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif

// Use volk as is a meta-loader for Vulkan
// Source: https://github.com/zeux/volk
// License: MIT
#include <ThirdParty/volk/volk.h>

// Use Vulkan Memory Allocator for buffer and image memory allocations
// Source: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/
// License: MIT
#define VMA_MIN(v1, v2) (Math::Min((v1), (v2)))
#define VMA_MAX(v1, v2) (Math::Max((v1), (v2)))
#define VMA_SWAP(v1, v2) (::Swap((v1), (v2)))
#define VMA_NULLABLE
#define VMA_NOT_NULL
#include <ThirdParty/VulkanMemoryAllocator/vk_mem_alloc.h>

#endif
