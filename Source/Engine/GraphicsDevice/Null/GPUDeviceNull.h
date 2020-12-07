// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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

    // Create new graphics device (returns null if failed)
    // @returns Created device or null
    static GPUDevice* Create();

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUDeviceNull"/> class.
    /// </summary>
    GPUDeviceNull();

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUDeviceNull"/> class.
    /// </summary>
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
    GPUSwapChain* CreateSwapChain(Window* window) override;
};

extern GPUDevice* CreateGPUDeviceNull();

#endif
