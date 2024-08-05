// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Model.h"

/// <summary>
/// Lighting rendering service. Handles dynamic lights diffuse and specular color calculations.
/// </summary>
class LightPass : public RendererPass<LightPass>
{
private:
    AssetReference<Shader> _shader;
    GPUPipelineStatePermutationsPs<2> _psLightDir;
    GPUPipelineStatePermutationsPs<4> _psLightPointNormal;
    GPUPipelineStatePermutationsPs<4> _psLightPointInverted;
    GPUPipelineStatePermutationsPs<4> _psLightSpotNormal;
    GPUPipelineStatePermutationsPs<4> _psLightSpotInverted;
    GPUPipelineStatePermutationsPs<2> _psLightSkyNormal;
    GPUPipelineStatePermutationsPs<2> _psLightSkyInverted;
    GPUPipelineState* _psClearDiffuse = nullptr;
    AssetReference<Model> _sphereModel;
    PixelFormat _shadowMaskFormat;

public:
    /// <summary>
    /// Performs the lighting rendering for the input task.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="lightBuffer">The light accumulation buffer (input and output).</param>
    void RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psLightDir.Release();
        _psLightPointNormal.Release();
        _psLightPointInverted.Release();
        _psLightSpotNormal.Release();
        _psLightSpotInverted.Release();
        _psLightSkyNormal.Release();
        _psLightSkyInverted.Release();
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
