// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceWebGPU.h"
#include "IncludeWebGPU.h"

#if GRAPHICS_API_WEBGPU

/// <summary>
/// The texture view for Web GPU backend.
/// </summary>
/// <seealso cref="GPUTextureView" />
class GPUTextureViewWebGPU : public GPUTextureView
{
public:
    GPUTextureViewWebGPU()
    {
        Ptr = { nullptr, this };
    }

    ~GPUTextureViewWebGPU()
    {
        Release();
    }

public:
    // Handle to the WebGPU texture object.
    WGPUTexture Texture = nullptr;
    // Handle to the WebGPU texture view object.
    WGPUTextureView View = nullptr;
    // Handle to the WebGPU texture view object for render passes (contains all views).
    WGPUTextureView ViewRender = nullptr;
    bool HasStencil = false;
    bool ReadOnly = false;
    uint32 DepthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    WGPUTextureFormat Format = WGPUTextureFormat_Undefined;
    WGPUTextureSampleType SampleType = WGPUTextureSampleType_Undefined;
    GPUResourceViewPtrWebGPU Ptr;

public:
    using GPUTextureView::Init;
    void Create(WGPUTexture texture, WGPUTextureViewDescriptor const* desc = nullptr);
    void Release();

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)&Ptr;
    }
};

/// <summary>
/// Texture object for Web GPU backend.
/// </summary>
class GPUTextureWebGPU : public GPUResourceWebGPU<GPUTexture>
{
private:
    GPUTextureViewWebGPU _handleArray;
    GPUTextureViewWebGPU _handleVolume;
    GPUTextureViewWebGPU _handleReadOnlyDepth;
    GPUTextureViewWebGPU _handleStencil;
    Array<GPUTextureViewWebGPU> _handlesPerSlice; // [slice]
    Array<Array<GPUTextureViewWebGPU>> _handlesPerMip; // [slice][mip]
#if GPU_ENABLE_RESOURCE_NAMING
    StringAnsi _name;
#endif
    WGPUTextureFormat _format = WGPUTextureFormat_Undefined;
    WGPUTextureViewDimension _viewDimension = WGPUTextureViewDimension_Undefined;
    WGPUTextureUsage _usage = 0;

public:
    GPUTextureWebGPU(GPUDeviceWebGPU* device, const StringView& name)
        : GPUResourceWebGPU<GPUTexture>(device, name)
    {
    }

public:
    // Handle to the WebGPU texture object.
    WGPUTexture Texture = nullptr;

public:
    // [GPUTexture]
    GPUTextureView* View(int32 arrayOrDepthIndex) const override
    {
        return (GPUTextureView*)&_handlesPerSlice[arrayOrDepthIndex];
    }
    GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
    {
        return (GPUTextureView*)&_handlesPerMip[arrayOrDepthIndex][mipMapIndex];
    }
    GPUTextureView* ViewArray() const override
    {
        ASSERT(ArraySize() > 1);
        return (GPUTextureView*)&_handleArray;
    }
    GPUTextureView* ViewVolume() const override
    {
        ASSERT(IsVolume());
        return (GPUTextureView*)&_handleVolume;
    }
    GPUTextureView* ViewReadOnlyDepth() const override
    {
        ASSERT(_desc.Flags & GPUTextureFlags::ReadOnlyDepthView);
        return (GPUTextureView*)&_handleReadOnlyDepth;
    }
    GPUTextureView* ViewStencil() const override
    {
        ASSERT(_desc.Flags & GPUTextureFlags::DepthStencil);
        return (GPUTextureView*)&_handleStencil;
    }
    void* GetNativePtr() const override
    {
        return Texture;
    }
    bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override;

protected:
    // [GPUTexture]
    bool OnInit() override;
    void OnResidentMipsChanged() override;
    void OnReleaseGPU() override;

private:
    void InitHandles();
};

#endif
