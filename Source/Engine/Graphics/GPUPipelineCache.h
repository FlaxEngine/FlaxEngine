// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

// Different modes for Pipeline State Objects (PSOs) precaching in the engine. Used as flags.
enum class GPUPipelineCacheModes
{
    // No precaching.
    None = 0,
    // GPU shaders will precache pipeline for Compute Shaders on asset load.
    Shader = 1,
    // All precaching modes active.
    Full = Shader,
};

#if COMPILE_WITH_PSO_PRECACHING

#include "Engine/Platform/Platform.h"

// Helper structure for PSO creating or using from cache used to track potential PSO hitches.
struct GPUPipelineLookup
{
    enum Flag
    {
        Empty = 0,
        Precaching = 1,
        Miss = 2,
    } Flags;
#if !BUILD_RELEASE
    double StartTime;

    GPUPipelineLookup()
    {
        StartTime = Platform::GetTimeSeconds();
        Flags = Empty;
    }

    ~GPUPipelineLookup();
#endif
};

extern GPUPipelineCacheModes PipelineCacheModes;

inline GPUPipelineLookup::Flag& operator|=(GPUPipelineLookup::Flag& a, GPUPipelineLookup::Flag b)
{
    return a = (GPUPipelineLookup::Flag)((uint32)a | (uint32)b);
}

#define GPU_PSO_PRECACHING(mode) ((uint32)PipelineCacheModes & (uint32)GPUPipelineCacheModes::mode)

#else

#define GPU_PSO_PRECACHING(mode) false

#endif

#define GPU_PSO_ZONE_COLOR_PRECACHE 0x04BABA
#define GPU_PSO_ZONE_COLOR_MISS 0xBA4404
