// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/Shader.h"

/// <summary>
/// Screen Space Reflections rendering service
/// </summary>
/// <remarks>
/// The following implementation is using Stochastic Screen-Space Reflections algorithm based on:
/// https://www.slideshare.net/DICEStudio/stochastic-screenspace-reflections
/// It's well optimized and provides solid visual effect.
///
/// Algorithm steps:
/// 1) Downscale depth [optional]
/// 2) Ray trace
/// 3) Resolve rays
/// 4) Temporal blur [optional]
/// 5) Combine final image (alpha blend into reflections buffer)
/// </remarks>
/// <seealso cref="RendererPass{ScreenSpaceReflectionsPass}" />
class ScreenSpaceReflectionsPass : public RendererPass<ScreenSpaceReflectionsPass>
{
private:

    AssetReference<Shader> _shader;
    GPUPipelineState* _psRayTracePass;
    GPUPipelineState* _psCombinePass;
    GPUPipelineStatePermutationsPs<4> _psResolvePass;
    GPUPipelineState* _psTemporalPass;
    GPUPipelineState* _psMixPass;

    AssetReference<Texture> _preIntegratedGF;

public:

    /// <summary>
    /// Init
    /// </summary>
    ScreenSpaceReflectionsPass();

public:

    /// <summary>
    /// Determinates whenever this pass requires motion vectors rendering.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>True if need to render motion vectors, otherwise false.</returns>
    static bool NeedMotionVectors(RenderContext& renderContext);

    /// <summary>
    /// Perform SSR rendering for the input task (blends reflections to given texture using alpha blending).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reflectionsRT">Temporary buffer to use for the reflections pass</param>
    /// <param name="lightBuffer">Light buffer</param>
    void Render(RenderContext& renderContext, GPUTextureView* reflectionsRT, GPUTextureView* lightBuffer);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psRayTracePass->ReleaseGPU();
        _psCombinePass->ReleaseGPU();
        _psResolvePass.Release();
        _psTemporalPass->ReleaseGPU();
        _psMixPass->ReleaseGPU();
        invalidateResources();
    }
#endif

public:

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

protected:

    // [RendererPass]
    bool setupResources() override;
};
