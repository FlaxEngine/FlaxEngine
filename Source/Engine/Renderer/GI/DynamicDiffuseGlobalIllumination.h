// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"
#include "Engine/Core/Math/Int4.h"
#include "Engine/Graphics/Textures/GPUTexture.h"

/// <summary>
/// Dynamic Diffuse Global Illumination rendering pass.
/// </summary>
class FLAXENGINE_API DynamicDiffuseGlobalIlluminationPass : public RendererPass<DynamicDiffuseGlobalIlluminationPass>
{
public:
    // Constant buffer data for DDGI access on a GPU.
    GPU_CB_STRUCT(ConstantsData {
        Float4 ProbesOriginAndSpacing[4];
        Int4 ProbesScrollOffsets[4];
        uint32 ProbesCounts[3];
        uint32 CascadesCount;
        float IrradianceGamma;
        float ProbeHistoryWeight;
        float RayMaxDistance;
        float IndirectLightingIntensity;
        Float3 ViewPos;
        uint32 RaysCount;
        Float3 FallbackIrradiance;
        float Padding0;
        });

    // Binding data for the GPU.
    struct BindingData
    {
        ConstantsData Constants;
        GPUTextureView* ProbesData;
        GPUTextureView* ProbesDistance;
        GPUTextureView* ProbesIrradiance;
    };

private:
    bool _supported = false;
    AssetReference<Shader> _shader;
    GPUConstantBuffer* _cb0 = nullptr;
    GPUConstantBuffer* _cb1 = nullptr;
    GPUShaderProgramCS* _csClassify;
    GPUShaderProgramCS* _csUpdateProbesInitArgs;
    GPUShaderProgramCS* _csTraceRays[4];
    GPUShaderProgramCS* _csUpdateProbesIrradiance;
    GPUShaderProgramCS* _csUpdateProbesDistance;
    GPUPipelineState* _psIndirectLighting[2] = {};
#if USE_EDITOR
    AssetReference<Model> _debugModel;
    AssetReference<MaterialBase> _debugMaterial;
#endif

public:
    /// <summary>
    /// Gets the DDGI binding data (only if enabled).
    /// </summary>
    /// <param name="buffers">The rendering context buffers.</param>
    /// <param name="result">The result DDGI data for binding to the shaders.</param>
    /// <returns>True if failed to render (platform doesn't support it, out of video memory, disabled feature or effect is not ready), otherwise false.</returns>
    bool Get(const RenderBuffers* buffers, BindingData& result);

    /// <summary>
    /// Renders the DDGI.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="lightBuffer">The light accumulation buffer (input and output).</param>
    /// <returns>True if failed to render (platform doesn't support it, out of video memory, disabled feature or effect is not ready), otherwise false.</returns>
    bool Render(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer);

private:
#if COMPILE_WITH_DEV_ENV
    uint64 LastFrameShaderReload = 0;
    void OnShaderReloading(Asset* obj);
#endif
    bool RenderInner(RenderContext& renderContext, GPUContext* context, class DDGICustomBuffer& ddgiData);

public:
    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

protected:
    // [RendererPass]
    bool setupResources() override;
};
