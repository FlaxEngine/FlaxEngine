// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PSO_PRECACHING

#include "GPUPipelineState.h"
#include "Engine/Platform/Platform.h"

// Helper structure for PSO creating or using from cache used to track potential PSO hitches.
struct GPUPipelineStateLookup
{
#if !BUILD_RELEASE
    double StartTime;

    GPUPipelineStateLookup()
    {
        StartTime = Platform::GetTimeSeconds();
    }

    ~GPUPipelineStateLookup();
#endif
};

#endif
