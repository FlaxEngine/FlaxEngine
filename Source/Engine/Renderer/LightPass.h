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
    GPUPipelineStatePermutationsPs<4> _psLightLocal;
    GPUPipelineStatePermutationsPs<4> _psLightLocalInside;
    GPUPipelineState* _psLightSky = nullptr;
    GPUPipelineState* _psLightSkyInside = nullptr;
    GPUPipelineState* _psClearDiffuse = nullptr;
    GPUPipelineState* _psComplexity = nullptr;
    GPUPipelineState* _psLightOverlap[2] = {};
    AssetReference<Model> _sphereModel;
    PixelFormat _shadowMaskFormat;
    bool _depthBounds = false;

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

    /// <summary>
    /// Renders the debug view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="output">The output buffer.</param>
    void RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psLightDir.Release();
        _psLightLocal.Release();
        _psLightLocalInside.Release();
        _psLightSky->ReleaseGPU();
        _psLightSkyInside->ReleaseGPU();
        if (_psClearDiffuse)
            _psClearDiffuse->ReleaseGPU();
        SAFE_DELETE_GPU_RESOURCE(_psComplexity);
        SAFE_DELETE_GPU_RESOURCE(_psLightOverlap[0]);
        SAFE_DELETE_GPU_RESOURCE(_psLightOverlap[1]);
        invalidateResources();
    }
#endif
    void RenderDebugSphere(RenderContext& renderContext, GPUContext* context, const struct RenderLightData& light, float radius) const;

public:
    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

protected:
    // [RendererPass]
    bool setupResources() override;
};
