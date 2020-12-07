// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/PostProcessSettings.h"

#define DOF_DEPTH_BLUR_FORMAT PixelFormat::R16G16_Float
#define DOF_RT_FORMAT PixelFormat::R11G11B10_Float

/// <summary>
/// Depth of Field rendering
/// </summary>
class DepthOfFieldPass : public RendererPass<DepthOfFieldPass>
{
private:

    PACK_STRUCT(struct Data {
        Vector2 ProjectionAB;
        float BokehDepthCullThreshold;
        float BokehDepthCutoff;

        Vector4 DOFDepths;

        float MaxBokehSize;
        float BokehBrightnessThreshold;
        float BokehBlurThreshold;
        float BokehFalloff;

        Vector2 BokehTargetSize;
        Vector2 DOFTargetSize;

        Vector2 InputSize;
        float DepthLimit;
        float BlurStrength;

        Vector3 Dummy;
        float BokehBrightness;
    });

    // Structure used for outputting bokeh points to an AppendStructuredBuffer
    struct BokehPoint
    {
        Vector3 Position;
        float Blur;
        Vector3 Color;
    };

    bool _platformSupportsDoF = false;
    bool _platformSupportsBokeh = false;
    GPUBuffer* _bokehBuffer = nullptr;
    GPUBuffer* _bokehIndirectArgsBuffer = nullptr;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psDofDepthBlurGeneration = nullptr;
    GPUPipelineState* _psBokehGeneration = nullptr;
    GPUPipelineState* _psDoNotGenerateBokeh = nullptr;
    GPUPipelineState* _psBokeh = nullptr;
    GPUPipelineState* _psBokehComposite = nullptr;
    AssetReference<Texture> _defaultBokehHexagon;
    AssetReference<Texture> _defaultBokehOctagon;
    AssetReference<Texture> _defaultBokehCircle;
    AssetReference<Texture> _defaultBokehCross;

public:

    DepthOfFieldPass();

public:

    /// <summary>
    /// Perform Depth Of Field rendering for the input task
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">Target with rendered HDR frame</param>
    /// <returns>Allocated temporary render target, should be released by the called. Can be null if pass skipped.</returns>
    GPUTexture* Render(RenderContext& renderContext, GPUTexture* input);

private:

    GPUTexture* getDofBokehShape(DepthOfFieldSettings& dofSettings);
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psDofDepthBlurGeneration->ReleaseGPU();
        _psBokehGeneration->ReleaseGPU();
        _psBokeh->ReleaseGPU();
        _psBokehComposite->ReleaseGPU();
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
