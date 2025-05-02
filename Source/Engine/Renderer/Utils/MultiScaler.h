// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Scales an input texture to an output texture (down or up, depending on the relative size between input and output). Can perform image blurring.
/// </summary>
class MultiScaler : public RendererPass<MultiScaler>
{
private:

    AssetReference<Shader> _shader;
    GPUPipelineState* _psHalfDepth = nullptr;
    GPUPipelineStatePermutationsPs<2> _psBlur5;
    GPUPipelineStatePermutationsPs<2> _psBlur9;
    GPUPipelineStatePermutationsPs<2> _psBlur13;
    GPUPipelineState* _psUpscale = nullptr;

public:

    /// <summary>
    /// Filter mode
    /// </summary>
    enum class FilterMode
    {
        /// <summary>
        /// Optimized 5-tap gaussian blur with linear sampling (3 texture fetches).
        /// </summary>
        GaussianBlur5 = 1,

        /// <summary>
        /// Optimized 9-tap gaussian blur with linear sampling (5 texture fetches).
        /// </summary>
        GaussianBlur9 = 2,

        /// <summary>
        /// Optimized 13-tap gaussian blur with linear sampling (7 texture fetches).
        /// </summary>
        GaussianBlur13 = 3,
    };

    /// <summary>
    /// Performs texture filtering.
    /// </summary>
    /// <param name="mode">The filter mode.</param>
    /// <param name="context">The context.</param>
    /// <param name="width">The output width.</param>
    /// <param name="height">The output height.</param>
    /// <param name="src">The source texture.</param>
    /// <param name="dst">The destination texture.</param>
    /// <param name="tmp">The temporary texture (should have the same size as destination texture).</param>
    void Filter(const FilterMode mode, GPUContext* context, const int32 width, const int32 height, GPUTextureView* src, GPUTextureView* dst, GPUTextureView* tmp);

    /// <summary>
    /// Performs texture filtering.
    /// </summary>
    /// <param name="mode">The filter mode.</param>
    /// <param name="context">The context.</param>
    /// <param name="width">The output width.</param>
    /// <param name="height">The output height.</param>
    /// <param name="srcDst">The source and destination texture.</param>
    /// <param name="tmp">The temporary texture (should have the same size as destination texture).</param>
    void Filter(const FilterMode mode, GPUContext* context, const int32 width, const int32 height, GPUTextureView* srcDst, GPUTextureView* tmp);

    /// <summary>
    /// Downscales the depth buffer (to half resolution).
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="dstWidth">The width of the destination texture (in pixels).</param>
    /// <param name="dstHeight">The height of the destination texture (in pixels).</param>
    /// <param name="src">The source texture.</param>
    /// <param name="dst">The destination texture.</param>
    void DownscaleDepth(GPUContext* context, int32 dstWidth, int32 dstHeight, GPUTexture* src, GPUTextureView* dst);

    /// <summary>
    /// Upscales the texture.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="viewport">The viewport of the destination texture.</param>
    /// <param name="src">The source texture.</param>
    /// <param name="dst">The destination texture.</param>
    void Upscale(GPUContext* context, const Viewport& viewport, GPUTexture* src, GPUTextureView* dst);

public:

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psHalfDepth->ReleaseGPU();
        _psUpscale->ReleaseGPU();
        _psBlur5.Release();
        _psBlur9.Release();
        _psBlur13.Release();
        invalidateResources();
    }
#endif

protected:

    // [RendererPass]
    bool setupResources() override;
};
