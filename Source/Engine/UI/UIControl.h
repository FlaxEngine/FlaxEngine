// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"

/// <summary>
/// Contains a single GUI control (on C# side).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API UIControl : public Actor
{
DECLARE_SCENE_OBJECT(UIControl);
public:

    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:

    // [Actor]
    void OnParentChanged() override;
    void OnTransformChanged() override;
    void OnBeginPlay() override;
    void OnEndPlay() override;
    void OnOrderInParentChanged() override;
    void OnActiveInTreeChanged() override;
};
