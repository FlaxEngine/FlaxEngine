// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUAdapter.h"

/// <summary>
/// Graphics Device adapter implementation for Null backend.
/// </summary>
class GPUAdapterNull : public GPUAdapter
{
public:
    // [GPUAdapter]
    bool IsValid() const override
    {
        return true;
    }
    void* GetNativePtr() const override
    {
        return nullptr;
    }
    uint32 GetVendorId() const override
    {
        return 0;
    }
    String GetDescription() const override
    {
        return TEXT("Null");
    }
    Version GetDriverVersion() const override
    {
        return Version(0, 0);
    }
};

#endif
