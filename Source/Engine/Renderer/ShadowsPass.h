// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "RenderList.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/RenderTask.h"

/// <summary>
/// Pixel format for fullscreen render target used for shadows calculations
/// </summary>
#define SHADOWS_PASS_SS_RR_FORMAT PixelFormat::R11G11B10_Float

template<typename T>
bool CanRenderShadow(const RenderView& view, const T& light)
{
    bool result = false;
    switch ((ShadowsCastingMode)light.ShadowsMode)
    {
    case ShadowsCastingMode::StaticOnly:
        result = view.IsOfflinePass;
        break;
    case ShadowsCastingMode::DynamicOnly:
        result = !view.IsOfflinePass;
        break;
    case ShadowsCastingMode::All:
        result = true;
        break;
    default:
        break;
    }
    return result && light.ShadowsStrength > ZeroTolerance;
}

/// <summary>
/// Shadows rendering service.
/// </summary>
class ShadowsPass : public RendererPass<ShadowsPass>
{
private:

    struct ShadowData
    {
        int32 ContextIndex;
        int32 ContextCount;
        bool BlendCSM;
        LightShadowData Constants;
    };

    // Shader stuff
    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2 * 2> _psShadowDir;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2> _psShadowPoint;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2> _psShadowSpot;
    PixelFormat _shadowMapFormat;

    // Shadow maps stuff
    int32 _shadowMapsSizeCSM;
    int32 _shadowMapsSizeCube;
    GPUTexture* _shadowMapCSM;
    GPUTexture* _shadowMapCube;
    Quality _currentShadowMapsQuality;

    // Shadow map rendering stuff
    AssetReference<Model> _sphereModel;
    Array<ShadowData> _shadowData;

    // Cached state for the current frame rendering (setup via Prepare)
    int32 maxShadowsQuality;

public:

    /// <summary>
    /// Init
    /// </summary>
    ShadowsPass();

public:

    /// <summary>
    /// Gets current GPU memory usage by the shadow maps
    /// </summary>
    /// <returns>GPU memory used in bytes</returns>
    uint64 GetShadowMapsMemoryUsage() const;

public:

    // TODO: use full scene shadow map atlas with dynamic slots allocation
    int32 LastDirLightIndex = -1;
    GPUTextureView* LastDirLightShadowMap = nullptr;
    LightShadowData LastDirLight;

public:
    void Prepare();

    /// <summary>
    /// Setups the shadows rendering for batched scene drawing. Checks which lights will cast a shadow.
    /// </summary>
    void SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch);

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(const RenderContext& renderContext, const RendererPointLightData& light);

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(const RenderContext& renderContext, const RendererSpotLightData& light);

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(const RenderContext& renderContext, const RendererDirectionalLightData& light);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContextBatch& renderContextBatch, RendererPointLightData& light, GPUTextureView* shadowMask);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContextBatch& renderContextBatch, RendererSpotLightData& light, GPUTextureView* shadowMask);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="light">The light.</param>
    /// <param name="index">The light index.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContextBatch& renderContextBatch, RendererDirectionalLightData& light, int32 index, GPUTextureView* shadowMask);

private:

    void updateShadowMapSize();
    void SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext);
    void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererDirectionalLightData& light);
    void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererPointLightData& light);
    void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererSpotLightData& light);

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psShadowDir.Release();
        _psShadowPoint.Release();
        _psShadowSpot.Release();
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
