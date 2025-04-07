// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/PostProcessSettings.h"

/// <summary>
/// Depth of Field rendering
/// </summary>
class DepthOfFieldPass : public RendererPass<DepthOfFieldPass>
{
private:
    // Structure used for outputting bokeh points to an AppendStructuredBuffer
    struct BokehPoint
    {
        Float3 Position;
        float Blur;
        Float3 Color;
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
    /// <param name="frame">Input and output frame (leave unchanged when not using this effect).</param>
    /// <param name="tmp">Temporary frame (the same format as frame)</param>
    void Render(RenderContext& renderContext, GPUTexture*& frame, GPUTexture*& tmp);

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
