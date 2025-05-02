// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

#if GRAPHICS_API_VULKAN

#include "Engine/Core/Math/Math.h"

#if PLATFORM_SWITCH

#define VK_USE_PLATFORM_VI_NN 1
#include <vulkan/vulkan.h>
#undef VK_EXT_debug_utils
#undef VK_EXT_validation_cache
#define VULKAN_USE_VALIDATION_CACHE 0
#pragma clang diagnostic ignored "-Wpointer-bool-conversion"
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"

#else

#if PLATFORM_WIN32
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif

// Use volk as is a meta-loader for Vulkan
// Source: https://github.com/zeux/volk
// License: MIT
#include <ThirdParty/volk/volk.h>

#endif

// Use Vulkan Memory Allocator for buffer and image memory allocations
// Source: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/
// License: MIT
#define VMA_MIN(v1, v2) (Math::Min((v1), (v2)))
#define VMA_MAX(v1, v2) (Math::Max((v1), (v2)))
#define VMA_SWAP(v1, v2) (::Swap((v1), (v2)))
#define VMA_SYSTEM_ALIGNED_MALLOC(size, alignment) Platform::Allocate(uint64(size), uint64(alignment))
#define VMA_SYSTEM_ALIGNED_FREE(ptr) Platform::Free(ptr)
#define VMA_NULLABLE
#define VMA_NOT_NULL
#include <ThirdParty/VulkanMemoryAllocator/vk_mem_alloc.h>

#if PLATFORM_APPLE_FAMILY
// Declare potentially missing extensions from newer SDKs
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME "VK_KHR_portability_enumeration"
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif
#endif

#endif
