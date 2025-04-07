// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Transform.h"

/// <summary>
/// The Inverse Kinematics (IK) utility library.
/// </summary>
class FLAXENGINE_API InverseKinematics
{
public:
    /// <summary>
    /// Rotates a node so it aims at a target. Solves the transformation (rotation) that needs to be applied to the node such that a provided forward vector (in node local space) aims at the target position (in skeleton model space).
    /// </summary>
    /// <param name="node">The node transformation (in model space).</param>
    /// <param name="target">The target position to aim at (in model space).</param>
    /// <param name="outNodeCorrection">The calculated output node correction (in model- space). It needs to be multiplied with node model space quaternion.</param>
    static void SolveAimIK(const Transform& node, const Vector3& target, Quaternion& outNodeCorrection);

    /// <summary>
    /// Performs inverse kinematic on a three nodes chain (must be ancestors).
    /// </summary>
    /// <param name="rootNode">The start node transformation (in model space).</param>
    /// <param name="jointNode">The middle node transformation (in model space).</param>
    /// <param name="targetNode">The end node transformation (in model space).</param>
    /// <param name="target">The target position of the end node to reach (in model space).</param>
    /// <param name="jointTarget">The target position of the middle node to face into (in model space).</param>
    /// <param name="allowStretching">True if allow bones stretching, otherwise bone lengths will be preserved when trying to reach the target.</param>
    /// <param name="maxStretchScale">The maximum scale when stretching bones. Used only if allowStretching is true.</param>
    static void SolveTwoBoneIK(Transform& rootNode, Transform& jointNode, Transform& targetNode, const Vector3& target, const Vector3& jointTarget, bool allowStretching = false, float maxStretchScale = 1.5f);
};
