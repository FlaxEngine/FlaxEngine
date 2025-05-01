// Copyright (c) Wojciech Figat. All rights reserved.

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
    /// Performs AA pass rendering for the input task.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The input render target.</param>
    /// <param name="output">The output render target.</param>
    void Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        if (_psTAA)
            _psTAA->ReleaseGPU();
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
