// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#if USE_EDITOR
#include "Engine/Core/Collections/Dictionary.h"
#endif

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
    class LightmapUVsDensityMaterialShader* _lightmapUVsDensity = nullptr;
    class VertexColorsMaterialShader* _vertexColors = nullptr;
    class LODPreviewMaterialShader* _lodPreview = nullptr;
    class MaterialComplexityMaterialShader* _materialComplexity = nullptr;
#endif

public:

    /// <summary>
    /// Fill GBuffer
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="lightBuffer">Light buffer to output material emissive light and precomputed indirect lighting</param>
    void Fill(RenderContext& renderContext, GPUTexture* lightBuffer);

    /// <summary>
    /// Render debug view
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void RenderDebug(RenderContext& renderContext);

    /// <summary>
    /// Renders the sky or skybox into low-resolution cubemap. Can be used to sample realtime sky lighting in GI passes.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <returns>Rendered cubemap or null if not ready or failed.</returns>
    GPUTextureView* RenderSkybox(RenderContext& renderContext, GPUContext* context);

#if USE_EDITOR
    // Temporary cache for faster debug previews drawing (used only during frame rendering).
    static Dictionary<GPUBuffer*, const ModelLOD*> IndexBufferToModelLOD;
    static CriticalSection Locker;

    FORCE_INLINE static void AddIndexBufferToModelLOD(GPUBuffer* indexBuffer, const ModelLOD* modelLod)
    {
        Locker.Lock();
        IndexBufferToModelLOD[indexBuffer] = modelLod;
        Locker.Unlock();
    }
    void PreOverrideDrawCalls(RenderContext& renderContext);
    void OverrideDrawCalls(RenderContext& renderContext);
    void DrawMaterialComplexity(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer);
#endif

public:

    static bool IsDebugView(ViewMode mode);

    /// <summary>
    /// Set GBuffer inputs structure for given render task
    /// </summary>
    /// <param name="view">The rendering view.</param>
    /// <param name="gBuffer">GBuffer input to setup</param>
    static void SetInputs(const RenderView& view, ShaderGBufferData& gBuffer);

private:

    void DrawSky(RenderContext& renderContext, GPUContext* context);
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
