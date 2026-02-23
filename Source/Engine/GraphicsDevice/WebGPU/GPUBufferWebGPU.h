// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/GPUBuffer.h"
#include "GPUDeviceWebGPU.h"

/// <summary>
/// The buffer view for Web GPU backend.
/// </summary>
/// <seealso cref="GPUBufferView" />
class GPUBufferViewWebGPU : public GPUBufferView
{
public:
    // Handle to the WebGPU buffer object.
    WGPUBuffer Buffer = nullptr;
    GPUResourceViewPtrWebGPU Ptr;

    void Set(GPUBuffer* parent, WGPUBuffer buffer)
    {
        _parent = parent;
        Buffer = buffer;
        Ptr = { this, nullptr };
    }

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)&Ptr;
    }
};

/// <summary>
/// GPU buffer for Web GPU backend.
/// </summary>
/// <seealso cref="GPUResourceWebGPU" />
class GPUBufferWebGPU : public GPUResourceWebGPU<GPUBuffer>
{
private:
    GPUBufferViewWebGPU _view;
    bool _mapped = false;
#if GPU_ENABLE_RESOURCE_NAMING
    StringAnsi _name;
#endif

public:
    GPUBufferWebGPU(GPUDeviceWebGPU* device, const StringView& name);

public:
    // Handle to the WebGPU buffer object.
    WGPUBuffer Buffer = nullptr;

public:
    // [GPUBuffer]
    GPUBufferView* View() const override;
    void* Map(GPUResourceMapMode mode) override;
    void Unmap() override;

protected:
    // [GPUBuffer]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
