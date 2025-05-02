// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"

/// <summary>
/// The empty actor that is useful to create hierarchy and/or hold scripts. See <see cref="Script"/>.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Actor\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API EmptyActor : public Actor
{
    DECLARE_SCENE_OBJECT(EmptyActor);
public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override;
#endif

protected:
    // [Actor]
    void OnTransformChanged() override;
};
