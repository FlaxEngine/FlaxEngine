// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Animations/Curve.h"

/// <summary>
/// Single node animation data container.
/// </summary>
struct NodeAnimationData
{
public:
    /// <summary>
    /// The target node name.
    /// </summary>
    String NodeName;

    /// <summary>
    /// The position channel animation.
    /// </summary>
    LinearCurve<Float3> Position;

    /// <summary>
    /// The rotation channel animation.
    /// </summary>
    LinearCurve<Quaternion> Rotation;

    /// <summary>
    /// The scale channel animation.
    /// </summary>
    LinearCurve<Float3> Scale;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="NodeAnimationData"/> class.
    /// </summary>
    NodeAnimationData()
        : Position(Float3::Zero)
        , Rotation(Quaternion::Identity)
        , Scale(Float3::One)
    {
    }

public:
    /// <summary>
    /// Evaluates the animation transformation at the specified time (only for the curves with non-empty data).
    /// </summary>
    /// <param name="time">The time to evaluate the curves at.</param>
    /// <param name="result">The interpolated value from the curve at provided time.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void Evaluate(float time, Transform* result, bool loop = true) const;

    /// <summary>
    /// Evaluates the animation transformation at the specified time.
    /// </summary>
    /// <param name="time">The time to evaluate the curves at.</param>
    /// <param name="result">The interpolated value from the curve at provided time.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateAll(float time, Transform* result, bool loop = true) const;

    /// <summary>
    /// Gets the total amount of keyframes in the animation curves.
    /// </summary>
    int32 GetKeyframesCount() const;

    uint64 GetMemoryUsage() const;
};

/// <summary>
/// Single track with events.
/// </summary>
struct EventAnimationData
{
    float Duration = 0.0f;
    StringAnsi TypeName;
    StringAnsi JsonData;
};

/// <summary>
/// Root Motion modes that can be applied by the animation. Used as flags for selective behavior.
/// </summary>
API_ENUM(Attributes="Flags") enum class AnimationRootMotionFlags : byte
{
    // No root motion.
    None = 0,
    // Root node position along XZ plane. Applies horizontal movement. Good for stationary animations (eg. idle).
    RootPositionXZ = 1 << 0,
    // Root node position along Y axis (up). Applies vertical movement. Good for all 'grounded' animations unless jumping is handled from code.
    RootPositionY = 1 << 1,
    // Root node rotation. Applies orientation changes. Good for animations that have baked-in root rotation (eg. turn animations).
    RootRotation = 1 << 2,
    // Root node position.
    RootPosition = RootPositionXZ | RootPositionY,
    // Root node position and rotation.
    RootTransform = RootPosition | RootRotation,
};

DECLARE_ENUM_OPERATORS(AnimationRootMotionFlags);

/// <summary>
/// Skeleton nodes animation data container. Includes metadata about animation sampling, duration and node animations curves.
/// </summary>
struct AnimationData
{
    /// <summary>
    /// The duration of the animation (in frames).
    /// </summary>
    double Duration = 0.0;

    /// <summary>
    /// The amount of the animation frames per second.
    /// </summary>
    double FramesPerSecond = 0.0;

    /// <summary>
    /// Enables root motion extraction support from this animation.
    /// </summary>
    AnimationRootMotionFlags RootMotionFlags = AnimationRootMotionFlags::None;

    /// <summary>
    /// The animation name.
    /// </summary>
    String Name;

    /// <summary>
    /// The custom node name to be used as a root motion source. If not specified the actual root node will be used.
    /// </summary>
    String RootNodeName;

    /// <summary>
    /// The per-skeleton node animation channels.
    /// </summary>
    Array<NodeAnimationData> Channels;

    /// <summary>
    /// The animation event tracks.
    /// </summary>
    Array<Pair<String, StepCurve<EventAnimationData>>> Events;

public:
    /// <summary>
    /// Gets the length of the animation (in seconds).
    /// </summary>
    FORCE_INLINE float GetLength() const
    {
#if BUILD_DEBUG
        ASSERT(FramesPerSecond > ZeroTolerance);
#endif
        return static_cast<float>(Duration / FramesPerSecond);
    }

    uint64 GetMemoryUsage() const;

    /// <summary>
    /// Gets the total amount of keyframes in the all animation channels.
    /// </summary>
    int32 GetKeyframesCount() const;

    NodeAnimationData* GetChannel(const StringView& name);

    /// <summary>
    /// Swaps the contents of object with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other object.</param>
    void Swap(AnimationData& other);

    /// <summary>
    /// Releases data.
    /// </summary>
    void Release();
};
