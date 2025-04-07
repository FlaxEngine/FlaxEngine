// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/Textures/GPUSampler.h"

/// <summary>
/// Sampler object for Null backend.
/// </summary>
class GPUSamplerNull : public GPUSampler
{
protected:

    // [GPUSampler]
    bool OnInit() override
    {
        return false;
    }
};

#endif
