// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"
#include "Engine/Core/Math/Vector3.h"

/// <summary>
/// Global Sign Distance Field (SDF) rendering pass. Composites scene geometry into series of 3D volume textures that cover the world around the camera for global distance field sampling.
/// </summary>
class FLAXENGINE_API GlobalSignDistanceFieldPass : public RendererPass<GlobalSignDistanceFieldPass>
{
public:
    // Constant buffer data for Global SDF access on a GPU.
    PACK_STRUCT(struct ConstantsData
        {
        Float4 CascadePosDistance[4];
        Float4 CascadeVoxelSize;
        Float2 Padding;
        uint32 CascadesCount;
        float Resolution;
        });

    // Binding data for the GPU.
    struct BindingData
    {
        GPUTexture* Texture;
        GPUTexture* TextureMip;
        ConstantsData Constants;
    };

private:
    bool _supported = false;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psDebug = nullptr;
    GPUShaderProgramCS* _csRasterizeModel0 = nullptr;
    GPUShaderProgramCS* _csRasterizeModel1 = nullptr;
    GPUShaderProgramCS* _csRasterizeHeightfield = nullptr;
    GPUShaderProgramCS* _csClearChunk = nullptr;
    GPUShaderProgramCS* _csGenerateMip = nullptr;
    GPUConstantBuffer* _cb0 = nullptr;
    GPUConstantBuffer* _cb1 = nullptr;

    // Rasterization cache
    class DynamicStructuredBuffer* _objectsBuffer = nullptr;
    Array<GPUTextureView*> _objectsTextures;
    uint16 _objectsBufferCount;
    int32 _cascadeIndex;
    float _voxelSize, _chunkSize;
    BoundingBox _cascadeBounds;
    BoundingBox _cascadeCullingBounds;
    class GlobalSignDistanceFieldCustomBuffer* _sdfData;
    Vector3 _sdfDataOriginMin;
    Vector3 _sdfDataOriginMax;

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

    void GetCullingData(BoundingBox& bounds) const
    {
        bounds = _cascadeCullingBounds;
    }

    // Rasterize Model SDF into the Global SDF. Call it from actor Draw() method during DrawPass::GlobalSDF.
    void RasterizeModelSDF(Actor* actor, const ModelBase::SDFData& sdf, const Transform& localToWorld, const BoundingBox& objectBounds);

    void RasterizeHeightfield(Actor* actor, GPUTexture* heightfield, const Transform& localToWorld, const BoundingBox& objectBounds, const Float4& localToUV);

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
