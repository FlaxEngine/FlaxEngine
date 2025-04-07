// Copyright (c) Wojciech Figat. All rights reserved.

#include "AnimationData.h"

void NodeAnimationData::Evaluate(float time, Transform* result, bool loop) const
{
    if (Position.GetKeyframes().HasItems())
#if USE_LARGE_WORLDS
    {
        Float3 position;
        Position.Evaluate(position, time, loop);
        result->Translation = position;
    }
#else
        Position.Evaluate(result->Translation, time, loop);
#endif
    if (Rotation.GetKeyframes().HasItems())
        Rotation.Evaluate(result->Orientation, time, loop);
    if (Scale.GetKeyframes().HasItems())
        Scale.Evaluate(result->Scale, time, loop);
}

void NodeAnimationData::EvaluateAll(float time, Transform* result, bool loop) const
{
    Float3 position;
    Position.Evaluate(position, time, loop);
    result->Translation = position;
    Rotation.Evaluate(result->Orientation, time, loop);
    Scale.Evaluate(result->Scale, time, loop);
}

int32 NodeAnimationData::GetKeyframesCount() const
{
    return Position.GetKeyframes().Count() + Rotation.GetKeyframes().Count() + Scale.GetKeyframes().Count();
}

uint64 NodeAnimationData::GetMemoryUsage() const
{
    return NodeName.Length() * sizeof(Char) + Position.GetMemoryUsage() + Rotation.GetMemoryUsage() + Scale.GetMemoryUsage();
}

uint64 AnimationData::GetMemoryUsage() const
{
    uint64 result = (Name.Length() + RootNodeName.Length()) * sizeof(Char) + Channels.Capacity() * sizeof(NodeAnimationData);
    for (const auto& e : Channels)
        result += e.GetMemoryUsage();
    return result;
}

int32 AnimationData::GetKeyframesCount() const
{
    int32 result = 0;
    for (int32 i = 0; i < Channels.Count(); i++)
        result += Channels[i].GetKeyframesCount();
    return result;
}

NodeAnimationData* AnimationData::GetChannel(const StringView& name)
{
    for (auto& e : Channels)
        if (e.NodeName == name)
            return &e;
    return nullptr;
}

void AnimationData::Swap(AnimationData& other)
{
    ::Swap(Duration, other.Duration);
    ::Swap(FramesPerSecond, other.FramesPerSecond);
    ::Swap(RootMotionFlags, other.RootMotionFlags);
    ::Swap(Name, other.Name);
    ::Swap(RootNodeName, other.RootNodeName);
    Channels.Swap(other.Channels);
}

void AnimationData::Release()
{
    Name.Clear();
    Duration = 0.0;
    FramesPerSecond = 0.0;
    RootNodeName.Clear();
    RootMotionFlags = AnimationRootMotionFlags::None;
    Channels.Resize(0);
}
