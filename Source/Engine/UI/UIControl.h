// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

/// <summary>
/// Contains a single GUI control (on C# side).
/// </summary>
API_CLASS(Sealed, Attributes="ActorContextMenu(\"New/UI/UI Control\"), ActorToolbox(\"GUI\", \"Empty UIControl\")")
class FLAXENGINE_API UIControl : public Actor
{
    DECLARE_SCENE_OBJECT(UIControl);
private:
    ScriptingObjectReference<UIControl> _navTargetUp, _navTargetDown, _navTargetLeft, _navTargetRight;

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
    void OnActiveChanged() override;

private:
#if !COMPILE_WITHOUT_CSHARP
    // Internal bindings
    API_FUNCTION(NoProxy) void GetNavTargets(API_PARAM(Out) UIControl*& up, API_PARAM(Out) UIControl*& down, API_PARAM(Out) UIControl*& left, API_PARAM(Out) UIControl*& right) const;
    API_FUNCTION(NoProxy) void SetNavTargets(UIControl* up, UIControl* down, UIControl* left, UIControl* right);
#endif
};
