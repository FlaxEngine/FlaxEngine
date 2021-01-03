// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

/// <summary>
/// Fast-Approximate Anti-Aliasing effect.
/// </summary>
class FXAA : public RendererPass<FXAA>
{
private:

    PACK_STRUCT(struct Data
        {
            Vector4 ScreenSize;
        });

    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX)> _psFXAA;

public:

    /// <summary>
    /// Performs AA pass rendering for the input task.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The source render target.</param>
    /// <param name="output">The result render target.</param>
    void Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psFXAA.Release();
        invalidateResources();
    }
#endif

public:

    // [RendererPass]
    String ToString() const override
    {
        return TEXT("FXAA");
    }

    bool Init() override;
    void Dispose() override;

protected:

    // [RendererPass]
    bool setupResources() override;
};
