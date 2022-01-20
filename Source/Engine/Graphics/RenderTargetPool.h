// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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
    static void Flush(bool force = false);

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
