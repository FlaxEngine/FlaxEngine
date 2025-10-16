// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "RenderToolsVulkan.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUResourceAccess.h"

// @formatter:off

VkFormat RenderToolsVulkan::PixelFormatToVkFormat[110] =
{
    VK_FORMAT_UNDEFINED,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_R16G16B16A16_UINT,
    VK_FORMAT_R16G16B16A16_SNORM,
    VK_FORMAT_R16G16B16A16_SINT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_UNDEFINED, // TODO: R32G8X24_Typeless
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_UNDEFINED, // TODO: R32_Float_X8X24_Typeless
    VK_FORMAT_UNDEFINED, // TODO: X32_Typeless_G8X24_UInt
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_A2B10G10R10_UINT_PACK32,
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_R8G8B8A8_UINT,
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16_UNORM,
    VK_FORMAT_R16G16_UINT,
    VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16_SINT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_X8_D24_UNORM_PACK32,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R8G8_SNORM,
    VK_FORMAT_R8G8_SINT,
    VK_FORMAT_R16_SFLOAT,
    VK_FORMAT_R16_SFLOAT,
    VK_FORMAT_D16_UNORM,
    VK_FORMAT_R16_UNORM,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SNORM,
    VK_FORMAT_R16_SINT,
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R8_SNORM,
    VK_FORMAT_R8_SINT,
    VK_FORMAT_UNDEFINED, // TODO: A8_UNorm
    VK_FORMAT_UNDEFINED, // TODO: R1_UNorm
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
    VK_FORMAT_UNDEFINED, // TODO: R8G8_B8G8_UNorm
    VK_FORMAT_UNDEFINED, // TODO: G8R8_G8B8_UNorm
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
    VK_FORMAT_BC2_UNORM_BLOCK,
    VK_FORMAT_BC2_UNORM_BLOCK,
    VK_FORMAT_BC2_SRGB_BLOCK,
    VK_FORMAT_BC3_UNORM_BLOCK,
    VK_FORMAT_BC3_UNORM_BLOCK,
    VK_FORMAT_BC3_SRGB_BLOCK,
    VK_FORMAT_BC4_UNORM_BLOCK,
    VK_FORMAT_BC4_UNORM_BLOCK,
    VK_FORMAT_BC4_SNORM_BLOCK,
    VK_FORMAT_BC5_UNORM_BLOCK,
    VK_FORMAT_BC5_UNORM_BLOCK,
    VK_FORMAT_BC5_SNORM_BLOCK,
    VK_FORMAT_B5G6R5_UNORM_PACK16,
    VK_FORMAT_B5G5R5A1_UNORM_PACK16,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_UNDEFINED, // TODO: R10G10B10_Xr_Bias_A2_UNorm
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_BC6H_UFLOAT_BLOCK,
    VK_FORMAT_BC6H_UFLOAT_BLOCK,
    VK_FORMAT_BC6H_SFLOAT_BLOCK,
    VK_FORMAT_BC7_UNORM_BLOCK,
    VK_FORMAT_BC7_UNORM_BLOCK,
    VK_FORMAT_BC7_SRGB_BLOCK,
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
    VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
    VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
    VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
    VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
    VK_FORMAT_G8B8G8R8_422_UNORM, // YUY2
    VK_FORMAT_G8_B8R8_2PLANE_420_UNORM, // NV12
};

VkBlendOp RenderToolsVulkan::OperationToVkBlendOp[6] =
{
    VK_BLEND_OP_MAX_ENUM,
    VK_BLEND_OP_ADD, // Add
    VK_BLEND_OP_SUBTRACT, // Subtract
    VK_BLEND_OP_REVERSE_SUBTRACT, // RevSubtract
    VK_BLEND_OP_MIN, // Min
    VK_BLEND_OP_MAX, // Max
};

VkCompareOp RenderToolsVulkan::ComparisonFuncToVkCompareOp[9] =
{
    VK_COMPARE_OP_MAX_ENUM,
    VK_COMPARE_OP_NEVER, // Never
    VK_COMPARE_OP_LESS, // Less
    VK_COMPARE_OP_EQUAL, // Equal
    VK_COMPARE_OP_LESS_OR_EQUAL, // LessEqual
    VK_COMPARE_OP_GREATER, // Grather
    VK_COMPARE_OP_NOT_EQUAL, // NotEqual
    VK_COMPARE_OP_GREATER_OR_EQUAL, // GratherEqual
    VK_COMPARE_OP_ALWAYS, // Always
};

// @formatter:on

#define VKERR(x) case x: sb.Append(TEXT(#x)); break

#if GPU_ENABLE_RESOURCE_NAMING

void RenderToolsVulkan::SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const String& name)
{
#if VK_EXT_debug_utils
    auto str = name.ToStringAnsi();
    SetObjectName(device, objectHandle, objectType, str.Get());
#endif
}

void RenderToolsVulkan::SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const char* name)
{
#if VK_EXT_debug_utils
    // Check for valid function pointer (may not be present if not running in a debugging application)
    if (vkSetDebugUtilsObjectNameEXT != nullptr && name != nullptr && *name != 0)
    {
        VkDebugUtilsObjectNameInfoEXT objectNameInfo;
        ZeroStruct(objectNameInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        objectNameInfo.objectType = objectType;
        objectNameInfo.objectHandle = objectHandle;
        objectNameInfo.pObjectName = name;
        const VkResult result = vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
        LOG_VULKAN_RESULT(result);
    }
#endif
}

#endif

String RenderToolsVulkan::GetVkErrorString(VkResult result)
{
    StringBuilder sb(64);

    // Switch error code
    switch (result)
    {
    VKERR(VK_SUCCESS);
    VKERR(VK_NOT_READY);
    VKERR(VK_TIMEOUT);
    VKERR(VK_EVENT_SET);
    VKERR(VK_EVENT_RESET);
    VKERR(VK_INCOMPLETE);
    VKERR(VK_ERROR_OUT_OF_HOST_MEMORY);
    VKERR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    VKERR(VK_ERROR_INITIALIZATION_FAILED);
    VKERR(VK_ERROR_DEVICE_LOST);
    VKERR(VK_ERROR_MEMORY_MAP_FAILED);
    VKERR(VK_ERROR_LAYER_NOT_PRESENT);
    VKERR(VK_ERROR_EXTENSION_NOT_PRESENT);
    VKERR(VK_ERROR_FEATURE_NOT_PRESENT);
    VKERR(VK_ERROR_INCOMPATIBLE_DRIVER);
    VKERR(VK_ERROR_TOO_MANY_OBJECTS);
    VKERR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    VKERR(VK_ERROR_FRAGMENTED_POOL);
    VKERR(VK_ERROR_OUT_OF_POOL_MEMORY);
    VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
    VKERR(VK_ERROR_SURFACE_LOST_KHR);
    VKERR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    VKERR(VK_SUBOPTIMAL_KHR);
    VKERR(VK_ERROR_OUT_OF_DATE_KHR);
    VKERR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    VKERR(VK_ERROR_VALIDATION_FAILED_EXT);
    VKERR(VK_ERROR_INVALID_SHADER_NV);
#if VK_HEADER_VERSION >= 89
    VKERR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
#endif
    VKERR(VK_ERROR_FRAGMENTATION_EXT);
    VKERR(VK_ERROR_NOT_PERMITTED_EXT);
#if VK_HEADER_VERSION < 140
    VKERR(VK_RESULT_RANGE_SIZE);
#endif
    default:
        sb.AppendFormat(TEXT("0x{0:x}"), static_cast<uint32>(result));
        break;
    }

    return sb.ToString();
}

void RenderToolsVulkan::LogVkResult(VkResult result, const char* file, uint32 line, bool fatal)
{
    ASSERT(result != VK_SUCCESS);

    // Process error and format message
    StringBuilder sb;
    sb.Append(TEXT("Vulkan error: "));
    sb.Append(GetVkErrorString(result));
    if (file)
        sb.Append(TEXT(" at ")).Append(file).Append(':').Append(line);
    const StringView msg(sb.ToStringView());

    // Handle error
    FatalErrorType errorType = FatalErrorType::None;
    if (result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_POOL_MEMORY)
        errorType = FatalErrorType::GPUOutOfMemory;
    else if (result == VK_TIMEOUT)
        errorType = FatalErrorType::GPUHang;
    else if (result == VK_ERROR_DEVICE_LOST || result == VK_ERROR_SURFACE_LOST_KHR || result == VK_ERROR_MEMORY_MAP_FAILED)
        errorType = FatalErrorType::GPUCrash;
    else if (fatal)
        errorType = FatalErrorType::Unknown;
    if (errorType != FatalErrorType::None)
        Platform::Fatal(msg, nullptr, errorType);
#if LOG_ENABLE
    else
        Log::Logger::Write(LogType::Error, msg);
#endif
}

VkAccessFlags RenderToolsVulkan::GetAccess(GPUResourceAccess access)
{
    switch (access)
    {
    case GPUResourceAccess::None:
        return VK_ACCESS_NONE;
    case GPUResourceAccess::CopyRead:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case GPUResourceAccess::CopyWrite:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case GPUResourceAccess::CpuRead:
        return VK_ACCESS_HOST_READ_BIT;
    case GPUResourceAccess::CpuWrite:
        return VK_ACCESS_HOST_WRITE_BIT;
    case GPUResourceAccess::DepthRead:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case GPUResourceAccess::DepthWrite:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case GPUResourceAccess::DepthBuffer:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case GPUResourceAccess::RenderTarget:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case GPUResourceAccess::UnorderedAccess:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case GPUResourceAccess::IndirectArgs:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    case GPUResourceAccess::ShaderReadCompute:
    case GPUResourceAccess::ShaderReadPixel:
    case GPUResourceAccess::ShaderReadNonPixel:
    case GPUResourceAccess::ShaderReadGraphics:
        return VK_ACCESS_SHADER_READ_BIT;
#if !BUILD_RELEASE
    default:
        LOG(Error, "Unsupported GPU Resource Access: {}", (uint32)access);
#endif
    }
    return VK_ACCESS_NONE;
}

VkImageLayout RenderToolsVulkan::GetImageLayout(GPUResourceAccess access)
{
    switch (access)
    {
    case GPUResourceAccess::None:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case GPUResourceAccess::CopyRead:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case GPUResourceAccess::CopyWrite:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case GPUResourceAccess::CpuRead:
    case GPUResourceAccess::CpuWrite:
        return VK_IMAGE_LAYOUT_GENERAL;
    case GPUResourceAccess::DepthRead:
        return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    case GPUResourceAccess::DepthWrite:
    case GPUResourceAccess::DepthBuffer:
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case GPUResourceAccess::RenderTarget:
        return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    case GPUResourceAccess::UnorderedAccess:
    case GPUResourceAccess::ShaderReadCompute:
    case GPUResourceAccess::ShaderReadPixel:
    case GPUResourceAccess::ShaderReadNonPixel:
    case GPUResourceAccess::ShaderReadGraphics:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#if !BUILD_RELEASE
    default:
        LOG(Error, "Unsupported GPU Resource Access: {}", (uint32)access);
#endif
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

bool RenderToolsVulkan::HasExtension(const Array<const char*>& extensions, const char* name)
{
    for (int32 i = 0; i < extensions.Count(); i++)
    {
        const char* extension = extensions[i];
        if (extension && StringUtils::Compare(extension, name) == 0)
            return true;
    }
    return false;
}

#endif
