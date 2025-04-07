// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

// Config
#define SSAO_DEPTH_MIP_LEVELS 4 // <- must match shader define
#define SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET (99) // <- must match shader define
#define SSAO_DEPTH_FORMAT PixelFormat::R16_Float
#define SSAO_AO_RESULT_FORMAT PixelFormat::R8G8_UNorm
#define SSAO_MAX_BLUR_PASS_COUNT 6

/// <summary>
/// Screen Space Ambient Occlusion rendering service
/// Current implementation is based on ASSAO: https://github.com/GameTechDev/ASSAO
/// </summary>
class AmbientOcclusionPass : public RendererPass<AmbientOcclusionPass>
{
private:

    // Packed shader constant buffer structure (this MUST match shader code)
    GPU_CB_STRUCT(ASSAOConstants {
        ShaderGBufferData GBuffer;

        Float2 ViewportPixelSize;
        Float2 HalfViewportPixelSize;

        int32 PerPassFullResCoordOffsetX;
        int32 PerPassFullResCoordOffsetY;
        int32 PassIndex;
        float EffectMaxDistance;

        Float2 Viewport2xPixelSize;
        Float2 Viewport2xPixelSize_x_025;

        float EffectRadius;
        float EffectShadowStrength;
        float EffectShadowPow;
        float EffectNearFadeMul;

        float EffectFadeOutMul;
        float EffectFadeOutAdd;
        float EffectHorizonAngleThreshold;
        float EffectSamplingRadiusNearLimitRec;

        float DepthPrecisionOffsetMod;
        float NegRecEffectRadius;
        float InvSharpness;
        float DetailAOStrength;

        Float4 PatternRotScaleMatrices[5];

        Matrix ViewMatrix;
        });

    // Effect visual settings
    struct ASSAO_Settings
    {
        float Radius; // [0.0,  ~ ] World (view) space size of the occlusion sphere.
        float ShadowMultiplier; // [0.0, 5.0] Effect strength linear multiplier
        float ShadowPower; // [0.5, 5.0] Effect strength pow modifier
        float HorizonAngleThreshold; // [0.0, 0.2] Limits self-shadowing (makes the sampling area less of a hemisphere, more of a spherical cone, to avoid self-shadowing and various artifacts due to low tessellation and depth buffer imprecision, etc.)
        float FadeOutFrom; // [0.0,  ~ ] Distance to start fading out the effect.
        float FadeOutTo; // [0.0,  ~ ] Distance at which the effect is faded out.
        int QualityLevel; // [  0,    ] Effect quality; 0 - low, 1 - medium, 2 - high, 3 - very high; each quality level is roughly 2x more costly than the previous, except the q3 which is variable but, in general, above q2.
        int BlurPassCount; // [  0,   6] Number of edge-sensitive smart blur passes to apply. Quality 0 is an exception with only one 'dumb' blur pass used.
        float Sharpness; // [0.0, 1.0] (How much to bleed over edges; 1: not at all, 0.5: half-half; 0.0: completely ignore edges)
        float DetailShadowStrength; // [0.0, 5.0] Used for high-res detail AO using neighboring depth pixels: adds a lot of detail but also reduces temporal stability (adds aliasing).
        bool SkipHalfPixels; // [true/false] Use half of the pixels (checkerboard pattern)

        ASSAO_Settings();
    };

private:

    AssetReference<Shader> _shader;

    // All shader programs
    GPUPipelineState* _psPrepareDepths;
    GPUPipelineState* _psPrepareDepthsHalf;
    GPUPipelineState* _psPrepareDepthMip[SSAO_DEPTH_MIP_LEVELS - 1];
    GPUPipelineState* _psGenerate[4];
    GPUPipelineState* _psSmartBlur;
    GPUPipelineState* _psSmartBlurWide;
    GPUPipelineState* _psNonSmartBlur;
    GPUPipelineState* _psApply;
    GPUPipelineState* _psApplyHalf;

    // Temporary render targets used by the effect
    GPUTexture* m_halfDepths[4];
    GPUTexture* m_pingPongHalfResultA;
    GPUTexture* m_pingPongHalfResultB;
    GPUTexture* m_finalResults;

    // Cached data
    float m_sizeX;
    float m_sizeY;
    float m_halfSizeX;
    float m_halfSizeY;
    ASSAOConstants _constantsBufferData;
    ASSAO_Settings settings;

public:

    /// <summary>
    /// Init
    /// </summary>
    AmbientOcclusionPass();

public:

    /// <summary>
    /// Perform SSAO rendering for the input task
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Render(RenderContext& renderContext);

private:

    void InitRTs(const RenderContext& renderContext);
    void ReleaseRTs(const RenderContext& renderContext);
    void UpdateCB(const RenderContext& renderContext, GPUContext* context, const int32 passIndex);
    void PrepareDepths(const RenderContext& renderContext);
    void GenerateSSAO(const RenderContext& renderContext);
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psPrepareDepths->ReleaseGPU();
        _psPrepareDepthsHalf->ReleaseGPU();
        for (int32 i = 0; i < ARRAY_COUNT(_psPrepareDepthMip); i++)
            _psPrepareDepthMip[i]->ReleaseGPU();
        for (int32 i = 0; i < ARRAY_COUNT(_psGenerate); i++)
            _psGenerate[i]->ReleaseGPU();
        _psSmartBlur->ReleaseGPU();
        _psSmartBlurWide->ReleaseGPU();
        _psNonSmartBlur->ReleaseGPU();
        _psApply->ReleaseGPU();
        _psApplyHalf->ReleaseGPU();
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
