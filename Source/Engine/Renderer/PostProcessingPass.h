// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

#define GB_RADIUS 6
#define GB_KERNEL_SIZE (GB_RADIUS * 2 + 1)

/// <summary>
/// Post processing rendering service
/// </summary>
class PostProcessingPass : public RendererPass<PostProcessingPass>
{
private:

    GPU_CB_STRUCT(Data{
        float BloomIntensity; // Overall bloom strength multiplier
        float BloomClamp; // Maximum brightness limit for bloom
        float BloomThreshold; // Luminance threshold where bloom begins
        float BloomThresholdKnee; // Controls the threshold rolloff curve

        float BloomBaseMix; // Base mip contribution
        float BloomHighMix; // High mip contribution
        float BloomMipCount;
        float BloomLayer;

        Float3 VignetteColor;
        float VignetteShapeFactor;

        Float2 InputSize;
        float InputAspect;
        float GrainAmount;

        float GrainTime;
        float GrainParticleSize;
        int32 Ghosts;
        float HaloWidth;

        float HaloIntensity;
        float Distortion;
        float GhostDispersal;
        float LensFlareIntensity;

        Float2 LensInputDistortion;
        float LensScale;
        float LensBias;

        Float2 InvInputSize;
        float ChromaticDistortion;
        float Time;

        float Dummy1;
        float PostExposure;
        float VignetteIntensity;
        float LensDirtIntensity;

        Color ScreenFadeColor;

        Matrix LensFlareStarMat;
        });

    GPU_CB_STRUCT(GaussianBlurData{
        Float2 Size;
        float Dummy3;
        float Dummy4;
        Float4 GaussianBlurCache[GB_KERNEL_SIZE]; // x-weight, y-offset
        });

    // Post Processing
    AssetReference<Shader> _shader;
    GPUPipelineState* _psBloomBrightPass;
    GPUPipelineState* _psBloomDownsample;
    GPUPipelineState* _psBloomDualFilterUpsample;
    GPUPipelineState* _psBlurH;
    GPUPipelineState* _psBlurV;
    GPUPipelineState* _psGenGhosts;
    GPUPipelineStatePermutationsPs<3> _psComposite;

    GaussianBlurData _gbData;
    Float4 GaussianBlurCacheH[GB_KERNEL_SIZE];
    Float4 GaussianBlurCacheV[GB_KERNEL_SIZE];

    AssetReference<Texture> _defaultLensColor;
    AssetReference<Texture> _defaultLensStar;
    AssetReference<Texture> _defaultLensDirt;

public:

    /// <summary>
    /// Init
    /// </summary>
    PostProcessingPass();

public:

    /// <summary>
    /// Perform postFx rendering for the input task
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">Target with rendered HDR frame to post process</param>
    /// <param name="output">Output frame</param>
    /// <param name="colorGradingLUT">The prebaked LUT for color grading and tonemapping.</param>
    void Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output, GPUTexture* colorGradingLUT);

private:

    GPUTexture* getCustomOrDefault(Texture* customTexture, AssetReference<Texture>& defaultTexture, const Char* defaultName);

    /// <summary>
    /// Calculates the Gaussian blur filter kernel. This implementation is
    /// ported from the original Java code appearing in chapter 16 of
    /// "Filthy Rich Clients: Developing Animated and Graphical Effects for Desktop Java".
    /// </summary>
    /// <param name="sigma">Gaussian Blur sigma parameter</param>
    /// <param name="width">Texture to blur width in pixels</param>
    /// <param name="height">Texture to blur height in pixels</param>
    void GB_ComputeKernel(float sigma, float width, float height);

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psBloomBrightPass->ReleaseGPU();
        _psBloomDownsample->ReleaseGPU();
        _psBloomDualFilterUpsample->ReleaseGPU();
        _psBlurH->ReleaseGPU();
        _psBlurV->ReleaseGPU();
        _psGenGhosts->ReleaseGPU();
        _psComposite.Release();
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
