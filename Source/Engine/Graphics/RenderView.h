// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingFrustum.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/LayersMask.h"
#include "Engine/Level/Types.h"
#include "Enums.h"

struct RenderContext;
struct Viewport;
class Camera;
class RenderList;
class RenderTask;
class SceneRenderTask;

/// <summary>
/// Rendering view description that defines how to render the objects (camera placement, rendering properties, etc.).
/// </summary>
API_STRUCT() struct FLAXENGINE_API RenderView
{
DECLARE_SCRIPTING_TYPE_MINIMAL(RenderView);

    /// <summary>
    /// The position of the view.
    /// </summary>
    API_FIELD() Vector3 Position;

    /// <summary>
    /// The far plane.
    /// </summary>
    API_FIELD() float Far = 10000.0f;

    /// <summary>
    /// The direction of the view.
    /// </summary>
    API_FIELD() Vector3 Direction;

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
    /// The static flags mask used to hide objects that don't have a given static flags. Eg. use StaticFlags::Lightmap to render only objects that can use lightmap.
    /// </summary>
    API_FIELD() StaticFlags StaticFlagsMask = StaticFlags::None;

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
    /// </summary>
    API_FIELD() int32 ShadowModelLODBias = 0;

    /// <summary>
    /// The model LOD distance scale factor. Default is 1. Applied to all the objects in the shadow maps render views. Higher values increase LODs quality. Can be used to improve shadows rendering performance or increase quality.
    /// </summary>
    API_FIELD() float ShadowModelLODDistanceFactor = 1.0f;

    /// <summary>
    /// The Temporal Anti-Aliasing jitter frame index.
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
    API_FIELD() Vector4 ViewInfo;

    /// <summary>
    /// The screen size packed (x - width, y - height, zw - inv width, w - inv height). Cached before rendering.
    /// </summary>
    API_FIELD() Vector4 ScreenSize;

    /// <summary>
    /// The temporal AA jitter packed (xy - this frame jitter, zw - previous frame jitter). Cached before rendering. Zero if TAA is disabled. The value added to projection matrix (in clip space).
    /// </summary>
    API_FIELD() Vector4 TemporalAAJitter;

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
    void PrepareCache(RenderContext& renderContext, float width, float height, const Vector2& temporalAAJitter);

    /// <summary>
    /// Determines whether view is perspective projection or orthographic.
    /// </summary>
    /// <returns>True if view is perspective, otherwise false if view is orthographic.</returns>
    FORCE_INLINE bool IsPerspectiveProjection() const
    {
        return Projection.M44 < 1.0f;
    }

    /// <summary>
    /// Determines whether view is orthographic projection or perspective.
    /// </summary>
    /// <returns>True if view is orthographic, otherwise false if view is perspective.</returns>
    FORCE_INLINE bool IsOrthographicProjection() const
    {
        return Projection.M44 >= 1.0f;
    }

public:

    // Set up view with custom params
    // @param viewProjection View * Projection matrix
    void SetUp(const Matrix& viewProjection)
    {
        // Copy data
        Matrix::Invert(viewProjection, IVP);
        Frustum.SetMatrix(viewProjection);
        CullingFrustum = Frustum;
    }

    // Set up view with custom params
    // @param view View matrix
    // @param projection Projection matrix
    void SetUp(const Matrix& view, const Matrix& projection)
    {
        // Copy data
        Projection = projection;
        NonJitteredProjection = projection;
        View = view;
        Matrix::Invert(View, IV);
        Matrix::Invert(Projection, IP);

        // Compute matrix
        Matrix viewProjection;
        Matrix::Multiply(View, Projection, viewProjection);
        Matrix::Invert(viewProjection, IVP);
        Frustum.SetMatrix(viewProjection);
        CullingFrustum = Frustum;
    }

    /// <summary>
    /// Set up view for cube rendering
    /// </summary>
    /// <param name="nearPlane">Near plane</param>
    /// <param name="farPlane">Far plane</param>
    /// <param name="position">Camera's position</param>
    void SetUpCube(float nearPlane, float farPlane, const Vector3& position)
    {
        // Copy data
        Near = nearPlane;
        Far = farPlane;
        Position = position;

        // Create projection matrix
        Matrix::PerspectiveFov(PI_OVER_2, 1.0f, nearPlane, farPlane, Projection);
        NonJitteredProjection = Projection;
        Matrix::Invert(Projection, IP);
    }

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
    void SetProjector(float nearPlane, float farPlane, const Vector3& position, const Vector3& direction, const Vector3& up, float angle)
    {
        // Copy data
        Near = nearPlane;
        Far = farPlane;
        Position = position;

        // Create projection matrix
        Matrix::PerspectiveFov(angle * DegreesToRadians, 1.0f, nearPlane, farPlane, Projection);
        NonJitteredProjection = Projection;
        Matrix::Invert(Projection, IP);

        // Create view matrix
        Direction = direction;
        Matrix::LookAt(Position, Position + Direction, up, View);
        Matrix::Invert(View, IV);

        // Compute frustum matrix
        Frustum.SetMatrix(View, Projection);
        Matrix::Invert(ViewProjection(), IVP);
        CullingFrustum = Frustum;
    }

    // Copy view data from camera
    // @param camera Camera to copy its data
    void CopyFrom(Camera* camera);

    // Copy view data from camera
    // @param camera Camera to copy its data
    // @param camera The custom viewport to use for view/projection matrices override.
    void CopyFrom(Camera* camera, Viewport* viewport);

public:

    DrawPass GetShadowsDrawPassMask(ShadowsCastingMode shadowsMode) const;

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
};
