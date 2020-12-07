// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUResource.h"

/// <summary>
/// Constant Buffer object used to pass parameters to the shaders on GPU.
/// </summary>
class FLAXENGINE_API GPUConstantBuffer : public GPUResource
{
protected:

    uint32 _size = 0;

public:

    /// <summary>
    /// Gets the buffer size (in bytes).
    /// </summary>
    /// <returns>The buffer size (in bytes).</returns>
    FORCE_INLINE uint32 GetSize() const
    {
        return _size;
    }

public:

    // [GPUResource]
    ResourceType GetResourceType() const override
    {
        return ResourceType::Buffer;
    }
};
