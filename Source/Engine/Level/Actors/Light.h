// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// Base class for all light types.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API Light : public Actor
{
    DECLARE_SCENE_OBJECT_ABSTRACT(Light);
protected:
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// Color of the light
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Light\")")
    Color Color = Color::White;

    /// <summary>
    /// Brightness of the light
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Light\"), Limit(0.0f, 100000000.0f, 0.1f)")
    float Brightness = 3.14f;

    /// <summary>
    /// Controls light visibility range. The distance at which the light becomes completely faded (blend happens on the last 10% of that range). Use a value of 0 to always draw light.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(35), Limit(0, float.MaxValue, 10.0f), EditorDisplay(\"Light\")")
    float ViewDistance = 0.0f;

    /// <summary>
    /// Controls how much this light will contribute indirect lighting. When set to 0, there is no GI from the light. The default value is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(1.0f), Limit(0, 100, 0.1f), EditorDisplay(\"Light\", \"Indirect Lighting Intensity\")")
    float IndirectLightingIntensity = 1.0f;

    /// <summary>
    /// Controls how much this light will contribute to the Volumetric Fog. When set to 0, there is no contribution.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110),DefaultValue(1.0f),  Limit(0, 100, 0.01f), EditorDisplay(\"Volumetric Fog\", \"Scattering Intensity\")")
    float VolumetricScatteringIntensity = 1.0f;

    /// <summary>
    /// Toggles whether or not to cast a volumetric shadow for lights contributing to Volumetric Fog. Also shadows casting by this light should be enabled in order to make it cast volumetric fog shadow.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(120), EditorDisplay(\"Volumetric Fog\", \"Cast Shadow\")")
    bool CastVolumetricShadow = true;

protected:
    // Adjust the light brightness used during rendering (called by light types inside SetupLightData callback)
    void AdjustBrightness(const RenderView& view, float& brightness) const;

    FORCE_INLINE bool CheckViewDistance(const Float3& viewPosition, const Float3& viewOrigin, Float3& position, float& brightness) const
    {
        position = _transform.Translation - viewOrigin;
        if (ViewDistance > ZeroTolerance)
        {
            const float dst2 = (float)Vector3::DistanceSquared(viewPosition, position);
            const float dst = Math::Sqrt(dst2);
            brightness *= Math::Remap(dst, 0.9f * ViewDistance, ViewDistance, 1.0f, 0.0f);
            return dst < ViewDistance;
        }
        return true;
    }

public:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }

    virtual void DrawLightsDebug(RenderView& view);
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};

/// <summary>
/// Base class for all light types that can cast dynamic or static shadow. Contains more shared properties for point/spot/directional lights.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API LightWithShadow : public Light
{
    DECLARE_SCENE_OBJECT_ABSTRACT(LightWithShadow);

    /// <summary>
    /// List of fixed resolutions for light shadow map.
    /// </summary>
    API_ENUM() enum class ShadowMapResolution
    {
        // Use automatic dynamic resolution based on distance to view.
        Dynamic = 0,
        // Shadow map of size 128x128.
        _128 = 128,
        // Shadow map of size 256x256.
        _256 = 256,
        // Shadow map of size 512x512.
        _512 = 512,
        // Shadow map of size 1024x1024.
        _1024 = 1024,
        // Shadow map of size 2048x2048.
        _2048 = 2048,
    };

protected:
    uint32 _invalidateShadowFrame = 0;

public:
    /// <summary>
    /// The minimum roughness value used to clamp material surface roughness during shading pixel.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Light\", \"Minimum Roughness\"), Limit(0.0f, 1.0f, 0.01f)")
    float MinRoughness = 0.04f;

    /// <summary>
    /// Shadows casting distance from view.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(80), EditorDisplay(\"Shadow\", \"Distance\"), Limit(0, 1000000)")
    float ShadowsDistance = 5000.0f;

    /// <summary>
    /// Shadows fade off distance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(90), EditorDisplay(\"Shadow\", \"Fade Distance\"), Limit(0.0f, 10000.0f, 0.1f)")
    float ShadowsFadeDistance = 500.0f;

    /// <summary>
    /// TheShadows edges sharpness.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(70), EditorDisplay(\"Shadow\", \"Sharpness\"), Limit(1.0f, 10.0f, 0.001f)")
    float ShadowsSharpness = 1.0f;

    /// <summary>
    /// Dynamic shadows blending strength. Default is 1 for fully opaque shadows, value 0 disables shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(75), EditorDisplay(\"Shadow\", \"Strength\"), Limit(0.0f, 1.0f, 0.001f)")
    float ShadowsStrength = 1.0f;

    /// <summary>
    /// The depth bias used for shadow map comparison.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(95), EditorDisplay(\"Shadow\", \"Depth Bias\"), Limit(0.0f, 10.0f, 0.0001f)")
    float ShadowsDepthBias = 0.005f;

    /// <summary>
    /// A factor specifying the offset to add to the calculated shadow map depth with respect to the surface normal.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(96), EditorDisplay(\"Shadow\", \"Normal Offset Scale\"), Limit(0.0f, 100.0f, 0.1f)")
    float ShadowsNormalOffsetScale = 10.0f;

    /// <summary>
    /// The length of the rays for contact shadows computed via the screen-space tracing. Set this to values higher than 0 to enable screen-space shadows rendering for this light. This improves the shadowing details. Actual ray distance is based on the pixel distance from the camera.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(99), EditorDisplay(\"Shadow\"), Limit(0.0f, 0.1f, 0.001f)")
    float ContactShadowsLength = 0.0f;

    /// <summary>
    /// Frequency of shadow updates. 1 - every frame, 0.5 - every second frame, 0 - on start or change. It's the inverse value of how many frames should happen in-between shadow map updates (eg. inverse of 0.5 is 2 thus shadow will update every 2nd frame).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Shadow\", \"Update Rate\"), Limit(0.0f, 1.0f)")
    float ShadowsUpdateRate = 1.0f;

    /// <summary>
    /// Frequency of shadow updates at the maximum distance from the view at which shadows are still rendered. This value is multiplied by ShadowsUpdateRate and allows scaling the update rate in-between the shadow range. For example, if light is near view, it will get normal shadow updates but will reduce this rate when far from view. See ShadowsUpdateRate to learn more.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(101), EditorDisplay(\"Shadow\", \"Update Rate At Distance\"), Limit(0.0f, 1.0f)")
    float ShadowsUpdateRateAtDistance = 1.0f;

    /// <summary>
    /// Defines the resolution of the shadow map texture used to draw objects projection from light-point-of-view. Higher values increase shadow quality at cost of performance.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(105), EditorDisplay(\"Shadow\", \"Resolution\")")
    ShadowMapResolution ShadowsResolution = ShadowMapResolution::Dynamic;

    /// <summary>
    /// Describes how a visual element casts shadows.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), EditorDisplay(\"Shadow\", \"Mode\")")
    ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// Marks the light shadow to be refreshes during next drawing. Invalidates any cached shadow map and redraws static shadows of the object (if any in use).
    /// </summary>
    API_FUNCTION() void InvalidateShadow();

public:
    // [Light]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
