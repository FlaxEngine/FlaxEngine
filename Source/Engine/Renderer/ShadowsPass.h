// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

/// <summary>
/// Shadows rendering service.
/// </summary>
class ShadowsPass : public RendererPass<ShadowsPass>
{
private:

    // Shader stuff
    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2 * 2> _psShadowDir;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2> _psShadowPoint;
    GPUPipelineStatePermutationsPs<static_cast<int32>(Quality::MAX) * 2> _psShadowSpot;
    bool _supportsShadows;

    // Shadow maps stuff
    int32 _shadowMapsSizeCSM;
    int32 _shadowMapsSizeCube;
    GPUTexture* _shadowMapCSM;
    GPUTexture* _shadowMapCube;
    Quality _currentShadowMapsQuality;

    // Shadow map rendering stuff
    RenderContext _shadowContext;
    RenderList _shadowCache;
    AssetReference<Model> _sphereModel;

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

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(RenderContext& renderContext, const RendererPointLightData& light);

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(RenderContext& renderContext, const RendererSpotLightData& light);

    /// <summary>
    /// Determines whether can render shadow for the specified light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
    bool CanRenderShadow(RenderContext& renderContext, const RendererDirectionalLightData& light);

    /// <summary>
    /// Prepares the shadows rendering. Called by the light pass once per frame.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU command context.</param>
    void Prepare(RenderContext& renderContext, GPUContext* context);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContext& renderContext, RendererPointLightData& light, GPUTextureView* shadowMask);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContext& renderContext, RendererSpotLightData& light, GPUTextureView* shadowMask);

    /// <summary>
    /// Renders the shadow mask for the given light.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="light">The light.</param>
    /// <param name="index">The light index.</param>
    /// <param name="shadowMask">The shadow mask (output).</param>
    void RenderShadow(RenderContext& renderContext, RendererDirectionalLightData& light, int32 index, GPUTextureView* shadowMask);

private:

    void updateShadowMapSize();

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
