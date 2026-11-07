// Copyright (c) Wojciech Figat. All rights reserved.

#include "OrientationWarp.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Models/SkeletonData.h"

namespace
{
    enum class TargetMode : uint8
    {
        None,
        Rotation,
        Transform,
    };

    bool IsValid(int32 index, int32 nodesCount)
    {
        return index >= 0 && index < nodesCount;
    }

    Transform TurnAboutOrigin(const Float3& axis, float angleDegrees, const Transform& modelTransform)
    {
        Quaternion turn;
        Quaternion::RotationAxis(axis, angleDegrees * DegreesToRadians, turn);
        Transform result = Transform(Vector3::Zero, turn).LocalToWorld(modelTransform);
        result.Orientation.Normalize();
        return result;
    }
}

float OrientationWarp::GetSpineShare(int32 chainIndex, int32 chainLength, float distribution)
{
    if (chainLength <= 0)
        return 1.0f;
    const float even = (float)(chainIndex + 1) / (float)chainLength;
    return Math::Lerp(1.0f, even, Math::Saturate(distribution));
}

float OrientationWarp::GetSpeedWeight(float speed, float minSpeed)
{
    if (minSpeed <= 0.0f)
        return 1.0f;
    return Math::Saturate((speed - minSpeed) / (minSpeed * 0.25f));
}

float OrientationWarp::ClampAngle(float angle, float maxDegrees)
{
    if (maxDegrees <= 0.0f)
        return angle;
    return Math::Clamp(angle, -maxDegrees, maxDegrees);
}

float OrientationWarp::InterpAngle(float current, float target, float speed, float deltaTime)
{
    if (speed <= 0.0f)
        return Math::UnwindDegrees(target);
    float difference = Math::UnwindDegrees(target - current);
    // Half way round either way: unwind toward zero, so a swing out and the swing back never add up to a full turn
    if (Math::Abs(Math::Abs(difference) - 180.0f) < 0.01f)
        difference = current > 0.0f ? -180.0f : 180.0f;
    const float share = Math::Saturate(speed * Math::Max(deltaTime, 0.0f));
    return Math::UnwindDegrees(current + difference * share);
}

void OrientationWarp::Warp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, float angle, Transform* pose)
{
    if (nodesCount <= 0 || Math::IsZero(angle))
        return;
    Float3 axis = settings.UpAxis;
    axis.Normalize();
    if (axis.IsZero())
        return;

    // Model space transformations of the input pose (parents are always processed before their children), followed by the warped nodes' model space targets
    // Note: skeletons with more than 96 nodes fall back to a heap allocation here
    Array<Transform, InlinedAllocation<192>> buffer;
    buffer.Resize(nodesCount * 2);
    Transform* model = buffer.Get();
    Transform* targets = model + nodesCount;
    for (int32 i = 0; i < nodesCount; i++)
    {
        const int32 parent = nodes[i].ParentIndex;
        model[i] = parent >= 0 && parent < i ? model[parent].LocalToWorld(pose[i]) : pose[i];
    }

    // Pick the model space target of each warped node
    Array<TargetMode, InlinedAllocation<256>> modes;
    modes.Resize(nodesCount);
    Platform::MemoryClear(modes.Get(), sizeof(TargetMode) * nodesCount);
    bool any = false;
    const int32 turned[2] = { settings.BodyRoot, settings.IKFootRoot };
    for (const int32 index : turned)
    {
        if (!IsValid(index, nodesCount))
            continue;
        targets[index] = TurnAboutOrigin(axis, angle, model[index]);
        modes[index] = TargetMode::Transform;
        any = true;
    }
    if (IsValid(settings.SpineEnd, nodesCount))
    {
        // Walk up from the spine end to the spine start (only the spine end itself when the start isn't one of its ancestors below the turned nodes)
        Array<int32, InlinedAllocation<32>> chain;
        int32 index = settings.SpineEnd;
        while (true)
        {
            chain.Add(index);
            if (index == settings.SpineStart)
                break;
            index = nodes[index].ParentIndex;
            if (!IsValid(index, nodesCount) || !IsValid(settings.SpineStart, nodesCount) || index == settings.BodyRoot || index == settings.IKFootRoot)
            {
                chain.Clear();
                chain.Add(settings.SpineEnd);
                break;
            }
        }

        // Each spine node keeps the part of the turn it doesn't take back (lowest node first)
        const int32 chainLength = chain.Count();
        for (int32 i = 0; i < chainLength; i++)
        {
            const int32 node = chain[chainLength - 1 - i];
            const float share = GetSpineShare(i, chainLength, settings.Distribution);
            targets[node] = TurnAboutOrigin(axis, angle * (1.0f - share), model[node]);
            modes[node] = TargetMode::Rotation;
        }
        any = true;
    }
    if (IsValid(settings.KeepNode, nodesCount))
    {
        targets[settings.KeepNode] = model[settings.KeepNode];
        modes[settings.KeepNode] = TargetMode::Transform;
        any = true;
    }
    if (!any)
        return;

    // Rebuild the local transformations: warped nodes take their targets, all other nodes keep their local transformation and follow their parents
    // Nodes before the first warped one don't change, so their model space transformations are already right
    int32 first = 0;
    while (modes[first] == TargetMode::None)
        first++;
    for (int32 i = first; i < nodesCount; i++)
    {
        const int32 parent = nodes[i].ParentIndex;
        const bool hasParent = parent >= 0 && parent < i;
        const TargetMode mode = modes[i];
        if (mode == TargetMode::None)
        {
            if (hasParent)
                model[i] = model[parent].LocalToWorld(pose[i]);
            else
                model[i] = pose[i];
            continue;
        }
        Transform target = targets[i];
        if (mode == TargetMode::Rotation)
        {
            // Keep the local translation and scale, only the model space rotation is set
            const Transform followed = hasParent ? model[parent].LocalToWorld(pose[i]) : pose[i];
            target.Translation = followed.Translation;
            target.Scale = followed.Scale;
        }
        model[i] = target;
        if (hasParent)
        {
            const Float3 scale = pose[i].Scale;
            pose[i] = model[parent].WorldToLocal(target);
            pose[i].Orientation.Normalize();
            if (mode == TargetMode::Rotation)
                pose[i].Scale = scale;
        }
        else
        {
            pose[i] = target;
        }
    }
}
