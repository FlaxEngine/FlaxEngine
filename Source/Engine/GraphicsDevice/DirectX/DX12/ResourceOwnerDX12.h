// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUResourceState.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

namespace D3D12MA
{
    class Allocation;
}
class GPUResource;
class GPUContextDX12;
class GPUAsyncContextDX12;

/// <summary>
/// Default amount of frames to wait until resource delete.
/// </summary>
#define DX12_RESOURCE_DELETE_SAFE_FRAMES_COUNT 30

/// <summary>
/// Custom resource state used to indicate invalid state (useful for debugging resource tracking issues).
/// </summary>
#define D3D12_RESOURCE_STATE_CORRUPT (D3D12_RESOURCE_STATES)-1

/// <summary>
/// Tracking of per-resource or per-subresource state for D3D12 resources that require to issue resource access barriers during rendering.
/// </summary>
class ResourceStateDX12 : public GPUResourceState<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATE_CORRUPT>
{
public:

    /// <summary>
    /// Returns true if resource state transition is needed in order to use resource in given state.
    /// </summary>
    /// <param name="before">The current resource state.</param>
    /// <param name="after">the destination resource state.</param>
    /// <returns>True if need to perform a transition, otherwise false.</returns>
    FORCE_INLINE static bool IsTransitionNeeded(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES& after)
    {
        if (before == D3D12_RESOURCE_STATE_DEPTH_WRITE && after == D3D12_RESOURCE_STATE_DEPTH_READ)
			return false;
        if (after == D3D12_RESOURCE_STATE_COMMON)
			return before != D3D12_RESOURCE_STATE_COMMON;
        if (after == D3D12_RESOURCE_STATE_DEPTH_READ)
            return ~(before & (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)) == 0;
        const D3D12_RESOURCE_STATES combined = before | after;
		if ((combined & (D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)) == combined)
			after = combined;
		return before != after;
    }
};

/// <summary>
/// Base class for objects in DirectX 12 layer that can own a resource
/// </summary>
class ResourceOwnerDX12
{
    friend GPUContextDX12;
    friend GPUAsyncContextDX12;

protected:
    D3D12MA::Allocation* _allocation = nullptr;
    ID3D12Resource* _resource = nullptr;
    uint32 _subresourcesCount = 0;

public:
    /// <summary>
    /// The resource state tracking helper. Used for resource barriers.
    /// </summary>
    ResourceStateDX12 State;

public:
    /// <summary>
    /// Gets the subresources count.
    /// </summary>
    FORCE_INLINE uint32 GetSubresourcesCount() const
    {
        return _subresourcesCount;
    }

    /// <summary>
    /// Gets DirectX 12 resource object handle
    /// </summary>
    FORCE_INLINE ID3D12Resource* GetResource() const
    {
        return _resource;
    }

    /// <summary>
    /// Gets resource owner object as a GPUResource type or returns null if cannot perform cast.
    /// </summary>
    virtual GPUResource* AsGPUResource() const = 0;

protected:
    FORCE_INLINE void initResource(ID3D12Resource* resource, const D3D12_RESOURCE_STATES initialState, const D3D12_RESOURCE_DESC& desc, bool usePerSubresourceTracking = false)
    {
        initResource(resource, initialState, desc.DepthOrArraySize * desc.MipLevels, usePerSubresourceTracking);
    }

    void initResource(ID3D12Resource* resource, const D3D12_RESOURCE_STATES initialState, const uint32 subresourceCount, bool usePerSubresourceTracking = false);
    void initResource(const D3D12_RESOURCE_STATES initialState, const uint32 subresourceCount, bool usePerSubresourceTracking = false);
    void releaseResource(uint32 safeFrameCount = DX12_RESOURCE_DELETE_SAFE_FRAMES_COUNT);
};

#endif
