// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Half.h"
#include "Engine/Graphics/PostProcessSettings.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "DrawCall.h"

enum class StaticFlags;
class RenderBuffers;
class LightWithShadow;
class IPostFxSettingsProvider;
class CubeTexture;
class PostProcessBase;
struct RenderContext;

struct RendererDirectionalLightData
{
    Vector3 Position;
    float MinRoughness;

    Vector3 Color;
    float ShadowsStrength;

    Vector3 Direction;
    float ShadowsFadeDistance;

    float ShadowsNormalOffsetScale;
    float ShadowsDepthBias;
    float ShadowsSharpness;
    float VolumetricScatteringIntensity;

    int8 CastVolumetricShadow : 1;
    int8 RenderedVolumetricFog : 1;

    float ShadowsDistance;
    int32 CascadeCount;
    float ContactShadowsLength;
    ShadowsCastingMode ShadowsMode;

    void SetupLightData(LightData* data, const RenderView& view, bool useShadow) const;
};

struct RendererSpotLightData
{
    Vector3 Position;
    float MinRoughness;

    Vector3 Color;
    float ShadowsStrength;

    Vector3 Direction;
    float ShadowsFadeDistance;

    float ShadowsNormalOffsetScale;
    float ShadowsDepthBias;
    float ShadowsSharpness;
    float VolumetricScatteringIntensity;

    float ShadowsDistance;
    float Radius;
    float FallOffExponent;
    float SourceRadius;

    Vector3 UpVector;
    float OuterConeAngle;

    float CosOuterCone;
    float InvCosConeDifference;
    float ContactShadowsLength;
    ShadowsCastingMode ShadowsMode;

    int8 CastVolumetricShadow : 1;
    int8 RenderedVolumetricFog : 1;
    int8 UseInverseSquaredFalloff : 1;

    GPUTexture* IESTexture;

    void SetupLightData(LightData* data, const RenderView& view, bool useShadow) const;
};

struct RendererPointLightData
{
    Vector3 Position;
    float MinRoughness;

    Vector3 Color;
    float ShadowsStrength;

    Vector3 Direction;
    float ShadowsFadeDistance;

    float ShadowsNormalOffsetScale;
    float ShadowsDepthBias;
    float ShadowsSharpness;
    float VolumetricScatteringIntensity;

    float ShadowsDistance;
    float Radius;
    float FallOffExponent;
    float SourceRadius;

    float SourceLength;
    float ContactShadowsLength;
    ShadowsCastingMode ShadowsMode;

    int8 CastVolumetricShadow : 1;
    int8 RenderedVolumetricFog : 1;
    int8 UseInverseSquaredFalloff : 1;

    GPUTexture* IESTexture;

    void SetupLightData(LightData* data, const RenderView& view, bool useShadow) const;
};

struct RendererSkyLightData
{
    Vector3 Position;
    float VolumetricScatteringIntensity;

    Vector3 Color;
    float Radius;

    Vector3 AdditiveColor;

    int8 CastVolumetricShadow : 1;
    int8 RenderedVolumetricFog : 1;

    CubeTexture* Image;

    void SetupLightData(LightData* data, const RenderView& view, bool useShadow) const;
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
    uint32 SortKey;

    /// <summary>
    /// The first draw call index.
    /// </summary>
    int32 StartIndex;

    /// <summary>
    /// A number of draw calls to be submitted at once.
    /// </summary>
    int32 BatchSize;

    /// <summary>
    /// The total amount of instances (sum from all draw calls in this batch).
    /// </summary>
    int32 InstanceCount;

    bool operator<(const DrawBatch& other) const
    {
        return SortKey < other.SortKey;
    }
};

/// <summary>
/// Represents a list of draw calls.
/// </summary>
struct DrawCallsList
{
    /// <summary>
    /// The list of draw calls indices to render.
    /// </summary>
    Array<int32> Indices;

    /// <summary>
    /// The draw calls batches (for instancing).
    /// </summary>
    Array<DrawBatch> Batches;

    /// <summary>
    /// True if draw calls batches list can be rendered using hardware instancing, otherwise false.
    /// </summary>
    bool CanUseInstancing;

    void Clear()
    {
        Indices.Clear();
        Batches.Clear();
        CanUseInstancing = true;
    }

    bool IsEmpty() const
    {
        return Indices.IsEmpty();
    }
};

/// <summary>
/// Rendering cache container object for the draw calls collecting, sorting and executing.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API RenderList : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE(RenderList);
public:

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
    /// Draw calls list (for all draw passes).
    /// </summary>
    Array<DrawCall> DrawCalls;

    /// <summary>
    /// The draw calls lists. Each for the separate draw pass.
    /// </summary>
    DrawCallsList DrawCallsLists[(int32)DrawCallsListType::MAX];

    /// <summary>
    /// Light pass members - directional lights
    /// </summary>
    Array<RendererDirectionalLightData> DirectionalLights;

    /// <summary>
    /// Light pass members - point lights
    /// </summary>
    Array<RendererPointLightData> PointLights;

    /// <summary>
    /// Light pass members - spot lights
    /// </summary>
    Array<RendererSpotLightData> SpotLights;

    /// <summary>
    /// Light pass members - sky lights
    /// </summary>
    Array<RendererSkyLightData> SkyLights;

    /// <summary>
    /// Environment probes to use for rendering reflections
    /// </summary>
    Array<EnvironmentProbe*> EnvironmentProbes;

    /// <summary>
    /// Decals registered for the rendering.
    /// </summary>
    Array<Decal*> Decals;

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
    /// Post effect to render (TEMPORARY! cleared after and before rendering).
    /// </summary>
    Array<PostProcessBase*> PostFx;

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
    Vector3 FrustumCornersWs[8];

    /// <summary>
    /// Camera frustum corners in View Space
    /// </summary>
    Vector3 FrustumCornersVs[8];

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
    /// Determines whether any Custom PostFx or Material PostFx has to be rendered after AA pass. Used to pick a faster rendering path by the frame rendering module.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>True if render any postFx after AA, otherwise false.</returns>
    bool HasAnyPostAA(RenderContext& renderContext) const;

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
    /// <param name="drawModes">The object draw modes.</param>
    /// <param name="staticFlags">The object static flags.</param>
    /// <param name="drawCall">The draw call data.</param>
    /// <param name="receivesDecals">True if the rendered mesh can receive decals.</param>
    void AddDrawCall(DrawPass drawModes, StaticFlags staticFlags, DrawCall& drawCall, bool receivesDecals);

    /// <summary>
    /// Sorts the collected draw calls list.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reverseDistance">If set to <c>true</c> reverse draw call distance to the view. Results in back to front sorting.</param>
    /// <param name="listType">The collected draw calls list type.</param>
    API_FUNCTION() FORCE_INLINE void SortDrawCalls(API_PARAM(Ref) const RenderContext& renderContext, bool reverseDistance, DrawCallsListType listType)
    {
        SortDrawCalls(renderContext, reverseDistance, DrawCallsLists[(int32)listType]);
    }

    /// <summary>
    /// Sorts the collected draw calls list.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="reverseDistance">If set to <c>true</c> reverse draw call distance to the view. Results in back to front sorting.</param>
    /// <param name="list">The collected draw calls list.</param>
    void SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list);

    /// <summary>
    /// Executes the collected draw calls.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="listType">The collected draw calls list type.</param>
    API_FUNCTION() FORCE_INLINE void ExecuteDrawCalls(API_PARAM(Ref) const RenderContext& renderContext, DrawCallsListType listType)
    {
        ExecuteDrawCalls(renderContext, DrawCallsLists[(int32)listType]);
    }

    /// <summary>
    /// Executes the collected draw calls.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="list">The collected draw calls list.</param>
    void ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list);
};

/// <summary>
/// Represents data per instance element used for instanced rendering.
/// </summary>
struct FLAXENGINE_API InstanceData
{
    Vector3 InstanceOrigin;
    float PerInstanceRandom;
    Vector3 InstanceTransform1;
    float LODDitherFactor;
    Vector3 InstanceTransform2;
    Vector3 InstanceTransform3;
    Half4 InstanceLightmapArea;
};

struct SurfaceDrawCallHandler
{
    static void GetHash(const DrawCall& drawCall, int32& batchKey);
    static bool CanBatch(const DrawCall& a, const DrawCall& b);
    static void WriteDrawCall(InstanceData* instanceData, const DrawCall& drawCall);
};
