// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Global Sign Distance Field (SDF) rendering pass. Composites scene geometry into series of 3D volume textures that cover the world around the camera for global distance field sampling.
/// </summary>
class FLAXENGINE_API GlobalSignDistanceFieldPass : public RendererPass<GlobalSignDistanceFieldPass>
{
public:
    // Constant buffer data for Global SDF access on a GPU.
    PACK_STRUCT(struct GlobalSDFData
        {
        Vector4 CascadePosDistance[4];
        Vector4 CascadeVoxelSize;
        Vector3 Padding;
        float Resolution;
        });

    // Binding data for the GPU.
    struct BindingData
    {
        GPUTexture* Cascades[4];
        GPUTexture* CascadeMips[4];
        GlobalSDFData GlobalSDF;
    };

private:
    bool _supported = false;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psDebug = nullptr;
    GPUShaderProgramCS* _csRasterizeModel0 = nullptr;
    GPUShaderProgramCS* _csRasterizeModel1 = nullptr;
    GPUShaderProgramCS* _csRasterizeHeightfield = nullptr;
    GPUShaderProgramCS* _csClearChunk = nullptr;
    GPUShaderProgramCS* _csGenerateMip0 = nullptr;
    GPUShaderProgramCS* _csGenerateMip1 = nullptr;
    GPUConstantBuffer* _cb0 = nullptr;
    GPUConstantBuffer* _cb1 = nullptr;

    // Rasterization cache
    class DynamicStructuredBuffer* _objectsBuffer = nullptr;
    Array<GPUTextureView*> _objectsTextures;
    uint16 _objectsBufferCount;
    int32 _cascadeIndex;
    float _voxelSize;
    BoundingBox _cascadeBounds;
    class GlobalSignDistanceFieldCustomBuffer* _sdfData;

public:
    /// <summary>
    /// Gets the Global SDF (only if enabled in Graphics Settings).
    /// </summary>
    /// <param name="buffers">The rendering context buffers.</param>
    /// <param name="result">The result Global SDF data for binding to the shaders.</param>
    /// <returns>True if there is no valid Global SDF rendered during this frame, otherwise false.</returns>
    bool Get(const RenderBuffers* buffers, BindingData& result);

    /// <summary>
    /// Renders the Global SDF.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="result">The result Global SDF data for binding to the shaders.</param>
    /// <returns>True if failed to render (platform doesn't support it, out of video memory, disabled feature or effect is not ready), otherwise false.</returns>
    bool Render(RenderContext& renderContext, GPUContext* context, BindingData& result);

    /// <summary>
    /// Renders the debug view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="output">The output buffer.</param>
    void RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output);

    // Rasterize Model SDF into the Global SDF. Call it from actor Draw() method during DrawPass::GlobalSDF.
    void RasterizeModelSDF(Actor* actor, const ModelBase::SDFData& sdf, const Matrix& localToWorld, const BoundingBox& objectBounds);

    void RasterizeHeightfield(Actor* actor, GPUTexture* heightfield, const Matrix& localToWorld, const BoundingBox& objectBounds, const Vector4& localToUV);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj);
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
