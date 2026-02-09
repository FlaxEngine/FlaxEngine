// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Color Grading and Tone Mapping rendering service. Generates HDR LUT for PostFx pass.
/// </summary>
class ColorGradingPass : public RendererPass<ColorGradingPass>
{
private:
    int32 _use3D = -1;
    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<4> _psLut;

public:
    /// <summary>
    /// Renders Look Up table with color grading parameters mixed in.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>Allocated temp render target with a rendered LUT - cached within Render Buffers, released automatically.</returns>
    GPUTexture* RenderLUT(RenderContext& renderContext);

private:
#if COMPILE_WITH_DEV_ENV
    uint64 _reloadedFrame = 0;
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
