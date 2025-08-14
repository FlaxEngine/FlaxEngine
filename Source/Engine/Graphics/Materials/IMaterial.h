// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialInfo.h"

struct MaterialParamsLink;
class GPUShader;
class GPUContext;
class GPUTextureView;
class GPUBufferView;
class GPUConstantBuffer;
class RenderBuffers;
class SceneRenderTask;
struct RenderView;
struct RenderContext;
struct DrawCall;

/// <summary>
/// Interface for material objects.
/// </summary>
class FLAXENGINE_API IMaterial
{
public:
    /// <summary>
    /// Gets the material info, structure which describes material surface.
    /// </summary>
    /// <returns>The constant reference to the material descriptor.</returns>
    virtual const MaterialInfo& GetInfo() const = 0;

    /// <summary>
    /// Gets the shader resource.
    /// </summary>
    /// <returns>The material shader resource.</returns>
    virtual GPUShader* GetShader() const = 0;

    /// <summary>
    /// Determines whether material is a surface shader.
    /// </summary>
    FORCE_INLINE bool IsSurface() const
    {
        return GetInfo().Domain == MaterialDomain::Surface;
    }

    /// <summary>
    /// Determines whether material is a post fx.
    /// </summary>
    FORCE_INLINE bool IsPostFx() const
    {
        return GetInfo().Domain == MaterialDomain::PostProcess;
    }

    /// <summary>
    /// Determines whether material is a decal.
    /// </summary>
    FORCE_INLINE bool IsDecal() const
    {
        return GetInfo().Domain == MaterialDomain::Decal;
    }

    /// <summary>
    /// Determines whether material is a GUI shader.
    /// </summary>
    FORCE_INLINE bool IsGUI() const
    {
        return GetInfo().Domain == MaterialDomain::GUI;
    }

    /// <summary>
    /// Determines whether material is a terrain shader.
    /// </summary>
    FORCE_INLINE bool IsTerrain() const
    {
        return GetInfo().Domain == MaterialDomain::Terrain;
    }

    /// <summary>
    /// Determines whether material is a particle shader.
    /// </summary>
    FORCE_INLINE bool IsParticle() const
    {
        return GetInfo().Domain == MaterialDomain::Particle;
    }

    /// <summary>
    /// Determines whether material is a deformable shader.
    /// </summary>
    FORCE_INLINE bool IsDeformable() const
    {
        return GetInfo().Domain == MaterialDomain::Deformable;
    }

    /// <summary>
    /// Returns true if material is ready for rendering.
    /// </summary>
    /// <returns>True if can render that material</returns>
    virtual bool IsReady() const = 0;

    /// <summary>
    /// Gets the mask of render passes supported by this material.
    /// </summary>
    /// <returns>The draw passes supported by this material.</returns>
    virtual DrawPass GetDrawModes() const
    {
        return DrawPass::None;
    }

    /// <summary>
    /// Returns true if material can use lightmaps (this includes lightmaps offline baking and sampling at runtime).
    /// </summary>
    /// <returns>True if can use lightmaps, otherwise false</returns>
    virtual bool CanUseLightmap() const
    {
        return false;
    }

    /// <summary>
    /// The instancing handling used to hash, batch and write draw calls.
    /// </summary>
    struct InstancingHandler
    {
        void (*GetHash)(const DrawCall& drawCall, uint32& batchKey);
        bool (*CanBatch)(const DrawCall& a, const DrawCall& b, DrawPass pass);
    };

    /// <summary>
    /// Returns true if material can use draw calls instancing.
    /// </summary>
    /// <param name="handler">The output data for the instancing handling used to hash, batch and write draw calls. Valid only when function returns true.</param>
    /// <returns>True if can use instancing, otherwise false.</returns>
    virtual bool CanUseInstancing(InstancingHandler& handler) const
    {
#if BUILD_DEBUG
        handler = { nullptr, nullptr };
#endif
        return false;
    }

public:
    /// <summary>
    /// Settings for the material binding to the graphics pipeline.
    /// </summary>
    struct BindParameters
    {
        GPUContext* GPUContext;
        const RenderContext& RenderContext;
        GPUBufferView* ObjectBuffer = nullptr;
        const ::DrawCall* DrawCall = nullptr;
        MaterialParamsLink* ParamsLink = nullptr;
        void* CustomData = nullptr;
        float Time, UnscaledTime;
        bool Instanced = false;

        /// <summary>
        /// The input scene color. It's optional and used in forward/postFx rendering.
        /// </summary>
        GPUTextureView* Input = nullptr;

        BindParameters(::GPUContext* context, const ::RenderContext& renderContext);
        BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const ::DrawCall& drawCall, bool instanced = false);

        // Per-view shared constant buffer (see ViewData in MaterialCommon.hlsl).
        static GPUConstantBuffer* PerViewConstants;
        // Per-draw shared constant buffer (see ViewData in MaterialCommon.hlsl).
        static GPUConstantBuffer* PerDrawConstants;

        // Binds the shared per-view constant buffer at slot 1 (see ViewData in MaterialCommon.hlsl)
        void BindViewData();
        // Binds the shared per-draw constant buffer at slot 2 (see DrawData in MaterialCommon.hlsl)
        void BindDrawData();
    };

    /// <summary>
    /// Binds the material state to the GPU pipeline. Should be called before the draw command.
    /// </summary>
    /// <param name="params">The material bind settings.</param>
    virtual void Bind(BindParameters& params) = 0;
};
