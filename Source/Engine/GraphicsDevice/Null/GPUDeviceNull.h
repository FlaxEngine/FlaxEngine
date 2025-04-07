// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUDevice.h"

class Engine;
class GPUContextNull;
class GPUAdapterNull;
class GPUSwapChainNull;

/// <summary>
/// Implementation of Graphics Device for Null backend.
/// </summary>
class GPUDeviceNull : public GPUDevice
{
    friend GPUContextNull;
    friend GPUSwapChainNull;

private:

    GPUContextNull* _mainContext;
    GPUAdapterNull* _adapter;

public:

    static GPUDevice* Create();
    GPUDeviceNull();
    ~GPUDeviceNull();

public:

    // [GPUDevice]
    GPUContext* GetMainContext() override;
    GPUAdapter* GetAdapter() const override;
    void* GetNativePtr() const override;
    bool Init() override;
    bool LoadContent() override;
    void Draw() override;
    void Dispose() override;
    void WaitForGPU() override;
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

extern GPUDevice* CreateGPUDeviceNull();

#endif
