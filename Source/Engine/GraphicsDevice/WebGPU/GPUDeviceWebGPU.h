// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUResource.h"
#include "IncludeWebGPU.h"

class GPUContextWebGPU;
class GPUAdapterWebGPU;
class GPUTextureWebGPU;
class GPUSamplerWebGPU;

namespace GPUBindGroupsWebGPU
{
    enum Stage
    {
        // Vertex shader stage
        Vertex = 0,
        // Pixel shader stage
        Pixel = 1,
        // Graphics pipeline stages count
        GraphicsMax,
        // Compute pipeline slot
        Compute = 0,
        // The maximum amount of slots for all stages
        Max = GraphicsMax,
    };
};

/// <summary>
/// Pool for uploading data to GPU buffers. It manages large buffers and suballocates for multiple small updates, minimizing the number of buffer creations and copies.
/// </summary>
class GPUDataUploaderWebGPU
{
    friend class GPUDeviceWebGPU;
private:
    struct Entry
    {
        WGPUBuffer Buffer;
        uint32 Size;
        uint32 ActiveOffset;
        uint64 ActiveFrame;
        WGPUBufferUsage Usage;
    };

    uint64 _frame = 0;
    WGPUDevice _device;
    Array<Entry> _entries;

public:
    struct Allocation
    {
        WGPUBuffer Buffer = nullptr;
        uint32 Offset = 0;
    };

    Allocation Allocate(uint32 size, WGPUBufferUsage usage, uint32 alignment = 16);
    void DrawBegin();
    void ReleaseGPU();
};

/// <summary>
/// Implementation of Graphics Device for Web GPU backend.
/// </summary>
class GPUDeviceWebGPU : public GPUDevice
{
private:
    GPUContextWebGPU* _mainContext = nullptr;

public:
    GPUDeviceWebGPU(WGPUInstance instance, GPUAdapterWebGPU* adapter);
    ~GPUDeviceWebGPU();

public:
    GPUAdapterWebGPU* Adapter = nullptr;
    WGPUInstance WebGPUInstance;
    WGPUDevice Device = nullptr;
    WGPUQueue Queue = nullptr;
    GPUSamplerWebGPU* DefaultSamplers[6] = {};
    GPUTextureWebGPU* DefaultTexture = nullptr;
    GPUDataUploaderWebGPU DataUploader;
    uint32 MinUniformBufferOffsetAlignment = 1;

public:
    // [GPUDeviceDX]
    GPUContext* GetMainContext() override
    {
        return (GPUContext*)_mainContext;
    }
    GPUAdapter* GetAdapter() const override
    {
        return (GPUAdapter*)Adapter;
    }
    void* GetNativePtr() const override
    {
        return Device;
    }
    bool Init() override;
    void DrawBegin() override;
    void Dispose() override;
    void WaitForGPU() override;
    bool GetQueryResult(uint64 queryID, uint64& result, bool wait = false) override;
    GPUTexture* CreateTexture(const StringView& name) override;
    GPUShader* CreateShader(const StringView& name) override;
    GPUPipelineState* CreatePipelineState() override;
    GPUTimerQuery* CreateTimerQuery() override;
    GPUBuffer* CreateBuffer(const StringView& name) override;
    GPUSampler* CreateSampler() override;
    GPUVertexLayout* CreateVertexLayout(const VertexElements& elements, bool explicitOffsets) override;
    GPUSwapChain* CreateSwapChain(Window* window) override;
    GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name) override;
};

/// <summary>
/// GPU resource implementation for Web GPU backend.
/// </summary>
template<class BaseType>
class GPUResourceWebGPU : public GPUResourceBase<GPUDeviceWebGPU, BaseType>
{
public:
    GPUResourceWebGPU(GPUDeviceWebGPU* device, const StringView& name) noexcept
        : GPUResourceBase<GPUDeviceWebGPU, BaseType>(device, name)
    {
    }
};

struct GPUResourceViewPtrWebGPU
{
    class GPUBufferViewWebGPU* BufferView;
    class GPUTextureViewWebGPU* TextureView;
};

extern GPUDevice* CreateGPUDeviceWebGPU();

#endif
