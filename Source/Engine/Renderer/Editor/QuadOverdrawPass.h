// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "../RendererPass.h"

class GPUContext;
class GPUTextureView;
class GPUPipelineState;
struct RenderContext;

/// <summary>
/// Rendering geometry overdraw to visualize performance of pixels rendering in editor.
/// </summary>
class QuadOverdrawPass : public RendererPass<QuadOverdrawPass>
{
private:

    AssetReference<Shader> _shader;
    GPUPipelineState* _ps = nullptr;

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _ps->ReleaseGPU();
        invalidateResources();
    }
#endif

public:

    void Render(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer);

    // [RendererPass]
    String ToString() const override;
    void Dispose() override;

protected:

    // [RendererPass]
    bool setupResources() override;
};

#endif
