// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"

// Different modes for Pipeline State Objects (PSOs) precaching in the engine. Used as flags.
enum class GPUPipelineCacheModes
{
    // No precaching.
    None = 0,
    // GPU shaders will precache pipeline for Compute Shaders on asset load.
    Shader = 1,
    // Rendering systems will precache pipeline states (such as post-processing and lighting shader).
    Renderer = 2,
    // All precaching modes active.
    Full = Shader | Renderer,
};

// Different modes for Pipeline State Objects (PSOs) precaching miss that define what to do if PSO is not ready on time.
enum class GPUPipelineMissModes
{
    // Ignores PSO miss and waits for it to be ready (can cause hitch).
    Wait = 0,
    // Skips drawing until PSO is ready.
    SkipDraw = 1,
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
#if GPU_ENABLE_DEVELOPMENT
    double StartTime;

    GPUPipelineLookup()
    {
        StartTime = Platform::GetTimeSeconds();
        Flags = Empty;
    }

    ~GPUPipelineLookup();
#endif
};

// Utility for PSO precaching.
class GPUPipelineCache
{
public:
    // Current modes for PSO precaching.
    static GPUPipelineCacheModes CacheModes;
    // Current modes for PSO misses handling.
    static GPUPipelineMissModes MissModes;

    // Starts async precaching of the PSOs by a given coroutine. Returns handle to the task which can be used to wait on it (optional).
    static void PrecacheAsync(const Function<void()>& callback, volatile int32* handle = nullptr);

    // Checks if specific async precaching job to be finished. Can be used to skip drawing or wait for PSO.
    static bool IsPrecacheReady(volatile int32* handle, bool wait = false);
};

inline GPUPipelineLookup::Flag& operator|=(GPUPipelineLookup::Flag& a, GPUPipelineLookup::Flag b)
{
    return a = (GPUPipelineLookup::Flag)((uint32)a | (uint32)b);
}

// Checks if perform PSO precaching for a specific mode.
#define GPU_PSO_PRECACHING(mode) ((uint32)GPUPipelineCache::CacheModes & (uint32)GPUPipelineCacheModes::mode)
// Checks if async PSO job is ready (returns true). Depending on current mode it can wait for PSO or skip draw by returning false.
#define GPU_PSO_READY(handle) GPUPipelineCache::IsPrecacheReady(&handle, GPUPipelineCache::MissModes == GPUPipelineMissModes::Wait)
// Waits on a PSO async handle until it's finished (to avoid race-condition).
#define GPU_PSO_WAIT(handle) GPUPipelineCache::IsPrecacheReady(&handle, true)

#else

#define GPU_PSO_PRECACHING(mode) false
#define GPU_PSO_READY(mode) true
#define GPU_PSO_WAIT(mode)

#endif

#define GPU_PSO_ZONE_NAME_PRECACHE "Precache PSOs"
#define GPU_PSO_ZONE_COLOR_PRECACHE 0x04BABA
#define GPU_PSO_ZONE_COLOR_MISS 0xBA4404
