// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Anti aliasing rendering service
/// </summary>
class MotionBlurPass : public RendererPass<MotionBlurPass>
{
private:
    PACK_STRUCT(struct Data {
        GBufferData GBuffer;
        Matrix CurrentVP;
        Matrix PreviousVP;
        Vector4 TemporalAAJitter;
        Vector2 TileMaxOffs;
        float VelocityScale;
        int32 TileMaxLoop;
        float MaxBlurRadius;
        float RcpMaxBlurRadius;
        Vector2 TexelSize1;
        Vector2 TexelSize2;
        Vector2 TexelSize4;
        Vector2 TexelSizeV;
        Vector2 TexelSizeNM;
        float LoopCount;
        float Dummy0;
        Vector2 MotionVectorsTexelSize;
        float DebugBlend;
        float DebugAmplitude;
        int32 DebugColumnCount;
        int32 DebugRowCount;
        });

    PixelFormat _motionVectorsFormat;
    PixelFormat _velocityFormat;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psCameraMotionVectors;
    GPUPipelineState* _psMotionVectorsDebug;
    GPUPipelineState* _psMotionVectorsDebugArrow;
    GPUPipelineState* _psVelocitySetup;
    GPUPipelineState* _psTileMax1;
    GPUPipelineState* _psTileMax2;
    GPUPipelineState* _psTileMax4;
    GPUPipelineState* _psTileMaxV;
    GPUPipelineState* _psNeighborMax;
    GPUPipelineState* _psReconstruction;

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
        _psMotionVectorsDebug->ReleaseGPU();
        _psMotionVectorsDebugArrow->ReleaseGPU();
        _psVelocitySetup->ReleaseGPU();
        _psTileMax1->ReleaseGPU();
        _psTileMax2->ReleaseGPU();
        _psTileMax4->ReleaseGPU();
        _psTileMaxV->ReleaseGPU();
        _psNeighborMax->ReleaseGPU();
        _psReconstruction->ReleaseGPU();
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
