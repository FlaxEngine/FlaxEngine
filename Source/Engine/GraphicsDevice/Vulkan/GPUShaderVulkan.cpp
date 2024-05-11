// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUShaderVulkan.h"
#include "GPUContextVulkan.h"
#include "GPUShaderProgramVulkan.h"
#include "RenderToolsVulkan.h"
#include "CmdBufferVulkan.h"
#include "Types.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

#if PLATFORM_DESKTOP
#define VULKAN_UNIFORM_RING_BUFFER_SIZE (24 * 1024 * 1024)
#else
#define VULKAN_UNIFORM_RING_BUFFER_SIZE (8 * 1024 * 1024)
#endif

UniformBufferUploaderVulkan::UniformBufferUploaderVulkan(GPUDeviceVulkan* device)
    : GPUResourceVulkan(device, TEXT("Uniform Buffer Uploader"))
    , _size(VULKAN_UNIFORM_RING_BUFFER_SIZE)
    , _offset(0)
    , _mapped(nullptr)
    , _fenceCmdBuffer(nullptr)
    , _fenceCounter(0)
{
    _minAlignment = (uint32)device->PhysicalDeviceLimits.minUniformBufferOffsetAlignment;

    // Setup buffer description
    VkBufferCreateInfo bufferInfo;
    RenderToolsVulkan::ZeroStruct(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    bufferInfo.size = _size;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    // Create buffer
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VkResult result = vmaCreateBuffer(_device->Allocator, &bufferInfo, &allocInfo, &_buffer, &_allocation, nullptr);
    LOG_VULKAN_RESULT(result);
    _memoryUsage = bufferInfo.size;

    // Map buffer
    result = vmaMapMemory(_device->Allocator, _allocation, (void**)&_mapped);
    LOG_VULKAN_RESULT(result);
}

UniformBufferUploaderVulkan::Allocation UniformBufferUploaderVulkan::Allocate(uint64 size, uint32 alignment, GPUContextVulkan* context)
{
    alignment = Math::Max(_minAlignment, alignment);
    uint64 offset = Math::AlignUp<uint64>(_offset, alignment);

    // Check if wrap around ring buffer
    if (offset + size >= _size)
    {
        auto cmdBuffer = context->GetCmdBufferManager()->GetActiveCmdBuffer();
        if (_fenceCmdBuffer && _fenceCounter == cmdBuffer->GetFenceSignaledCounter())
        {
            LOG(Error, "Wrapped around the ring buffer! Need to wait on the GPU!");
            context->Flush();
            cmdBuffer = context->GetCmdBufferManager()->GetActiveCmdBuffer();
        }

        offset = 0;
        _offset = size;

        _fenceCmdBuffer = cmdBuffer;
        _fenceCounter = cmdBuffer->GetSubmittedFenceCounter();
    }
    else
    {
        // Move within the buffer
        _offset = offset + size;
    }

    Allocation result;
    result.Offset = offset;
    result.Size = size;
    result.Buffer = _buffer;
    result.CPUAddress = _mapped + offset;
    return result;
}

void UniformBufferUploaderVulkan::OnReleaseGPU()
{
    if (_allocation != VK_NULL_HANDLE)
    {
        if (_mapped)
        {
            vmaUnmapMemory(_device->Allocator, _allocation);
            _mapped = nullptr;
        }
        vmaDestroyBuffer(_device->Allocator, _buffer, _allocation);
        _buffer = VK_NULL_HANDLE;
        _allocation = VK_NULL_HANDLE;
    }
}

GPUShaderProgram* GPUShaderVulkan::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream)
{
    // Extract the SPIR-V shader header from the cache
    SpirvShaderHeader* header = (SpirvShaderHeader*)cacheBytes;
    cacheBytes += sizeof(SpirvShaderHeader);
    cacheSize -= sizeof(SpirvShaderHeader);

    // Extract the SPIR-V bytecode
    BytesContainer spirv;
    ASSERT(header->Type == SpirvShaderHeader::Types::Raw);
    spirv.Link(cacheBytes, cacheSize);

    // Create shader module from SPIR-V bytecode
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo createInfo;
    RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    createInfo.codeSize = (size_t)spirv.Length();
    createInfo.pCode = (const uint32_t*)spirv.Get();
#if VULKAN_USE_VALIDATION_CACHE
    VkShaderModuleValidationCacheCreateInfoEXT validationInfo;
    if (_device->ValidationCache != VK_NULL_HANDLE)
    {
        RenderToolsVulkan::ZeroStruct(validationInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT);
        validationInfo.validationCache = _device->ValidationCache;
        createInfo.pNext = &validationInfo;
    }
#endif
    VALIDATE_VULKAN_RESULT(vkCreateShaderModule(_device->Device, &createInfo, nullptr, &shaderModule));
#if GPU_ENABLE_RESOURCE_NAMING
    VK_SET_DEBUG_NAME(_device, shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, initializer.Name.GetText());
#endif
#if 0
    // Helper debug print of the shader module handle (Debug Layer sometimes doesn't print the debug name but the handle)
    LOG(Info, "VK_OBJECT_TYPE_SHADER_MODULE=0x{0:x}, {1}", (uintptr)shaderModule, String(initializer.Name.GetText()));
#endif

    GPUShaderProgram* shader = nullptr;
    switch (type)
    {
    case ShaderStage::Vertex:
    {
        // Create object
        auto vsShader = New<GPUShaderProgramVSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule);
        shader = vsShader;
        VkPipelineVertexInputStateCreateInfo& inputState = vsShader->VertexInputState;
        VkVertexInputBindingDescription* vertexBindingDescriptions = vsShader->VertexBindingDescriptions;
        VkVertexInputAttributeDescription* vertexAttributeDescriptions = vsShader->VertexAttributeDescriptions;
        RenderToolsVulkan::ZeroStruct(inputState, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        for (int32 i = 0; i < VERTEX_SHADER_MAX_INPUT_ELEMENTS; i++)
        {
            vertexBindingDescriptions[i].binding = i;
            vertexBindingDescriptions[i].stride = 0;
            vertexBindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        // Load Input Layout (it may be empty)
        byte inputLayoutSize;
        stream.ReadByte(&inputLayoutSize);
        ASSERT(inputLayoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
        uint32 attributesCount = inputLayoutSize;
        uint32 bindingsCount = 0;
        int32 offset = 0;
        for (int32 a = 0; a < inputLayoutSize; a++)
        {
            // Read description
            GPUShaderProgramVS::InputElement inputElement;
            stream.Read(inputElement);

            const auto size = PixelFormatExtensions::SizeInBytes((PixelFormat)inputElement.Format);
            if (inputElement.AlignedByteOffset != INPUT_LAYOUT_ELEMENT_ALIGN)
                offset = inputElement.AlignedByteOffset;

            auto& vertexBindingDescription = vertexBindingDescriptions[inputElement.InputSlot];
            vertexBindingDescription.binding = inputElement.InputSlot;
            vertexBindingDescription.stride = Math::Max(vertexBindingDescription.stride, (uint32_t)(offset + size));
            vertexBindingDescription.inputRate = inputElement.InputSlotClass == INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            ASSERT(inputElement.InstanceDataStepRate == 0 || inputElement.InstanceDataStepRate == 1);

            auto& vertexAttributeDescription = vertexAttributeDescriptions[a];
            vertexAttributeDescription.location = a;
            vertexAttributeDescription.binding = inputElement.InputSlot;
            vertexAttributeDescription.format = RenderToolsVulkan::ToVulkanFormat((PixelFormat)inputElement.Format);
            vertexAttributeDescription.offset = offset;

            bindingsCount = Math::Max(bindingsCount, (uint32)inputElement.InputSlot + 1);
            offset += size;
        }

        inputState.vertexBindingDescriptionCount = bindingsCount;
        inputState.pVertexBindingDescriptions = vertexBindingDescriptions;

        inputState.vertexAttributeDescriptionCount = attributesCount;
        inputState.pVertexAttributeDescriptions = vertexAttributeDescriptions;

        break;
    }
#if GPU_ALLOW_TESSELLATION_SHADERS
    case ShaderStage::Hull:
    {
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);
        shader = New<GPUShaderProgramHSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule, controlPointsCount);
        break;
    }
    case ShaderStage::Domain:
    {
        shader = New<GPUShaderProgramDSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule);
        break;
    }
#else
    case ShaderStage::Hull:
    {
        int32 controlPointsCount;
        stream.ReadInt32(&controlPointsCount);
        break;
    }
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    case ShaderStage::Geometry:
    {
        shader = New<GPUShaderProgramGSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule);
        break;
    }
#endif
    case ShaderStage::Pixel:
    {
        shader = New<GPUShaderProgramPSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule);
        break;
    }
    case ShaderStage::Compute:
    {
        shader = New<GPUShaderProgramCSVulkan>(_device, initializer, header->DescriptorInfo, shaderModule);
        break;
    }
    }
    return shader;
}

#endif
