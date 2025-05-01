// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUTimerQuery.h"

/// <summary>
/// GPU timer query object for Null backend.
/// </summary>
class GPUTimerQueryNull : public GPUTimerQuery
{
public:

    // [GPUTimerQuery]
    void Begin() override
    {
    }

    void End() override
    {
    }

    bool HasResult() override
    {
        return true;
    }

    float GetResult() override
    {
        return 0.0f;
    }
};

#endif
