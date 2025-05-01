// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

#define SMAA_AREA_TEX TEXT("Engine/Textures/SMAA_AreaTex")
#define SMAA_SEARCH_TEX TEXT("Engine/Textures/SMAA_SearchTex")

/// <summary>
/// Subpixel Morphological Anti-Aliasing effect.
/// </summary>
class SMAA : public RendererPass<SMAA>
{
private:

    GPU_CB_STRUCT(Data {
        Float4 RtSize;
        });

    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX)> _psEdge;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX)> _psBlend;
    GPUPipelineState* _psNeighbor = nullptr;
    AssetReference<Texture> _areaTex;
    AssetReference<Texture> _searchTex;

public:

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
        _psEdge.Release();
        _psBlend.Release();
        _psNeighbor->ReleaseGPU();
        invalidateResources();
    }
#endif

public:

    // [RendererPass]
    String ToString() const override
    {
        return TEXT("SMAA");
    }

    void Dispose() override;

protected:

    // [RendererPass]
    bool setupResources() override;
};
