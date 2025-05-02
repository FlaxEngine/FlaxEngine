// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "IncludeVulkanHeaders.h"

#if GRAPHICS_API_VULKAN

enum class VulkanValidationLevel
{
    Disabled = 0,
    ErrorsOnly = 1,
    ErrorsAndWarnings = 2,
    ErrorsAndWarningsPerf = 3,
    ErrorsAndWarningsPerfInfo = 4,
    All = 5,
};

/// <summary>
/// The base implementation for the Vulkan API platform support.
/// </summary>
class VulkanPlatformBase
{
public:
    static void GetInstanceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
    {
    }

    static void GetDeviceExtensions(Array<const char*>& extensions, Array<const char*>& layers)
    {
    }

    static void CreateSurface(VkSurfaceKHR* outSurface)
    {
    }

    static void RestrictEnabledPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures, VkPhysicalDeviceFeatures& featuresToEnable)
    {
        featuresToEnable = deviceFeatures;
        featuresToEnable.shaderResourceResidency = VK_FALSE;
        featuresToEnable.shaderResourceMinLod = VK_FALSE;
        featuresToEnable.sparseBinding = VK_FALSE;
        featuresToEnable.sparseResidencyBuffer = VK_FALSE;
        featuresToEnable.sparseResidencyImage2D = VK_FALSE;
        featuresToEnable.sparseResidencyImage3D = VK_FALSE;
        featuresToEnable.sparseResidency2Samples = VK_FALSE;
        featuresToEnable.sparseResidency4Samples = VK_FALSE;
        featuresToEnable.sparseResidency8Samples = VK_FALSE;
        featuresToEnable.sparseResidencyAliased = VK_FALSE;
    }
};

#endif
