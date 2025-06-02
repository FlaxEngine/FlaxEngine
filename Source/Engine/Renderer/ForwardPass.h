// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/RenderView.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"

/// <summary>
/// Forward rendering pass for transparent geometry.
/// </summary>
class ForwardPass : public RendererPass<ForwardPass>
{
private:

    AssetReference<Shader> _shader;
    GPUPipelineState* _psApplyDistortion;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ForwardPass"/> class.
    /// </summary>
    ForwardPass();

public:

    /// <summary>
    /// Performs forward pass rendering for the input task. Renders transparent objects.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">Target with renderer frame ready for further processing.</param>
    /// <param name="output">The output frame.</param>
    void Render(RenderContext& renderContext, GPUTexture*& input, GPUTexture*& output);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psApplyDistortion->ReleaseGPU();
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
