// Copyright (c) Wojciech Figat. All rights reserved.

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
    GPUPipelineStatePermutationsPs<4> _psLightPoint;
    GPUPipelineStatePermutationsPs<4> _psLightPointInside;
    GPUPipelineStatePermutationsPs<4> _psLightSpot;
    GPUPipelineStatePermutationsPs<4> _psLightSpotInside;
    GPUPipelineState* _psLightSky = nullptr;
    GPUPipelineState* _psLightSkyInside = nullptr;
    GPUPipelineState* _psClearDiffuse = nullptr;
    AssetReference<Model> _sphereModel;
    PixelFormat _shadowMaskFormat;

public:
    /// <summary>
    /// Setups the lights rendering for batched scene drawing.
    /// </summary>
    void SetupLights(RenderContext& renderContext, RenderContextBatch& renderContextBatch);

    /// <summary>
    /// Performs the lighting rendering for the input task.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="lightBuffer">The light accumulation buffer (input and output).</param>
    void RenderLights(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psLightDir.Release();
        _psLightPoint.Release();
        _psLightPointInside.Release();
        _psLightSpot.Release();
        _psLightSpotInside.Release();
        _psLightSky->ReleaseGPU();
        _psLightSkyInside->ReleaseGPU();
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
