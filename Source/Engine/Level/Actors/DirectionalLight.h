// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Light.h"

/// <summary>
/// Directional light emits light from direction in space.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"Lights/Directional Light\"), ActorToolbox(\"Lights\")")
class FLAXENGINE_API DirectionalLight : public LightWithShadow
{
    DECLARE_SCENE_OBJECT(DirectionalLight);
public:
    /// <summary>
    /// The number of cascades used for slicing the range of depth covered by the light during shadow rendering. Values are 1, 2 or 4 cascades; a typical scene uses 4 cascades.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(65), DefaultValue(4), Limit(1, 4), EditorDisplay(\"Shadow\")")
    int32 CascadeCount = 4;

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
