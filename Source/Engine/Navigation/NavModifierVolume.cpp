// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NavModifierVolume.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#include "NavMeshBuilder.h"
#endif

NavModifierVolume::NavModifierVolume(const SpawnParams& params)
    : BoxVolume(params)
{
    _size = 100.0f;
}

void NavModifierVolume::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    BoxVolume::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(NavModifierVolume);

    SERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
}

void NavModifierVolume::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    BoxVolume::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
}

#if USE_EDITOR

void NavModifierVolume::OnBoundsChanged(const BoundingBox& prevBounds)
{
    // Auto-rebuild modified navmesh area
    if (IsDuringPlay() && IsActiveInHierarchy() && !Editor::IsPlayMode && Editor::Managed->CanAutoBuildNavMesh())
    {
        BoundingBox dirtyBounds;
        BoundingBox::Merge(prevBounds, _box, dirtyBounds);
        NavMeshBuilder::Build(GetScene(), dirtyBounds, ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs);
    }
}

Color NavModifierVolume::GetWiresColor()
{
    return Color::Red;
}

#endif
