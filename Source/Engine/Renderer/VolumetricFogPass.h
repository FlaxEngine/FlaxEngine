// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Graphics/GPUPipelineStatePermutations.h"

struct VolumetricFogOptions;
struct RenderSpotLightData;
struct RenderPointLightData;

/// <summary>
/// Volumetric fog rendering service.
/// </summary>
class VolumetricFogPass : public RendererPass<VolumetricFogPass>
{
public:
    // Data used by particle material for rasterization into Volumetric Fog buffers (see VolumeParticleMaterialShader)
    struct CustomData
    {
        GPUShader* Shader;
        Float3 GridSize;
        float VolumetricFogMaxDistance;
        Float4 GridSliceParameters;
        int32 ParticleIndex;
    };

private:
    AssetReference<Shader> _shader;
    GPUShaderProgramCS* _csInitialize = nullptr;
    ComputeShaderPermutation<2> _csLightScattering;
    GPUShaderProgramCS* _csFinalIntegration = nullptr;
    GPUPipelineStatePermutationsPs<2> _psInjectLight;
    GPUBuffer* _vbCircleRasterize = nullptr;
    GPUBuffer* _ibCircleRasterize = nullptr;
    bool _isSupported = false;

public:
    /// <summary>
    /// Renders the volumetric fog (generates integrated light scattering 3D texture). Does nothing if feature is disabled or not supported.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Render(RenderContext& renderContext);

private:
    bool Init(struct FrameCache& cache, RenderContext& renderContext, GPUContext* context);
    bool InitSphereRasterize(FrameCache& cache, struct RasterizeSphere& sphere, RenderView& view, const Float3& center, float radius);
    GPUTextureView* GetLocalShadowedLightScattering(FrameCache& cache, RenderContext& renderContext, GPUContext* context) const;
    void InitCircleBuffer();
    template<typename T>
    void RenderRadialLight(FrameCache& cache, RenderContext& renderContext, GPUContext* context, T& light, struct PerLight& perLight, GPUConstantBuffer* cb2);
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        // TODO: this should reload all Materials that use VolumeParticleMaterialShader
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
