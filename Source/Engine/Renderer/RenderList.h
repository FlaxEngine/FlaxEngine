// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Half.h"
#include "Engine/Graphics/PostProcessSettings.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "DrawCall.h"
#include "RenderListBuffer.h"
#include "RendererAllocation.h"
#include "RenderSetup.h"

enum class StaticFlags;
class RenderBuffers;
class PostProcessEffect;
class SceneRendering;
class LightWithShadow;
class IPostFxSettingsProvider;
class CubeTexture;
struct RenderContext;
struct RenderContextBatch;

struct RenderLightData
{
    Guid ID;

    Float3 Position;
    float MinRoughness;

    Float3 Color;
    float ShadowsStrength;

    Float3 Direction;
    float ShadowsFadeDistance;

    float ShadowsNormalOffsetScale;
    float ShadowsDepthBias;
    float ShadowsSharpness;
    float ShadowsDistance;

    StaticFlags StaticFlags;
    ShadowsCastingMode ShadowsMode;
    float IndirectLightingIntensity;
    uint8 HasShadow : 1;
    uint8 CastVolumetricShadow : 1;
    uint8 UseInverseSquaredFalloff : 1;
    uint8 IsDirectionalLight : 1;
    uint8 IsPointLight : 1;
    uint8 IsSpotLight : 1;
    uint8 IsSkyLight : 1;

    float VolumetricScatteringIntensity;
    float ContactShadowsLength;
    float ScreenSize;
    uint32 ShadowsBufferAddress;

    float ShadowsUpdateRate;
    float ShadowsUpdateRateAtDistance;
    uint32 ShadowFrame;
    int32 ShadowsResolution;

    bool CanRenderShadow(const RenderView& view) const;
};

struct RenderDirectionalLightData : RenderLightData
{
    float Cascade1Spacing;
    float Cascade2Spacing;
    float Cascade3Spacing;
    float Cascade4Spacing;

    PartitionMode PartitionMode;
    int32 CascadeCount;

    RenderDirectionalLightData()
    {
        Platform::MemoryClear(this, sizeof(RenderDirectionalLightData));
        IsDirectionalLight = 1;
    }

    POD_COPYABLE(RenderDirectionalLightData);

    void SetShaderData(ShaderLightData& data, bool useShadow) const;
};

struct RenderLocalLightData : RenderLightData
{
    GPUTexture* IESTexture;

    float Radius;
    float SourceRadius;

    bool CanRenderShadow(const RenderView& view) const;
};

struct RenderSpotLightData : RenderLocalLightData
{
    Float3 UpVector;
    float OuterConeAngle;

    float CosOuterCone;
    float InvCosConeDifference;
    float FallOffExponent;

    RenderSpotLightData()
    {
        Platform::MemoryClear(this, sizeof(RenderSpotLightData));
        IsSpotLight = 1;
    }

    POD_COPYABLE(RenderSpotLightData);

    void SetShaderData(ShaderLightData& data, bool useShadow) const;
};

struct RenderPointLightData : RenderLocalLightData
{
    float FallOffExponent;
    float SourceLength;

    RenderPointLightData()
    {
        Platform::MemoryClear(this, sizeof(RenderPointLightData));
        IsPointLight = 1;
    }

    POD_COPYABLE(RenderPointLightData);

    void SetShaderData(ShaderLightData& data, bool useShadow) const;
};

struct RenderSkyLightData : RenderLightData
{
    Float3 AdditiveColor;
    float Radius;

    CubeTexture* Image;

    RenderSkyLightData()
    {
        Platform::MemoryClear(this, sizeof(RenderSkyLightData));
        IsSkyLight = 1;
    }

    POD_COPYABLE(RenderSkyLightData);

    void SetShaderData(ShaderLightData& data, bool useShadow) const;
};

struct RenderEnvironmentProbeData
{
    GPUTexture* Texture;
    Float3 Position;
    float Radius;
    float Brightness;
    int32 SortOrder;
    uint32 HashID;

    void SetShaderData(ShaderEnvProbeData& data) const;
};

struct RenderDecalData
{
    Matrix World;
    MaterialBase* Material;
    int32 SortOrder;
};

/// <summary>
/// The draw calls list types.
/// </summary>
API_ENUM() enum class DrawCallsListType
{
    /// <summary>
    /// Hardware depth rendering.
    /// </summary>
    Depth,

    /// <summary>
    /// GBuffer rendering.
    /// </summary>
    GBuffer,

    /// <summary>
    /// GBuffer rendering after decals.
    /// </summary>
    GBufferNoDecals,

    /// <summary>
    /// Transparency rendering.
    /// </summary>
    Forward,

    /// <summary>
    /// Distortion accumulation rendering.
    /// </summary>
    Distortion,

    /// <summary>
    /// Motion vectors rendering.
    /// </summary>
    MotionVectors,

    MAX,
};

/// <summary>
/// Represents a patch of draw calls that can be submitted to rendering.
/// </summary>
struct DrawBatch
{
    /// <summary>
    /// Draw calls sorting key (shared by the all draw calls withing a patch).
    /// </summary>
    uint64 SortKey;

    /// <summary>
    /// The first draw call index.
    /// </summary>
    uint16 StartIndex;

    /// <summary>
    /// A number of draw calls to be submitted at once.
    /// </summary>
    uint16 BatchSize;

    /// <summary>
    /// The total amount of instances (sum from all draw calls in this batch).
    /// </summary>
    uint32 InstanceCount;

    bool operator<(const DrawBatch& other) const
    {
        return SortKey < other.SortKey;
    }
};

struct BatchedDrawCall
{
    DrawCall DrawCall;
    uint16 ObjectsStartIndex = 0; // Index of the instances start in the ObjectsBuffer (set internally).
    Array<struct ShaderObjectData, RendererAllocation> Instances;
};

/// <summary>
/// Represents a list of draw calls.
/// </summary>
struct DrawCallsList
{
    /// <summary>
    /// The list of draw calls indices to render.
    /// </summary>
    RenderListBuffer<int32> Indices;

    /// <summary>
    /// The list of external draw calls indices to render.
    /// </summary>
    RenderListBuffer<int32> PreBatchedDrawCalls;

    /// <summary>
    /// The draw calls batches (for instancing).
    /// </summary>
    Array<DrawBatch> Batches;

    /// <summary>
    /// True if draw calls batches list can be rendered using hardware instancing, otherwise false.
    /// </summary>
    bool CanUseInstancing;

    void Clear();
    bool IsEmpty() const;
};

/// <summary>
/// Rendering cache container object for the draw calls collecting, sorting and executing.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API RenderList : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(RenderList);

    /// <summary>
    /// Allocates the new renderer list object or reuses already allocated one.
    /// </summary>
    /// <returns>The cache object.</returns>
    API_FUNCTION() static RenderList* GetFromPool();

    /// <summary>
    /// Frees the list back to the pool.
    /// </summary>
    /// <param name="cache">The cache.</param>
    API_FUNCTION() static void ReturnToPool(RenderList* cache);

    /// <summary>
    /// Cleanups the static data cache used to accelerate draw calls sorting. Use it to reduce memory pressure.
    /// </summary>
    static void CleanupCache();

public:
    /// <summary>
    /// All scenes for rendering.
    /// </summary>
    Array<SceneRendering*> Scenes;

    /// <summary>
    /// Draw calls list (for all draw passes).
    /// </summary>
    RenderListBuffer<DrawCall> DrawCalls;

    /// <summary>
    /// Draw calls list with pre-batched instances (for all draw passes).
    /// </summary>
    RenderListBuffer<BatchedDrawCall> BatchedDrawCalls;

    /// <summary>
    /// The draw calls lists. Each for the separate draw pass.
    /// </summary>
    DrawCallsList DrawCallsLists[(int32)DrawCallsListType::MAX];

    /// <summary>
    /// The additional draw calls list for Depth drawing into Shadow Projections that use DrawCalls from main render context. This assumes that RenderContextBatch contains main context and shadow projections only.
    /// </summary>
    DrawCallsList ShadowDepthDrawCallsList;

    /// <summary>
    /// Light pass members - directional lights
    /// </summary>
    Array<RenderDirectionalLightData> DirectionalLights;

    /// <summary>
    /// Light pass members - point lights
    /// </summary>
    Array<RenderPointLightData> PointLights;

    /// <summary>
    /// Light pass members - spot lights
    /// </summary>
    Array<RenderSpotLightData> SpotLights;

    /// <summary>
    /// Light pass members - sky lights
    /// </summary>
    Array<RenderSkyLightData> SkyLights;

    /// <summary>
    /// Environment probes to use for rendering reflections
    /// </summary>
    Array<RenderEnvironmentProbeData> EnvironmentProbes;

    /// <summary>
    /// Decals registered for the rendering.
    /// </summary>
    Array<RenderDecalData> Decals;

    /// <summary>
    /// Local volumetric fog particles registered for the rendering.
    /// </summary>
    Array<DrawCall> VolumetricFogParticles;

    /// <summary>
    /// Sky/skybox renderer proxy to use (only one per frame)
    /// </summary>
    ISkyRenderer* Sky;

    /// <summary>
    /// Atmospheric fog renderer proxy to use (only one per frame)
    /// </summary>
    IAtmosphericFogRenderer* AtmosphericFog;

    /// <summary>
    /// Fog renderer proxy to use (only one per frame)
    /// </summary>
    IFogRenderer* Fog;

    /// <summary>
    /// Post effects to render.
    /// </summary>
    Array<PostProcessEffect*> PostFx;

    /// <summary>
    /// The renderer setup for the frame drawing.
    /// </summary>
    RenderSetup Setup;

    /// <summary>
    /// The post process settings.
    /// </summary>
    PostProcessSettings Settings;

    struct FLAXENGINE_API BlendableSettings
    {
        IPostFxSettingsProvider* Provider;
        float Weight;
        int32 Priority;
        float VolumeSizeSqr;

        bool operator<(const BlendableSettings& other) const;
    };

    /// <summary>
    /// The blendable postFx volumes collected during frame draw calls gather pass.
    /// </summary>
    Array<BlendableSettings> Blendable;

    void AddSettingsBlend(IPostFxSettingsProvider* provider, float weight, int32 priority, float volumeSizeSqr);

    /// <summary>
    /// Camera frustum corners in World Space
    /// </summary>
    Float3 FrustumCornersWs[8];

    /// <summary>
    /// Camera frustum corners in View Space
    /// </summary>
    Float3 FrustumCornersVs[8];

    /// <summary>
    /// Objects buffer that contains ShaderObjectData for each DrawCall.
    /// </summary>
    DynamicTypedBuffer ObjectBuffer;

    /// <summary>
    /// Temporary objects buffer that contains ShaderObjectData for each DrawCall reused during scene rendering (eg. by skybox).
    /// </summary>
    DynamicTypedBuffer TempObjectBuffer;

private:
    DynamicVertexBuffer _instanceBuffer;

public:
    /// <summary>
    /// Blends the postprocessing settings into the final options.
    /// </summary>
    void BlendSettings();

    /// <summary>
    /// Runs the post fx materials pass. Uses input/output buffer to render all materials. Uses temporary render target as a ping pong buffer if required (the same format and description).
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="locationA">The material postFx location.</param>
    /// <param name="locationB">The custom postFx location.</param>
    /// <param name="inputOutput">The input and output texture.</param>
    void RunPostFxPass(GPUContext* context, RenderContext& renderContext, MaterialPostFxLocation locationA, PostProcessEffectLocation locationB, GPUTexture*& inputOutput);

    /// <summary>
    /// Runs the material post fx pass. Uses input and output buffers as a ping pong to render all materials.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="location">The material postFx location.</param>
    /// <param name="input">The input texture.</param>
    /// <param name="output">The output texture.</param>
    void RunMaterialPostFxPass(GPUContext* context, RenderContext& renderContext, MaterialPostFxLocation location, GPUTexture*& input, GPUTexture*& output);

    /// <summary>
    /// Runs the custom post fx pass. Uses input and output buffers as a ping pong to render all effects.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="location">The custom postFx location.</param>
    /// <param name="input">The input texture.</param>
    /// <param name="output">The output texture.</param>
    void RunCustomPostFxPass(GPUContext* context, RenderContext& renderContext, PostProcessEffectLocation location, GPUTexture*& input, GPUTexture*& output);

    /// <summary>
    /// Determines whether any Custom PostFx specified by given type. Used to pick a faster rendering path by the frame rendering module.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="postProcess">The PostFx location to check (for scripts).</param>
    /// <returns>True if render any postFx of the given type, otherwise false.</returns>
    bool HasAnyPostFx(const RenderContext& renderContext, PostProcessEffectLocation postProcess) const;

    /// <summary>
    /// Determines whether any Material PostFx specified by given type. Used to pick a faster rendering path by the frame rendering module.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="materialPostFx">The PostFx location to check (for materials).</param>
    /// <returns>True if render any postFx of the given type, otherwise false.</returns>
    bool HasAnyPostFx(const RenderContext& renderContext, MaterialPostFxLocation materialPostFx) const;

    /// <summary>
    /// Determines whether any Custom PostFx or Material PostFx specified by given type. Used to pick a faster rendering path by the frame rendering module.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="postProcess">The PostFx location to check (for scripts).</param>
    /// <param name="materialPostFx">The PostFx location to check (for materials).</param>
    /// <returns>True if render any postFx of the given type, otherwise false.</returns>
    bool HasAnyPostFx(const RenderContext& renderContext, PostProcessEffectLocation postProcess, MaterialPostFxLocation materialPostFx) const
    {
        return HasAnyPostFx(renderContext, postProcess) || HasAnyPostFx(renderContext, materialPostFx);
    }

public:
    /// <summary>
    /// Init cache for given task
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Init(RenderContext& renderContext);

    /// <summary>
    /// Clear cached data
    /// </summary>
    void Clear();

public:
    /// <summary>
    /// Adds the draw call to the draw lists.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="drawModes">The object draw modes.</param>
    /// <param name="staticFlags">The object static flags.</param>
    /// <param name="drawCall">The draw call data.</param>
    /// <param name="receivesDecals">True if the rendered mesh can receive decals.</param>
    /// <param name="sortOrder">Object sorting key.</param>
    void AddDrawCall(const RenderContext& renderContext, DrawPass drawModes, StaticFlags staticFlags, DrawCall& drawCall, bool receivesDecals = true, int8 sortOrder = 0);

    /// <summary>
    /// Adds the draw call to the draw lists and references it in other render contexts. Performs additional per-context frustum culling.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch. This assumes that RenderContextBatch contains main context and shadow projections only.</param>
    /// <param name="drawModes">The object draw modes.</param>
    /// <param name="staticFlags">The object static flags.</param>
    /// <param name="shadowsMode">The object shadows casting mode.</param>
    /// <param name="bounds">The object bounds.</param>
    /// <param name="drawCall">The draw call data.</param>
    /// <param name="receivesDecals">True if the rendered mesh can receive decals.</param>
    /// <param name="sortOrder">Object sorting key.</param>
    void AddDrawCall(const RenderContextBatch& renderContextBatch, DrawPass drawModes, StaticFlags staticFlags, ShadowsCastingMode shadowsMode, const BoundingSphere& bounds, DrawCall& drawCall, bool receivesDecals = true, int8 sortOrder = 0);

    /// <summary>
    /// Writes all draw calls into large objects buffer (used for random-access object data access on a GPU). Can be executed in async.
    /// </summary>
    void BuildObjectsBuffer();

    /// <summary>
    /// Sorts the collected draw calls list.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reverseDistance">If set to <c>true</c> reverse draw call distance to the view. Results in back to front sorting.</param>
    /// <param name="listType">The collected draw calls list type.</param>
    /// <param name="pass">The draw pass (optional).</param>
    API_FUNCTION() FORCE_INLINE void SortDrawCalls(API_PARAM(Ref) const RenderContext& renderContext, bool reverseDistance, DrawCallsListType listType, DrawPass pass = DrawPass::All)
    {
        const bool stable = listType == DrawCallsListType::Forward;
        SortDrawCalls(renderContext, reverseDistance, DrawCallsLists[(int32)listType], DrawCalls, pass, stable);
    }

    /// <summary>
    /// Sorts the collected draw calls list.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reverseDistance">If set to <c>true</c> reverse draw call distance to the view. Results in back to front sorting.</param>
    /// <param name="list">The collected draw calls indices list.</param>
    /// <param name="drawCalls">The collected draw calls list.</param>
    /// <param name="pass">The draw pass (optional).</param>
    /// <param name="stable">If set to <c>true</c> draw batches will be additionally sorted to prevent any flickering, otherwise Depth Buffer will smooth out any non-stability in sorting.</param>
    void SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls, DrawPass pass = DrawPass::All, bool stable = false);

    /// <summary>
    /// Executes the collected draw calls.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="listType">The collected draw calls list type.</param>
    /// <param name="input">The input scene color. It's optional and used in forward/postFx rendering.</param>
    API_FUNCTION() FORCE_INLINE void ExecuteDrawCalls(API_PARAM(Ref) const RenderContext& renderContext, DrawCallsListType listType, GPUTextureView* input = nullptr)
    {
        ExecuteDrawCalls(renderContext, DrawCallsLists[(int32)listType], this, input);
    }

    /// <summary>
    /// Executes the collected draw calls.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="list">The collected draw calls indices list.</param>
    /// <param name="input">The input scene color. It's optional and used in forward/postFx rendering.</param>
    FORCE_INLINE void ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list, GPUTextureView* input = nullptr)
    {
        ExecuteDrawCalls(renderContext, list, this, input);
    }

    /// <summary>
    /// Executes the collected draw calls.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="list">The collected draw calls indices list.</param>
    /// <param name="drawCallsList">The collected draw calls list owner.</param>
    /// <param name="input">The input scene color. It's optional and used in forward/postFx rendering.</param>
    void ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list, RenderList* drawCallsList, GPUTextureView* input);
};

/// <summary>
/// Represents a single object information for GPU rendering.
/// </summary>
GPU_CB_STRUCT(ShaderObjectData
    {
    Float4 Raw[8];

    void FLAXENGINE_API Store(const Matrix& worldMatrix, const Matrix& prevWorldMatrix, const Rectangle& lightmapUVsArea, const Float3& geometrySize, float perInstanceRandom = 0.0f, float worldDeterminantSign = 1.0f, float lodDitherFactor = 0.0f);
    void FLAXENGINE_API Load(Matrix& worldMatrix, Matrix& prevWorldMatrix, Rectangle& lightmapUVsArea, Float3& geometrySize, float& perInstanceRandom, float& worldDeterminantSign, float& lodDitherFactor) const;

    FORCE_INLINE void Store(const DrawCall& drawCall)
    {
    Store(drawCall.World, drawCall.Surface.PrevWorld, drawCall.Surface.LightmapUVsArea, drawCall.Surface.GeometrySize, drawCall.PerInstanceRandom, drawCall.WorldDeterminantSign, drawCall.Surface.LODDitherFactor);
    }

    FORCE_INLINE void Load(DrawCall& drawCall) const
    {
    Load(drawCall.World, drawCall.Surface.PrevWorld, drawCall.Surface.LightmapUVsArea, drawCall.Surface.GeometrySize, drawCall.PerInstanceRandom, drawCall.WorldDeterminantSign, drawCall.Surface.LODDitherFactor);
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    }
    });

/// <summary>
/// Represents data passed to Vertex Shader used for instanced rendering (per-instance element).
/// </summary>
PACK_STRUCT(struct ShaderObjectDrawInstanceData
    {
    uint32 ObjectIndex;
    });

struct SurfaceDrawCallHandler
{
    static void GetHash(const DrawCall& drawCall, uint32& batchKey);
    static bool CanBatch(const DrawCall& a, const DrawCall& b, DrawPass pass);
};
