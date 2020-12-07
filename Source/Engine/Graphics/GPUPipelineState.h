// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Shaders/GPUShaderProgram.h"
#include "Enums.h"
#include "GPUResource.h"

/// <summary>
/// Describes full graphics pipeline state within single object.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API GPUPipelineState : public GPUResource
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUPipelineState);
    static GPUPipelineState* Spawn(const SpawnParams& params);
    static GPUPipelineState* New();

public:

    /// <summary>
    /// Pipeline state description
    /// </summary>
    API_STRUCT() struct Description
    {
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Description);

        /// <summary>
        /// Enable/disable depth write
        /// </summary>
        API_FIELD() bool DepthWriteEnable;

        /// <summary>
        /// Enable/disable depth test
        /// </summary>
        API_FIELD() bool DepthTestEnable;

        /// <summary>
        /// Enable/disable depth clipping
        /// </summary>
        API_FIELD() bool DepthClipEnable;

        /// <summary>
        /// A function that compares depth data against existing depth data
        /// </summary>
        API_FIELD() ComparisonFunc DepthFunc;

        /// <summary>
        /// Vertex shader program
        /// </summary>
        API_FIELD() GPUShaderProgramVS* VS;

        /// <summary>
        /// Hull shader program
        /// </summary>
        API_FIELD() GPUShaderProgramHS* HS;

        /// <summary>
        /// Domain shader program
        /// </summary>
        API_FIELD() GPUShaderProgramDS* DS;

        /// <summary>
        /// Geometry shader program
        /// </summary>
        API_FIELD() GPUShaderProgramGS* GS;

        /// <summary>
        /// Pixel shader program
        /// </summary>
        API_FIELD() GPUShaderProgramPS* PS;

        /// <summary>
        /// Input primitives topology
        /// </summary>
        API_FIELD() PrimitiveTopologyType PrimitiveTopologyType;

        /// <summary>
        /// True if use wireframe rendering, otherwise false
        /// </summary>
        API_FIELD() bool Wireframe;

        /// <summary>
        /// Primitives culling mode
        /// </summary>
        API_FIELD() CullMode CullMode;

        /// <summary>
        /// Colors blending mode
        /// </summary>
        API_FIELD() BlendingMode BlendMode;

    public:

        /// <summary>
        /// Default description
        /// </summary>
        API_FIELD(ReadOnly) static Description Default;

        /// <summary>
        /// Default description without using depth buffer at all
        /// </summary>
        API_FIELD(ReadOnly) static Description DefaultNoDepth;

        /// <summary>
        /// Default description for fullscreen triangle rendering
        /// </summary>
        API_FIELD(ReadOnly) static Description DefaultFullscreenTriangle;
    };

protected:

    ShaderBindings _meta;

public:

#if BUILD_DEBUG

    /// <summary>
    /// The description of the pipeline state cached on creation in debug builds. Can be used to help with rendering crashes or issues and validation.
    /// </summary>
    Description DebugDesc;

#endif

public:

    /// <summary>
    /// Gets constant buffers usage mask (each set bit marks usage of the constant buffer at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    /// <returns>The constant buffers usage mask.</returns>
    FORCE_INLINE uint32 GetUsedCBsMask() const
    {
        return _meta.UsedCBsMask;
    }

    /// <summary>
    /// Gets shader resources usage mask (each set bit marks usage of the shader resource slot at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    /// <returns>The shader resource usage mask.</returns>
    FORCE_INLINE uint32 GetUsedSRsMask() const
    {
        return _meta.UsedSRsMask;
    }

    /// <summary>
    /// Gets unordered access usage mask (each set bit marks usage of the unordered access slot at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    /// <returns>The unordered access usage mask.</returns>
    FORCE_INLINE uint32 GetUsedUAsMask() const
    {
        return _meta.UsedUAsMask;
    }

public:

    /// <summary>
    /// Returns true if pipeline state is valid and ready to use
    /// </summary>
    /// <returns>True if is valid</returns>
    API_PROPERTY() virtual bool IsValid() const = 0;

    /// <summary>
    /// Create new state data
    /// </summary>
    /// <param name="desc">Full pipeline state description</param>
    /// <returns>True if cannot create state, otherwise false</returns>
    API_FUNCTION() virtual bool Init(API_PARAM(Ref) const Description& desc)
    {
        // Cache description in debug builds
#if BUILD_DEBUG
        DebugDesc = desc;
#endif

        // Cache shader stages usage flags for pipeline state
        _meta.InstructionsCount = 0;
        _meta.UsedCBsMask = 0;
        _meta.UsedSRsMask = 0;
        _meta.UsedUAsMask = 0;
#define CHECK_STAGE(stage) \
			if(desc.stage) { \
			_meta.UsedCBsMask |= desc.stage->GetBindings().UsedCBsMask; \
			_meta.UsedSRsMask |= desc.stage->GetBindings().UsedSRsMask; \
			_meta.UsedUAsMask |= desc.stage->GetBindings().UsedUAsMask; \
			}
        CHECK_STAGE(VS);
        CHECK_STAGE(HS);
        CHECK_STAGE(DS);
        CHECK_STAGE(GS);
        CHECK_STAGE(PS);
#undef CHECK_STAGE

        return false;
    }

public:

    // [GPUResource]
    ResourceType GetResourceType() const final override
    {
        return ResourceType::PipelineState;
    }
};
