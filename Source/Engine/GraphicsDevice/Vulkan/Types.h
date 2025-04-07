// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_VK_SHADER_COMPILER || GRAPHICS_API_VULKAN

#include "Engine/Graphics/PixelFormat.h"

#if GRAPHICS_API_VULKAN
#include "Engine/GraphicsDevice/Vulkan/IncludeVulkanHeaders.h"
#else
// Hack to use VkDescriptorType if Vulkan runtime is not used
typedef enum VkDescriptorType {
    VK_DESCRIPTOR_TYPE_SAMPLER = 0,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT = 1000138000,
    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV = 1000165000,
    VK_DESCRIPTOR_TYPE_BEGIN_RANGE = VK_DESCRIPTOR_TYPE_SAMPLER,
    VK_DESCRIPTOR_TYPE_END_RANGE = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
    VK_DESCRIPTOR_TYPE_RANGE_SIZE = (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT - VK_DESCRIPTOR_TYPE_SAMPLER + 1),
    VK_DESCRIPTOR_TYPE_MAX_ENUM = 0x7FFFFFFF
} VkDescriptorType;
#endif

enum class SpirvShaderResourceType
{
    Unknown = 0,
    ConstantBuffer = 1,
    Buffer = 2,
    Sampler = 3,
    Texture1D = 4,
    Texture2D = 5,
    Texture3D = 6,
    TextureCube = 7,
    Texture1DArray = 8,
    Texture2DArray = 9,
};

enum class SpirvShaderResourceBindingType : byte
{
    INVALID = 0,
    CB = 1,
    SAMPLER = 2,
    SRV = 3,
    UAV = 4,
    MAX
};

struct SpirvShaderDescriptorInfo
{
    enum
    {
        MaxDescriptors = 64,
    };

    /// <summary>
    /// A single descriptor data.
    /// </summary>
    struct Descriptor
    {
        /// <summary>
        /// The binding slot (the descriptors slot).
        /// </summary>
        byte Binding;

        /// <summary>
        /// The layout slot (the descriptors set slot).
        /// </summary>
        byte Set;

        /// <summary>
        /// The input slot (the pipeline slot).
        /// </summary>
        byte Slot;

        /// <summary>
        /// The resource binding type (the graphics pipeline abstraction binding layer type).
        /// </summary>
        SpirvShaderResourceBindingType BindingType;

        /// <summary>
        /// The Vulkan descriptor type.
        /// </summary>
        VkDescriptorType DescriptorType;

        /// <summary>
        /// The resource type.
        /// </summary>
        SpirvShaderResourceType ResourceType;

        /// <summary>
        /// The resource format.
        /// </summary>
        PixelFormat ResourceFormat;

        /// <summary>
        /// The amount of slots used by the descriptor (eg. array of textures size).
        /// </summary>
        uint32 Count;
    };

    uint16 ImageInfosCount;
    uint16 BufferInfosCount;
    uint32 TexelBufferViewsCount;
    uint32 DescriptorTypesCount;
    Descriptor DescriptorTypes[MaxDescriptors];
};

struct SpirvShaderHeader
{
    enum class Types
    {
        /// <summary>
        /// The raw SPIR-V byte code.
        /// </summary>
        Raw = 0,
    };

    /// <summary>
    /// The data type.
    /// </summary>
    Types Type;

    /// <summary>
    /// The shader descriptors usage information.
    /// </summary>
    SpirvShaderDescriptorInfo DescriptorInfo;
};

#endif
