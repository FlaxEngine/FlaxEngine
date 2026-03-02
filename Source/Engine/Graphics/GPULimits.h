// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "PixelFormat.h"
#include "Enums.h"

/// <summary>
/// Which resources are supported for a given format and given device.
/// </summary>
API_ENUM(Attributes="Flags") enum class FormatSupport : uint64
{
    /// <summary>
    /// No features supported.	
    /// </summary>
    None = 0x0,

    /// <summary>
    /// Buffer resources supported.
    /// </summary>
    Buffer = 0x1,

    /// <summary>
    /// Vertex buffers supported.
    /// </summary>
    VertexBuffer = 0x2,

    /// <summary>
    /// Vertex buffers supported.
    /// [Deprecated in 1.12]
    /// </summary>
    InputAssemblyVertexBuffer = 2,

    /// <summary>
    /// Index buffers supported.
    /// </summary>
    IndexBuffer = 0x4,

    /// <summary>
    /// Index buffers supported.
    /// [Deprecated in 1.12]
    /// </summary>
    InputAssemblyIndexBuffer = 4,

    /// <summary>
    /// Streaming output buffers supported.
    /// </summary>
    StreamOutputBuffer = 0x8,

    /// <summary>
    /// 1D texture resources supported.
    /// </summary>
    Texture1D = 0x10,

    /// <summary>
    /// 2D texture resources supported.
    /// </summary>
    Texture2D = 0x20,

    /// <summary>
    /// 3D texture resources supported.
    /// </summary>
    Texture3D = 0x40,

    /// <summary>
    /// Cube texture resources supported.
    /// </summary>
    TextureCube = 0x80,

    /// <summary>
    /// The shader Load function for texture objects is supported.
    /// </summary>
    ShaderLoad = 0x100,

    /// <summary>
    /// The shader Sample function for texture objects is supported.
    /// </summary>
    ShaderSample = 0x200,

    /// <summary>
    /// The shader SampleCmp and SampleCmpLevelZero functions for texture objects are supported.
    /// </summary>
    ShaderSampleComparison = 0x400,

    /// <summary>
    /// Unused.
    /// [Deprecated in 1.12]
    /// </summary>
    ShaderSampleMonoText = 2048,

    /// <summary>
    /// Mipmaps are supported.
    /// </summary>
    Mip = 0x1000,

    /// <summary>
    /// Automatic generation of mipmaps is supported.
    /// [Deprecated in 1.12]
    /// </summary>
    MipAutogen = 8192,

    /// <summary>
    /// Render targets are supported.
    /// </summary>
    RenderTarget = 0x4000,

    /// <summary>
    /// Blend operations supported.
    /// </summary>
    Blendable = 0x8000,

    /// <summary>
    /// Depth stencils supported.
    /// </summary>
    DepthStencil = 0x10000,

    /// <summary>
    /// CPU locking supported.
    /// </summary>
    CpuLockable = 131072,

    /// <summary>
    /// Multisample antialiasing (MSAA) resolve operations are supported.
    /// </summary>
    MultisampleResolve = 0x40000,

    /// <summary>
    /// Format can be displayed on screen.
    /// </summary>
    Display = 0x80000,

    /// <summary>
    /// Format can't be cast to another format.
    /// </summary>
    CastWithinBitLayout = 0x100000,

    /// <summary>
    /// Format can be used as a multi-sampled render target.
    /// </summary>
    MultisampleRenderTarget = 0x200000,

    /// <summary>
    /// Format can be used as a multi-sampled texture and read into a shader with the shader Load function.
    /// </summary>
    MultisampleLoad = 0x400000,

    /// <summary>
    /// Format can be used with the shader gather function.
    /// </summary>
    ShaderGather = 0x800000,

    /// <summary>
    /// Format supports casting when the resource is a back buffer.
    /// </summary>
    BackBufferCast = 0x1000000,

    /// <summary>
    /// Format can be used for an unordered access view.
    /// </summary>
    TypedUnorderedAccessView = 0x2000000,

    /// <summary>
    /// Format can be used with the shader gather with comparison function.
    /// </summary>
    ShaderGatherComparison = 0x4000000,

    /// <summary>
    /// Format can be used with the decoder output.
    /// </summary>
    DecoderOutput = 0x8000000,

    /// <summary>
    /// Format can be used with the video processor output.
    /// </summary>
    VideoProcessorOutput = 0x10000000,

    /// <summary>
    /// Format can be used with the video processor input.
    /// </summary>
    VideoProcessorInput = 0x20000000,

    /// <summary>
    /// Format can be used with the video encoder.
    /// </summary>
    VideoEncoder = 0x40000000,

    /// <summary>
    /// Format can be used as unorder access resource for read-only operations (shader load).
    /// </summary>
    UnorderedAccessReadOnly = 0x80000000,

    /// <summary>
    /// Format can be used as unorder access resource for write-only operations (shader store).
    /// </summary>
    UnorderedAccessWriteOnly = 0x100000000,

    /// <summary>
    /// Format can be used as unorder access resource for both read-write operations (shader load and store).
    /// </summary>
    UnorderedAccessReadWrite = 0x200000000,

    /// <summary>
    /// Format can be used as unorder access resource for read/write operations.
    /// </summary>
    UnorderedAccess = UnorderedAccessReadOnly | UnorderedAccessWriteOnly | UnorderedAccessReadWrite,
};

DECLARE_ENUM_OPERATORS(FormatSupport);

/// <summary>
/// The features exposed for a particular format.
/// </summary>
API_STRUCT(NoDefault) struct FormatFeatures
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(FormatFeatures);

    /// <summary>
    /// Gets the maximum MSAA sample count for a particular <see cref="PixelFormat"/>.
    /// </summary>
    API_FIELD() MSAALevel MSAALevelMax;

    /// <summary>
    /// Support of a given format on the installed video device.
    /// </summary>
    API_FIELD() FormatSupport Support;

    FormatFeatures()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="FeaturesPerFormat"/> struct.
    /// [Deprecated in v1.12]
    /// </summary>
    /// <param name="format">The format.</param>
    /// <param name="msaaLevelMax">The MSAA level maximum.</param>
    /// <param name="formatSupport">The format support.</param>
    DEPRECATED("Skip format argument as it's unsued.") FormatFeatures(const PixelFormat format, MSAALevel msaaLevelMax, FormatSupport formatSupport)
        : MSAALevelMax(msaaLevelMax)
        , Support(formatSupport)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="FeaturesPerFormat"/> struct.
    /// </summary>
    /// <param name="msaaLevelMax">The MSAA level maximum.</param>
    /// <param name="formatSupport">The format support.</param>
    FormatFeatures(MSAALevel msaaLevelMax, FormatSupport formatSupport)
        : MSAALevelMax(msaaLevelMax)
        , Support(formatSupport)
    {
    }
};

/// <summary>
/// Graphics Device limits and constraints descriptor.
/// </summary>
API_STRUCT(NoDefault) struct GPULimits
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(GPULimits);

    /// <summary>
    /// True if device supports Compute shaders.
    /// </summary>
    API_FIELD() bool HasCompute = false;

    /// <summary>
    /// True if device supports Tessellation shaders (domain and hull shaders).
    /// </summary>
    API_FIELD() bool HasTessellation = false;

    /// <summary>
    /// True if device supports Geometry shaders.
    /// </summary>
    API_FIELD() bool HasGeometryShaders = false;

    /// <summary>
    /// True if device supports hardware geometry instancing.
    /// </summary>
    API_FIELD() bool HasInstancing = false;

    /// <summary>
    /// True if device supports rendering to volume textures using Geometry shaders.
    /// </summary>
    API_FIELD() bool HasVolumeTextureRendering = false;

    /// <summary>
    /// True if device supports indirect drawing (including pixel shader write to UAV).
    /// </summary>
    API_FIELD() bool HasDrawIndirect = false;

    /// <summary>
    /// True if device supports append/consume buffers with counters.
    /// </summary>
    API_FIELD() bool HasAppendConsumeBuffers = false;

    /// <summary>
    /// True if device supports separate render target blending states.
    /// </summary>
    API_FIELD() bool HasSeparateRenderTargetBlendState = false;

    /// <summary>
    /// True if device supports depth buffer texture as a shader resource view.
    /// </summary>
    API_FIELD() bool HasDepthAsSRV = false;

    /// <summary>
    /// True if device supports depth buffer clipping (see GPUPipelineState::Description::DepthClipEnable).
    /// </summary>
    API_FIELD() bool HasDepthClip = false;

    /// <summary>
    /// True if device supports depth buffer bounds testing (see GPUPipelineState::Description::DepthBoundsEnable and GPUContext::SetDepthBounds).
    /// </summary>
    API_FIELD() bool HasDepthBounds = false;

    /// <summary>
    /// True if device supports depth buffer texture as a readonly depth buffer (can be sampled in the shader while performing depth-test).
    /// </summary>
    API_FIELD() bool HasReadOnlyDepth = false;

    /// <summary>
    /// True if device supports multisampled depth buffer texture as a shader resource view.
    /// </summary>
    API_FIELD() bool HasMultisampleDepthAsSRV = false;

    /// <summary>
    /// True if device supports reading from typed UAV in shader (common types such as R32G32B32A32, R16G16B16A16, R16, R8). This doesn't apply to single-component 32-bit formats.
    /// </summary>
    API_FIELD() bool HasTypedUAVLoad = false;

    /// <summary>
    /// The maximum amount of texture mip levels.
    /// </summary>
    API_FIELD() int32 MaximumMipLevelsCount = 1;

    /// <summary>
    /// The maximum size of the 1D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture1DSize = 0;

    /// <summary>
    /// The maximum length of 1D textures array.
    /// </summary>
    API_FIELD() int32 MaximumTexture1DArraySize = 0;

    /// <summary>
    /// The maximum size of the 2D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture2DSize = 0;

    /// <summary>
    /// The maximum length of 2D textures array.
    /// </summary>
    API_FIELD() int32 MaximumTexture2DArraySize = 0;

    /// <summary>
    /// The maximum size of the 3D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture3DSize = 0;

    /// <summary>
    /// The maximum size of the cube texture (both width and height).
    /// </summary>
    API_FIELD() int32 MaximumTextureCubeSize = 0;

    /// <summary>
    /// The maximum degree of anisotropic filtering used for texture sampling.
    /// </summary>
    API_FIELD() float MaximumSamplerAnisotropy = 1.0f;
};
