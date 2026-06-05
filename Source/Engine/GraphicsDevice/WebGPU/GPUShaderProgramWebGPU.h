// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Engine/GraphicsDevice/Vulkan/Types.h"
#include <webgpu/webgpu.h>

/// <summary>
/// Bundle of the current bound state to the Web GPU context (used to properly handle different texture layouts or samplers when building bind group layout).
/// </summary>
struct GPUContextBindingsWebGPU
{
    GPUResourceView** ShaderResources; // [GPU_MAX_SR_BINDED]
};

/// <summary>
/// Batch of bind group descriptions for the layout. Used as a key for caching created bind groups.
/// </summary>
struct GPUBindGroupKeyWebGPU
{
    uint32 Hash;
    WGPUBindGroupLayout Layout;
    mutable uint64 LastFrameUsed;
    WGPUBindGroupEntry Entries[64];
    uint8 EntriesCount;
    uint32 Versions[64]; // Versions of descriptors used to differentiate when texture residency gets changed

    bool operator==(const GPUBindGroupKeyWebGPU& other) const;
};

/// <summary>
/// Reusable utility for caching bind group objects. Handles reusing bind groups for the same key and releasing them when they are not used for a long time (based on the frame number).
/// </summary>
struct GPUBindGroupCacheWebGPU
{
private:
    uint64 _lastFrameBindGroupsGC = 0;
    Dictionary<GPUBindGroupKeyWebGPU, WGPUBindGroup> _bindGroups; // TODO: try using LRU cache

public:
    WGPUBindGroup Get(WGPUDevice device, GPUBindGroupKeyWebGPU& key, const StringAnsiView& debugName, uint64 gcFrames = 50);
};

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

/// <summary>
/// Compute Shader for Web GPU backend.
/// </summary>
class GPUShaderProgramCSWebGPU : public GPUShaderProgramWebGPU<GPUShaderProgramCS>
{
private:
    WGPUComputePipeline _pipeline = nullptr;
    WGPUBindGroupLayout _bindGroupLayout = nullptr;
    GPUBindGroupCacheWebGPU _bindGroupCache;

public:
    GPUShaderProgramCSWebGPU(const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, WGPUShaderModule shaderModule)
        : GPUShaderProgramWebGPU(initializer, descriptorInfo, shaderModule)
    {
    }

    ~GPUShaderProgramCSWebGPU()
    {
        if (_bindGroupLayout)
            wgpuBindGroupLayoutRelease(_bindGroupLayout);
        if (_pipeline)
            wgpuComputePipelineRelease(_pipeline);
    }

public:
    // Gets the pipeline.
    WGPUComputePipeline GetPipeline(WGPUDevice device, const GPUContextBindingsWebGPU& bindings, WGPUBindGroupLayout& resultBindGroupLayout);

    // Gets the bind group for the given key (unhashed). Bind groups are cached and reused for the same key.
    FORCE_INLINE WGPUBindGroup GetBindGroup(WGPUDevice device, GPUBindGroupKeyWebGPU& key)
    {
        return _bindGroupCache.Get(device, key, _name, 60 * 60);
    }
};

#endif
