// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actors/BoxVolume.h"
#include "NavigationTypes.h"

/// <summary>
/// A special type of volume that defines the area of the scene in which navigation meshes are generated.
/// </summary>
API_CLASS() class FLAXENGINE_API NavMeshBoundsVolume : public BoxVolume
{
DECLARE_SCENE_OBJECT(NavMeshBoundsVolume);
public:

    /// <summary>
    /// The agent types used by this navmesh bounds volume (from navigation settings). Can be used to generate navmesh for a certain set of agents.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"Box Volume\"), EditorOrder(10)")
    NavAgentMask AgentsMask;

public:

    // [BoxVolume]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:

    // [BoxVolume]
    void OnEnable() override;
    void OnDisable() override;
#if USE_EDITOR
    void OnBoundsChanged(const BoundingBox& prevBounds) override;
    Color GetWiresColor() override;
#endif
};
