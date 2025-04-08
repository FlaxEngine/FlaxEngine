// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actors/BoxVolume.h"
#include "NavigationTypes.h"

/// <summary>
/// A special type of volume that defines the area of the scene in which navigation is restricted (eg. higher traversal cost or dynamic obstacle block).
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Navigation/Nav Modifier Volume\"), ActorToolbox(\"Other\")") class FLAXENGINE_API NavModifierVolume : public BoxVolume
{
    DECLARE_SCENE_OBJECT(NavModifierVolume);
public:
    /// <summary>
    /// The agent types used by this navmesh modifier volume (from navigation settings). Can be used to adjust navmesh for a certain set of agents.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Navigation\"), EditorOrder(10)")
    NavAgentMask AgentsMask;

    /// <summary>
    /// The name of the nav area to apply within the modifiers volume. Nav area properties are picked from the Navigation Settings asset.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Navigation\"), EditorOrder(20)")
    String AreaName;

public:
    /// <summary>
    /// Gets the properties of the nav area used by this volume. Null if missing or invalid area name.
    /// </summary>
    NavAreaProperties* GetNavArea() const;

public:
    // [BoxVolume]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnEnable() override;
    void OnDisable() override;

protected:
    // [BoxVolume]
    void OnBoundsChanged(const BoundingBox& prevBounds) override;
#if USE_EDITOR
    Color GetWiresColor() override;
#endif
};
