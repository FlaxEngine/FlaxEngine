// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    /// Returns true if sky is realtime, otherwise it's static.
    /// </summary>
    virtual bool IsDynamicSky() const = 0;

    virtual float GetIndirectLightingIntensity() const = 0;

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
    Float4 FogParameters;

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
    virtual void GetExponentialHeightFogData(const RenderView& view, ShaderExponentialHeightFogData& result) const = 0;

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
    /// <summary>
    /// The material to use for rendering.
    /// </summary>
    IMaterial* Material;

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
    } Geometry;

    /// <summary>
    /// The amount of instances of the geometry to draw. Set to 0 if use indirect draw arguments buffer.
    /// </summary>
    int32 InstanceCount;

    union
    {
        struct
        {
            /// <summary>
            /// The location of the first index read by the GPU from the index buffer.
            /// </summary>
            int32 StartIndex;

            /// <summary>
            /// The indices count.
            /// </summary>
            int32 IndicesCount;
        };

        struct
        {
            /// <summary>
            /// The indirect draw arguments offset.
            /// </summary>
            uint32 IndirectArgsOffset;

            /// <summary>
            /// The indirect draw arguments buffer.
            /// </summary>
            GPUBuffer* IndirectArgsBuffer;
        };
    } Draw;

    // Per-material shader data packed into union
    union
    {
        struct
        {
            const Lightmap* Lightmap;
            Rectangle LightmapUVsArea;
        } Features;

        struct
        {
            const Lightmap* Lightmap;
            Rectangle LightmapUVsArea;
            SkinnedMeshDrawData* Skinning;
            Float3 GeometrySize; // Object geometry size in the world (unscaled).
            float LODDitherFactor; // The model LOD transition dither progress.
            Matrix PrevWorld;
        } Surface;

        struct
        {
            const Lightmap* Lightmap;
            Rectangle LightmapUVsArea;
            Float4 HeightmapUVScaleBias;
            Float4 NeighborLOD;
            Float2 OffsetUV;
            float CurrentLOD;
            float ChunkSizeNextLOD;
            float TerrainChunkSizeLOD0;
            const class TerrainPatch* Patch;
        } Terrain;

        struct
        {
            class ParticleBuffer* Particles;
            class ParticleEmitterGraphCPUNode* Module;

            struct
            {
                int32 OrderOffset;
                float UVTilingDistance;
                float UVScaleX;
                float UVScaleY;
                float UVOffsetX;
                float UVOffsetY;
                uint32 SegmentCount;
            } Ribbon;

            struct
            {
                Float3 Position;
                float Radius;
                int32 ParticleIndex;
            } VolumetricFog;
        } Particle;

        struct
        {
            GPUBuffer* SplineDeformation;
            Matrix LocalMatrix; // Geometry transformation applied before deformation.
            Float3 GeometrySize; // Object geometry size in the world (unscaled).
            float Segment;
            float ChunksPerSegment;
            float MeshMinZ;
            float MeshMaxZ;
        } Deformable;

        struct
        {
            byte Raw[96];
        } Custom;
    };

    /// <summary>
    /// Object world transformation matrix.
    /// </summary>
    Matrix World;

    /// <summary>
    /// Object location in the world used for draw calls sorting.
    /// </summary>
    Float3 ObjectPosition;

    /// <summary>
    /// Object bounding sphere radius that contains it whole (sphere at ObjectPosition).
    /// </summary>
    float ObjectRadius;

    /// <summary>
    /// The world matrix determinant sign (used for geometry that is two sided or has inverse scale - needs to flip normal vectors and change triangles culling).
    /// </summary>
    float WorldDeterminantSign;

    /// <summary>
    /// The random per-instance value (normalized to range 0-1).
    /// </summary>
    float PerInstanceRandom;

    /// <summary>
    /// The sorting key for the draw call calculate by RenderList.
    /// </summary>
    uint64 SortKey;

    /// <summary>
    /// Zero-init.
    /// </summary>
    FORCE_INLINE DrawCall()
    {
        Platform::MemoryClear(this, sizeof(DrawCall));
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
    Matrix PrevWorld = Matrix::Identity;

    /// <summary>
    /// The previous frame index. In sync with Engine::FrameCount used to detect new frames and rendering gaps to reset state.
    /// </summary>
    uint64 PrevFrame = 0;

    /// <summary>
    /// The previous frame model LOD index used. It's locked during LOD transition to cache the transition start LOD.
    /// </summary>
    char PrevLOD = -1;

    /// <summary>
    /// The LOD transition timer. Value 255 means the end of the transition (aka no transition), value 0 means transition started. 
    /// Interpolated between 0-255 to smooth transition over several frames and reduce LOD changing artifacts.
    /// </summary>
    byte LODTransition = 255;
};

template<>
struct TIsPODType<GeometryDrawStateData>
{
    enum { Value = true };
};

#define GEOMETRY_DRAW_STATE_EVENT_BEGIN(drawState, worldMatrix) \
    const auto frame = Engine::FrameCount; \
	if (drawState.PrevFrame + 1 < frame && !renderContext.View.IsSingleFrame) \
	{ \
        drawState.PrevWorld = worldMatrix; \
    }

#define GEOMETRY_DRAW_STATE_EVENT_END(drawState, worldMatrix) \
	if (drawState.PrevFrame != frame && !renderContext.View.IsSingleFrame) \
	{ \
		drawState.PrevWorld = worldMatrix; \
		drawState.PrevFrame = frame; \
	}

#if USE_LARGE_WORLDS
#define ACTOR_GET_WORLD_MATRIX(actor, view, world) Real4x4 worldReal; actor->GetLocalToWorldMatrix(worldReal); renderContext.View.GetWorldMatrix(worldReal); Matrix world = (Matrix)worldReal;
#else
#define ACTOR_GET_WORLD_MATRIX(actor, view, world) Real4x4 world; actor->GetLocalToWorldMatrix(world); renderContext.View.GetWorldMatrix(world);
#endif
