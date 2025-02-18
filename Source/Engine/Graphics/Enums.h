// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Config.h"

/// <summary>
/// Graphics rendering backend system types.
/// </summary>
API_ENUM() enum class RendererType
{
    /// <summary>
    /// Unknown type
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// DirectX 10
    /// </summary>
    DirectX10 = 1,

    /// <summary>
    /// DirectX 10.1
    /// </summary>
    DirectX10_1 = 2,

    /// <summary>
    /// DirectX 11
    /// </summary>
    DirectX11 = 3,

    /// <summary>
    /// DirectX 12
    /// </summary>
    DirectX12 = 4,

    /// <summary>
    /// OpenGL 4.1
    /// </summary>
    OpenGL4_1 = 5,

    /// <summary>
    /// OpenGL 4.4
    /// </summary>
    OpenGL4_4 = 6,

    /// <summary>
    /// OpenGL ES 3
    /// </summary>
    OpenGLES3 = 7,

    /// <summary>
    /// OpenGL ES 3.1
    /// </summary>
    OpenGLES3_1 = 8,

    /// <summary>
    /// Null backend
    /// </summary>
    Null = 9,

    /// <summary>
    /// Vulkan
    /// </summary>
    Vulkan = 10,

    /// <summary>
    /// PlayStation 4
    /// </summary>
    PS4 = 11,

    /// <summary>
    /// PlayStation 5
    /// </summary>
    PS5 = 12,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

const Char* ToString(RendererType value);

/// <summary>
/// Shader profile types define the version and type of the shading language used by the graphics backend.
/// </summary>
API_ENUM() enum class ShaderProfile
{
    /// <summary>
    /// Unknown
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// DirectX (Shader Model 4 compatible)
    /// </summary>
    DirectX_SM4 = 1,

    /// <summary>
    /// DirectX (Shader Model 5 compatible)
    /// </summary>
    DirectX_SM5 = 2,

    /// <summary>
    /// GLSL 410
    /// </summary>
    GLSL_410 = 3,

    /// <summary>
    /// GLSL 440
    /// </summary>
    GLSL_440 = 4,

    /// <summary>
    /// Vulkan (Shader Model 5 compatible)
    /// </summary>
    Vulkan_SM5 = 5,

    /// <summary>
    /// PlayStation 4
    /// </summary>
    PS4 = 6,

    /// <summary>
    /// DirectX (Shader Model 6 compatible)
    /// </summary>
    DirectX_SM6 = 7,

    /// <summary>
    /// PlayStation 5
    /// </summary>
    PS5 = 8,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

const Char* ToString(ShaderProfile value);

/// <summary>
/// Graphics feature levels indicates what level of support can be relied upon. 
/// They are named after the graphics API to indicate the minimum level of the features set to support. 
/// Feature levels are ordered from the lowest to the most high-end so feature level enum can be used to switch between feature levels (e.g. don't use geometry shader if not supported).
/// </summary>
API_ENUM() enum class FeatureLevel
{
    /// <summary>
    /// The features set defined by the core capabilities of OpenGL ES2.
    /// </summary>
    ES2 = 0,

    /// <summary>
    /// The features set defined by the core capabilities of OpenGL ES3.
    /// </summary>
    ES3 = 1,

    /// <summary>
    /// The features set defined by the core capabilities of OpenGL ES3.1.
    /// </summary>
    ES3_1 = 2,

    /// <summary>
    /// The features set defined by the core capabilities of DirectX 10 Shader Model 4.
    /// </summary>
    SM4 = 3,

    /// <summary>
    /// The features set defined by the core capabilities of DirectX 11 Shader Model 5.
    /// </summary>
    SM5 = 4,

    /// <summary>
    /// The features set defined by the core capabilities of DirectX 12 Shader Model 6.
    /// </summary>
    SM6 = 5,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

const Char* ToString(FeatureLevel value);

/// <summary>
/// Multisample count level.
/// </summary>
API_ENUM() enum class MSAALevel : int32
{
    /// <summary>
    /// Disabled multisampling.
    /// </summary>
    None = 1,

    /// <summary>
    /// Two samples per pixel.
    /// </summary>
    X2 = 2,

    /// <summary>
    /// Four samples per pixel.
    /// </summary>
    X4= 4,

    /// <summary>
    /// Eight samples per pixel.
    /// </summary>
    X8 = 8,
};

const Char* ToString(MSAALevel value);

/// <summary>
/// Shadows casting modes by visual elements.
/// </summary>
API_ENUM(Attributes="Flags") enum class ShadowsCastingMode
{
    /// <summary>
    /// Never render shadows.
    /// </summary>
    None = 0,

    /// <summary>
    /// Render shadows only in static views (env probes, lightmaps, etc.).
    /// </summary>
    StaticOnly = 1,

    /// <summary>
    /// Render shadows only in dynamic views (game, editor, etc.).
    /// </summary>
    DynamicOnly = 2,

    /// <summary>
    /// Always render shadows.
    /// </summary>
    All = StaticOnly | DynamicOnly,
};

DECLARE_ENUM_OPERATORS(ShadowsCastingMode);

/// <summary>
/// The partitioning mode for shadow cascades.
/// </summary>
API_ENUM() enum class PartitionMode
{
    /// <summary>
    /// Internally defined cascade splits.
    /// </summary>
    Manual = 0,

    /// <summary>
    /// Logarithmic cascade splits.
    /// </summary>
    Logarithmic = 1,

    /// <summary>
    /// Parallel-Split Shadow Maps cascade splits.
    /// </summary>
    PSSM = 2,
};

/// <summary>
/// Identifies expected GPU resource use during rendering. The usage directly reflects whether a resource is accessible by the CPU and/or the GPU.	
/// </summary>
API_ENUM() enum class GPUResourceUsage
{
    /// <summary>
    /// A resource that requires read and write access by the GPU. 
    /// This is likely to be the most common usage choice. 
    /// Memory will be used on device only, so fast access from the device is preferred. 
    /// It usually means device-local GPU (video) memory.
    /// </summary>
    /// <remarks>
    /// Usage:
    /// - Resources written and read by device, e.g. images used as render targets.
    /// - Resources transferred from host once (immutable) or infrequently and read by
    ///   device multiple times, e.g. textures to be sampled, vertex buffers, constant
    ///   buffers, and majority of other types of resources used on GPU.
    /// </remarks>
    Default = 0,

    /// <summary>
    /// A resource that is accessible by both the GPU (read only) and the CPU (write only). 
    /// A dynamic resource is a good choice for a resource that will be updated by the CPU at least once per frame. 
    /// Dynamic buffers or textures are usually used to upload data to GPU and use it within a single frame.
    /// </summary>
    /// <remarks>
    /// Usage:
    /// - Resources written frequently by CPU (dynamic), read by device. 
    ///   E.g. textures, vertex buffers, uniform buffers updated every frame or every draw call.
    /// </remarks>
    Dynamic = 1,

    /// <summary>
    /// A resource that supports data transfer (copy) from the CPU to the GPU.  
    /// It usually means CPU (system) memory. Resources created in this pool may still be accessible to the device, but access to them can be slow.
    /// </summary>
    /// <remarks>
    /// Usage:
    /// - Staging copy of resources used as transfer source.
    /// </remarks>
    StagingUpload = 2,

    /// <summary>
    /// A resource that supports data transfer (copy) from the GPU to the CPU. 
    /// </summary>
    /// <remarks>
    /// Usage:
    /// - Resources written by device, read by host - results of some computations, e.g. screen capture, average scene luminance for HDR tone mapping.
    /// - Any resources read or accessed randomly on host, e.g. CPU-side copy of vertex buffer used as source of transfer, but also used for collision detection.
    /// </remarks>
    StagingReadback = 3,

    /// <summary>
    /// A resource that supports both read and write from the CPU. 
    /// This is likely to be the common choice for read-write buffers to transfer data between GPU compute buffers and CPU memory. 
    /// It usually means CPU (system) memory.
    /// </summary>
    /// <remarks>
    /// Usage:
    /// - Staging memory to upload to GPU for compute and gather results back after processing.
    /// </remarks>
    Staging = 4,
};

/// <summary>
/// Describes how a mapped GPU resource will be accessed.
/// </summary>
API_ENUM(Attributes="Flags") enum class GPUResourceMapMode
{
    /// <summary>
    /// The resource is mapped for reading.
    /// </summary>
    Read = 0x01,

    /// <summary>
    /// The resource is mapped for writing.
    /// </summary>
    Write = 0x02,

    /// <summary>
    /// The resource is mapped for reading and writing.
    /// </summary>
    ReadWrite = Read | Write,
};

/// <summary>
/// Primitives types.
/// </summary>
API_ENUM() enum class PrimitiveTopologyType
{
    /// <summary>
    /// Unknown topology.
    /// </summary>
    Undefined = 0,

    /// <summary>
    /// Points list.
    /// </summary>
    Point = 1,

    /// <summary>
    /// Line list.
    /// </summary>
    Line = 2,

    /// <summary>
    /// Triangle list.
    /// </summary>
    Triangle = 3,
};

/// <summary>
/// Primitives culling mode.
/// </summary>
API_ENUM() enum class CullMode : byte
{
    /// <summary>
    /// Cull back-facing primitives only.
    /// </summary>
    Normal = 0,

    /// <summary>
    /// Cull front-facing primitives only.
    /// </summary>
    Inverted = 1,

    /// <summary>
    /// Disable face culling.
    /// </summary>
    TwoSided = 2,
};

/// <summary>
/// Render target blending mode descriptor.
/// </summary>
API_STRUCT() struct FLAXENGINE_API BlendingMode
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(BlendingMode);

    /// <summary>
    /// Blending mode.
    /// </summary>
    API_ENUM() enum class Blend
    {
        // The blend factor is (0, 0, 0, 0). No pre-blend operation.
        Zero = 1,
        // The blend factor is (1, 1, 1, 1). No pre-blend operation.
        One = 2,
        // The blend factor is (Rs, Gs, Bs, As), that is color data (RGB) from a pixel shader. No pre-blend operation.
        SrcColor = 3,
        // The blend factor is (1 - Rs, 1 - Gs, 1 - Bs, 1 - As), that is color data (RGB) from a pixel shader. The pre-blend operation inverts the data, generating 1 - RGB.
        InvSrcColor = 4,
        // The blend factor is (As, As, As, As), that is alpha data (A) from a pixel shader. No pre-blend operation.
        SrcAlpha = 5,
        // The blend factor is ( 1 - As, 1 - As, 1 - As, 1 - As), that is alpha data (A) from a pixel shader. The pre-blend operation inverts the data, generating 1 - A.
        InvSrcAlpha = 6,
        // The blend factor is (Ad Ad Ad Ad), that is alpha data from a render target. No pre-blend operation.
        DestAlpha = 7,
        // The blend factor is (1 - Ad 1 - Ad 1 - Ad 1 - Ad), that is alpha data from a render target. The pre-blend operation inverts the data, generating 1 - A.
        InvDestAlpha = 8,
        // The blend factor is (Rd, Gd, Bd, Ad), that is color data from a render target. No pre-blend operation.
        DestColor = 9,
        // The blend factor is (1 - Rd, 1 - Gd, 1 - Bd, 1 - Ad), that is color data from a render target. The pre-blend operation inverts the data, generating 1 - RGB.
        InvDestColor = 10,
        // The blend factor is (f, f, f, 1); where f = min(As, 1 - Ad). The pre-blend operation clamps the data to 1 or less.
        SrcAlphaSat = 11,
        // The blend factor is the blend factor set with GPUContext::SetBlendFactor. No pre-blend operation.
        BlendFactor = 14,
        // The blend factor is the blend factor set with GPUContext::SetBlendFactor. The pre-blend operation inverts the blend factor, generating 1 - blend_factor.
        BlendInvFactor = 15,
        // The blend factor is data sources both as color data output by a pixel shader. There is no pre-blend operation. This blend factor supports dual-source color blending.
        Src1Color = 16,
        // The blend factor is data sources both as color data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - RGB. This blend factor supports dual-source color blending.
        InvSrc1Color = 17,
        // The blend factor is data sources as alpha data output by a pixel shader. There is no pre-blend operation. This blend factor supports dual-source color blending.
        Src1Alpha = 18,
        // The blend factor is data sources as alpha data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - A. This blend factor supports dual-source color blending.
        InvSrc1Alpha = 19,

        API_ENUM(Attributes="HideInEditor") MAX
    };

    /// <summary>
    /// Blending operation.
    /// </summary>
    API_ENUM() enum class Operation
    {
        // Add source 1 and source 2.
        Add = 1,
        // Subtract source 1 from source 2.
        Subtract = 2,
        // Subtract source 2 from source 1.
        RevSubtract = 3,
        // Find the minimum of source 1 and source 2.
        Min = 4,
        // Find the maximum of source 1 and source 2.
        Max = 5,

        API_ENUM(Attributes="HideInEditor") MAX
    };

    /// <summary>
    /// Render target write mask
    /// </summary>
    API_ENUM() enum class ColorWrite
    {
        // No color writing.
        None = 0,

        // Allow data to be stored in the red component.
        Red = 1,
        // Allow data to be stored in the green component.
        Green = 2,
        // Allow data to be stored in the blue component.
        Blue = 4,
        // Allow data to be stored in the alpha component.
        Alpha = 8,

        // Allow data to be stored in all components.
        All = Red | Green | Blue | Alpha,
        // Allow data to be stored in red and green components.
        RG = Red | Green,
        // Allow data to be stored in red, green and blue components.
        RGB = Red | Green | Blue,
        // Allow data to be stored in all components.
        RGBA = Red | Green | Blue | Alpha,
    };

public:
    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() bool AlphaToCoverageEnable;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() bool BlendEnable;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Blend SrcBlend;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Blend DestBlend;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Operation BlendOp;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Blend SrcBlendAlpha;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Blend DestBlendAlpha;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() Operation BlendOpAlpha;

    /// <summary>
    /// Render target blending mode descriptor.
    /// </summary>
    API_FIELD() ColorWrite RenderTargetWriteMask;

public:
    bool operator==(const BlendingMode& other) const;

public:
    /// <summary>
    /// Gets the opaque rendering (default). No blending is being performed.
    /// </summary>
    API_FIELD(ReadOnly) static BlendingMode Opaque;

    /// <summary>
    /// Gets the additive rendering. Adds the color and the alpha channel. Source color is multiplied by the alpha.
    /// </summary>
    API_FIELD(ReadOnly) static BlendingMode Additive;

    /// <summary>
    /// Gets the alpha blending. Source alpha controls the output color (0 - use destination color, 1 - use source color).
    /// </summary>
    API_FIELD(ReadOnly) static BlendingMode AlphaBlend;

    /// <summary>
    /// Gets the additive blending with pre-multiplied color.
    /// </summary>
    API_FIELD(ReadOnly) static BlendingMode Add;

    /// <summary>
    /// Gets the multiply blending (multiply output color with texture color).
    /// </summary>
    API_FIELD(ReadOnly) static BlendingMode Multiply;
};

uint32 GetHash(const BlendingMode& key);

/// <summary>
/// Comparison function modes
/// </summary>
API_ENUM() enum class ComparisonFunc : byte
{
    // Never pass the comparison.
    Never = 1,
    // If the source data is less than the destination data, the comparison passes.
    Less = 2,
    // If the source data is equal to the destination data, the comparison passes.
    Equal = 3,
    // If the source data is less than or equal to the destination data, the comparison passes.
    LessEqual = 4,
    // If the source data is greater than the destination data, the comparison passes.
    Greater = 5,
    // If the source data is not equal to the destination data, the comparison passes.
    NotEqual = 6,
    // If the source data is greater than or equal to the destination data, the comparison passes.
    GreaterEqual = 7,
    // Always pass the comparison.
    Always = 8,

    API_ENUM(Attributes="HideInEditor") MAX
};

/// <summary>
/// Rendering quality levels.
/// </summary>
API_ENUM() enum class Quality : byte
{
    /// <summary>
    /// The low quality.
    /// </summary>
    Low = 0,

    /// <summary>
    /// The medium quality.
    /// </summary>
    Medium = 1,

    /// <summary>
    /// The high quality.
    /// </summary>
    High = 2,

    /// <summary>
    /// The ultra, mega, fantastic quality!
    /// </summary>
    Ultra = 3,

    API_ENUM(Attributes="HideInEditor")
    MAX
};

/// <summary>
/// Post Fx material rendering locations.
/// </summary>
API_ENUM() enum class MaterialPostFxLocation : byte
{
    /// <summary>
    /// Render the material after the post processing pass using *LDR* input frame.
    /// </summary>
    AfterPostProcessingPass = 0,

    /// <summary>
    /// Render the material before the post processing pass using *HDR* input frame.
    /// </summary>
    BeforePostProcessingPass = 1,

    /// <summary>
    /// Render the material before the forward pass but after *GBuffer* with *HDR* input frame.
    /// </summary>
    BeforeForwardPass = 2,

    /// <summary>
    /// Render the material after custom post effects (scripted).
    /// </summary>
    AfterCustomPostEffects = 3,

    /// <summary>
    /// Render the material before the reflections pass but after the lighting pass using *HDR* input frame. It can be used to implement a custom light types that accumulate lighting to the light buffer.
    /// </summary>
    BeforeReflectionsPass = 4,

    /// <summary>
    /// Render the material after anti-aliasing into the output backbuffer.
    /// </summary>
    AfterAntiAliasingPass = 5,

    /// <summary>
    /// Render the material after the forward pass but before any post processing.
    /// </summary>
    AfterForwardPass = 6,

    API_ENUM(Attributes="HideInEditor")
    MAX,
};

/// <summary>
/// The Post Process effect rendering location within the rendering pipeline.
/// </summary>
API_ENUM() enum class PostProcessEffectLocation
{
    /// <summary>
    /// The default location after the in-build PostFx pass (bloom, color grading, etc.) but before anti-aliasing effect.
    /// </summary>
    Default = 0,

    /// <summary>
    /// The 'before' in-build PostFx pass (bloom, color grading, etc.). After Forward Pass (transparency) and fog effects.
    /// </summary>
    BeforePostProcessingPass = 1,

    /// <summary>
    /// The 'before' Forward pass (transparency) and fog effects. After the Light pass and Reflections pass.
    /// </summary>
    BeforeForwardPass = 2,

    /// <summary>
    /// The 'before' Reflections pass. After the Light pass. Can be used to implement a custom light types that accumulate lighting to the light buffer.
    /// </summary>
    BeforeReflectionsPass = 3,

    /// <summary>
    /// The 'after' AA filter pass.
    /// </summary>
    AfterAntiAliasingPass = 4,

    /// <summary>
    /// The custom frame up-scaling that replaces default implementation. Rendering is done to the output backbuffer (use OutputView and OutputViewport as render destination).
    /// </summary>
    CustomUpscale = 5,

    /// <summary>
    /// The 'after' GBuffer rendering pass. Can be used to render custom geometry into GBuffer. Output is light buffer, single-target only (no output).
    /// </summary>
    AfterGBufferPass = 6,

    /// <summary>
    /// The 'after' forward pass but before any post processing.
    /// </summary>
    AfterForwardPass = 7,

    API_ENUM(Attributes="HideInEditor")
    MAX,
};

/// <summary>
/// The objects drawing pass types. Used as a flags for objects drawing masking.
/// </summary>
API_ENUM(Attributes="Flags") enum class DrawPass : int32
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// The hardware depth rendering to the depth buffer (used for shadow maps rendering).
    /// </summary>
    Depth = 1,

    /// <summary>
    /// The base pass rendering to the GBuffer (for opaque materials).
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(name: \"GBuffer\")")
    GBuffer = 1 << 1,

    /// <summary>
    /// The forward pass rendering (for transparent materials).
    /// </summary>
    Forward = 1 << 2,

    /// <summary>
    /// The transparent objects distortion vectors rendering (with blending).
    /// </summary>
    Distortion = 1 << 3,

    /// <summary>
    /// The motion vectors (velocity) rendering pass (for movable objects).
    /// </summary>
    MotionVectors = 1 << 4,

    /// <summary>
    /// The Global Sign Distance Field (SDF) rendering pass. Used for software raytracing though the scene on a GPU.
    /// </summary>
    GlobalSDF = 1 << 5,

    /// <summary>
    /// The Global Surface Atlas rendering pass. Used for software raytracing though the scene on a GPU to evaluate the object surface material properties.
    /// </summary>
    GlobalSurfaceAtlas = 1 << 6,

    /// <summary>
    /// The debug quad overdraw rendering (editor-only).
    /// </summary>
    API_ENUM(Attributes="HideInEditor")
    QuadOverdraw = 1 << 20,

    /// <summary>
    /// The default set of draw passes for the scene objects.
    /// </summary>
    API_ENUM(Attributes="HideInEditor")
    Default = Depth | GBuffer | Forward | Distortion | MotionVectors | GlobalSDF | GlobalSurfaceAtlas,

    /// <summary>
    /// The all draw passes combined into a single mask.
    /// </summary>
    API_ENUM(Attributes="HideInEditor")
    All = Depth | GBuffer | Forward | Distortion | MotionVectors | GlobalSDF | GlobalSurfaceAtlas,
};

DECLARE_ENUM_OPERATORS(DrawPass);

/// <summary>
/// Describes frame rendering modes.
/// </summary>
API_ENUM() enum class ViewMode
{
    /// <summary>
    /// Full rendering
    /// </summary>
    Default = 0,

    /// <summary>
    /// Without post-process pass
    /// </summary>
    NoPostFx = 1,

    /// <summary>
    /// Draw Diffuse
    /// </summary>
    Diffuse = 2,

    /// <summary>
    /// Draw Normals
    /// </summary>
    Normals = 3,

    /// <summary>
    /// Draw Emissive
    /// </summary>
    Emissive = 4,

    /// <summary>
    /// Draw Depth
    /// </summary>
    Depth = 5,

    /// <summary>
    /// Draw Ambient Occlusion
    /// </summary>
    AmbientOcclusion = 6,

    /// <summary>
    /// Draw Material's Metalness
    /// </summary>
    Metalness = 7,

    /// <summary>
    /// Draw Material's Roughness
    /// </summary>
    Roughness = 8,

    /// <summary>
    /// Draw Material's Specular
    /// </summary>
    Specular = 9,

    /// <summary>
    /// Draw Material's Specular Color
    /// </summary>
    SpecularColor = 10,

    /// <summary>
    /// Draw Shading Model
    /// </summary>
    ShadingModel = 11,

    /// <summary>
    /// Draw Lights buffer
    /// </summary>
    LightBuffer = 12,

    /// <summary>
    /// Draw reflections buffer
    /// </summary>
    Reflections = 13,

    /// <summary>
    /// Draw scene objects in wireframe mode
    /// </summary>
    Wireframe = 14,

    /// <summary>
    /// Draw motion vectors debug view
    /// </summary>
    MotionVectors = 15,

    /// <summary>
    /// Draw materials subsurface color debug view
    /// </summary>
    SubsurfaceColor = 16,

    /// <summary>
    /// Draw materials colors with ambient occlusion
    /// </summary>
    Unlit = 17,

    /// <summary>
    /// Draw meshes lightmaps coordinates density
    /// </summary>
    LightmapUVsDensity = 18,

    /// <summary>
    /// Draw meshes vertex colors
    /// </summary>
    VertexColors = 19,

    /// <summary>
    /// Draw physics colliders debug view
    /// </summary>
    PhysicsColliders = 20,

    /// <summary>
    /// Draw Level Of Detail number as colors to debug LOD switches.
    /// </summary>
    LODPreview = 21,

    /// <summary>
    /// Draw material shaders complexity to visualize performance of pixels rendering.
    /// </summary>
    MaterialComplexity = 22,

    /// <summary>
    /// Draw geometry overdraw to visualize performance of pixels rendering.
    /// </summary>
    QuadOverdraw = 23,

    /// <summary>
    /// Draw Global Sign Distant Field (SDF) preview.
    /// </summary>
    GlobalSDF = 24,

    /// <summary>
    /// Draw Global Surface Atlas preview.
    /// </summary>
    GlobalSurfaceAtlas = 25,

    /// <summary>
    /// Draw Global Illumination debug preview (eg. irradiance probes).
    /// </summary>
    GlobalIllumination = 26,
};

/// <summary>
/// Frame rendering flags used to switch between graphics features.
/// </summary>
API_ENUM(Attributes="Flags") enum class ViewFlags : uint64
{
    /// <summary>
    /// Nothing.
    /// </summary>
    None = 0,

    /// <summary>
    /// Shows/hides the debug shapes rendered using Debug Draw.
    /// </summary>
    DebugDraw = 1,

    /// <summary>
    /// Shows/hides Editor sprites
    /// </summary>
    EditorSprites = 1 << 1,

    /// <summary>
    /// Shows/hides reflections
    /// </summary>
    Reflections = 1 << 2,

    /// <summary>
    /// Shows/hides Screen Space Reflections
    /// </summary>
    SSR = 1 << 3,

    /// <summary>
    /// Shows/hides Ambient Occlusion effect
    /// </summary>
    AO = 1 << 4,

    /// <summary>
    /// Shows/hides Global Illumination effect
    /// </summary>
    GI = 1 << 5,

    /// <summary>
    /// Shows/hides directional lights
    /// </summary>
    DirectionalLights = 1 << 6,

    /// <summary>
    /// Shows/hides point lights
    /// </summary>
    PointLights = 1 << 7,

    /// <summary>
    /// Shows/hides spot lights
    /// </summary>
    SpotLights = 1 << 8,

    /// <summary>
    /// Shows/hides sky lights
    /// </summary>
    SkyLights = 1 << 9,

    /// <summary>
    /// Shows/hides shadows
    /// </summary>
    Shadows = 1 << 10,

    /// <summary>
    /// Shows/hides specular light rendering
    /// </summary>
    SpecularLight = 1 << 11,

    /// <summary>
    /// Shows/hides Anti-Aliasing
    /// </summary>
    AntiAliasing = 1 << 12,

    /// <summary>
    /// Shows/hides custom Post-Process effects
    /// </summary>
    CustomPostProcess = 1 << 13,

    /// <summary>
    /// Shows/hides bloom effect
    /// </summary>
    Bloom = 1 << 14,

    /// <summary>
    /// Shows/hides tone mapping effect
    /// </summary>
    ToneMapping = 1 << 15,

    /// <summary>
    /// Shows/hides eye adaptation effect
    /// </summary>
    EyeAdaptation = 1 << 16,

    /// <summary>
    /// Shows/hides camera artifacts
    /// </summary>
    CameraArtifacts = 1 << 17,

    /// <summary>
    /// Shows/hides lens flares
    /// </summary>
    LensFlares = 1 << 18,

    /// <summary>
    /// Shows/hides deferred decals.
    /// </summary>
    Decals = 1 << 19,

    /// <summary>
    /// Shows/hides depth of field effect
    /// </summary>
    DepthOfField = 1 << 20,

    /// <summary>
    /// Shows/hides physics debug shapes.
    /// </summary>
    PhysicsDebug = 1 << 21,

    /// <summary>
    /// Shows/hides fogging effects.
    /// </summary>
    Fog = 1 << 22,

    /// <summary>
    /// Shows/hides the motion blur effect.
    /// </summary>
    MotionBlur = 1 << 23,

    /// <summary>
    /// Shows/hides the contact shadows effect.
    /// </summary>
    ContactShadows = 1 << 24,

    /// <summary>
    /// Shows/hides the Global Sign Distant Fields rendering.
    /// </summary>
    GlobalSDF = 1 << 25,

    /// <summary>
    /// Shows/hides the Sky/Skybox rendering.
    /// </summary>
    Sky = 1 << 26,

    /// <summary>
    /// Shows/hides light debug shapes.
    /// </summary>
    LightsDebug = 1 << 27,

    /// <summary>
    /// Default flags for Game.
    /// </summary>
    DefaultGame = Reflections | DepthOfField | Fog | Decals | MotionBlur | SSR | AO | GI | DirectionalLights | PointLights | SpotLights | SkyLights | Shadows | SpecularLight | AntiAliasing | CustomPostProcess | Bloom | ToneMapping | EyeAdaptation | CameraArtifacts | LensFlares | ContactShadows | GlobalSDF | Sky,

    /// <summary>
    /// Default flags for Editor.
    /// </summary>
    DefaultEditor = Reflections | Fog | Decals | DebugDraw | SSR | AO | GI | DirectionalLights | PointLights | SpotLights | SkyLights | Shadows | SpecularLight | AntiAliasing | CustomPostProcess | Bloom | ToneMapping | EyeAdaptation | CameraArtifacts | LensFlares | EditorSprites | ContactShadows | GlobalSDF | Sky,

    /// <summary>
    /// Default flags for materials/models previews generating.
    /// </summary>
    DefaultAssetPreview = Reflections | Decals | DirectionalLights | PointLights | SpotLights | SkyLights | SpecularLight | AntiAliasing | Bloom | ToneMapping | EyeAdaptation | CameraArtifacts | LensFlares | ContactShadows | Sky,
};

DECLARE_ENUM_OPERATORS(ViewFlags);

/// <summary>
/// Describes the different tessellation methods supported by the graphics system.
/// </summary>
API_ENUM() enum class TessellationMethod
{
    /// <summary>
    /// No tessellation.
    /// </summary>
    None = 0,

    /// <summary>
    /// Flat tessellation. Also known as dicing tessellation.
    /// </summary>
    Flat = 1,

    /// <summary>
    /// Point normal tessellation.
    /// </summary>
    PointNormal = 2,

    /// <summary>
    /// Geometric version of Phong normal interpolation, not applied on normals but on the vertex positions.
    /// </summary>
    Phong = 3,
};

/// <summary>
/// Describes the shader function flags used for shader compilation.
/// </summary>
enum class ShaderFlags : uint32
{
    /// <summary>
    /// The default set for flags.
    /// </summary>
    Default = 0,

    /// <summary>
    /// Hides the shader. It will exist in source and will be parsed but won't be compiled for the rendering.
    /// </summary>
    Hidden = 1,

    /// <summary>
    /// Disables any fast-math optimizations performed by the shader compiler.
    /// </summary>
    NoFastMath = 2,

    /// <summary>
    /// Indicates that vertex shader function outputs data for the geometry shader.
    /// </summary>
    VertexToGeometryShader = 4,
};

DECLARE_ENUM_OPERATORS(ShaderFlags);

/// <summary>
/// The environment probes cubemap texture resolutions.
/// </summary>
API_ENUM() enum class ProbeCubemapResolution
{
    // Graphics Settings default option.
    UseGraphicsSettings = 0,
    // Cubemap with 32x32.
    _32 = 32,
    // Cubemap with 64x64.
    _64 = 64,
    // Cubemap with 128x128.
    _128 = 128,
    // Cubemap with 256x256.
    _256 = 256,
    // Cubemap with 512x512.
    _512 = 512,
    // Cubemap with 1024x1024.
    _1024 = 1024,
    // Cubemap with 2048x2048.
    _2048 = 2048,
};
