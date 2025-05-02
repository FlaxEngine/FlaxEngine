// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"

/// <summary>
/// Physics joint that maintains a fixed distance and orientation between its two attached bodies.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Joints/Fixed Joint\"), ActorToolbox(\"Physics\")")
class FLAXENGINE_API FixedJoint : public Joint
{
    DECLARE_SCENE_OBJECT(FixedJoint);
public:
    // [Joint]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif

protected:
    // [Joint]
    void* CreateJoint(const PhysicsJointDesc& desc) override;
};
