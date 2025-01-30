// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/Textures/GPUSamplerDescription.h"
#include "IncludeVulkanHeaders.h"

#if GRAPHICS_API_VULKAN

#if GPU_ENABLE_ASSERTION

// Vulkan results validation
#define VALIDATE_VULKAN_RESULT(x) { VkResult result = x; if (result != VK_SUCCESS) RenderToolsVulkan::ValidateVkResult(result, __FILE__, __LINE__); }
#define LOG_VULKAN_RESULT(result) if (result != VK_SUCCESS) RenderToolsVulkan::LogVkResult(result, __FILE__, __LINE__)
#define LOG_VULKAN_RESULT_WITH_RETURN(result) if (result != VK_SUCCESS) { RenderToolsVulkan::LogVkResult(result, __FILE__, __LINE__); return true; }
#define VK_SET_DEBUG_NAME(device, handle, type, name) RenderToolsVulkan::SetObjectName(device->Device, (uint64)handle, type, name)

#else

#define VALIDATE_VULKAN_RESULT(x) x
#define LOG_VULKAN_RESULT(result) if (result != VK_SUCCESS) RenderToolsVulkan::LogVkResult(result)
#define LOG_VULKAN_RESULT_WITH_RETURN(result) if (result != VK_SUCCESS) { RenderToolsVulkan::LogVkResult(result); return true; }
#define VK_SET_DEBUG_NAME(device, handle, type, name)

#endif

/// <summary>
/// Set of utilities for rendering on Vulkan platform.
/// </summary>
class RenderToolsVulkan
{
private:
    static VkFormat PixelFormatToVkFormat[static_cast<int32>(PixelFormat::MAX)];
    static VkBlendFactor BlendToVkBlendFactor[static_cast<int32>(BlendingMode::Blend::MAX)];
    static VkBlendOp OperationToVkBlendOp[static_cast<int32>(BlendingMode::Operation::MAX)];
    static VkCompareOp ComparisonFuncToVkCompareOp[static_cast<int32>(ComparisonFunc::MAX)];

public:
#if GPU_ENABLE_RESOURCE_NAMING
    static void SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const String& name);
    static void SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const char* name);
#endif

    static String GetVkErrorString(VkResult result);
    static void ValidateVkResult(VkResult result, const char* file, uint32 line);
    static void LogVkResult(VkResult result, const char* file, uint32 line);
    static void LogVkResult(VkResult result);

    static inline VkPipelineStageFlags GetBufferBarrierFlags(VkAccessFlags accessFlags)
    {
        VkPipelineStageFlags stageFlags = (VkPipelineStageFlags)0;
        switch (accessFlags)
        {
        case 0:
            stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
            stageFlags = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            break;
        case VK_ACCESS_TRANSFER_WRITE_BIT:
            stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
            stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
            stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_ACCESS_TRANSFER_READ_BIT:
            stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_ACCESS_SHADER_READ_BIT:
        case VK_ACCESS_SHADER_WRITE_BIT:
            stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
        case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
            stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        default:
            CRASH;
            break;
        }
        return stageFlags;
    }

    static inline VkPipelineStageFlags GetImageBarrierFlags(VkImageLayout layout, VkAccessFlags& accessFlags)
    {
        VkPipelineStageFlags stageFlags;
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            accessFlags = 0;
            stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
            stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
            accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            accessFlags = VK_ACCESS_TRANSFER_READ_BIT;
            stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            accessFlags = 0;
            stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            accessFlags = VK_ACCESS_SHADER_READ_BIT;
            stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
        case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
            accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        default:
            CRASH;
            break;
        }
        return stageFlags;
    }

    template<class T>
    static FORCE_INLINE void ZeroStruct(T& data, VkStructureType type)
    {
        static_assert(!TIsPointer<T>::Value, "Don't use a pointer.");
        static_assert(OFFSET_OF(T, sType) == 0, "Assumes type is the first member in the Vulkan type.");
        data.sType = type;
        Platform::MemoryClear((uint8*)&data + sizeof(VkStructureType), sizeof(T) - sizeof(VkStructureType));
    }

    /// <summary>
    /// Converts Flax Pixel Format to the Vulkan Format.
    /// </summary>
    /// <param name="value">The Flax Pixel Format.</param>
    /// <returns>The Vulkan Format.</returns>
    static FORCE_INLINE VkFormat ToVulkanFormat(const PixelFormat value)
    {
        return PixelFormatToVkFormat[(int32)value];
    }

    /// <summary>
    /// Converts Flax blend operation to the Vulkan blend operation.
    /// </summary>
    /// <param name="value">The Flax blend operation.</param>
    /// <returns>The Vulkan blend operation.</returns>
    static FORCE_INLINE VkBlendOp ToVulkanBlendOp(const BlendingMode::Operation value)
    {
        return OperationToVkBlendOp[(int32)value];
    }

    /// <summary>
    /// Converts Flax comparison function to the Vulkan comparison operation.
    /// </summary>
    /// <param name="value">The Flax comparison function.</param>
    /// <returns>The Vulkan comparison operation.</returns>
    static FORCE_INLINE VkCompareOp ToVulkanCompareOp(const ComparisonFunc value)
    {
        return ComparisonFuncToVkCompareOp[(int32)value];
    }

    static VkSamplerMipmapMode ToVulkanMipFilterMode(GPUSamplerFilter filter)
    {
        VkSamplerMipmapMode result;
        switch (filter)
        {
        case GPUSamplerFilter::Point:
            result = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case GPUSamplerFilter::Bilinear:
            result = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case GPUSamplerFilter::Trilinear:
            result = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case GPUSamplerFilter::Anisotropic:
            result = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        default:
            CRASH;
            break;
        }
        return result;
    }

    static VkFilter ToVulkanMagFilterMode(GPUSamplerFilter filter)
    {
        VkFilter result;
        switch (filter)
        {
        case GPUSamplerFilter::Point:
            result = VK_FILTER_NEAREST;
            break;
        case GPUSamplerFilter::Bilinear:
            result = VK_FILTER_LINEAR;
            break;
        case GPUSamplerFilter::Trilinear:
            result = VK_FILTER_LINEAR;
            break;
        case GPUSamplerFilter::Anisotropic:
            result = VK_FILTER_LINEAR;
            break;
        default:
            CRASH;
            break;
        }
        return result;
    }

    static VkFilter ToVulkanMinFilterMode(GPUSamplerFilter filter)
    {
        VkFilter result;
        switch (filter)
        {
        case GPUSamplerFilter::Point:
            result = VK_FILTER_NEAREST;
            break;
        case GPUSamplerFilter::Bilinear:
            result = VK_FILTER_LINEAR;
            break;
        case GPUSamplerFilter::Trilinear:
            result = VK_FILTER_LINEAR;
            break;
        case GPUSamplerFilter::Anisotropic:
            result = VK_FILTER_LINEAR;
            break;
        default:
            CRASH;
            break;
        }
        return result;
    }

    static VkSamplerAddressMode ToVulkanWrapMode(GPUSamplerAddressMode addressMode, const bool supportsMirrorClampToEdge)
    {
        VkSamplerAddressMode result;
        switch (addressMode)
        {
        case GPUSamplerAddressMode::Wrap:
            result = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case GPUSamplerAddressMode::Clamp:
            result = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case GPUSamplerAddressMode::Mirror:
            result = supportsMirrorClampToEdge ? VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case GPUSamplerAddressMode::Border:
            result = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
        default:
            CRASH;
            break;
        }
        return result;
    }

    static VkCompareOp ToVulkanSamplerCompareFunction(GPUSamplerCompareFunction samplerComparisonFunction)
    {
        VkCompareOp result;
        switch (samplerComparisonFunction)
        {
        case GPUSamplerCompareFunction::Less:
            result = VK_COMPARE_OP_LESS;
            break;
        case GPUSamplerCompareFunction::Never:
            result = VK_COMPARE_OP_NEVER;
            break;
        default:
            CRASH;
            break;
        }
        return result;
    }

    static bool HasExtension(const Array<const char*, HeapAllocation>& extensions, const char* name);
};

#endif
