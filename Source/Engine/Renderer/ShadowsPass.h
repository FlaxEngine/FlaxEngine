// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "RenderList.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/RenderTask.h"

/// <summary>
/// Shadows rendering service.
/// </summary>
class ShadowsPass : public RendererPass<ShadowsPass>
{
private:
    AssetReference<Shader> _shader;
    AssetReference<Model> _sphereModel;
    GPUPipelineState* _psDepthClear = nullptr;
    GPUPipelineState* _psDepthCopy = nullptr;
    GPUPipelineStatePermutationsPs<int32(Quality::MAX) * 2 * 2> _psShadowDir;
    GPUPipelineStatePermutationsPs<int32(Quality::MAX) * 2> _psShadowPoint;
    GPUPipelineStatePermutationsPs<int32(Quality::MAX) * 2> _psShadowPointInside;
    GPUPipelineStatePermutationsPs<int32(Quality::MAX) * 2> _psShadowSpot;
    GPUPipelineStatePermutationsPs<int32(Quality::MAX) * 2> _psShadowSpotInside;
    PixelFormat _shadowMapFormat; // Cached on initialization

public:
    /// <summary>
    /// Setups the shadows rendering for batched scene drawing. Checks which lights will cast a shadow.
    /// </summary>
    void SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch);

    /// <summary>
    /// Renders the shadow maps for all lights (into atlas).
    /// </summary>
    void RenderShadowMaps(RenderContextBatch& renderContextBatch);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadowMask(RenderContextBatch& renderContextBatch, RenderLightData& light, GPUTextureView* shadowMask);

    /// <summary>
    /// Gets the shadow atlas texture and shadows buffer for shadow projection in shaders.
    /// </summary>
    /// <param name="renderBuffers">The render buffers that store frame context.</param>
    /// <param name="shadowMapAtlas">The output shadow map atlas texture or null if unused.</param>
    /// <param name="shadowsBuffer">The output shadows buffer or null if unused.</param>
    static void GetShadowAtlas(const RenderBuffers* renderBuffers, GPUTexture*& shadowMapAtlas, GPUBufferView*& shadowsBuffer);

private:
    static void SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext, struct ShadowAtlasLight* atlasLight = nullptr, RenderContext* dynamicContext = nullptr);
    static void SetupLight(class ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLightData& light, ShadowAtlasLight& atlasLight);
    static bool SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderLocalLightData& light, ShadowAtlasLight& atlasLight);
    static void SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderDirectionalLightData& light, ShadowAtlasLight& atlasLight);
    static void SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderPointLightData& light, ShadowAtlasLight& atlasLight);
    static void SetupLight(ShadowsCustomBuffer& shadows, RenderContext& renderContext, RenderContextBatch& renderContextBatch, RenderSpotLightData& light, ShadowAtlasLight& atlasLight);
    void ClearShadowMapTile(GPUContext* context, GPUConstantBuffer* quadShaderCB, struct QuadShaderData& quadShaderData) const;
    void CopyShadowMapTile(GPUContext* context, GPUConstantBuffer* quadShaderCB, struct QuadShaderData& quadShaderData, const GPUTexture* srcShadowMap, const struct ShadowsAtlasRectTile* srcTile) const;

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psShadowDir.Release();
        _psShadowPoint.Release();
        _psShadowPointInside.Release();
        _psShadowSpot.Release();
        _psShadowSpotInside.Release();
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
