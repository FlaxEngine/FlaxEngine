// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUContext.h"
#include "Engine/Graphics/GPUResourceAccess.h"

/// <summary>
/// Base for GPU rendering passes that control low-level memory access and GPU resources states with usage to optimize rendering.
/// </summary>
struct FLAXENGINE_API GPUPass
{
    NON_COPYABLE(GPUPass);

    GPUContext* Context;

    GPUPass(GPUContext* context)
        : Context(context)
    {
        Context->_pass++;
    }

    ~GPUPass()
    {
        Context->_pass--;
    }

    // Performs resource state transition into a specific access (mask). Can be done preemptively in the prologue of the pass to execute more efficient barriers.
    void Transition(GPUResource* resource, GPUResourceAccess access)
    {
        Context->Transition(resource, access);
    }
};

/// <summary>
/// GPU pass that manually controls memory barriers and cache flushes when performing batched copy/upload operations with GPU context. Can be used to optimize GPU buffers usage by running different copy operations simultaneously.
/// </summary>
struct FLAXENGINE_API GPUMemoryPass : GPUPass
{
    GPUMemoryPass(GPUContext* context)
        : GPUPass(context)
    {
    }

    ~GPUMemoryPass()
    {
        Context->MemoryBarrier();
    }

    // Inserts a global memory barrier on data copies between resources. Use to ensure all writes and before submitting another commands.
    void MemoryBarrier()
    {
        Context->MemoryBarrier();
    }
};

/// <summary>
/// GPU pass that controls memory barriers when performing batched Compute shader dispatches with GPU context. Can be used to optimize GPU utilization by running different dispatches simultaneously (by overlapping work).
/// </summary>
struct FLAXENGINE_API GPUComputePass : GPUPass
{
    GPUComputePass(GPUContext* context)
        : GPUPass(context)
    {
        Context->OverlapUA(false);
    }

    ~GPUComputePass()
    {
        Context->OverlapUA(true);
    }
};

// TODO: add GPUDrawPass for render targets and depth/stencil setup with optimized clear for faster drawing on tiled-GPUs (mobile)
