// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Anti aliasing rendering service
/// </summary>
class MotionBlurPass : public RendererPass<MotionBlurPass>
{
private:

    PixelFormat _motionVectorsFormat;
    PixelFormat _velocityFormat;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psCameraMotionVectors = nullptr;

public:

    /// <summary>
    /// Init
    /// </summary>
    MotionBlurPass();

public:

    /// <summary>
    /// Renders the motion vectors texture for the current task. Skips if motion blur is disabled or no need to render motion vectors.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void RenderMotionVectors(RenderContext& renderContext);

    /// <summary>
    /// Renders the motion vectors debug view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="frame">The source frame.</param>
    void RenderDebug(RenderContext& renderContext, GPUTextureView* frame);

    /// <summary>
    /// Renders the motion blur. Swaps the input with output if rendering is performed. Does nothing if rendering is not performed.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">The input frame.</param>
    /// <param name="output">The output frame.</param>
    void Render(RenderContext& renderContext, GPUTexture*& input, GPUTexture*& output);

private:

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psCameraMotionVectors->ReleaseGPU();
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
