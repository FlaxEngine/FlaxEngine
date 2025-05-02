// Copyright (c) Wojciech Figat. All rights reserved.

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
    GPUPipelineStatePermutationsPs<2> _psRayTracePass;
    GPUPipelineStatePermutationsPs<4> _psResolvePass;
    GPUPipelineState* _psCombinePass = nullptr;
    GPUPipelineState* _psTemporalPass = nullptr;
    GPUPipelineState* _psMixPass = nullptr;

    AssetReference<Shader> _shader;
    AssetReference<Texture> _preIntegratedGF;

public:
    /// <summary>
    /// Perform SSR rendering for the input task (blends reflections to given texture using alpha blending).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reflectionsRT">Temporary buffer to use for the reflections pass</param>
    /// <param name="lightBuffer">Light buffer</param>
    void Render(RenderContext& renderContext, GPUTextureView* reflectionsRT, GPUTextureView* lightBuffer);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj);
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
