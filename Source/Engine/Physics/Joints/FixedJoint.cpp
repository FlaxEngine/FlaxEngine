// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "FixedJoint.h"
#include "Engine/Physics/PhysicsBackend.h"

FixedJoint::FixedJoint(const SpawnParams& params)
    : Joint(params)
{
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void FixedJoint::OnDebugDrawSelected()
{
    auto p0 = GetPoseActor0();
    auto p1 = GetPoseActor1();
    DEBUG_DRAW_LINE(p0.Translation, p1.Translation, Color::BlueViolet * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void* FixedJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    return PhysicsBackend::CreateFixedJoint(desc);
}
