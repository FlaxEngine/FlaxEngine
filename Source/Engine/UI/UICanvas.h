// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"

/// <summary>
/// Root of the UI structure. Renders GUI and handles input events forwarding.
/// </summary>
API_CLASS(Sealed, NoConstructor, Attributes="ActorContextMenu(\"New/UI/UI Canvas\"), ActorToolbox(\"GUI\")")
class FLAXENGINE_API UICanvas : public Actor
{
    DECLARE_SCENE_OBJECT(UICanvas);
public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void OnBeginPlay() override;
    void OnEndPlay() override;
    void OnParentChanged() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() final override;
#if USE_EDITOR
    void OnActiveInTreeChanged() override;
#endif
};
