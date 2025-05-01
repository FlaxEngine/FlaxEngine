// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "PixelFormat.h"
#include "Enums.h"

/// <summary>
/// Which resources are supported for a given format and given device.
/// </summary>
API_ENUM(Attributes="Flags") enum class FormatSupport : int32
{
    /// <summary>
    /// No features supported.	
    /// </summary>
    None = 0,

    /// <summary>
    /// Buffer resources supported.
    /// </summary>
    Buffer = 1,

    /// <summary>
    /// Vertex buffers supported.
    /// </summary>
    InputAssemblyVertexBuffer = 2,

    /// <summary>
    /// Index buffers supported.
    /// </summary>
    InputAssemblyIndexBuffer = 4,

    /// <summary>
    /// Streaming output buffers supported.
    /// </summary>
    StreamOutputBuffer = 8,

    /// <summary>
    /// 1D texture resources supported.
    /// </summary>
    Texture1D = 16,

    /// <summary>
    /// 2D texture resources supported.
    /// </summary>
    Texture2D = 32,

    /// <summary>
    /// 3D texture resources supported.
    /// </summary>
    Texture3D = 64,

    /// <summary>
    /// Cube texture resources supported.
    /// </summary>
    TextureCube = 128,

    /// <summary>
    /// The shader Load function for texture objects is supported.
    /// </summary>
    ShaderLoad = 256,

    /// <summary>
    /// The shader Sample function for texture objects is supported.
    /// </summary>
    ShaderSample = 512,

    /// <summary>
    /// The shader SampleCmp and SampleCmpLevelZero functions for texture objects are supported.
    /// </summary>
    ShaderSampleComparison = 1024,

    /// <summary>
    /// Unused.
    /// </summary>
    ShaderSampleMonoText = 2048,

    /// <summary>
    /// Mipmaps are supported.
    /// </summary>
    Mip = 4096,

    /// <summary>
    /// Automatic generation of mipmaps is supported.
    /// </summary>
    MipAutogen = 8192,

    /// <summary>
    /// Render targets are supported.
    /// </summary>
    RenderTarget = 16384,

    /// <summary>
    /// Blend operations supported.
    /// </summary>
    Blendable = 32768,

    /// <summary>
    /// Depth stencils supported.
    /// </summary>
    DepthStencil = 65536,

    /// <summary>
    /// CPU locking supported.
    /// </summary>
    CpuLockable = 131072,

    /// <summary>
    /// Multisample antialiasing (MSAA) resolve operations are supported.
    /// </summary>
    MultisampleResolve = 262144,

    /// <summary>
    /// Format can be displayed on screen.
    /// </summary>
    Display = 524288,

    /// <summary>
    /// Format can't be cast to another format.
    /// </summary>
    CastWithinBitLayout = 1048576,

    /// <summary>
    /// Format can be used as a multi-sampled render target.
    /// </summary>
    MultisampleRenderTarget = 2097152,

    /// <summary>
    /// Format can be used as a multi-sampled texture and read into a shader with the shader Load function.
    /// </summary>
    MultisampleLoad = 4194304,

    /// <summary>
    /// Format can be used with the shader gather function.
    /// </summary>
    ShaderGather = 8388608,

    /// <summary>
    /// Format supports casting when the resource is a back buffer.
    /// </summary>
    BackBufferCast = 16777216,

    /// <summary>
    /// Format can be used for an unordered access view.
    /// </summary>
    TypedUnorderedAccessView = 33554432,

    /// <summary>
    /// Format can be used with the shader gather with comparison function.
    /// </summary>
    ShaderGatherComparison = 67108864,

    /// <summary>
    /// Format can be used with the decoder output.
    /// </summary>
    DecoderOutput = 134217728,

    /// <summary>
    /// Format can be used with the video processor output.
    /// </summary>
    VideoProcessorOutput = 268435456,

    /// <summary>
    /// Format can be used with the video processor input.
    /// </summary>
    VideoProcessorInput = 536870912,

    /// <summary>
    /// Format can be used with the video encoder.
    /// </summary>
    VideoEncoder = 1073741824,
};

DECLARE_ENUM_OPERATORS(FormatSupport);

/// <summary>
/// The features exposed for a particular format.
/// </summary>
API_STRUCT() struct FormatFeatures
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
    /// </summary>
    /// <param name="format">The format.</param>
    /// <param name="msaaLevelMax">The MSAA level maximum.</param>
    /// <param name="formatSupport">The format support.</param>
    FormatFeatures(const PixelFormat format, MSAALevel msaaLevelMax, FormatSupport formatSupport)
        : MSAALevelMax(msaaLevelMax)
        , Support(formatSupport)
    {
    }
};

/// <summary>
/// Graphics Device limits and constraints descriptor.
/// </summary>
API_STRUCT() struct GPULimits
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(GPULimits);

    /// <summary>
    /// True if device supports Compute shaders.
    /// </summary>
    API_FIELD() bool HasCompute;

    /// <summary>
    /// True if device supports Tessellation shaders (domain and hull shaders).
    /// </summary>
    API_FIELD() bool HasTessellation;

    /// <summary>
    /// True if device supports Geometry shaders.
    /// </summary>
    API_FIELD() bool HasGeometryShaders;

    /// <summary>
    /// True if device supports hardware geometry instancing.
    /// </summary>
    API_FIELD() bool HasInstancing;

    /// <summary>
    /// True if device supports rendering to volume textures using Geometry shaders.
    /// </summary>
    API_FIELD() bool HasVolumeTextureRendering;

    /// <summary>
    /// True if device supports indirect drawing (including pixel shader write to UAV).
    /// </summary>
    API_FIELD() bool HasDrawIndirect;

    /// <summary>
    /// True if device supports append/consume buffers with counters.
    /// </summary>
    API_FIELD() bool HasAppendConsumeBuffers;

    /// <summary>
    /// True if device supports separate render target blending states.
    /// </summary>
    API_FIELD() bool HasSeparateRenderTargetBlendState;

    /// <summary>
    /// True if device supports depth buffer texture as a shader resource view.
    /// </summary>
    API_FIELD() bool HasDepthAsSRV;

    /// <summary>
    /// True if device supports depth buffer clipping (see GPUPipelineState::Description::DepthClipEnable).
    /// </summary>
    API_FIELD() bool HasDepthClip;

    /// <summary>
    /// True if device supports depth buffer texture as a readonly depth buffer (can be sampled in the shader while performing depth-test).
    /// </summary>
    API_FIELD() bool HasReadOnlyDepth;

    /// <summary>
    /// True if device supports multisampled depth buffer texture as a shader resource view.
    /// </summary>
    API_FIELD() bool HasMultisampleDepthAsSRV;

    /// <summary>
    /// True if device supports reading from typed UAV in shader (common types such as R32G32B32A32, R16G16B16A16, R16, R8). This doesn't apply to single-component 32-bit formats.
    /// </summary>
    API_FIELD() bool HasTypedUAVLoad;

    /// <summary>
    /// The maximum amount of texture mip levels.
    /// </summary>
    API_FIELD() int32 MaximumMipLevelsCount;

    /// <summary>
    /// The maximum size of the 1D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture1DSize;

    /// <summary>
    /// The maximum length of 1D textures array.
    /// </summary>
    API_FIELD() int32 MaximumTexture1DArraySize;

    /// <summary>
    /// The maximum size of the 2D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture2DSize;

    /// <summary>
    /// The maximum length of 2D textures array.
    /// </summary>
    API_FIELD() int32 MaximumTexture2DArraySize;

    /// <summary>
    /// The maximum size of the 3D texture.
    /// </summary>
    API_FIELD() int32 MaximumTexture3DSize;

    /// <summary>
    /// The maximum size of the cube texture (both width and height).
    /// </summary>
    API_FIELD() int32 MaximumTextureCubeSize;

    /// <summary>
    /// The maximum degree of anisotropic filtering used for texture sampling.
    /// </summary>
    API_FIELD() float MaximumSamplerAnisotropy;
};
