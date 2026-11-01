// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"

struct SkeletonNode;

/// <summary>
/// Stride warping: lengthens or shortens the steps of a movement pose so a walk or run cycle can match the actual movement speed with little or no play rate change.
/// Each IK foot moves along the travel direction, away from or toward the pelvis, by the stride scale. The pelvis lowers so the legs can still reach, and each leg (thigh, calf, foot) is solved onto its moved foot target with two bone IK.
/// The up axis is the model Y axis.
/// </summary>
class FLAXENGINE_API StrideWarp
{
public:
    /// <summary>
    /// A leg of the warp. Node indices are skeleton node indices, -1 when unused.
    /// </summary>
    struct Leg
    {
        /// <summary>
        /// The IK foot node (eg. ik_foot_l). Its position sets the stride and it moves to the warped foot position. When unused, the foot itself sets the stride.
        /// </summary>
        int32 IKFoot = -1;

        /// <summary>
        /// The foot node (eg. foot_l). Its parent is the leg's joint (calf).
        /// </summary>
        int32 Foot = -1;

        /// <summary>
        /// The thigh node (eg. thigh_l). Must be the parent of the foot's parent, otherwise the leg isn't solved.
        /// </summary>
        int32 Thigh = -1;
    };

    /// <summary>
    /// The warping setup.
    /// </summary>
    struct Settings
    {
        /// <summary>
        /// The pelvis node. The stride is measured from it along the travel direction, and it lowers so the legs can reach (only when it is an ancestor of the thighs). Required.
        /// </summary>
        int32 Pelvis = -1;

        /// <summary>
        /// The legs to warp (left and right).
        /// </summary>
        Leg Legs[2];

        /// <summary>
        /// The most the pelvis may lower (in model units). 0 or less keeps the pelvis where it is, and legs that can't reach their targets stay short of them.
        /// </summary>
        float MaxPelvisDrop = 50.0f;
    };

    /// <summary>
    /// Gets how far a foot moves for a stride scale: its offset from the pelvis along the travel direction (both on the ground plane) times the stride scale minus one.
    /// </summary>
    /// <param name="footPosition">The foot position (in model space).</param>
    /// <param name="pelvisPosition">The pelvis position (in model space).</param>
    /// <param name="direction">The travel direction on the ground plane (in model space, normalized).</param>
    /// <param name="strideScale">The stride scale.</param>
    /// <returns>The foot translation (in model space).</returns>
    static Vector3 GetFootOffset(const Vector3& footPosition, const Vector3& pelvisPosition, const Vector3& direction, float strideScale);

    /// <summary>
    /// Checks if the setup can warp anything: the pelvis is set and at least one leg is a valid thigh, calf and foot chain.
    /// </summary>
    /// <param name="nodes">The skeleton nodes.</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="settings">The warping setup.</param>
    /// <returns>True if Warp can change the pose.</returns>
    static bool CanWarp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings);

    /// <summary>
    /// Gets how much the pelvis lowers for a stride scale so no leg reaches further from its thigh than it does in the input pose, limited by the settings' maximum pelvis drop.
    /// </summary>
    /// <param name="nodes">The skeleton nodes (sorted so parents come before their children).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="settings">The warping setup.</param>
    /// <param name="direction">The travel direction (in model space). Only its part on the ground plane is used.</param>
    /// <param name="strideScale">The stride scale.</param>
    /// <param name="pose">The pose node transformations (in local space).</param>
    /// <returns>The pelvis drop (in model units, 0 or more).</returns>
    static float GetPelvisDrop(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, const Float3& direction, float strideScale, const Transform* pose);

    /// <summary>
    /// Moves a value toward a target, covering the given part of the remaining distance per second (frame rate independent). An interpolation speed of 0 or less returns the target.
    /// </summary>
    /// <param name="current">The current value.</param>
    /// <param name="target">The target value.</param>
    /// <param name="speed">The interpolation speed (eg. 10 covers about 63% of the remaining distance in 0.1s).</param>
    /// <param name="deltaTime">The time step (in seconds).</param>
    /// <returns>The eased value.</returns>
    static float Ease(float current, float target, float speed, float deltaTime);

    /// <summary>
    /// Warps the pose in place.
    /// </summary>
    /// <param name="nodes">The skeleton nodes (sorted so parents come before their children).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="settings">The warping setup.</param>
    /// <param name="direction">The travel direction (in model space). Only its part on the ground plane is used.</param>
    /// <param name="strideScale">The stride scale (1 keeps the stride, 1.5 makes each foot reach 1.5 times as far ahead of or behind the pelvis).</param>
    /// <param name="pelvisDrop">How much to lower the pelvis (in model units), eg. from GetPelvisDrop.</param>
    /// <param name="pose">The pose node transformations (in local space), modified in place.</param>
    static void Warp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, const Float3& direction, float strideScale, float pelvisDrop, Transform* pose);
};
