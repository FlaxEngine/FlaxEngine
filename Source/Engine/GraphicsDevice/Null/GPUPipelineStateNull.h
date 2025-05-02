// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUPipelineState.h"

/// <summary>
/// Graphics pipeline state object for Null backend.
/// </summary>
class GPUPipelineStateNull : public GPUPipelineState
{
public:

    // [GPUPipelineState]
    bool IsValid() const override
    {
        return true;
    }
};

#endif
