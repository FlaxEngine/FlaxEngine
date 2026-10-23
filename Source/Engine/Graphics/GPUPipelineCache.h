// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PSO_PRECACHING

// Different modes for Pipeline State Objects (PSOs) precaching in the engine. Used as flags.
enum class GPUPipelineCacheModes
{
    // No precaching.
    None = 0,
    // GPU shaders will precache pipeline for Compute Shaders on asset load.
    Shader = 1,
    // Rendering systems will precache pipeline states (such as post-processing and lighting shader).
    Renderer = 2,
    // Visual actors will precache pipeline states (such as model material shaders).
    Level = 4,
    // All precaching modes active.
    Full = Shader | Renderer | Level,
};

// Different modes for Pipeline State Objects (PSOs) precaching miss that define what to do if PSO is not ready on time.
enum class GPUPipelineMissModes
{
    // Ignores PSO miss and waits for it to be ready (can cause hitch).
    Wait = 0,
    // Skips drawing until PSO is ready.
    SkipDraw = 1,
};

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Materials/MaterialPrecacheFlags.h"

// Helper structure for PSO creating or using from cache used to track potential PSO hitches.
struct GPUPipelineLookup
{
    enum Flag
    {
        Empty = 0,
        Precaching = 1,
        Miss = 2,
    } Flags;
#if (!BUILD_RELEASE || USE_EDITOR) // GPU_ENABLE_DEVELOPMENT
    double StartTime;

    GPUPipelineLookup();
    ~GPUPipelineLookup();
#endif
};

enum class DrawPass : int32;

// Utility for collecting PSOs in batches for a set of objects (eg. actors) for efficient precaching at scale for diferent rendering passes at once.
struct GPUPipelineCacheBatch
{
    struct MaterialPSO
    {
        DrawPass DrawModes = (DrawPass)0;
        MaterialPrecacheFlags MaterialFlags = {};
        Array<class GPUVertexLayout*, InlinedAllocation<4>> VertexLayouts;

        friend MaterialPSO& operator|=(MaterialPSO& a, const MaterialPSO& b);
    };

    struct DrawPassPSO
    {
        void (*Callback)(void* paramsPtr);
        DrawPassPSO* Next;

        DrawPassPSO(void(*callback)(void* paramsPtr));
    };

    // Pending PSOs to precache for each material.
    Dictionary<class Material*, MaterialPSO> Materials;
};

// Utility for PSO precaching.
class GPUPipelineCache
{
public:
    // Current modes for PSO precaching.
    static GPUPipelineCacheModes CacheModes;
    // Current modes for PSO misses handling.
    static GPUPipelineMissModes MissModes;

    typedef Function<void()> Callback;
    typedef Function<void(GPUPipelineCacheBatch& batch)> BatchCallback;

    // Starts async precaching of the PSOs by a given coroutine. Writes a handle to the task which can be used to wait on it (optional).
    static void PrecacheAsync(const Callback& callback, volatile int32* handle = nullptr);

    // Starts async precaching of the PSOs by a given coroutine (with batching). Writes a handle to the task which can be used to wait on it (optional).
    static void PrecacheAsync(const BatchCallback& callback, volatile int32* handle = nullptr);

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

// Utility macro for render passes that process draw calls to precache PSOs
#define GPU_PSO_PRECACHING_PASS(name, callback) \
    void On##name(void* paramsPtr) \
    { \
        auto& params = *(IMaterial::PrecacheParameters*)paramsPtr; \
        auto& context = *(GPUPipelineState::PrecacheContext*)params.PrecacheContext; \
        callback; \
    } \
    GPUPipelineCacheBatch::DrawPassPSO name(On##name)

#else

#define GPU_PSO_PRECACHING(mode) false
#define GPU_PSO_READY(mode) true
#define GPU_PSO_WAIT(mode)

#endif

#define GPU_PSO_ZONE_NAME_PRECACHE "Precache PSOs"
#define GPU_PSO_ZONE_COLOR_PRECACHE 0x04BABA
#define GPU_PSO_ZONE_COLOR_MISS 0xBA4404
