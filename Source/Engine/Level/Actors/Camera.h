// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/BoundingFrustum.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Types/LayersMask.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#if USE_EDITOR
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/Models/ModelInstanceEntry.h"
#include "Engine/Content/Assets/Model.h"
#endif

/// <summary>
/// Describes the camera projection and view. Provides information about how to render scene (viewport location and direction, etc.).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API Camera : public Actor
{
    DECLARE_SCENE_OBJECT(Camera);

    // List with all created cameras actors on the scene
    static Array<Camera*> Cameras;

    // The current cut-scene camera. Set by the Scene Animation Player to the current shot camera.
    static Camera* CutSceneCamera;

    // The overriden main camera.
    API_FIELD() static ScriptingObjectReference<Camera> OverrideMainCamera;

    // Gets the main camera.
    API_PROPERTY() static Camera* GetMainCamera();

private:
    BoundingFrustum _frustum;

    // Camera Settings
    bool _usePerspective;
    float _fov;
    float _customAspectRatio;
    float _near;
    float _far;
    float _orthoScale;

#if USE_EDITOR
    AssetReference<Model> _previewModel;
    ModelInstanceEntries _previewModelBuffer;
    BoundingBox _previewModelBox;
    int32 _sceneRenderingKey = -1;
#endif

public:
    /// <summary>
    /// Gets the frustum.
    /// </summary>
    API_PROPERTY() FORCE_INLINE BoundingFrustum GetFrustum() const
    {
        return _frustum;
    }

public:
    /// <summary>
    /// Gets the value indicating if camera should use perspective rendering mode, otherwise it will use orthographic projection.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(true), EditorDisplay(\"Camera\"), Tooltip(\"Enables perspective projection mode, otherwise uses orthographic.\")")
    FORCE_INLINE bool GetUsePerspective() const
    {
        return _usePerspective;
    }

    /// <summary>
    /// Sets the value indicating if camera should use perspective rendering mode, otherwise it will use orthographic projection.
    /// </summary>
    API_PROPERTY() void SetUsePerspective(bool value);

    /// <summary>
    /// Gets the camera's field of view (in degrees).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(60.0f), Limit(0, 179), EditorDisplay(\"Camera\", \"Field Of View\"), Tooltip(\"Field of view angle in degrees.\")")
    FORCE_INLINE float GetFieldOfView() const
    {
        return _fov;
    }

    /// <summary>
    /// Sets camera's field of view (in degrees).
    /// </summary>
    API_PROPERTY() void SetFieldOfView(float value);

    /// <summary>
    /// Gets the custom aspect ratio. 0 if not use custom value.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(0.0f), Limit(0, 10, 0.01f), EditorDisplay(\"Camera\"), Tooltip(\"Custom aspect ratio to use. Set to 0 to disable.\")")
    FORCE_INLINE float GetCustomAspectRatio() const
    {
        return _customAspectRatio;
    }

    /// <summary>
    /// Sets the custom aspect ratio. 0 if not use custom value.
    /// </summary>
    API_PROPERTY() void SetCustomAspectRatio(float value);

    /// <summary>
    /// Gets camera's near plane distance.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(10.0f), Limit(0, 1000, 0.05f), EditorDisplay(\"Camera\"), Tooltip(\"Near clipping plane distance\")")
    FORCE_INLINE float GetNearPlane() const
    {
        return _near;
    }

    /// <summary>
    /// Sets camera's near plane distance.
    /// </summary>
    API_PROPERTY() void SetNearPlane(float value);

    /// <summary>
    /// Gets camera's far plane distance.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(40000.0f), Limit(0, float.MaxValue, 5), EditorDisplay(\"Camera\"), Tooltip(\"Far clipping plane distance\")")
    FORCE_INLINE float GetFarPlane() const
    {
        return _far;
    }

    /// <summary>
    /// Sets camera's far plane distance.
    /// </summary>
    API_PROPERTY() void SetFarPlane(float value);

    /// <summary>
    /// Gets the orthographic projection scale.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(1.0f), Limit(0.0001f, 1000, 0.01f), EditorDisplay(\"Camera\"), Tooltip(\"Orthographic projection scale\")")
    FORCE_INLINE float GetOrthographicScale() const
    {
        return _orthoScale;
    }

    /// <summary>
    /// Sets the orthographic projection scale.
    /// </summary>
    API_PROPERTY() void SetOrthographicScale(float value);

    /// <summary>
    /// The layers mask used for rendering using this camera. Can be used to include or exclude specific actor layers from the drawing.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Camera\")")
    LayersMask RenderLayersMask;

public:
    /// <summary>
    /// Projects the point from 3D world-space to game window coordinates (in screen pixels for default viewport calculated from <see cref="Viewport"/>).
    /// </summary>
    /// <param name="worldSpaceLocation">The input world-space location (XYZ in world).</param>
    /// <param name="gameWindowSpaceLocation">The output game window coordinates (XY in screen pixels).</param>
    API_FUNCTION() void ProjectPoint(const Vector3& worldSpaceLocation, API_PARAM(Out) Float2& gameWindowSpaceLocation) const;

    /// <summary>
    /// Projects the point from 3D world-space to the camera viewport-space (in screen pixels for given viewport).
    /// </summary>
    /// <param name="worldSpaceLocation">The input world-space location (XYZ in world).</param>
    /// <param name="cameraViewportSpaceLocation">The output camera viewport-space location (XY in screen pixels).</param>
    /// <param name="viewport">The viewport.</param>
    API_FUNCTION() void ProjectPoint(const Vector3& worldSpaceLocation, API_PARAM(Out) Float2& cameraViewportSpaceLocation, API_PARAM(Ref) const Viewport& viewport) const;

    /// <summary>
    /// Converts the mouse position to 3D ray.
    /// </summary>
    /// <param name="mousePosition">The mouse position.</param>
    /// <returns>Mouse ray</returns>
    API_FUNCTION() Ray ConvertMouseToRay(const Float2& mousePosition) const;

    /// <summary>
    /// Converts the mouse position to 3D ray.
    /// </summary>
    /// <param name="mousePosition">The mouse position.</param>
    /// <param name="viewport">The viewport.</param>
    /// <returns>Mouse ray</returns>
    API_FUNCTION() Ray ConvertMouseToRay(const Float2& mousePosition, API_PARAM(Ref) const Viewport& viewport) const;

    /// <summary>
    /// Gets the camera viewport.
    /// </summary>
    API_PROPERTY() Viewport GetViewport() const;

    /// <summary>
    /// Calculates the view and the projection matrices for the camera.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    API_FUNCTION() void GetMatrices(API_PARAM(Out) Matrix& view, API_PARAM(Out) Matrix& projection) const;

    /// <summary>
    /// Calculates the view and the projection matrices for the camera. Support using custom viewport.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    /// <param name="viewport">The custom output viewport.</param>
    API_FUNCTION() void GetMatrices(API_PARAM(Out) Matrix& view, API_PARAM(Out) Matrix& projection, API_PARAM(Ref) const Viewport& viewport) const;

    /// <summary>
    /// Calculates the view and the projection matrices for the camera. Support using custom viewport and view origin.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    /// <param name="viewport">The custom output viewport.</param>
    /// <param name="origin">The rendering view origin (for relative-to-camera rendering).</param>
    API_FUNCTION() void GetMatrices(API_PARAM(Out) Matrix& view, API_PARAM(Out) Matrix& projection, API_PARAM(Ref) const Viewport& viewport, API_PARAM(Ref) const Vector3& origin) const;

#if USE_EDITOR
    // Intersection check for editor picking the camera
    API_FUNCTION() bool IntersectsItselfEditor(API_PARAM(Ref) const Ray& ray, API_PARAM(Out) Real& distance);
#endif

private:
#if USE_EDITOR
    void OnPreviewModelLoaded();
#endif
    void UpdateCache();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override;
    bool HasContentLoaded() const override;
    void Draw(RenderContext& renderContext) override;
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
#if USE_EDITOR
    void BeginPlay(SceneBeginData* data) override;
#endif
};
