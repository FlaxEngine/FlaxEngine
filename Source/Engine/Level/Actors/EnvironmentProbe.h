// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Environment Probe can capture space around the objects to provide reflections.
/// </summary>
API_CLASS() class FLAXENGINE_API EnvironmentProbe : public Actor
{
    DECLARE_SCENE_OBJECT(EnvironmentProbe);
private:
    float _radius;
    bool _isUsingCustomProbe;
    int32 _sceneRenderingKey = -1;
    AssetReference<CubeTexture> _probe;

public:
    /// <summary>
    /// The reflections brightness.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(1.0f), Limit(0, 1000, 0.01f), EditorDisplay(\"Probe\")")
    float Brightness = 1.0f;

    /// <summary>
    /// Value indicating if probe should be updated automatically on change.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(false), EditorDisplay(\"Probe\")")
    bool AutoUpdate = false;

    /// <summary>
    /// The probe capture camera near plane distance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(10.0f), Limit(0, float.MaxValue, 0.01f), EditorDisplay(\"Probe\")")
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
    /// Returns true if env probe has cube texture assigned.
    /// </summary>
    API_PROPERTY() bool HasProbe() const
    {
        return _probe != nullptr;
    }

    /// <summary>
    /// Returns true if env probe has cube texture assigned.
    /// </summary>
    API_PROPERTY() bool HasProbeLoaded() const
    {
        return _probe != nullptr && _probe->IsLoaded();
    }

    /// <summary>
    /// Gets the probe texture used during rendering (baked or custom one).
    /// </summary>
    API_PROPERTY() CubeTexture* GetProbe() const
    {
        return _probe;
    }

    /// <summary>
    /// True if probe is using custom cube texture (not baked).
    /// </summary>
    API_PROPERTY() bool IsUsingCustomProbe() const
    {
        return _isUsingCustomProbe;
    }

    /// <summary>
    /// Setup probe data structure
    /// </summary>
    /// <param name="data">Rendering context</param>
    /// <param name="data">Packed probe data to set</param>
    void SetupProbeData(const RenderContext& renderContext, struct ProbeData* data) const;

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
    /// Action fired when probe has been baked.
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
