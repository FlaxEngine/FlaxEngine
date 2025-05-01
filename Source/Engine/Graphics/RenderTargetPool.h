// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Textures/GPUTexture.h"

/// <summary>
/// Utility for pooling render target resources with reusing and sharing resources during rendering.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API RenderTargetPool
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(RenderTargetPool);
public:
    /// <summary>
    /// Flushes the temporary render targets.
    /// </summary>
    /// <param name="force">True if release unused render targets by force, otherwise will use a few frames of delay.</param>
    /// <param name="framesOffset">Amount of previous frames that should persist in the pool after flush. Resources used more than given value wil be freed. Use value of -1 to auto pick default duration.</param>
    static void Flush(bool force = false, int32 framesOffset = -1);

    /// <summary>
    /// Gets a temporary render target.
    /// </summary>
    /// <param name="desc">The texture description.</param>
    /// <returns>The allocated render target or reused one.</returns>
    API_FUNCTION() static GPUTexture* Get(API_PARAM(Ref) const GPUTextureDescription& desc);

    /// <summary>
    /// Releases a temporary render target.
    /// </summary>
    /// <param name="rt">The reference to temporary target to release.</param>
    API_FUNCTION() static void Release(GPUTexture* rt);
};

// Utility to set name to the pooled render target (compiled-put in Release builds)
#if GPU_ENABLE_RESOURCE_NAMING
#define RENDER_TARGET_POOL_SET_NAME(rt, name) rt->SetName(TEXT(name));
#else
#define RENDER_TARGET_POOL_SET_NAME(rt, name)
#endif
