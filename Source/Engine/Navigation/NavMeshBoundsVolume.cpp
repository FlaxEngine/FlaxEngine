// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavMeshBoundsVolume.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#include "Navigation.h"
#endif

NavMeshBoundsVolume::NavMeshBoundsVolume(const SpawnParams& params)
    : BoxVolume(params)
{
}

void NavMeshBoundsVolume::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    BoxVolume::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(NavMeshBoundsVolume);

    SERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
}

void NavMeshBoundsVolume::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    BoxVolume::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
}

void NavMeshBoundsVolume::OnEnable()
{
    GetScene()->Navigation.Volumes.Add(this);

    // Base
    Actor::OnEnable();
}

void NavMeshBoundsVolume::OnDisable()
{
    GetScene()->Navigation.Volumes.Remove(this);

    // Base
    Actor::OnDisable();
}

#if USE_EDITOR

void NavMeshBoundsVolume::OnBoundsChanged(const BoundingBox& prevBounds)
{
    // Auto-rebuild modified navmesh area
    if (IsDuringPlay() && IsActiveInHierarchy() && !Editor::IsPlayMode && Editor::Managed->CanAutoBuildNavMesh())
    {
        if (_box.Intersects(prevBounds))
        {
            // Bounds were moved a bit so merge into a single request (for performance reasons)
            BoundingBox dirtyBounds;
            BoundingBox::Merge(prevBounds, _box, dirtyBounds);
            Navigation::BuildNavMesh(dirtyBounds, ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs);
        }
        else
        {
            // Dirty each bounds in separate
            Navigation::BuildNavMesh(prevBounds, ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs);
            Navigation::BuildNavMesh(_box, ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs);
        }
    }
}

void NavMeshBoundsVolume::OnActiveInTreeChanged()
{
    BoxVolume::OnActiveInTreeChanged();

    // Auto-rebuild
    if (IsDuringPlay() && !Editor::IsPlayMode && Editor::Managed->CanAutoBuildNavMesh())
    {
        Navigation::BuildNavMesh(_box, ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs);
    }
}

Color NavMeshBoundsVolume::GetWiresColor()
{
    return Color::Green;
}

#endif
