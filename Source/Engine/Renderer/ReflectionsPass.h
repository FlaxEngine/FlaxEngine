// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/Shader.h"

#define GENERATE_GF_CACHE 0
#define PRE_INTEGRATED_GF_ASSET_NAME TEXT("Engine/Textures/PreIntegratedGF")

/// <summary>
/// Reflections rendering service
/// </summary>
class ReflectionsPass : public RendererPass<ReflectionsPass>
{
private:

    PACK_STRUCT(struct Data {
        ProbeData PData;
        Matrix WVP;
        GBufferData GBuffer;
    });

    AssetReference<Shader> _shader;
    GPUPipelineState* _psProbeNormal;
    GPUPipelineState* _psProbeInverted;
    GPUPipelineState* _psCombinePass;

    AssetReference<Model> _sphereModel;
    AssetReference<Texture> _preIntegratedGF;

public:

    /// <summary>
    /// Init
    /// </summary>
    ReflectionsPass();

public:

    /// <summary>
    /// Perform reflections pass rendering for the input task.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="lightBuffer">The light buffer.</param>
    void Render(RenderContext& renderContext, GPUTextureView* lightBuffer);

public:

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psProbeNormal->ReleaseGPU();
        _psProbeInverted->ReleaseGPU();
        _psCombinePass->ReleaseGPU();
        invalidateResources();
    }
#endif

protected:

    // [RendererPass]
    bool setupResources() override;
};
