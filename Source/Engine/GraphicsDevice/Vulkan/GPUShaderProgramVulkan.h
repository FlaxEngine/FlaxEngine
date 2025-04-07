// Copyright (c) Wojciech Figat. All rights reserved.

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
    GPUShaderProgramVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : _device(device)
        , ShaderModule(shaderModule)
        , DescriptorInfo(descriptorInfo)
    {
        BaseType::Init(initializer);
    }

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
    GPUShaderProgramVSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule, GPUVertexLayout* inputLayout, GPUVertexLayout* vertexLayout)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
    {
        InputLayout = inputLayout;
        Layout = vertexLayout;
    }
};

#if GPU_ALLOW_TESSELLATION_SHADERS
/// <summary>
/// Hull Shader for Vulkan backend.
/// </summary>
class GPUShaderProgramHSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramHS>
{
public:
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
    GPUShaderProgramCSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
        : GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
        , _pipelineState(nullptr)
    {
    }

    ~GPUShaderProgramCSVulkan();

public:
    /// <summary>
    /// Gets the state of the pipeline for the compute shader execution or creates a new one if missing.
    /// </summary>
    ComputePipelineStateVulkan* GetOrCreateState();
};

#endif
