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

/// <summary>
/// GPU pass that explicitly defines render targets and depth buffer within a rendering pass. Can be used to optimize GPU rendering on tiled GPUs with manual control over attachment operations (load, store, clear, discard, etc.) that reduce memory bandwidth usage.
/// </summary>
/// <remarks>Draw Pass discards any render targets or depth buffer bound to the context (reduces state-tracking).</remarks>
struct FLAXENGINE_API GPUDrawPass : GPUPass
{
    GPUTextureView* DepthBuffer;
    GPUTextureView** RenderTargets;
    GPUDrawPassAction* RenderTargetsActions;
    int32 RenderTargetsCount;
    GPUDrawPassAction DepthAction;

    GPUDrawPass(GPUContext* context, Span<GPUTextureView*> renderTargets)
        : GPUPass(context)
        , DepthBuffer(nullptr)
        , RenderTargets(renderTargets.Get())
        , RenderTargetsActions(nullptr)
        , RenderTargetsCount(renderTargets.Length())
        , DepthAction(GPUDrawPassAction::None)
    {
        Context->BeginDrawPass(*this);
    }

    GPUDrawPass(GPUContext* context, GPUTextureView* depthBuffer, Span<GPUTextureView*> renderTargets)
        : GPUPass(context)
        , DepthBuffer(depthBuffer)
        , RenderTargets(renderTargets.Get())
        , RenderTargetsActions(nullptr)
        , RenderTargetsCount(renderTargets.Length())
        , DepthAction(GPUDrawPassAction::LoadStore)
    {
        Context->BeginDrawPass(*this);
    }

    GPUDrawPass(GPUContext* context, GPUTextureView* depthBuffer, GPUDrawPassAction depthAction, Span<GPUTextureView*> renderTargets, Span<GPUDrawPassAction> renderTargetsActions)
        : GPUPass(context)
        , DepthBuffer(depthBuffer)
        , RenderTargets(renderTargets.Get())
        , RenderTargetsActions(renderTargetsActions.Get())
        , RenderTargetsCount(renderTargets.Length())
        , DepthAction(depthAction)
    {
        ASSERT_LOW_LAYER(renderTargets.Length() == renderTargetsActions.Length());
        Context->BeginDrawPass(*this);
    }

    GPUDrawPass(GPUContext* context, GPUTextureView* depthBuffer, GPUDrawPassAction depthAction, GPUTextureView* renderTarget, GPUDrawPassAction renderTargetAction = GPUDrawPassAction::LoadStore)
        : GPUPass(context)
        , _inlinedRT(renderTarget)
        , _inlinedRTAction(renderTargetAction)
        , DepthBuffer(depthBuffer)
        , RenderTargets(&_inlinedRT)
        , RenderTargetsActions(&_inlinedRTAction)
        , RenderTargetsCount(1)
        , DepthAction(depthAction)
    {
        Context->BeginDrawPass(*this);
    }

    GPUDrawPass(GPUContext* context, GPUTextureView* renderTarget, GPUDrawPassAction renderTargetAction = GPUDrawPassAction::LoadStore)
        : GPUPass(context)
        , _inlinedRT(renderTarget)
        , _inlinedRTAction(renderTargetAction)
        , DepthBuffer(nullptr)
        , RenderTargets(&_inlinedRT)
        , RenderTargetsActions(&_inlinedRTAction)
        , RenderTargetsCount(1)
        , DepthAction(GPUDrawPassAction::None)
    {
        Context->BeginDrawPass(*this);
    }

    GPUDrawPass(GPUContext* context, Span<GPUTextureView*> renderTargets, Span<GPUDrawPassAction> renderTargetsActions)
        : GPUPass(context)
        , DepthBuffer(nullptr)
        , RenderTargets(renderTargets.Get())
        , RenderTargetsActions(renderTargetsActions.Get())
        , RenderTargetsCount(renderTargets.Length())
        , DepthAction(GPUDrawPassAction::None)
    {
        ASSERT_LOW_LAYER(renderTargets.Length() == renderTargetsActions.Length());
        Context->BeginDrawPass(*this);
    }

    ~GPUDrawPass()
    {
        Context->EndDrawPass();
    }

private:
    GPUTextureView* _inlinedRT;
    GPUDrawPassAction _inlinedRTAction;
};
