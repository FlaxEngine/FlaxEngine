// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "NavModifierVolume.h"
#include "NavigationSettings.h"
#include "NavMeshBuilder.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"
#endif

NavModifierVolume::NavModifierVolume(const SpawnParams& params)
    : BoxVolume(params)
{
    _size = 100.0f;
}

NavAreaProperties* NavModifierVolume::GetNavArea() const
{
    auto settings = NavigationSettings::Get();
    for (auto& navArea : settings->NavAreas)
    {
        if (navArea.Name == AreaName)
            return &navArea;
    }
    return nullptr;
}

void NavModifierVolume::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    BoxVolume::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(NavModifierVolume);

    SERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
    SERIALIZE(AreaName);
}

void NavModifierVolume::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    BoxVolume::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(AgentsMask, AgentsMask.Mask);
    DESERIALIZE(AreaName);
}

void NavModifierVolume::OnBoundsChanged(const BoundingBox& prevBounds)
{
#if COMPILE_WITH_NAV_MESH_BUILDER
    // Auto-rebuild modified navmesh area
    if (
        IsDuringPlay() && IsActiveInHierarchy() && HasStaticFlag(StaticFlags::Navigation) &&
        (
            // Build at runtime for dynamic modifiers
            !HasStaticFlag(StaticFlags::Transform)
#if USE_EDITOR
            // Build in editor when using auto-rebuild option
            || (!Editor::IsPlayMode && Editor::Managed->CanAutoBuildNavMesh())
#endif
        ))
    {
        BoundingBox dirtyBounds;
        BoundingBox::Merge(prevBounds, _box, dirtyBounds);
#if USE_EDITOR
        const float timeoutMs = ManagedEditor::ManagedEditorOptions.AutoRebuildNavMeshTimeoutMs;
#else
        const float timeoutMs = 0.0f;
#endif
        NavMeshBuilder::Build(GetScene(), dirtyBounds, timeoutMs);
    }
#endif
}

#if USE_EDITOR

Color NavModifierVolume::GetWiresColor()
{
    return Color::Red;
}

#endif
