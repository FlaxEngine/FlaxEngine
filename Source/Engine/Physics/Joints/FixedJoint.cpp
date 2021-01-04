// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "FixedJoint.h"
#include "Engine/Physics/Utilities.h"
#include <ThirdParty/PhysX/extensions/PxFixedJoint.h>

FixedJoint::FixedJoint(const SpawnParams& params)
    : Joint(params)
{
}

PxJoint* FixedJoint::CreateJoint(JointData& data)
{
    const PxTransform trans0(C2P(data.Pos0), C2P(data.Rot0));
    const PxTransform trans1(C2P(data.Pos1), C2P(data.Rot1));
    return PxFixedJointCreate(*data.Physics, data.Actor0, trans0, data.Actor1, trans1);
}
