// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"
#include "RendererPass.h"
#include "GI/DynamicDiffuseGlobalIllumination.h"

struct VolumetricFogOptions;
struct RendererSpotLightData;
struct RendererPointLightData;

/// <summary>
/// Volumetric fog rendering service.
/// </summary>
class VolumetricFogPass : public RendererPass<VolumetricFogPass>
{
public:

    struct CustomData
    {
        GPUShader* Shader;
        Float3 GridSize;
        float VolumetricFogMaxDistance;
        int32 ParticleIndex;
    };

private:

    PACK_STRUCT(struct SkyLightData {
        Float3 MultiplyColor;
        float VolumetricScatteringIntensity;
        Float3 AdditiveColor;
        float Dummt0;
        });

    PACK_STRUCT(struct Data {
        GBufferData GBuffer;

        Float3 GlobalAlbedo;
        float GlobalExtinctionScale;

        Float3 GlobalEmissive;
        float HistoryWeight;

        Float3 GridSize;
        uint32 MissedHistorySamplesCount;

        uint32 GridSizeIntX;
        uint32 GridSizeIntY;
        uint32 GridSizeIntZ;
        float PhaseG;

        Float2 Dummy0;
        float VolumetricFogMaxDistance;
        float InverseSquaredLightDistanceBiasScale;

        Float4 FogParameters;

        Matrix PrevWorldToClip;

        Float4 FrameJitterOffsets[8];

        LightData DirectionalLight;
        LightShadowData DirectionalLightShadow;
        SkyLightData SkyLight;
        DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
        });

    PACK_STRUCT(struct PerLight {
        Float2 SliceToDepth;
        int32 MinZ;
        float LocalLightScatteringIntensity;

        Float4 ViewSpaceBoundingSphere;
        Matrix ViewToVolumeClip;

        LightData LocalLight;
        LightShadowData LocalLightShadow;
        });

    // Shader stuff
    AssetReference<Shader> _shader;
    GPUShaderProgramCS* _csInitialize = nullptr;
    ComputeShaderPermutation<2> _csLightScattering;
    GPUShaderProgramCS* _csFinalIntegration = nullptr;
    GPUPipelineStatePermutationsPs<2> _psInjectLight;

    GPUBuffer* _vbCircleRasterize = nullptr;
    GPUBuffer* _ibCircleRasterize = nullptr;

    /// <summary>
    /// The current frame cache (initialized during Init)
    /// </summary>
    struct FrameCache
    {
        /// <summary>
        /// The XY size of a cell in the voxel grid, in pixels.
        /// </summary>
        int32 GridPixelSize;

        /// <summary>
        /// How many Volumetric Fog cells to use in z (depth from camera).
        /// </summary>
        int32 GridSizeZ;

        /// <summary>
        ///  Whether to apply jitter to each frame's volumetric fog. Should be used with temporal reprojection to improve the quality.
        /// </summary>
        bool FogJitter;

        /// <summary>
        /// How much the history value should be weighted each frame. This is a tradeoff between visible jittering and responsiveness.
        /// </summary>
        float HistoryWeight;

        /// <summary>
        /// The amount of lighting samples to compute for voxels whose history value is not available. This reduces noise when panning or on camera cuts, but introduces a variable cost to volumetric fog computation. Valid range [1, 8].
        /// </summary>
        int32 MissedHistorySamplesCount;

        /// <summary>
        /// Scales the amount added to the inverse squared falloff denominator. This effectively removes the spike from inverse squared falloff that causes extreme aliasing.
        /// </summary>
        float InverseSquaredLightDistanceBiasScale;

        /// <summary>
        /// The calculated size of the volume texture.
        /// </summary>
        Float3 GridSize;

        /// <summary>
        /// The cached per-frame data for the constant buffer.
        /// </summary>
        Data Data;
    };

    FrameCache _cache;
    bool _isSupported;

public:

    /// <summary>
    /// Init
    /// </summary>
    VolumetricFogPass();

public:

    /// <summary>
    /// Renders the light to the volumetric fog light scattering volume texture. Called by the light pass after shadow map rendering. Used by the shadows casting lights.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMap">The shadow map.</param>
    /// <param name="shadow">The light shadow data.</param>
    void RenderLight(RenderContext& renderContext, GPUContext* context, RendererPointLightData& light, GPUTextureView* shadowMap, LightShadowData& shadow);

    /// <summary>
    /// Renders the light to the volumetric fog light scattering volume texture. Called by the light pass after shadow map rendering. Used by the shadows casting lights.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU commands context.</param>
    /// <param name="light">The light.</param>
    /// <param name="shadowMap">The shadow map.</param>
    /// <param name="shadow">The light shadow data.</param>
    void RenderLight(RenderContext& renderContext, GPUContext* context, RendererSpotLightData& light, GPUTextureView* shadowMap, LightShadowData& shadow);

    /// <summary>
    /// Renders the volumetric fog (generates integrated light scattering 3D texture). Does nothing if feature is disabled or not supported.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Render(RenderContext& renderContext);

private:

    bool Init(RenderContext& renderContext, GPUContext* context, VolumetricFogOptions& options);
    GPUTextureView* GetLocalShadowedLightScattering(RenderContext& renderContext, GPUContext* context, VolumetricFogOptions& options) const;
    void InitCircleBuffer();
    template<typename T>
    void RenderRadialLight(RenderContext& renderContext, GPUContext* context, T& light, LightShadowData& shadow);
    template<typename T>
    void RenderRadialLight(RenderContext& renderContext, GPUContext* context, RenderView& view, VolumetricFogOptions& options, T& light, PerLight& perLight, GPUConstantBuffer* cb1);
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psInjectLight.Release();
        _csInitialize = nullptr;
        _csLightScattering.Clear();
        _csFinalIntegration = nullptr;
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
