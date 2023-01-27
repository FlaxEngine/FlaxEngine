// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Light.h"

/// <summary>
/// Directional light emits light from direction in space.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Lights/Directional Light\"), ActorToolbox(\"Lights\")")
class FLAXENGINE_API DirectionalLight : public LightWithShadow
{
    DECLARE_SCENE_OBJECT(DirectionalLight);
public:
    /// <summary>
    /// The number of cascades used for slicing the range of depth covered by the light. Values are 1, 2 or 4 cascades; a typical scene uses 4 cascades.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(65), DefaultValue(4), Limit(1, 4), EditorDisplay(\"Shadow\")")
    int32 CascadeCount = 4;

    /// <summary>
    /// Delayed shadows only works when you have one window open, if you have the editor and game window open at the same time it will not work. For best result set shadow distance to 8000+ and change resolution to 2048.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(66), DefaultValue(false), EditorDisplay(\"Shadow\")")
    bool DelayedShadows = false;

public:
    // [LightWithShadow]
    void Draw(RenderContext& renderContext) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [LightWithShadow]
    void OnTransformChanged() override;
};
