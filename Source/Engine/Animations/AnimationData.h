// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Animations/Curve.h"
#include "Engine/Core/Math/Transform.h"

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
    void Evaluate(float time, Transform* result, bool loop = true) const
    {
        if (Position.GetKeyframes().HasItems())
        {
            Float3 position;
            Position.Evaluate(position, time, loop);
            result->Translation = position;
        }
        if (Rotation.GetKeyframes().HasItems())
            Rotation.Evaluate(result->Orientation, time, loop);
        if (Scale.GetKeyframes().HasItems())
            Scale.Evaluate(result->Scale, time, loop);
    }

    /// <summary>
    /// Evaluates the animation transformation at the specified time.
    /// </summary>
    /// <param name="time">The time to evaluate the curves at.</param>
    /// <param name="result">The interpolated value from the curve at provided time.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateAll(float time, Transform* result, bool loop = true) const
    {
        Float3 position;
        Position.Evaluate(position, time, loop);
        result->Translation = position;
        Rotation.Evaluate(result->Orientation, time, loop);
        Scale.Evaluate(result->Scale, time, loop);
    }

    /// <summary>
    /// Gets the total amount of keyframes in the animation curves.
    /// </summary>
    int32 GetKeyframesCount() const
    {
        return Position.GetKeyframes().Count() + Rotation.GetKeyframes().Count() + Scale.GetKeyframes().Count();
    }

    uint64 GetMemoryUsage() const
    {
        return NodeName.Length() * sizeof(Char) + Position.GetMemoryUsage() + Rotation.GetMemoryUsage() + Scale.GetMemoryUsage();
    }
};

/// <summary>
/// Skeleton nodes animation data container. Includes metadata about animation sampling, duration and node animations curves.
/// </summary>
struct AnimationData
{
public:
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
    bool EnableRootMotion = false;

    /// <summary>
    /// The custom node name to be used as a root motion source. If not specified the actual root node will be used.
    /// </summary>
    String RootNodeName;

    /// <summary>
    /// The per skeleton node animation channels.
    /// </summary>
    Array<NodeAnimationData> Channels;

public:
    /// <summary>
    /// Gets the length of the animation (in seconds).
    /// </summary>
    FORCE_INLINE float GetLength() const
    {
#if BUILD_DEBUG
        ASSERT(FramesPerSecond != 0);
#endif
        return static_cast<float>(Duration / FramesPerSecond);
    }

    uint64 GetMemoryUsage() const
    {
        uint64 result = RootNodeName.Length() * sizeof(Char) + Channels.Capacity() * sizeof(NodeAnimationData);
        for (const auto& e : Channels)
            result += e.GetMemoryUsage();
        return result;
    }

    /// <summary>
    /// Gets the total amount of keyframes in the all animation channels.
    /// </summary>
    int32 GetKeyframesCount() const
    {
        int32 result = 0;
        for (int32 i = 0; i < Channels.Count(); i++)
            result += Channels[i].GetKeyframesCount();
        return result;
    }

    /// <summary>
    /// Swaps the contents of object with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other object.</param>
    void Swap(AnimationData& other)
    {
        ::Swap(Duration, other.Duration);
        ::Swap(FramesPerSecond, other.FramesPerSecond);
        ::Swap(EnableRootMotion, other.EnableRootMotion);
        ::Swap(RootNodeName, other.RootNodeName);
        Channels.Swap(other.Channels);
    }

    /// <summary>
    /// Releases data.
    /// </summary>
    void Dispose()
    {
        Duration = 0.0;
        FramesPerSecond = 0.0;
        RootNodeName.Clear();
        EnableRootMotion = false;
        Channels.Resize(0);
    }
};
