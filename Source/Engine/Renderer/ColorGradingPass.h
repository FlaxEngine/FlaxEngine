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
    /// <returns>Allocated temp render target with a rendered LUT. Can be 2d or 3d based on current graphics hardware caps. Release after usage (via RenderTargetPool::Release).</returns>
    GPUTexture* RenderLUT(RenderContext& renderContext);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psLut.Release();
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
