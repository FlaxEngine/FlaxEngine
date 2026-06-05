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
    GPUPipelineStatePermutationsPs<2> _psBlur5;
    GPUPipelineStatePermutationsPs<2> _psBlur9;
    GPUPipelineStatePermutationsPs<2> _psBlur13;
    GPUPipelineStatePermutationsPs<3> _psHalfDepth;
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
    void Filter(FilterMode mode, GPUContext* context, int32 width, int32 height, GPUTextureView* src, GPUTextureView* dst, GPUTextureView* tmp);

    /// <summary>
    /// Performs texture filtering.
    /// </summary>
    /// <param name="mode">The filter mode.</param>
    /// <param name="context">The context.</param>
    /// <param name="width">The output width.</param>
    /// <param name="height">The output height.</param>
    /// <param name="srcDst">The source and destination texture.</param>
    /// <param name="tmp">The temporary texture (should have the same size as destination texture).</param>
    void Filter(FilterMode mode, GPUContext* context, int32 width, int32 height, GPUTextureView* srcDst, GPUTextureView* tmp);

    /// <summary>
    /// Downscales the depth buffer (to half resolution). Uses `min` operator (`max` for inverted depth) to output the furthest depths for conservative usage.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="dstWidth">The width of the destination texture (in pixels).</param>
    /// <param name="dstHeight">The height of the destination texture (in pixels).</param>
    /// <param name="src">The source texture (has to have ShaderResource flag).</param>
    /// <param name="dst">The destination texture (has to have DepthStencil or RenderTarget flag).</param>
    void DownscaleDepth(GPUContext* context, int32 dstWidth, int32 dstHeight, GPUTexture* src, GPUTextureView* dst);

    /// <summary>
    /// Generates the Hierarchical Z-Buffer (HiZ). Uses `min` operator (`max` for inverted depth) to output the furthest depths for conservative usage.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="srcDepth">The source depth buffer texture (has to have ShaderResource flag).</param>
    /// <param name="dstHiZ">The destination HiZ texture (has to have DepthStencil or RenderTarget flag).</param>
    void BuildHiZ(GPUContext* context, GPUTexture* srcDepth, GPUTexture* dstHiZ);

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
        _psUpscale->ReleaseGPU();
        _psBlur5.Release();
        _psBlur9.Release();
        _psBlur13.Release();
        _psHalfDepth.Release();
        invalidateResources();
    }
#endif

protected:
    // [RendererPass]
    bool setupResources() override;
};
