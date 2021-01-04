// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Joint.h"

/// <summary>
/// Physics joint that maintains a fixed distance and orientation between its two attached bodies.
/// </summary>
/// <seealso cref="Joint" />
API_CLASS() class FLAXENGINE_API FixedJoint : public Joint
{
DECLARE_SCENE_OBJECT(FixedJoint);
protected:

    // [Joint]
    PxJoint* CreateJoint(JointData& data) override;
};
