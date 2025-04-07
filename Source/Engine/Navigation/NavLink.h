// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"

/// <summary>
/// The off-mesh link objects used to define a custom point-to-point edge within the navigation graph. An off-mesh connection is a user defined traversable connection made up to two vertices, at least one of which resides within a navigation mesh polygon allowing movement outside the navigation mesh.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Navigation/Nav Link\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API NavLink : public Actor
{
    DECLARE_SCENE_OBJECT(NavLink);
public:
    /// <summary>
    /// The start location which transform is representing link start position. It is defined in local-space of the actor.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Nav Link\"), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorOrder(0)")
    Vector3 Start;

    /// <summary>
    /// The end location which transform is representing link end position. It is defined in local-space of the actor.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Nav Link\"), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorOrder(10)")
    Vector3 End;

    /// <summary>
    /// The maximum radius of the agents that can go through the link.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Nav Link\"), DefaultValue(30.0f), EditorOrder(20)")
    float Radius;

    /// <summary>
    /// Flag used to define links that can be traversed in both directions. When set to false the link can only be traversed from start to end.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Nav Link\"), DefaultValue(true), EditorOrder(30)")
    bool BiDirectional;

private:
    void UpdateBounds();

public:
    // [Actor]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnEnable() override;
    void OnDisable() override;

protected:
    // [Actor]
    void OnTransformChanged() override;
};
