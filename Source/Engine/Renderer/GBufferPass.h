// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Rendering scene to the GBuffer
/// </summary>
class GBufferPass : public RendererPass<GBufferPass>
{
private:

    AssetReference<Shader> _gBufferShader;
    GPUPipelineState* _psDebug = nullptr;
    AssetReference<Model> _skyModel;
    AssetReference<Model> _boxModel;
#if USE_EDITOR
    class LightmapUVsDensityMaterialShader* _lightmapUVsDensityMaterialShader = nullptr;
    class VertexColorsMaterialShader* _vertexColorsMaterialShader = nullptr;
#endif

public:

    /// <summary>
    /// Fill GBuffer
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="lightBuffer">Light buffer to output material emissive light and precomputed indirect lighting</param>
    void Fill(RenderContext& renderContext, GPUTextureView* lightBuffer);

    /// <summary>
    /// Render debug view
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void RenderDebug(RenderContext& renderContext);

public:

    static bool IsDebugView(ViewMode mode);

    /// <summary>
    /// Set GBuffer inputs structure for given render task
    /// </summary>
    /// <param name="view">The rendering view.</param>
    /// <param name="gBuffer">GBuffer input to setup</param>
    static void SetInputs(const RenderView& view, GBufferData& gBuffer);

private:

    void DrawDecals(RenderContext& renderContext, GPUTextureView* lightBuffer);

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psDebug->ReleaseGPU();
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
