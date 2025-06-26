// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// Environment Probe can capture space around the objects to provide reflections.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Visuals/Lighting & PostFX/Environment Probe\"), ActorToolbox(\"Visuals\")")
class FLAXENGINE_API EnvironmentProbe : public Actor
{
    DECLARE_SCENE_OBJECT(EnvironmentProbe);
public:
    /// <summary>
    /// The environment probe update mode.
    /// </summary>
    API_ENUM() enum class ProbeUpdateMode
    {
        // Probe can be updated manually (eg. in Editor or from script).
        Manual = 0,
        // Probe will be automatically updated when is moved.
        WhenMoved = 1,
        // Probe will be automatically updated in real-time (only if in view and frequency depending on distance to the camera).
        Realtime = 2,
    };

private:
    float _radius;
    bool _isUsingCustomProbe;
    int32 _sceneRenderingKey = -1;
    AssetReference<CubeTexture> _probe;
    GPUTexture* _probeTexture = nullptr;

public:
    ~EnvironmentProbe();

public:
    /// <summary>
    /// The reflections texture resolution.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Probe\")")
    ProbeCubemapResolution CubemapResolution = ProbeCubemapResolution::UseGraphicsSettings;

    /// <summary>
    /// The reflections brightness.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), Limit(0, 1000, 0.01f), EditorDisplay(\"Probe\")")
    float Brightness = 1.0f;

    /// <summary>
    /// The probe rendering order. The higher values are render later (on top).
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(25), EditorDisplay(\"Probe\")")
    int32 SortOrder = 0;

    /// <summary>
    /// The probe update mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Probe\")")
    ProbeUpdateMode UpdateMode = ProbeUpdateMode::Manual;

    /// <summary>
    /// The probe capture camera near plane distance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), Limit(0, float.MaxValue, 0.01f), EditorDisplay(\"Probe\")")
    float CaptureNearPlane = 10.0f;

public:
    /// <summary>
    /// Gets the probe radius.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(3000.0f), Limit(0), EditorDisplay(\"Probe\")")
    float GetRadius() const;

    /// <summary>
    /// Sets the probe radius.
    /// </summary>
    API_PROPERTY() void SetRadius(float value);

    /// <summary>
    /// Gets probe scaled radius.
    /// </summary>
    API_PROPERTY() float GetScaledRadius() const;

    /// <summary>
    /// Gets the probe texture used during rendering (baked or custom one).
    /// </summary>
    API_PROPERTY() GPUTexture* GetProbe() const;

    /// <summary>
    /// True if probe is using custom cube texture (not baked).
    /// </summary>
    API_PROPERTY() bool IsUsingCustomProbe() const;

    /// <summary>
    /// Gets the custom probe (null if using baked one or none).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(null), EditorDisplay(\"Probe\")")
    CubeTexture* GetCustomProbe() const;

    /// <summary>
    /// Sets the custom probe (null to disable that feature).
    /// </summary>
    /// <param name="probe">Probe cube texture asset to use</param>
    API_PROPERTY() void SetCustomProbe(CubeTexture* probe);

public:
    /// <summary>
    /// Bakes that probe. It won't be performed now but on async graphics rendering task.
    /// </summary>
    /// <param name="timeout">The timeout in seconds left to bake it (aka startup time).</param>
    API_FUNCTION() void Bake(float timeout = 0);

    /// <summary>
    /// Action fired when probe has been baked. Copies data to the texture memory (GPU-only for real-time probes).
    /// </summary>
    /// <param name="context">The GPU context to use for probe data copying.</param>
    /// <param name="data">The new probe data (GPU texture).</param>
    void SetProbeData(GPUContext* context, GPUTexture* data);

    /// <summary>
    /// Action fired when probe has been baked. Imports data to the texture asset (virtual if running in game).
    /// </summary>
    /// <param name="data">The new probe data.</param>
    void SetProbeData(TextureData& data);

private:
    void UpdateBounds();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif
    void Draw(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void OnLayerChanged() override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool HasContentLoaded() const override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
