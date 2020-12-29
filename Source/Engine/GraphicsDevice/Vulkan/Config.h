// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
#define VULKAN_BACK_BUFFERS_COUNT 2
#define VULKAN_BACK_BUFFERS_COUNT_MAX 16

/// <summary>
/// Default amount of frames to wait until resource delete.
/// </summary>
#define VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT 10

#define VULKAN_ENABLE_API_DUMP 0
#define VULKAN_RESET_QUERY_POOLS 0
#define VULKAN_HASH_POOLS_WITH_TYPES_USAGE_ID 1
#define VULKAN_USE_DEBUG_LAYER GPU_ENABLE_DIAGNOSTICS

#ifndef VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2
#define VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2 0
#endif

#endif
