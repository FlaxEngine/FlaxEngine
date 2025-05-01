// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Light.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Sky light captures the distant parts of the scene and applies it as a light. Allows to add ambient light.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Lights/Sky Light\"), ActorToolbox(\"Lights\")")
class FLAXENGINE_API SkyLight : public Light
{
    DECLARE_SCENE_OBJECT(SkyLight);
public:
    /// <summary>
    /// Sky light source mode.
    /// </summary>
    API_ENUM() enum class Modes
    {
        /// <summary>
        /// The captured scene will be used as a light source.
        /// </summary>
        CaptureScene = 0,

        /// <summary>
        /// The custom cube texture will be used as a light source.
        /// </summary>
        CustomTexture = 1,
    };

private:
    AssetReference<CubeTexture> _bakedProbe;
    float _radius;

public:
    /// <summary>
    /// Additional color to add. Source texture colors are summed with it. Can be used to apply custom ambient color.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(21), DefaultValue(typeof(Color), \"0,0,0,1\"), EditorDisplay(\"Light\")")
    ::Color AdditiveColor = Color::Black;

    /// <summary>
    /// Distance from the light at which any geometry should be treated as part of the sky.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(45), DefaultValue(150000.0f), Limit(0), EditorDisplay(\"Probe\")")
    float SkyDistanceThreshold = 150000.0f;

    /// <summary>
    /// The current light source mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(Modes.CustomTexture), EditorDisplay(\"Probe\")")
    Modes Mode = Modes::CustomTexture;

    /// <summary>
    /// The custom texture.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(null), EditorDisplay(\"Probe\")")
    AssetReference<CubeTexture> CustomTexture;

public:
    /// <summary>
    /// Gets the radius.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(29), DefaultValue(1000000.0f), Limit(0), EditorDisplay(\"Light\")")
    FORCE_INLINE float GetRadius() const
    {
        return _radius;
    }

    /// <summary>
    /// Sets the radius.
    /// </summary>
    API_PROPERTY() void SetRadius(float value);

    /// <summary>
    /// Gets the scaled radius of the sky light.
    /// </summary>
    float GetScaledRadius() const;

    /// <summary>
    /// Gets the light source texture.
    /// </summary>
    CubeTexture* GetSource() const;

public:
    /// <summary>
    /// Bakes that probe.
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
    // [Light]
    void Draw(RenderContext& renderContext) override;
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool HasContentLoaded() const override;

protected:
    // [Light]
    void OnTransformChanged() override;
};
