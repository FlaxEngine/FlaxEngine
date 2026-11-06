// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#include "Engine/Core/Core.h"
#include "Engine/Graphics/GPUAdapter.h"
#include "IncludeDirectXHeaders.h"

/// <summary>
/// Graphics Device adapter implementation for DirectX backend.
/// </summary>
class GPUAdapterDX : public GPUAdapter
{
public:
    int32 Index = INVALID_INDEX;
    D3D_FEATURE_LEVEL MaxFeatureLevel;
    DXGI_ADAPTER_DESC Description;
    Version DriverVersion = Version(0, 0);

public:
    void GetDriverVersion();

    // Packs the locally unique identifier (LUID) into a single value (see GPUDeviceCreationHints::RequiredAdapterLuid).
    static uint64 PackLuid(const LUID& luid)
    {
        return ((uint64)(uint32)luid.HighPart << 32) | (uint64)luid.LowPart;
    }

    // Gets the locally unique identifier (LUID) of the adapter packed into a single value.
    uint64 GetLuid() const
    {
        return PackLuid(Description.AdapterLuid);
    }

    // Selects the adapter required by the graphics device creation hints (if any). Keeps the selected adapter if the required one is missing.
    static void SelectRequired(const Array<GPUAdapterDX>& adapters, GPUAdapterDX& selectedAdapter);

private:
    void SetDriverVersion(Version& ver);

public:
    // [GPUAdapter]
    bool IsValid() const override
    {
        return Index != INVALID_INDEX && MaxFeatureLevel != static_cast<D3D_FEATURE_LEVEL>(0);
    }
    void* GetNativePtr() const override
    {
        return (void*)(intptr)Index;
    }
    uint32 GetVendorId() const override
    {
        return (uint32)Description.VendorId;
    }
    String GetDescription() const override
    {
        return Description.Description;
    }
    Version GetDriverVersion() const override
    {
        return DriverVersion;
    }
};

#endif
