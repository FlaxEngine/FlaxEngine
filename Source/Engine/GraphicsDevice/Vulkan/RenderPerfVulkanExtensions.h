// Copyright (c) Flax Engine. NVIDIA PerfSDK Vulkan device setup helpers.

#pragma once

#if COMPILE_WITH_RENDER_PERF_NVPERF

#include "Engine/Core/Collections/Array.h"

struct VkDeviceCreateInfo;
typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

/// <summary>
/// Appends NVIDIA PerfSDK required Vulkan instance extensions when available on the driver.
/// </summary>
void RenderPerfAppendVulkanInstanceExtensions(Array<const char*>& outInstanceExtensions, const Array<const char*>& availableExtensions, uint32 apiVersion);

/// <summary>
/// Appends NVIDIA PerfSDK required Vulkan device extensions when available on the driver.
/// </summary>
void RenderPerfAppendVulkanDeviceExtensions(VkInstance instance, VkPhysicalDevice physicalDevice, Array<const char*>& outDeviceExtensions, const Array<const char*>& availableExtensions, uint32 physicalDeviceApiVersion);

/// <summary>
/// Chains required feature structs onto VkDeviceCreateInfo for enabled PerfSDK extensions.
/// </summary>
void RenderPerfChainVulkanDeviceCreateInfo(VkDeviceCreateInfo* deviceCreateInfo, VkPhysicalDevice physicalDevice, const Array<const char*>& enabledDeviceExtensions);

#endif
