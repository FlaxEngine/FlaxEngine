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
    auto wcaa = GetWorldConstrainActorA();
    auto wcab = GetWorldConstrainActorB();
    DEBUG_DRAW_LINE(wcaa.Translation, wcab.Translation, Color::BlueViolet * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void* FixedJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    return PhysicsBackend::CreateFixedJoint(desc);
}
