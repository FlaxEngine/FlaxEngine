// Copyright (c) Wojciech Figat. All rights reserved.

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
    DEBUG_DRAW_LINE(GetPosition(), GetTargetPosition(), Color::BlueViolet * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void* FixedJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    return PhysicsBackend::CreateFixedJoint(desc);
}
