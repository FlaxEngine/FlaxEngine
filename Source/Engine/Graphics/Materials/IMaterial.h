// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/Task.h"
#include "MaterialInfo.h"

struct MaterialParamsLink;
class GPUContext;
class GPUTextureView;
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
    /// Determines whether material is a surface shader.
    /// </summary>
    /// <returns><c>true</c> if material is surface shader; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsSurface() const
    {
        return GetInfo().Domain == MaterialDomain::Surface;
    }

    /// <summary>
    /// Determines whether material is a post fx.
    /// </summary>
    /// <returns><c>true</c> if material is post fx; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsPostFx() const
    {
        return GetInfo().Domain == MaterialDomain::PostProcess;
    }

    /// <summary>
    /// Determines whether material is a decal.
    /// </summary>
    /// <returns><c>true</c> if material is decal; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsDecal() const
    {
        return GetInfo().Domain == MaterialDomain::Decal;
    }

    /// <summary>
    /// Determines whether material is a GUI shader.
    /// </summary>
    /// <returns><c>true</c> if material is GUI shader; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsGUI() const
    {
        return GetInfo().Domain == MaterialDomain::GUI;
    }

    /// <summary>
    /// Determines whether material is a terrain shader.
    /// </summary>
    /// <returns><c>true</c> if material is terrain shader; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsTerrain() const
    {
        return GetInfo().Domain == MaterialDomain::Terrain;
    }

    /// <summary>
    /// Determines whether material is a particle shader.
    /// </summary>
    /// <returns><c>true</c> if material is particle shader; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsParticle() const
    {
        return GetInfo().Domain == MaterialDomain::Particle;
    }

    /// <summary>
    /// Checks if material needs vertex color vertex buffer data for rendering.
    /// </summary>
    /// <returns>True if mesh renderer have to provide vertex color buffer to render that material</returns>
    FORCE_INLINE bool RequireVertexColor() const
    {
        return (GetInfo().UsageFlags & MaterialUsageFlags::UseVertexColor) != 0;
    }

    /// <summary>
    /// Checks if material supports dithered LOD transitions.
    /// </summary>
    /// <returns>True if material supports dithered LOD transitions, otherwise false.</returns>
    FORCE_INLINE bool IsDitheredLODTransition() const
    {
        return (GetInfo().FeaturesFlags & MaterialFeaturesFlags::DitheredLODTransition) != 0;
    }

    /// <summary>
    /// Returns true if material is ready for rendering.
    /// </summary>
    /// <returns>True if can render that material</returns>
    virtual bool IsReady() const = 0;

    /// <summary>
    /// Gets the mask of render passes supported by this material.
    /// </summary>
    /// <returns>The drw passes supported by this material.</returns>
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
    /// Returns true if material can use draw calls instancing.
    /// </summary>
    /// <returns>True if can use instancing, otherwise false.</returns>
    virtual bool CanUseInstancing() const
    {
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
        const DrawCall* FirstDrawCall;
        int32 DrawCallsCount;
        MaterialParamsLink* ParamsLink = nullptr;
        void* CustomData = nullptr;

        /// <summary>
        /// The input scene color. It's optional and used in forward/postFx rendering.
        /// </summary>
        GPUTextureView* Input = nullptr;

        BindParameters(::GPUContext* context, const ::RenderContext& renderContext)
            : GPUContext(context)
            , RenderContext(renderContext)
            , FirstDrawCall(nullptr)
            , DrawCallsCount(0)
        {
        }

        BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const DrawCall& drawCall)
            : GPUContext(context)
            , RenderContext(renderContext)
            , FirstDrawCall(&drawCall)
            , DrawCallsCount(1)
        {
        }

        BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const DrawCall* firstDrawCall, int32 drawCallsCount)
            : GPUContext(context)
            , RenderContext(renderContext)
            , FirstDrawCall(firstDrawCall)
            , DrawCallsCount(drawCallsCount)
        {
        }
    };

    /// <summary>
    /// Binds the material state to the GPU pipeline. Should be called before the draw command.
    /// </summary>
    /// <param name="params">The material bind settings.</param>
    virtual void Bind(BindParameters& params) = 0;
};
