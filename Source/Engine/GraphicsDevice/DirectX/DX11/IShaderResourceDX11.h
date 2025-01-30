// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// Interface for objects that can be bound to the shader slots in DirectX 11.
/// </summary>
class IShaderResourceDX11
{
public:
    IShaderResourceDX11()
    {
    }

public:
    /// <summary>
    /// Gets handle to the shader resource view object.
    /// </summary>
    /// <returns>SRV</returns>
    virtual ID3D11ShaderResourceView* SRV() const = 0;

    /// <summary>
    /// Gets CPU to the unordered access view object.
    /// </summary>
    virtual ID3D11UnorderedAccessView* UAV() const = 0;
};

#endif
