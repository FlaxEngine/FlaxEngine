// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

class Spline;

/// <summary>
/// Physical simulation actor for ropes, chains and cables represented by a spline.
/// </summary>
/// <seealso cref="Spline" />
API_CLASS() class FLAXENGINE_API SplineRopeBody : public Actor
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCENE_OBJECT(SplineRopeBody);
private:
    struct Mass
    {
        Vector3 Position;
        Real SegmentLength;
        Vector3 PrevPosition;
        bool Unconstrained;
    };

    Spline* _spline = nullptr;
    Transform _splineTransform;
    float _time = 0.0f;
    Array<Mass> _masses;

public:
    /// <summary>
    /// The target actor to attach the rope end to. If unset the rope end will run freely.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), DefaultValue(null), EditorDisplay(\"Rope\")")
    ScriptingObjectReference<Actor> AttachEnd;

    /// <summary>
    /// The world gravity scale. Can be used to adjust gravity force or disable it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Rope\")")
    float GravityScale = 1.0f;

    /// <summary>
    /// The scale of velocity applied to rope. The greater the value, the more suppressed the movement.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(15), EditorDisplay(\"Rope\"), Limit(0, 0.99f, 0.001f)")
    float Drag = 0.1f;

    /// <summary>
    /// The maximum bones stretch scale. The value of 0 means no stretch, 1 - can stretch up to double length.
    /// </summary>
    API_FIELD(Attributes = "EditorOrder(19), EditorDisplay(\"Rope\"), Limit(0)")
    float StretchLimit = 0.0f;

    /// <summary>
    /// The additional, external force applied to rope (world-space). This can be eg. wind force.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Rope\")")
    Vector3 AdditionalForce = Vector3::Zero;

    /// <summary>
    /// If checked, the physics solver will use stiffness constraint for rope. It will be less likely to bend over and will preserve more it's shape.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Rope\")")
    bool EnableStiffness = false;

    /// <summary>
    /// The rope simulation update substep (in seconds). Defines the frequency of physics update.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), Limit(0, 0.1f, 0.0001f), EditorDisplay(\"Rope\")")
    float SubstepTime = 0.02f;

private:
    void Tick();

public:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
    void OnParentChanged() override;
};
