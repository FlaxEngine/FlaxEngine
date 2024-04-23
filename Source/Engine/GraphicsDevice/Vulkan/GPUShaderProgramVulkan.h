// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "IncludeVulkanHeaders.h"
#include "Types.h"

#if GRAPHICS_API_VULKAN

class GPUDeviceVulkan;
class ComputePipelineStateVulkan;

/// <summary>
/// Shaders base class for Vulkan backend.
/// </summary>
template<typename BaseType>
class GPUShaderProgramVulkan : public BaseType
{
protected:
    GPUDeviceVulkan* _device;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : _device(device)
        , ShaderModule(shaderModule)
        , DescriptorInfo(descriptorInfo)
    {
        BaseType::Init(initializer);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgramVulkan"/> class.
    /// </summary>
    ~GPUShaderProgramVulkan()
    {
        if (ShaderModule)
        {
            _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::ShaderModule, ShaderModule);
        }
    }

public:
    /// <summary>
    /// The Vulkan shader module.
    /// </summary>
    VkShaderModule ShaderModule;

    /// <summary>
    /// The descriptor information container.
    /// </summary>
    SpirvShaderDescriptorInfo DescriptorInfo;

public:
    // [BaseType]
    uint32 GetBufferSize() const override
    {
        return 0;
    }

    void* GetBufferHandle() const override
    {
        return (void*)ShaderModule;
    }
};

/// <summary>
/// Vertex Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramVSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramVS>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramVSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramVSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
    }

public:
    VkPipelineVertexInputStateCreateInfo VertexInputState;
    VkVertexInputBindingDescription VertexBindingDescriptions[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
    VkVertexInputAttributeDescription VertexAttributeDescriptions[VERTEX_SHADER_MAX_INPUT_ELEMENTS];

public:
    // [GPUShaderProgramVulkan]
    void* GetInputLayout() const override
    {
        return (void*)&VertexInputState;
    }

    byte GetInputLayoutSize() const override
    {
        return 0;
    }
};

#if GPU_ALLOW_TESSELLATION_SHADERS
/// <summary>
/// Hull Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramHSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramHS>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramHSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    /// <param name="controlPointsCount">The control points used by the hull shader for processing.</param>
    GPUShaderProgramHSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule, int32 controlPointsCount)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
        _controlPointsCount = controlPointsCount;
    }
};

/// <summary>
/// Domain Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramDSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramDS>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramDSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramDSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
    }
};
#endif

#if GPU_ALLOW_GEOMETRY_SHADERS
/// <summary>
/// Geometry Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramGSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramGS>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramGSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramGSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
    }
};
#endif

/// <summary>
/// Pixel Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramPSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramPS>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramPSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramPSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
    }
};

/// <summary>
/// Compute Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramCSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramCS>
{
private:
    ComputePipelineStateVulkan* _pipelineState;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramCSVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="initializer">The program initialization data.</param>
    /// <param name="descriptorInfo">The program descriptors usage info.</param>
    /// <param name="shaderModule">The shader module object.</param>
    GPUShaderProgramCSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
        , _pipelineState(nullptr)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgramCSVulkan"/> class.
    /// </summary>
    ~GPUShaderProgramCSVulkan();

public:
    /// <summary>
    /// Gets the state of the pipeline for the compute shader execution or creates a new one if missing.
    /// </summary>
    /// <returns>The compute pipeline state.</returns>
    ComputePipelineStateVulkan* GetOrCreateState();
};

#endif
