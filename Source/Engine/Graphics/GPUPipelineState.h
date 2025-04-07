// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Shaders/GPUShaderProgram.h"
#include "Enums.h"
#include "GPUResource.h"

/// <summary>
/// Stencil operation modes.
/// </summary>
API_ENUM() enum class StencilOperation : byte
{
    // Keep the existing stencil data.
    Keep,
    // Set the stencil data to 0.
    Zero,
    // Set the stencil data to the reference value (set via GPUContext::SetStencilRef).
    Replace,
    // Increment the stencil value by 1, and clamp the result.
    IncrementSaturated,
    // Decrement the stencil value by 1, and clamp the result.
    DecrementSaturated,
    // Invert the stencil data.
    Invert,
    // Increment the stencil value by 1, and wrap the result if necessary.
    Increment,
    // Decrement the stencil value by 1, and wrap the result if necessary.
    Decrement,

    API_ENUM(Attributes="HideInEditor") MAX
};

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
    API_STRUCT() struct FLAXENGINE_API Description
    {
        DECLARE_SCRIPTING_TYPE_NO_SPAWN(Description);

        /// <summary>
        /// Enable/disable depth (DepthFunc and DepthWriteEnable)
        /// </summary>
        API_FIELD() bool DepthEnable;

        /// <summary>
        /// Enable/disable depth write
        /// </summary>
        API_FIELD() bool DepthWriteEnable;

        /// <summary>
        /// Enable/disable depth clipping
        /// </summary>
        API_FIELD() bool DepthClipEnable;

        /// <summary>
        /// A function that compares depth data against existing depth data
        /// </summary>
        API_FIELD() ComparisonFunc DepthFunc;

        /// <summary>
        /// Enable/disable stencil buffer usage
        /// </summary>
        API_FIELD() bool StencilEnable;

        /// <summary>
        /// The read mask applied to the reference value and each stencil buffer entry to determine the significant bits for the stencil test.
        /// </summary>
        API_FIELD() uint8 StencilReadMask;

        /// <summary>
        /// The write mask applied to values written into the stencil buffer.
        /// </summary>
        API_FIELD() uint8 StencilWriteMask;

        /// <summary>
        /// The comparison function for the stencil test.
        /// </summary>
        API_FIELD() ComparisonFunc StencilFunc;
        
        /// <summary>
        /// The stencil operation to perform when stencil testing fails.
        /// </summary>
        API_FIELD() StencilOperation StencilFailOp;
        
        /// <summary>
        /// The stencil operation to perform when stencil testing passes and depth testing fails.
        /// </summary>
        API_FIELD() StencilOperation StencilDepthFailOp;
        
        /// <summary>
        /// The stencil operation to perform when stencil testing and depth testing both pass.
        /// </summary>
        API_FIELD() StencilOperation StencilPassOp;

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
        API_FIELD() PrimitiveTopologyType PrimitiveTopology;

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

    GPUPipelineState();

public:
#if BUILD_DEBUG
    /// <summary>
    /// The description of the pipeline state cached on creation in debug builds. Can be used to help with rendering crashes or issues and validation.
    /// </summary>
    Description DebugDesc;
#endif
#if USE_EDITOR
    int32 Complexity;
#endif

public:
    /// <summary>
    /// Gets constant buffers usage mask (each set bit marks usage of the constant buffer at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    FORCE_INLINE uint32 GetUsedCBsMask() const
    {
        return _meta.UsedCBsMask;
    }

    /// <summary>
    /// Gets shader resources usage mask (each set bit marks usage of the shader resource slot at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    FORCE_INLINE uint32 GetUsedSRsMask() const
    {
        return _meta.UsedSRsMask;
    }

    /// <summary>
    /// Gets unordered access usage mask (each set bit marks usage of the unordered access slot at the bit index slot). Combined from all the used shader stages.
    /// </summary>
    FORCE_INLINE uint32 GetUsedUAsMask() const
    {
        return _meta.UsedUAsMask;
    }

public:
    /// <summary>
    /// Returns true if pipeline state is valid and ready to use
    /// </summary>
    API_PROPERTY() virtual bool IsValid() const = 0;

    /// <summary>
    /// Create new state data
    /// </summary>
    /// <param name="desc">Full pipeline state description</param>
    /// <returns>True if cannot create state, otherwise false</returns>
    API_FUNCTION() virtual bool Init(API_PARAM(Ref) const Description& desc);

public:
    // [GPUResource]
    GPUResourceType GetResourceType() const final override;
};
