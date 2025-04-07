// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUBuffer.h"

/// <summary>
/// GPU buffer for Null backend.
/// </summary>
class GPUBufferNull : public GPUBuffer
{
public:

    // [GPUBuffer]
    GPUBufferView* View() const override
    {
        return nullptr;
    }

    void* Map(GPUResourceMapMode mode) override
    {
        return nullptr;
    }

    void Unmap() override
    {
    }

protected:

    // [GPUBuffer]
    bool OnInit() override
    {
        return false;
    }
};

#endif
