// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "ResourceOwnerDX12.h"
#include "GPUDeviceDX12.h"

void ResourceOwnerDX12::initResource(ID3D12Resource* resource, const D3D12_RESOURCE_STATES initialState, const uint32 subresourceCount, bool usePerSubresourceTracking)
{
    ASSERT(resource);
    _resource = resource;
    _subresourcesCount = subresourceCount;
    State.Initialize(subresourceCount, initialState, usePerSubresourceTracking);
}

void ResourceOwnerDX12::initResource(const D3D12_RESOURCE_STATES initialState, const uint32 subresourceCount, bool usePerSubresourceTracking)
{
    // Note: this is used by the dynamic buffers (which don't own resource but just part of other one)
    _subresourcesCount = subresourceCount;
    State.Initialize(subresourceCount, initialState, usePerSubresourceTracking);
}

void ResourceOwnerDX12::releaseResource(uint32 safeFrameCount)
{
    if (_resource)
    {
        OnRelease(this);

        auto resource = _resource;
        _resource = nullptr;
        _subresourcesCount = 0;
        State.Release();

        ((GPUDeviceDX12*)GPUDevice::Instance)->AddResourceToLateRelease(resource, safeFrameCount);
    }
}

#endif
