// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "IncludeVulkanHeaders.h"

#if GRAPHICS_API_VULKAN

// Per platform configuration
#include "VulkanPlatform.h"

// Vulkan API version to target
#ifndef VULKAN_API_VERSION
#define VULKAN_API_VERSION VK_API_VERSION_1_0
#endif

// Amount of back buffers to use
#ifndef VULKAN_BACK_BUFFERS_COUNT
#define VULKAN_BACK_BUFFERS_COUNT 2
#endif
#ifndef VULKAN_BACK_BUFFERS_COUNT_MAX
#define VULKAN_BACK_BUFFERS_COUNT_MAX 4
#endif

/// <summary>
/// Default amount of frames to wait until resource delete.
/// </summary>
#define VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT 20

#define VULKAN_ENABLE_API_DUMP 0
#define VULKAN_RESET_QUERY_POOLS 0
#define VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES 1
#define VULKAN_USE_DEBUG_LAYER GPU_ENABLE_DIAGNOSTICS
#define VULKAN_USE_DEBUG_DATA (GPU_ENABLE_DIAGNOSTICS && COMPILE_WITH_DEV_ENV)

#ifndef VULKAN_USE_QUERIES
#define VULKAN_USE_QUERIES 1
#endif

#endif
