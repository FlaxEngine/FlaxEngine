// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Luminance histogram rendering pass. Uses compute shaders.
/// </summary>
class HistogramPass : public RendererPass<HistogramPass>
{
private:

    AssetReference<Shader> _shader;
    GPUShaderProgramCS* _csClearHistogram;
    GPUShaderProgramCS* _csGenerateHistogram;
    GPUBuffer* _histogramBuffer = nullptr;
    bool _isSupported;

public:

    /// <summary>
    /// Performs the histogram rendering.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="colorBuffer">The input color buffer to use as a luminance source.</param>
    /// <returns>The created histogram, or null if failed or not supported.</param>
    GPUBuffer* Render(RenderContext& renderContext, GPUTexture* colorBuffer);

    /// <summary>
    /// Gets the multiply and add value to pack or unpack data for histogram buffer.
    /// </summary>
    /// <param name="multiply">The multiply factor.</param>
    /// <param name="add">The add factor.</param>
    void GetHistogramMad(float& multiply, float& add);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _csClearHistogram = nullptr;
        _csGenerateHistogram = nullptr;
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
