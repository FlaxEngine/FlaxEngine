// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Color.h"

struct RenderView;
struct RenderContext;
struct DrawCall;
class IMaterial;
class RenderTask;
class SceneRenderTask;
class DirectionalLight;
class PointLight;
class SpotLight;
class SkyLight;
class EnvironmentProbe;
class Skybox;
class Decal;
class MaterialBase;
class SkinnedMeshDrawData;
class Lightmap;
class RenderBuffers;
class GPUTextureView;
class GPUContext;
class GPUBuffer;
class GPUTexture;

/// <summary>
/// Interface for objects that can render custom sky
/// </summary>
class ISkyRenderer
{
public:

    /// <summary>
    /// Apply sky material/shader state to the GPU pipeline with custom parameters set (render to GBuffer).
    /// </summary>
    /// <param name="context">The context responsible for rendering commands.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="world">The world matrix to use during sky model vertices transformations.</param>
    virtual void ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world) = 0;
};

/// <summary>
/// Volumetric fog feature settings
/// </summary>
struct VolumetricFogOptions
{
    bool Enable;
    float ScatteringDistribution;
    Color Albedo;
    Color Emissive;
    float ExtinctionScale;
    float Distance;
    Vector4 FogParameters;

    bool UseVolumetricFog() const
    {
        return Enable && Distance > 0;
    }
};

/// <summary>
/// Interface for objects that can render custom fog/atmosphere
/// </summary>
class IFogRenderer
{
public:

    /// <summary>
    /// Gets the volumetric fog options.
    /// </summary>
    /// <param name="result">The result.</param>
    virtual void GetVolumetricFogOptions(VolumetricFogOptions& result) const = 0;

    /// <summary>
    /// Gets the exponential height fog data.
    /// </summary>
    /// <param name="view">The rendering view.</param>
    /// <param name="result">The result.</param>
    virtual void GetExponentialHeightFogData(const RenderView& view, ExponentialHeightFogData& result) const = 0;

    /// <summary>
    /// Draw fog using GBuffer inputs
    /// </summary>
    /// <param name="context">Context responsible for rendering commands</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="output">Output buffer</param>
    virtual void DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output) = 0;
};

/// <summary>
/// Interface for objects that can render custom atmospheric fog
/// </summary>
class IAtmosphericFogRenderer
{
public:

    /// <summary>
    /// Draw fog using GBuffer inputs
    /// </summary>
    /// <param name="context">Context responsible for rendering commands</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="output">Output buffer</param>
    virtual void DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output) = 0;
};

/// <summary>
/// Renderer draw call used for dynamic batching process.
/// </summary>
struct DrawCall
{
    struct
    {
        /// <summary>
        /// The geometry index buffer (cannot be null).
        /// </summary>
        GPUBuffer* IndexBuffer;

        /// <summary>
        /// The geometry vertex buffers.
        /// </summary>
        GPUBuffer* VertexBuffers[3];

        /// <summary>
        /// The geometry vertex buffers byte offsets.
        /// </summary>
        uint32 VertexBuffersOffsets[3];

        /// <summary>
        /// The location of the first index read by the GPU from the index buffer.
        /// </summary>
        int32 StartIndex;

        /// <summary>
        /// The indices count.
        /// </summary>
        int32 IndicesCount;
    } Geometry;

    /// <summary>
    /// The amount of instances of the geometry to draw. Set to 0 if use indirect draw arguments buffer.
    /// </summary>
    int32 InstanceCount;

    /// <summary>
    /// The indirect draw arguments offset.
    /// </summary>
    uint32 IndirectArgsOffset;

    /// <summary>
    /// The indirect draw arguments buffer.
    /// </summary>
    GPUBuffer* IndirectArgsBuffer;

    /// <summary>
    /// The target material to use.
    /// </summary>
    IMaterial* Material;

    // Particles don't use skinning nor lightmaps so pack those stuff together
    union
    {
        struct
        {
            /// <summary>
            /// Pointer to lightmap for static object with prebaked lighting.
            /// </summary>
            const Lightmap* Lightmap;

            /// <summary>
            /// The skinning data. If set then material should use GPU skinning during rendering.
            /// </summary>
            SkinnedMeshDrawData* Skinning;
        };

        struct
        {
            /// <summary>
            /// The particles data. Used only by the particles shaders.
            /// </summary>
            class ParticleBuffer* Particles;

            /// <summary>
            /// The particle module to draw.
            /// </summary>
            class ParticleEmitterGraphCPUNode* Module;
        };
    };

    /// <summary>
    /// Object world transformation matrix.
    /// </summary>
    Matrix World;

    // Terrain and particles don't use previous world matrix so pack those stuff together
    union
    {
        /// <summary>
        /// Object world transformation matrix using during previous frame.
        /// </summary>
        Matrix PrevWorld;

        struct
        {
            Vector4 HeightmapUVScaleBias;
            Vector4 NeighborLOD;
            Vector2 OffsetUV;
            float CurrentLOD;
            float ChunkSizeNextLOD;
            float TerrainChunkSizeLOD0;
            const class TerrainPatch* Patch;
        } TerrainData;

        struct
        {
            int32 RibbonOrderOffset;
            float UVTilingDistance;
            float UVScaleX;
            float UVScaleY;
            float UVOffsetX;
            float UVOffsetY;
            uint32 SegmentCount;
            GPUBuffer* SegmentDistances;
        } Ribbon;
    };

    /// <summary>
    /// Lightmap UVs area that entry occupies.
    /// </summary>
    Rectangle LightmapUVsArea;

    /// <summary>
    /// Object location in the world used for draw calls sorting.
    /// </summary>
    Vector3 ObjectPosition;

    /// <summary>
    /// The world matrix determinant sign (used for geometry that is two sided or has inverse scale - needs to flip normal vectors and change triangles culling).
    /// </summary>
    float WorldDeterminantSign;

    /// <summary>
    /// Object geometry size in the world (unscaled).
    /// </summary>
    Vector3 GeometrySize;

    /// <summary>
    /// The random per-instance value (normalized to range 0-1).
    /// </summary>
    float PerInstanceRandom;

    /// <summary>
    /// The model LOD transition dither factor.
    /// </summary>
    float LODDitherFactor;

    /// <summary>
    /// Does nothing.
    /// </summary>
    DrawCall()
    {
    }

    /// <summary>
    /// Determines whether world transform matrix is performing negative scale (then model culling should be inverted).
    /// </summary>
    /// <returns><c>true</c> if world matrix contains negative scale; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsNegativeScale() const
    {
        return WorldDeterminantSign < 0;
    }
};

template<>
struct TIsPODType<DrawCall>
{
    enum { Value = true };
};

/// <summary>
/// Data container for meshes and skinned meshes rendering with minimal state caching. 
/// Used to update previous world transformation matrix for motion vectors pass and handle LOD transitions blending.
/// </summary>
struct GeometryDrawStateData
{
    /// <summary>
    /// The previous frame world transformation matrix for the given geometry instance.
    /// </summary>
    Matrix PrevWorld;

    /// <summary>
    /// The previous frame index. In sync with Engine::FrameCount used to detect new frames and rendering gaps to reset state.
    /// </summary>
    uint64 PrevFrame;

    /// <summary>
    /// The previous frame model LOD index used. It's locked during LOD transition to cache the transition start LOD.
    /// </summary>
    char PrevLOD;

    /// <summary>
    /// The LOD transition timer. Value 255 means the end of the transition (aka no transition), value 0 means transition started. 
    /// Interpolated between 0-255 to smooth transition over several frames and reduce LOD changing artifacts.
    /// </summary>
    byte LODTransition;

    GeometryDrawStateData()
    {
        PrevWorld = Matrix::Identity;
        PrevFrame = 0;
        PrevLOD = -1;
        LODTransition = 255;
    }
};

template<>
struct TIsPODType<GeometryDrawStateData>
{
    enum { Value = true };
};

#define GEOMETRY_DRAW_STATE_EVENT_BEGIN(drawState, worldMatrix) \
    const auto frame = Engine::FrameCount; \
	if (drawState.PrevFrame + 1 < frame) \
	{ \
        drawState.PrevWorld = worldMatrix; \
    }

#define GEOMETRY_DRAW_STATE_EVENT_END(drawState, worldMatrix) \
	if (drawState.PrevFrame != frame) \
	{ \
		drawState.PrevWorld = worldMatrix; \
		drawState.PrevFrame = frame; \
	}
