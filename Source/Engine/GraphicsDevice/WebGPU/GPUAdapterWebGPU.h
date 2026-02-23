// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/GPUAdapter.h"
#include "Engine/Utilities/StringConverter.h"
#include "IncludeWebGPU.h"

/// <summary>
/// Graphics Device adapter implementation for Web GPU backend.
/// </summary>
class GPUAdapterWebGPU : public GPUAdapter
{
public:
    WGPUAdapter Adapter;
    WGPUAdapterInfo Info;

    GPUAdapterWebGPU(WGPUAdapter adapter)
        : Adapter(adapter)
        , Info(WGPU_ADAPTER_INFO_INIT)
    {
        wgpuAdapterGetInfo(adapter, &Info);
    }

    ~GPUAdapterWebGPU()
    {
        wgpuAdapterRelease(Adapter);
    }

public:
    // [GPUAdapter]
    bool IsValid() const override
    {
        return true;
    }
    void* GetNativePtr() const override
    {
        return Adapter;
    }
    uint32 GetVendorId() const override
    {
        return Info.vendorID;
    }
    String GetDescription() const override
    {
        return String::Format(TEXT("{} {} {} {}"), WEBGPU_TO_STR(Info.vendor), WEBGPU_TO_STR(Info.architecture), WEBGPU_TO_STR(Info.device), WEBGPU_TO_STR(Info.description)).TrimTrailing();
    }
    Version GetDriverVersion() const override
    {
        return Platform::GetSystemVersion();
    }
};

#endif
