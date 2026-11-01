// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"

struct SkeletonNode;

/// <summary>
/// Orientation warping: turns the lower body of a pose toward the actual movement direction about the up axis, while a spine chain turns back so the upper body keeps facing where it did.
/// Used to play a few directional movement animations (eg. forward/back/left/right) at any travel angle.
/// Only the pose is warped: root motion isn't turned, so it suits in-place locomotion driven by the character movement.
/// </summary>
class FLAXENGINE_API OrientationWarp
{
public:
    /// <summary>
    /// The warping setup. Node indices are skeleton node indices, -1 when unused.
    /// </summary>
    struct Settings
    {
        /// <summary>
        /// The lower body node (eg. pelvis or root). It turns about the up axis going through the model origin, together with everything below it.
        /// </summary>
        int32 BodyRoot = -1;

        /// <summary>
        /// The IK feet parent node (eg. ik_foot_root). It turns the same way as the body root, together with the IK feet below it. Use it when the IK feet don't sit below the body root.
        /// </summary>
        int32 IKFootRoot = -1;

        /// <summary>
        /// The first (lowest) node of the spine chain that turns back. Must be an ancestor of SpineEnd (or the same node) below BodyRoot. Otherwise (or when unused) only SpineEnd turns back.
        /// </summary>
        int32 SpineStart = -1;

        /// <summary>
        /// The last (highest) node of the spine chain that turns back (eg. the chest). It ends up with the model space rotation it had in the input pose.
        /// </summary>
        int32 SpineEnd = -1;

        /// <summary>
        /// An extra node that keeps its whole model space transformation from the input pose (eg. ik_hand_root, when the body root is the skeleton root and the hand IK targets sit outside of the spine).
        /// </summary>
        int32 KeepNode = -1;

        /// <summary>
        /// How the spine chain shares the turn back (0-1). At 0 the first spine node turns back all the way and the nodes above follow it. At 1 every spine node turns back an equal share, so the twist spreads evenly up the chain. Values in between blend the two.
        /// </summary>
        float Distribution = 0.75f;

        /// <summary>
        /// The up axis (in model space, normalized) the body turns about.
        /// </summary>
        Float3 UpAxis = Float3::UnitY;
    };

    /// <summary>
    /// Gets how much of the turn back the spine chain node at the given position has once it and the nodes below it in the chain are processed (0-1, the chain's last node always gets 1).
    /// </summary>
    /// <param name="chainIndex">The position of the node in the chain, counted from 0 at the first (lowest) spine node.</param>
    /// <param name="chainLength">The amount of nodes in the chain.</param>
    /// <param name="distribution">The distribution setting (0-1).</param>
    /// <returns>The accumulated share of the turn back at that node.</returns>
    static float GetSpineShare(int32 chainIndex, int32 chainLength, float distribution);

    /// <summary>
    /// Gets how much of the warp applies at the given speed (0-1): none below the minimum speed, then fading in linearly up to full over the next quarter of the minimum speed (eg. 80-100 for a minimum speed of 80), so crossing the minimum speed never snaps the pose. A minimum speed of 0 or less always gives a full warp.
    /// </summary>
    /// <param name="speed">The current movement speed.</param>
    /// <param name="minSpeed">The minimum speed.</param>
    /// <returns>The warp weight.</returns>
    static float GetSpeedWeight(float speed, float minSpeed);

    /// <summary>
    /// Warps the pose in place.
    /// </summary>
    /// <param name="nodes">The skeleton nodes (sorted so parents come before their children).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="settings">The warping setup.</param>
    /// <param name="angle">The angle (in degrees) to turn the lower body by, about the up axis (the same direction as Quaternion::RotationAxis with that axis: with the Y axis, a positive angle turns +X toward -Z).</param>
    /// <param name="pose">The pose node transformations (in local space), modified in place.</param>
    static void Warp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, float angle, Transform* pose);
};
