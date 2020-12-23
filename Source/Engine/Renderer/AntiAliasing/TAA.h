// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Temporal Anti-Aliasing effect.
/// </summary>
class TAA : public RendererPass<TAA>
{
private:

    AssetReference<Shader> _shader;
    GPUPipelineState* _psTAA;

public:

    /// <summary>
    /// Determinates whenever this pass requires motion vectors rendering.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>True if need to render motion vectors, otherwise false.</returns>
    static bool NeedMotionVectors(RenderContext& renderContext);

    /// <summary>
    /// Performs AA pass rendering for the input task.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The input render target.</param>
    /// <param name="output">The output render target.</param>
    void Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psTAA = nullptr;
        invalidateResources();
    }
#endif

public:

    // [RendererPass]
    String ToString() const override
    {
        return TEXT("TAA");
    }

    bool Init() override;
    void Dispose() override;

protected:

    // [RendererPass]
    bool setupResources() override;
};
