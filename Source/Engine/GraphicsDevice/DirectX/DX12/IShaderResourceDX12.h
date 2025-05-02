// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ResourceOwnerDX12.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

/// <summary>
/// Interface for objects that can be bound to the shader slots in DirectX 12.
/// </summary>
class IShaderResourceDX12
{
public:

    IShaderResourceDX12()
        : SubresourceIndex(-1)
    {
    }

    IShaderResourceDX12(int32 subresourceIndex)
        : SubresourceIndex(subresourceIndex)
    {
    }

public:

    /// <summary>
    /// Affected subresource index or -1 if use whole resource.
    /// This solves only resource states tracking per single subresource, not subresources range, if need to here should be range of subresources (for texture arrays, volume textures and cubemaps).
    /// </summary>
    int32 SubresourceIndex;

    D3D12_SRV_DIMENSION SrvDimension = D3D12_SRV_DIMENSION_UNKNOWN;
    D3D12_UAV_DIMENSION UavDimension = D3D12_UAV_DIMENSION_UNKNOWN;

public:

    /// <summary>
    /// Determines whether this resource is depth/stencil buffer.
    /// </summary>
    virtual bool IsDepthStencilResource() const = 0;

    /// <summary>
    /// Gets CPU handle to the shader resource view descriptor.
    /// </summary>
    virtual D3D12_CPU_DESCRIPTOR_HANDLE SRV() const = 0;

    /// <summary>
    /// Gets CPU handle to the unordered access view descriptor.
    /// </summary>
    virtual D3D12_CPU_DESCRIPTOR_HANDLE UAV() const = 0;

    /// <summary>
    /// Gets the resource owner.
    /// </summary>
    virtual ResourceOwnerDX12* GetResourceOwner() const = 0;
};

#endif
