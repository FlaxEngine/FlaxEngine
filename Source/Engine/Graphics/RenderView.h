// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingFrustum.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Vector3.h"
#if USE_LARGE_WORLDS
#include "Engine/Core/Math/Double4x4.h"
#endif
#include "Engine/Core/Types/LayersMask.h"
#include "Engine/Level/Types.h"
#include "Enums.h"

struct RenderContext;
struct RenderContextBatch;
struct Viewport;
class Camera;
class RenderList;
class RenderTask;
class SceneRenderTask;

/// <summary>
/// Rendering view description that defines how to render the objects (camera placement, rendering properties, etc.).
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API RenderView
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(RenderView);

    /// <summary>
    /// The position of the view origin (in world-units). Used for camera-relative rendering to achieve large worlds support with keeping 32-bit precision for coordinates in scene rendering.
    /// </summary>
    API_FIELD() Vector3 Origin = Vector3::Zero;

    /// <summary>
    /// The global position of the view (Origin+Position).
    /// </summary>
    API_FIELD() Vector3 WorldPosition;

    /// <summary>
    /// The position of the view (relative to the origin).
    /// </summary>
    API_FIELD() Float3 Position;

    /// <summary>
    /// The far plane.
    /// </summary>
    API_FIELD() float Far = 10000.0f;

    /// <summary>
    /// The direction of the view.
    /// </summary>
    API_FIELD() Float3 Direction;

    /// <summary>
    /// The near plane.
    /// </summary>
    API_FIELD() float Near = 0.1f;

    /// <summary>
    /// The view matrix.
    /// </summary>
    API_FIELD() Matrix View;

    /// <summary>
    /// The projection matrix.
    /// </summary>
    API_FIELD() Matrix Projection;

    /// <summary>
    /// The projection matrix with no camera offset (no jittering). 
    /// For many temporal image effects, the camera that is currently rendering needs to be slightly offset from the default projection (that is, the camera is ‘jittered’). 
    /// If you use motion vectors and camera jittering together, use this property to keep the motion vectors stable between frames.
    /// </summary>
    API_FIELD() Matrix NonJitteredProjection;

    /// <summary>
    /// The inverted view matrix.
    /// </summary>
    API_FIELD() Matrix IV;

    /// <summary>
    /// The inverted projection matrix.
    /// </summary>
    API_FIELD() Matrix IP;

    /// <summary>
    /// The inverted projection view matrix.
    /// </summary>
    API_FIELD() Matrix IVP;

    /// <summary>
    /// The view frustum.
    /// </summary>
    API_FIELD() BoundingFrustum Frustum;

    /// <summary>
    /// The view frustum used for culling (can be different than Frustum in some cases e.g. cascaded shadow map rendering).
    /// </summary>
    API_FIELD() BoundingFrustum CullingFrustum;

public:
    /// <summary>
    /// The draw passes mask for the current view rendering.
    /// </summary>
    API_FIELD() DrawPass Pass = DrawPass::None;

    /// <summary>
    /// Flag used by static, offline rendering passes (eg. reflections rendering, lightmap rendering etc.)
    /// </summary>
    API_FIELD() bool IsOfflinePass = false;

    /// <summary>
    /// Flag used by single-frame rendering passes (eg. thumbnail rendering, model view caching) to reject LOD transitions animations and other temporal draw effects.
    /// </summary>
    API_FIELD() bool IsSingleFrame = false;

    /// <summary>
    /// Flag used by custom passes to skip any object culling when drawing.
    /// </summary>
    API_FIELD() bool IsCullingDisabled = false;

    /// <summary>
    /// True if TAA has been resolved when rendering view and frame doesn't contain jitter anymore. Rendering geometry after this point should not use jitter anymore (eg. editor gizmos or custom geometry as overlay).
    /// </summary>
    API_FIELD() bool IsTaaResolved = false;

    /// <summary>
    /// The static flags mask used to hide objects that don't have a given static flags. Eg. use StaticFlags::Lightmap to render only objects that can use lightmap.
    /// </summary>
    API_FIELD() StaticFlags StaticFlagsMask = StaticFlags::None;

    /// <summary>
    /// The static flags mask comparision rhs. Allows to draw objects that don't pass the static flags mask. Objects are checked with the following formula: (ObjectStaticFlags and StaticFlagsMask) == StaticFlagsMaskCompare.
    /// </summary>
    API_FIELD() StaticFlags StaticFlagsCompare = StaticFlags::None;

    /// <summary>
    /// The view flags.
    /// </summary>
    API_FIELD() ViewFlags Flags = ViewFlags::DefaultGame;

    /// <summary>
    /// The view mode.
    /// </summary>
    API_FIELD() ViewMode Mode = ViewMode::Default;

    /// <summary>
    /// Maximum allowed shadows quality for this view
    /// </summary>
    API_FIELD() Quality MaxShadowsQuality = Quality::Ultra;

    /// <summary>
    /// The model LOD bias. Default is 0. Applied to all the objects in the render view.
    /// </summary>
    API_FIELD() int32 ModelLODBias = 0;

    /// <summary>
    /// The model LOD distance scale factor. Default is 1. Applied to all the objects in the render view. Higher values increase LODs quality.
    /// </summary>
    API_FIELD() float ModelLODDistanceFactor = 1.0f;

    /// <summary>
    /// The model LOD bias. Default is 0. Applied to all the objects in the shadow maps render views. Can be used to improve shadows rendering performance or increase quality.
    /// [Deprecated on 26.10.2022, expires on 26.10.2024]
    /// </summary>
    API_FIELD() DEPRECATED() int32 ShadowModelLODBias = 0;

    /// <summary>
    /// The model LOD distance scale factor. Default is 1. Applied to all the objects in the shadow maps render views. Higher values increase LODs quality. Can be used to improve shadows rendering performance or increase quality.
    /// [Deprecated on 26.10.2022, expires on 26.10.2024]
    /// </summary>
    API_FIELD() DEPRECATED() float ShadowModelLODDistanceFactor = 1.0f;

    /// <summary>
    /// Temporal Anti-Aliasing jitter frame index.
    /// </summary>
    API_FIELD() int32 TaaFrameIndex = 0;

    /// <summary>
    /// The rendering mask for layers. Used to exclude objects from rendering.
    /// </summary>
    API_FIELD() LayersMask RenderLayersMask;

public:
    /// <summary>
    /// The view information vector with packed components to reconstruct linear depth and view position from the hardware depth buffer. Cached before rendering.
    /// </summary>
    API_FIELD() Float4 ViewInfo;

    /// <summary>
    /// The screen size packed (x - width, y - height, zw - inv width, w - inv height). Cached before rendering.
    /// </summary>
    API_FIELD() Float4 ScreenSize;

    /// <summary>
    /// The temporal AA jitter packed (xy - this frame jitter, zw - previous frame jitter). Cached before rendering. Zero if TAA is disabled. The value added to projection matrix (in clip space).
    /// </summary>
    API_FIELD() Float4 TemporalAAJitter;

    /// <summary>
    /// The previous frame rendering view origin.
    /// </summary>
    API_FIELD() Vector3 PrevOrigin;

    /// <summary>
    /// The previous frame view matrix.
    /// </summary>
    API_FIELD() Matrix PrevView;

    /// <summary>
    /// The previous frame projection matrix.
    /// </summary>
    API_FIELD() Matrix PrevProjection;

    /// <summary>
    /// The previous frame view * projection matrix.
    /// </summary>
    API_FIELD() Matrix PrevViewProjection;

    /// <summary>
    /// The main viewport view * projection matrix.
    /// </summary>
    API_FIELD() Matrix MainViewProjection;

    /// <summary>
    /// The main viewport screen size packed (x - width, y - height, zw - inv width, w - inv height).
    /// </summary>
    API_FIELD() Float4 MainScreenSize;

    /// <summary>
    /// Square of <see cref="ModelLODDistanceFactor"/>. Cached by rendering backend.
    /// </summary>
    API_FIELD() float ModelLODDistanceFactorSqrt;

    /// <summary>
    /// Prepares view for rendering a scene. Called before rendering so other parts can reuse calculated value.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    void Prepare(RenderContext& renderContext);

    /// <summary>
    /// Prepares the cached data.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="width">The rendering width.</param>
    /// <param name="height">The rendering height.</param>
    /// <param name="temporalAAJitter">The temporal jitter for this frame.</param>
    /// <param name="mainView">The main rendering viewport. Use null if it's top level view; pass pointer to main view for sub-passes like shadow depths.</param>
    void PrepareCache(const RenderContext& renderContext, float width, float height, const Float2& temporalAAJitter, const RenderView* mainView = nullptr);

    /// <summary>
    /// Determines whether view is perspective projection or orthographic.
    /// </summary>
    FORCE_INLINE bool IsPerspectiveProjection() const
    {
        return Projection.M44 < 1.0f;
    }

    /// <summary>
    /// Determines whether view is orthographic projection or perspective.
    /// </summary>
    FORCE_INLINE bool IsOrthographicProjection() const
    {
        return Projection.M44 >= 1.0f;
    }

public:
    // Ignore deprecation warnings in defaults
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    RenderView() = default;
    RenderView(const RenderView& other) = default;
    RenderView(RenderView&& other) = default;
    RenderView& operator=(const RenderView& other) = default;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS

    /// <summary>
    /// Updates the cached data for the view (inverse matrices, etc.).
    /// </summary>
    void UpdateCachedData();

    // Set up view with custom params
    // @param viewProjection View * Projection matrix
    void SetUp(const Matrix& viewProjection);

    // Set up view with custom params
    // @param view View matrix
    // @param projection Projection matrix
    void SetUp(const Matrix& view, const Matrix& projection);

    /// <summary>
    /// Set up view for cube rendering
    /// </summary>
    /// <param name="nearPlane">Near plane</param>
    /// <param name="farPlane">Far plane</param>
    /// <param name="position">Camera's position</param>
    void SetUpCube(float nearPlane, float farPlane, const Float3& position);

    /// <summary>
    /// Set up view for given face of the cube rendering
    /// </summary>
    /// <param name="faceIndex">Face index(0-5)</param>
    void SetFace(int32 faceIndex);

    /// <summary>
    /// Set up view for cube rendering
    /// </summary>
    /// <param name="nearPlane">Near plane</param>
    /// <param name="farPlane">Far plane</param>
    /// <param name="position">Camera's position</param>
    /// <param name="direction">Camera's direction vector</param>
    /// <param name="up">Camera's up vector</param>
    /// <param name="angle">Camera's FOV angle (in degrees)</param>
    void SetProjector(float nearPlane, float farPlane, const Float3& position, const Float3& direction, const Float3& up, float angle);

    /// <summary>
    /// Copies view data from camera to the view.
    /// </summary>
    /// <param name="camera">The camera to copy its data.</param>
    /// <param name="viewport">The custom viewport to use for view/projection matrices override.</param>
    void CopyFrom(const Camera* camera, const Viewport* viewport = nullptr);

public:
    FORCE_INLINE DrawPass GetShadowsDrawPassMask(ShadowsCastingMode shadowsMode) const
    {
        switch (shadowsMode)
        {
        case ShadowsCastingMode::All:
            return DrawPass::All;
        case ShadowsCastingMode::DynamicOnly:
            return IsOfflinePass ? ~DrawPass::Depth : DrawPass::All;
        case ShadowsCastingMode::StaticOnly:
            return IsOfflinePass ? DrawPass::All : ~DrawPass::Depth;
        case ShadowsCastingMode::None:
            return ~DrawPass::Depth;
        default:
            return DrawPass::All;
        }
    }

public:
    // Camera's View * Projection matrix
    FORCE_INLINE const Matrix& ViewProjection() const
    {
        return Frustum.GetMatrix();
    }

    // Camera's View * Projection matrix
    FORCE_INLINE Matrix ViewProjection()
    {
        return Frustum.GetMatrix();
    }

    // Calculates the world matrix for the given transformation instance rendering.
    void GetWorldMatrix(const Transform& transform, Matrix& world) const;

    // Applies the render origin to the transformation instance matrix.
    FORCE_INLINE void GetWorldMatrix(Matrix& world) const
    {
        world.M41 -= (float)Origin.X;
        world.M42 -= (float)Origin.Y;
        world.M43 -= (float)Origin.Z;
    }

    // Applies the render origin to the transformation instance matrix.
    void GetWorldMatrix(Double4x4& world) const;
};

// Removes TAA jitter from the RenderView when drawing geometry after TAA has been resolved to prevent unwanted jittering.
struct TaaJitterRemoveContext
{
private:
    RenderView* _view = nullptr;
    Matrix _prevProjection, _prevNonJitteredProjection;

public:
    TaaJitterRemoveContext(const RenderView& view);
    ~TaaJitterRemoveContext();
};
