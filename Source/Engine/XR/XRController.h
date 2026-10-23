// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "XR.h"

/// <summary>
/// The actor that follows the pose of the XR controller. Add the controller model and the objects held in hand as its children.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Other/XR Controller\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API XRController : public Actor
{
    DECLARE_SCENE_OBJECT(XRController);
private:
    bool _isTracked = false;

public:
    /// <summary>
    /// The hand of the controller to follow.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"XR Controller\"), DefaultValue(XRHand.Right), EditorOrder(0)")
    XRHand Hand;

    /// <summary>
    /// The pose of the controller to follow. Use Grip to hold objects and Aim to point at objects.
    /// </summary>
    API_FIELD(Attributes="EditorDisplay(\"XR Controller\"), DefaultValue(XRPoseType.Grip), EditorOrder(10)")
    XRPoseType PoseType;

public:
    /// <summary>
    /// Gets a value indicating whether the controller is tracked. The actor keeps the last tracked transformation when the controller is not tracked.
    /// </summary>
    API_PROPERTY() bool IsTracked() const
    {
        return _isTracked;
    }

    /// <summary>
    /// Updates the transformation of the enabled controllers to follow the current poses.
    /// </summary>
    static void UpdateAll();

public:
    // [Actor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnEnable() override;
    void OnDisable() override;
};
