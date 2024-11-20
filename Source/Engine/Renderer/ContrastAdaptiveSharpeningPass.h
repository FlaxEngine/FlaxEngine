// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/PostProcessSettings.h"

/// <summary>
/// Contrast Adaptive Sharpening (CAS) provides a mixed ability to sharpen and optionally scale an image. Based on AMD FidelityFX implementation.
/// </summary>
class ContrastAdaptiveSharpeningPass : public RendererPass<ContrastAdaptiveSharpeningPass>
{
private:
    AssetReference<Shader> _shader;
    GPUPipelineState* _psCAS = nullptr;
    bool _lazyInit = true;

public:
    bool CanRender(const RenderContext& renderContext);
    void Render(const RenderContext& renderContext, GPUTexture* input, GPUTextureView* output);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psCAS->ReleaseGPU();
        invalidateResources();
    }
#endif

public:
    // [RendererPass]
    String ToString() const override;
    void Dispose() override;

protected:
    // [RendererPass]
    bool setupResources() override;
};
