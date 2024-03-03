// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../RendererPass.h"

/// <summary>
/// Global Surface Atlas rendering pass. Captures scene geometry into a single atlas texture which contains surface diffuse color, normal vector, emission light, and calculates direct+indirect lighting. Used by Global Illumination and Reflections.
/// </summary>
class FLAXENGINE_API GlobalSurfaceAtlasPass : public RendererPass<GlobalSurfaceAtlasPass>
{
public:
    // Constant buffer data for Global Surface Atlas access on a GPU.
    PACK_STRUCT(struct ConstantsData
        {
        Float3 ViewPos;
        float Padding0;
        float Padding1;
        float Resolution;
        float ChunkSize;
        uint32 ObjectsCount;
        });

    // Binding data for the GPU.
    struct BindingData
    {
        union
        {
            struct
            {
                GPUTexture* AtlasDepth;
                GPUTexture* AtlasGBuffer0;
                GPUTexture* AtlasGBuffer1;
                GPUTexture* AtlasGBuffer2;
                GPUTexture* AtlasLighting;
            };
            GPUTexture* Atlas[5];
        };
        GPUBuffer* Chunks;
        GPUBuffer* CulledObjects;
        GPUBuffer* Objects;
        ConstantsData Constants;
    };

private:
    bool _supported = false;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psClear = nullptr;
    GPUPipelineState* _psClearLighting = nullptr;
    GPUPipelineState* _psDirectLighting0 = nullptr;
    GPUPipelineState* _psDirectLighting1 = nullptr;
    GPUPipelineState* _psIndirectLighting = nullptr;
    GPUPipelineState* _psDebug0 = nullptr;
    GPUPipelineState* _psDebug1 = nullptr;
    GPUConstantBuffer* _cb0 = nullptr;
    GPUShaderProgramCS* _csCullObjects;

    // Cache
    class GPUBuffer* _culledObjectsSizeBuffer = nullptr;
    class DynamicVertexBuffer* _vertexBuffer = nullptr;
    class GlobalSurfaceAtlasCustomBuffer* _surfaceAtlasData;
    Array<void*> _dirtyObjectsBuffer;
    uint64 _culledObjectsSizeFrames[8];
    Vector4 _cullingPosDistance;
    void* _currentActorObject;

public:
    /// <summary>
    /// Renders the Global Surface Atlas.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="result">The result Global Surface Atlas data for binding to the shaders.</param>
    /// <returns>True if failed to render (platform doesn't support it, out of video memory, disabled feature or effect is not ready), otherwise false.</returns>
    bool Render(RenderContext& renderContext, GPUContext* context, BindingData& result);

    /// <summary>
    /// Renders the debug view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="output">The output buffer.</param>
    void RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output);

    // Gets the culling view position (xyz) and view distance (w)
    void GetCullingData(Vector4& cullingPosDistance) const
    {
        cullingPosDistance = _cullingPosDistance;
    }
    
    // Gets the current object of the actor that is drawn into atlas.
    void* GetCurrentActorObject() const
    {
        return _currentActorObject;
    }

    // Rasterize actor into the Global Surface Atlas. Call it from actor Draw() method during DrawPass::GlobalSurfaceAtlas.
    void RasterizeActor(Actor* actor, void* actorObject, const BoundingSphere& actorObjectBounds, const Transform& localToWorld, const BoundingBox& localBounds, uint32 tilesMask = MAX_uint32, bool useVisibility = true, float qualityScale = 1.0f);

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
