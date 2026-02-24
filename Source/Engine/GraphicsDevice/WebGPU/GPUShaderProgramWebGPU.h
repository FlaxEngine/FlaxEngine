// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include <webgpu/webgpu.h>

/// <summary>
/// Shaders base class for Web GPU backend.
/// </summary>
template<typename BaseType>
class GPUShaderProgramWebGPU : public BaseType
{
public:
    GPUShaderProgramWebGPU(const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, WGPUShaderModule shaderModule)
        : DescriptorInfo(descriptorInfo)
        , ShaderModule(shaderModule)
    {
        BaseType::Init(initializer);
    }

    ~GPUShaderProgramWebGPU()
    {
        wgpuShaderModuleRelease(ShaderModule);
    }

public:
    SpirvShaderDescriptorInfo DescriptorInfo;
    WGPUShaderModule ShaderModule;

public:
    // [BaseType]
    uint32 GetBufferSize() const override
    {
        return 0;
    }
    void* GetBufferHandle() const override
    {
        return ShaderModule;
    }
};

/// <summary>
/// Vertex Shader for Web GPU backend.
/// </summary>
class GPUShaderProgramVSWebGPU : public GPUShaderProgramWebGPU<GPUShaderProgramVS>
{
public:
    GPUShaderProgramVSWebGPU(const GPUShaderProgramInitializer& initializer, GPUVertexLayout* inputLayout, GPUVertexLayout* vertexLayout, const SpirvShaderDescriptorInfo& descriptorInfo, WGPUShaderModule shaderModule)
        : GPUShaderProgramWebGPU(initializer, descriptorInfo, shaderModule)
    {
        InputLayout = inputLayout;
        Layout = vertexLayout;
    }
};

/// <summary>
/// Pixel Shader for Web GPU backend.
/// </summary>
class GPUShaderProgramPSWebGPU : public GPUShaderProgramWebGPU<GPUShaderProgramPS>
{
public:
    GPUShaderProgramPSWebGPU(const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, WGPUShaderModule shaderModule)
        : GPUShaderProgramWebGPU(initializer, descriptorInfo, shaderModule)
    {
    }
};

#endif
