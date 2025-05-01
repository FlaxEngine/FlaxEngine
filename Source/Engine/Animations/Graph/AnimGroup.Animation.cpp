// Copyright (c) Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Core/Types/VariantValueCast.h"
#include "Engine/Content/Assets/Animation.h"
#include "Engine/Content/Assets/SkeletonMask.h"
#include "Engine/Content/Assets/AnimationGraphFunction.h"
#include "Engine/Animations/AlphaBlend.h"
#include "Engine/Animations/AnimEvent.h"
#include "Engine/Animations/InverseKinematics.h"
#include "Engine/Level/Actors/AnimatedModel.h"

struct AnimSampleData
{
    Animation* Anim;
    float TimePos;
    float PrevTimePos;
    float Length;
    float Speed;
    ANIM_GRAPH_MULTI_BLEND_INDEX MultiBlendIndex; // Index of the animation in the multi-blend node data array

    AnimSampleData(Animation* anim, float speed = 1.0f, ANIM_GRAPH_MULTI_BLEND_INDEX multiBlendIndex = 0)
        : Anim(anim)
        , TimePos(0.0f)
        , PrevTimePos(0.0f)
        , Length(anim->GetLength())
        , Speed(speed)
        , MultiBlendIndex(multiBlendIndex)
    {
    }
};

struct MultiBlendAnimData
{
    float TimePosition;
    ANIM_GRAPH_MULTI_BLEND_INDEX Animation;

    typedef Array<MultiBlendAnimData, FixedAllocation<AnimGraphInstanceData::MultiBlendBucket::MaxCount>> List;

    static void BeforeSample(const AnimGraphContext& context, const AnimGraphInstanceData::MultiBlendBucket& bucket, const List& prevList, AnimSampleData& sample, float speed = 1.0f)
    {
        // Find time position in the previous frame
        sample.PrevTimePos = 0.0f;
        for (const auto& e : prevList)
        {
            if (e.Animation == sample.MultiBlendIndex)
            {
                sample.PrevTimePos = e.TimePosition;
                break;
            }
        }

        if (speed < 0.0f && bucket.LastUpdateFrame < context.CurrentFrameIndex - 1)
        {
            // If speed is negative and it's the first node update then start playing from end
            sample.PrevTimePos = sample.Length;
        }

        // Calculate new time position
        sample.TimePos = sample.PrevTimePos + context.DeltaTime * speed;
    }

    static void AfterSample(List& newList, const AnimSampleData& sample)
    {
        CHECK(newList.Count() < AnimGraphInstanceData::MultiBlendBucket::MaxCount);

        // Save animation position for the next frame
        newList.Add({ sample.TimePos, sample.MultiBlendIndex });
    }

    static void GetList(const AnimGraphInstanceData::MultiBlendBucket& multiBlend, List& list)
    {
        list.Resize(multiBlend.Count);
        for (int32 i = 0; i < multiBlend.Count; i++)
            list[i] = { multiBlend.TimePositions[i], multiBlend.Animations[i] };
    }

    static void SetList(AnimGraphInstanceData::MultiBlendBucket& multiBlend, const List& list)
    {
        multiBlend.Count = list.Count();
        for (int32 i = 0; i < multiBlend.Count; i++)
        {
            auto& e = list[i];
            multiBlend.TimePositions[i] = e.TimePosition;
            multiBlend.Animations[i] = e.Animation;
        }
    }
};

namespace
{
    FORCE_INLINE void BlendAdditiveWeightedRotation(Quaternion& base, Quaternion& additive, float weight)
    {
        // Pick a shortest path between rotation to fix blending artifacts
        additive *= weight;
        if (Quaternion::Dot(base, additive) < 0)
            additive *= -1;
        base += additive;
    }

    FORCE_INLINE void NormalizeRotations(AnimGraphImpulse* nodes, RootMotionExtraction rootMotionMode)
    {
        for (int32 i = 0; i < nodes->Nodes.Count(); i++)
        {
            nodes->Nodes.Get()[i].Orientation.Normalize();
        }
        if (rootMotionMode != RootMotionExtraction::NoExtraction)
        {
            nodes->RootMotion.Orientation.Normalize();
        }
    }

    Matrix ComputeWorldMatrixRecursive(const SkeletonData& skeleton, int32 index, Matrix localMatrix)
    {
        const auto& node = skeleton.Nodes[index];
        index = node.ParentIndex;
        while (index != -1)
        {
            const auto& parent = skeleton.Nodes[index];
            localMatrix *= parent.LocalTransform.GetWorld();
            index = parent.ParentIndex;
        }
        return localMatrix;
    }

    Matrix ComputeInverseParentMatrixRecursive(const SkeletonData& skeleton, int32 index)
    {
        Matrix inverseParentMatrix = Matrix::Identity;
        const auto& node = skeleton.Nodes[index];
        if (node.ParentIndex != -1)
        {
            inverseParentMatrix = ComputeWorldMatrixRecursive(skeleton, index, inverseParentMatrix);
            inverseParentMatrix = Matrix::Invert(inverseParentMatrix);
        }
        return inverseParentMatrix;
    }
}

void RetargetSkeletonNode(const SkeletonData& sourceSkeleton, const SkeletonData& targetSkeleton, const SkinnedModel::SkeletonMapping& sourceMapping, Transform& node, int32 targetIndex)
{
    // sourceSkeleton - skeleton of Anim Graph (Base Locomotion pack)
    // targetSkeleton - visual mesh skeleton (City Characters pack)
    // target - anim graph input/output transformation of that node
    const auto& targetNode = targetSkeleton.Nodes[targetIndex];
    const int32 sourceIndex = sourceMapping.NodesMapping[targetIndex];
    if (sourceIndex == -1)
    {
        // Use T-pose
        node = targetNode.LocalTransform;
        return;
    }
    const auto& sourceNode = sourceSkeleton.Nodes[sourceIndex];

    // [Reference: https://wickedengine.net/2022/09/animation-retargeting/comment-page-1/]

    // Calculate T-Pose of source node, target node and target parent node
    Matrix bindMatrix = ComputeWorldMatrixRecursive(sourceSkeleton, sourceIndex, sourceNode.LocalTransform.GetWorld());
    Matrix inverseBindMatrix = Matrix::Invert(bindMatrix);
    Matrix targetMatrix = ComputeWorldMatrixRecursive(targetSkeleton, targetIndex, targetNode.LocalTransform.GetWorld());
    Matrix inverseParentMatrix = ComputeInverseParentMatrixRecursive(targetSkeleton, targetIndex);

    // Target node animation is world-space difference of the animated source node inside the target's parent node world-space
    Matrix localMatrix = inverseBindMatrix * ComputeWorldMatrixRecursive(sourceSkeleton, sourceIndex, node.GetWorld());
    localMatrix = targetMatrix * localMatrix * inverseParentMatrix;

    // Extract local node transformation
    localMatrix.Decompose(node);
}

void RetargetSkeletonPose(const SkeletonData& sourceSkeleton, const SkeletonData& targetSkeleton, const SkinnedModel::SkeletonMapping& mapping, const Transform* sourceNodes, Transform* targetNodes)
{
    // TODO: cache source and target skeletons world-space poses for faster retargeting (use some pooled memory)
    ASSERT_LOW_LAYER(targetSkeleton.Nodes.Count() == mapping.NodesMapping.Length());
    for (int32 targetIndex = 0; targetIndex < targetSkeleton.Nodes.Count(); targetIndex++)
    {
        auto& targetNode = targetSkeleton.Nodes.Get()[targetIndex];
        const int32 sourceIndex = mapping.NodesMapping.Get()[targetIndex];
        Transform node;
        if (sourceIndex == -1)
        {
            // Use T-pose
            node = targetNode.LocalTransform;
        }
        else
        {
            // Retarget
            node = sourceNodes[sourceIndex];
            RetargetSkeletonNode(sourceSkeleton, targetSkeleton, mapping, node, targetIndex);
        }
        targetNodes[targetIndex] = node;
    }
}

AnimGraphTraceEvent& AnimGraphContext::AddTraceEvent(const AnimGraphNode* node)
{
    auto& trace = Data->TraceEvents.AddOne();
    trace.Value = 0.0f;
    trace.NodeId = node->ID;
    const auto* nodePath = NodePath.Get();
    for (int32 i = 0; i < NodePath.Count(); i++)
        trace.NodePath[i] = nodePath[i];
    return trace;
}

int32 AnimGraphExecutor::GetRootNodeIndex(Animation* anim)
{
    // TODO: cache the root node index (use dictionary with Animation* -> int32 for fast lookups)
    int32 rootNodeIndex = 0;
    if (anim->Data.RootNodeName.HasChars())
    {
        auto& skeleton = _graph.BaseModel->Skeleton;
        for (int32 i = 0; i < skeleton.Nodes.Count(); i++)
        {
            if (skeleton.Nodes[i].Name == anim->Data.RootNodeName)
            {
                rootNodeIndex = i;
                break;
            }
        }
    }
    return rootNodeIndex;
}

void AnimGraphExecutor::ProcessAnimEvents(AnimGraphNode* node, bool loop, float length, float animPos, float animPrevPos, Animation* anim, float speed)
{
    if (anim->Events.Count() == 0)
        return;
    ANIM_GRAPH_PROFILE_EVENT("Events");
    auto& context = *Context.Get();
    float eventTimeMin = animPrevPos;
    float eventTimeMax = animPos;
    if (loop && context.DeltaTime * speed < 0)
    {
        // Check if animation looped (for anim events shooting during backwards playback)
        //const float posNotLooped = startTimePos + oldTimePos;
        //if (posNotLooped < 0.0f || posNotLooped > length)
        //const int32 animPosCycle = Math::CeilToInt(animPos / anim->GetDuration());
        //const int32 animPrevPosCycle = Math::CeilToInt(animPrevPos / anim->GetDuration());
        //if (animPosCycle != animPrevPosCycle)
        {
            Swap(eventTimeMin, eventTimeMax);
        }
    }
    const float eventTime = (float)(animPos / anim->Data.FramesPerSecond);
    const float eventDeltaTime = (float)((animPos - animPrevPos) / anim->Data.FramesPerSecond);
    for (const auto& track : anim->Events)
    {
        for (const auto& k : track.Second.GetKeyframes())
        {
            if (!k.Value.Instance)
                continue;
            const float duration = k.Value.Duration > 1 ? k.Value.Duration : 0.0f;
#define ADD_OUTGOING_EVENT(type) context.Data->OutgoingEvents.Add({ k.Value.Instance, (AnimatedModel*)context.Data->Object, anim, eventTime, eventDeltaTime, AnimGraphInstanceData::OutgoingEvent::type })
            if (k.Time <= eventTimeMax && eventTimeMin <= k.Time + duration)
            {
                int32 stateIndex = -1;
                if (duration > 1)
                {
                    // Begin for continuous event
                    for (stateIndex = 0; stateIndex < context.Data->ActiveEvents.Count(); stateIndex++)
                    {
                        const auto& e = context.Data->ActiveEvents[stateIndex];
                        if (e.Instance == k.Value.Instance && e.Node == node)
                            break;
                    }
                    if (stateIndex == context.Data->ActiveEvents.Count())
                    {
                        ASSERT(k.Value.Instance->Is<AnimContinuousEvent>());
                        auto& e = context.Data->ActiveEvents.AddOne();
                        e.Instance = (AnimContinuousEvent*)k.Value.Instance;
                        e.Anim = anim;
                        e.Node = node;
                        ADD_OUTGOING_EVENT(OnBegin);
                    }
                }

                // Event
                ADD_OUTGOING_EVENT(OnEvent);
                if (stateIndex != -1)
                    context.Data->ActiveEvents[stateIndex].Hit = true;
            }
            else if (duration > 1)
            {
                // End for continuous event
                for (int32 i = 0; i < context.Data->ActiveEvents.Count(); i++)
                {
                    const auto& e = context.Data->ActiveEvents[i];
                    if (e.Instance == k.Value.Instance && e.Node == node)
                    {
                        ADD_OUTGOING_EVENT(OnEnd);
                        context.Data->ActiveEvents.RemoveAt(i);
                        break;
                    }
                }
            }
#undef ADD_OUTGOING_EVENT
        }
    }
}

float GetAnimPos(float& timePos, float startTimePos, float speed, bool loop, float length)
{
    // Apply animation offset and looping to calculate the animation sampling position within [0;length]
    float result = startTimePos + timePos * speed;
    if (result < 0.0f)
    {
        if (loop)
        {
            // Animation looped (revered playback)
            result = length - result;
        }
        else
        {
            // Animation ended (revered playback)
            result = 0;
        }
        timePos = result;
    }
    else if (result > length)
    {
        if (loop)
        {
            // Animation looped
            result = Math::Mod(result, length);

            // Remove start time offset to properly loop from animation start during the next frame
            timePos = result - startTimePos;
        }
        else
        {
            // Animation ended
            timePos = result = length;
        }
    }
    return result;
}

float GetAnimSamplePos(float length, Animation* anim, float pos)
{
    // Convert into animation local time (track length may be bigger so fill the gaps with animation clip and include playback speed)
    // Also, scale the animation to fit the total animation node length without cut in a middle
    const auto animLength = anim->GetLength();
    const int32 cyclesCount = Math::Max(Math::FloorToInt(length / animLength), 1);
    const float cycleLength = animLength * (float)cyclesCount;
    const float adjustRateScale = length / cycleLength;
    auto animPos = pos * adjustRateScale;
    while (animPos > animLength)
    {
        animPos -= animLength;
    }
    if (animPos < 0)
        animPos = animLength + animPos;
    animPos = (float)(animPos * anim->Data.FramesPerSecond);
    return animPos;
}

FORCE_INLINE void GetAnimPos(bool loop, float length, float speed, float startTimePos, float prevTimePos, float& newTimePos, float& pos, float& prevPos)
{
    // Calculate actual time position within the animation node (defined by length and loop mode)
    pos = GetAnimPos(newTimePos, startTimePos, speed, loop, length);
    prevPos = GetAnimPos(prevTimePos, startTimePos, speed, loop, length);
}

void AnimGraphExecutor::ProcessAnimation(AnimGraphImpulse* nodes, AnimGraphNode* node, bool loop, float length, float pos, float prevPos, Animation* anim, float speed, float weight, ProcessAnimationMode mode, BitArray<InlinedAllocation<8>>* usedNodes)
{
    PROFILE_CPU_ASSET(anim);

    // Get animation position (animation track position for channels sampling)
    const float animPos = GetAnimSamplePos(length, anim, pos);
    const float animPrevPos = GetAnimSamplePos(length, anim, prevPos);

    // Add to trace
    auto& context = *Context.Get();
    if (context.Data->EnableTracing)
    {
        auto& trace = context.AddTraceEvent(node);
        trace.Asset = anim;
        trace.Value = animPos;
    }

    // Evaluate nested animations
    BitArray<InlinedAllocation<8>> usedNodesThis;
    if (anim->NestedAnims.Count() != 0)
    {
        if (usedNodes == nullptr)
        {
            // Per-channel bit to indicate which channels were used by nested
            usedNodesThis.Resize(nodes->Nodes.Count());
            usedNodesThis.SetAll(false);
            usedNodes = &usedNodesThis;
        }

        for (auto& e : anim->NestedAnims)
        {
            const auto& nestedAnim = e.Second;
            float nestedAnimPos = animPos - nestedAnim.Time;
            if (nestedAnimPos >= 0.0f &&
                nestedAnimPos < nestedAnim.Duration &&
                nestedAnim.Enabled &&
                nestedAnim.Anim &&
                nestedAnim.Anim->IsLoaded())
            {
                // Get nested animation time position
                float nestedAnimPrevPos = animPrevPos - nestedAnim.Time;
                const float nestedAnimLength = nestedAnim.Anim->GetLength();
                const float nestedAnimSpeed = nestedAnim.Speed * speed;
                const float frameRateMatchScale = (float)(nestedAnimSpeed / anim->Data.FramesPerSecond);
                nestedAnimPos = nestedAnimPos * frameRateMatchScale;
                nestedAnimPrevPos = nestedAnimPrevPos * frameRateMatchScale;
                GetAnimPos(nestedAnim.Loop, nestedAnimLength, speed, nestedAnim.StartTime, nestedAnimPrevPos, nestedAnimPos, nestedAnimPos, nestedAnimPrevPos);

                ProcessAnimation(nodes, node, true, nestedAnimLength, nestedAnimPos, nestedAnimPrevPos, nestedAnim.Anim, 1.0f, weight, mode, usedNodes);
            }
        }
    }

    // Get skeleton nodes mapping descriptor
    const SkinnedModel::SkeletonMapping mapping = _graph.BaseModel->GetSkeletonMapping(anim);
    if (mapping.NodesMapping.IsInvalid())
        return;

    // Evaluate nodes animations
    const bool weighted = weight < 1.0f;
    const bool retarget = mapping.SourceSkeleton && mapping.SourceSkeleton != mapping.TargetSkeleton;
    const auto emptyNodes = GetEmptyNodes();
    SkinnedModel::SkeletonMapping sourceMapping;
    if (retarget)
        sourceMapping = _graph.BaseModel->GetSkeletonMapping(mapping.SourceSkeleton);
    for (int32 nodeIndex = 0; nodeIndex < nodes->Nodes.Count(); nodeIndex++)
    {
        const int32 nodeToChannel = mapping.NodesMapping[nodeIndex];
        Transform& dstNode = nodes->Nodes[nodeIndex];
        Transform srcNode = emptyNodes->Nodes[nodeIndex];
        if (nodeToChannel != -1)
        {
            // Calculate the animated node transformation
            anim->Data.Channels[nodeToChannel].Evaluate(animPos, &srcNode, false);

            // Optionally retarget animation into the skeleton used by the Anim Graph
            if (retarget)
            {
                RetargetSkeletonNode(mapping.SourceSkeleton->Skeleton, mapping.TargetSkeleton->Skeleton, sourceMapping, srcNode, nodeIndex);
            }

            // Mark node as used
            if (usedNodes)
                usedNodes->Set(nodeIndex, true);
        }
        else if (usedNodes && (usedNodes != &usedNodesThis || usedNodes->Get(nodeIndex)))
        {
            // Skip for nested animations so other one or top-level anim will update remaining nodes
            continue;
        }

        // Blend node
        if (mode == ProcessAnimationMode::BlendAdditive)
        {
            dstNode.Translation += srcNode.Translation * weight;
            dstNode.Scale += srcNode.Scale * weight;
            BlendAdditiveWeightedRotation(dstNode.Orientation, srcNode.Orientation, weight);
        }
        else if (mode == ProcessAnimationMode::Add)
        {
            dstNode.Translation += srcNode.Translation * weight;
            dstNode.Scale += srcNode.Scale * weight;
            dstNode.Orientation += srcNode.Orientation * weight;
        }
        else if (weighted)
        {
            dstNode.Translation = srcNode.Translation * weight;
            dstNode.Scale = srcNode.Scale * weight;
            dstNode.Orientation = srcNode.Orientation * weight;
        }
        else
        {
            dstNode = srcNode;
        }
    }

    // Handle root motion
    if (_rootMotionMode != RootMotionExtraction::NoExtraction && anim->Data.RootMotionFlags != AnimationRootMotionFlags::None)
    {
        // Calculate the root motion node transformation
        const bool motionPositionXZ = EnumHasAnyFlags(anim->Data.RootMotionFlags, AnimationRootMotionFlags::RootPositionXZ);
        const bool motionPositionY = EnumHasAnyFlags(anim->Data.RootMotionFlags, AnimationRootMotionFlags::RootPositionY);
        const bool motionRotation = EnumHasAnyFlags(anim->Data.RootMotionFlags, AnimationRootMotionFlags::RootRotation);
        const Vector3 motionPositionMask(motionPositionXZ ? 1.0f : 0.0f, motionPositionY ? 1.0f : 0.0f, motionPositionXZ ? 1.0f : 0.0f);
        const bool motionPosition = motionPositionXZ | motionPositionY;
        const int32 rootNodeIndex = GetRootNodeIndex(anim);
        const Transform& refPose = emptyNodes->Nodes[rootNodeIndex];
        Transform& rootNode = nodes->Nodes[rootNodeIndex];
        Transform& dstNode = nodes->RootMotion;
        Transform srcNode = Transform::Identity;
        const int32 nodeToChannel = mapping.NodesMapping[rootNodeIndex];
        if (_rootMotionMode == RootMotionExtraction::Enable && nodeToChannel != -1)
        {
            // Get the root bone transformation
            Transform rootBefore = refPose, rootNow = refPose;
            const NodeAnimationData& rootChannel = anim->Data.Channels[nodeToChannel];
            rootChannel.Evaluate(animPrevPos, &rootBefore, false);
            rootChannel.Evaluate(animPos, &rootNow, false);

            // Check if animation looped
            if (animPos < animPrevPos)
            {
                const float endPos = (float)(anim->GetLength() * anim->Data.FramesPerSecond);

                Transform rootBegin = refPose;
                rootChannel.Evaluate(0, &rootBegin, false);

                Transform rootEnd = refPose;
                rootChannel.Evaluate(endPos, &rootEnd, false);

                // Complex motion calculation to preserve the looped movement
                // (end - before + now - begin)
                // It sums the motion since the last update to anim end and since the start to now
                if (motionPosition)
                    srcNode.Translation = (rootEnd.Translation - rootBefore.Translation + rootNow.Translation - rootBegin.Translation) * motionPositionMask;
                if (motionRotation)
                    srcNode.Orientation = (rootBefore.Orientation.Conjugated() * rootEnd.Orientation) * (rootBegin.Orientation.Conjugated() * rootNow.Orientation);
            }
            else
            {
                // Simple motion delta
                // (now - before)
                if (motionPosition)
                    srcNode.Translation = (rootNow.Translation - rootBefore.Translation) * motionPositionMask;
                if (motionRotation)
                    srcNode.Orientation = rootBefore.Orientation.Conjugated() * rootNow.Orientation;
            }

            // Convert root motion from local-space to the actor-space (eg. if root node is not actually a root and its parents have rotation/scale)
            auto& skeleton = _graph.BaseModel->Skeleton;
            int32 parentIndex = skeleton.Nodes[rootNodeIndex].ParentIndex;
            while (parentIndex != -1)
            {
                const Transform& parentNode = nodes->Nodes[parentIndex];
                srcNode.Translation = parentNode.LocalToWorld(srcNode.Translation);
                parentIndex = skeleton.Nodes[parentIndex].ParentIndex;
            }
        }

        // Remove root node motion after extraction (only extracted components)
        if (motionPosition)
            rootNode.Translation = refPose.Translation * motionPositionMask + rootNode.Translation * (Vector3::One - motionPositionMask);
        if (motionRotation)
            rootNode.Orientation = refPose.Orientation;

        // Blend root motion
        if (mode == ProcessAnimationMode::BlendAdditive)
        {
            if (motionPosition)
                dstNode.Translation += srcNode.Translation * weight * motionPositionMask;
            if (motionRotation)
                BlendAdditiveWeightedRotation(dstNode.Orientation, srcNode.Orientation, weight);
        }
        else if (mode == ProcessAnimationMode::Add)
        {
            if (motionPosition)
                dstNode.Translation += srcNode.Translation * weight * motionPositionMask;
            if (motionRotation)
                dstNode.Orientation += srcNode.Orientation * weight;
        }
        else if (weighted)
        {
            if (motionPosition)
                dstNode.Translation = srcNode.Translation * weight * motionPositionMask;
            if (motionRotation)
                dstNode.Orientation = srcNode.Orientation * weight;
        }
        else
        {
            if (motionPosition)
                dstNode.Translation = srcNode.Translation * motionPositionMask;
            if (motionRotation)
                dstNode.Orientation = srcNode.Orientation;
        }
    }

    // Collect events
    if (weight > 0.5f)
    {
        ProcessAnimEvents(node, loop, length, animPos, animPrevPos, anim, speed);
    }
}

Variant AnimGraphExecutor::SampleAnimation(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* anim, float speed)
{
    if (anim == nullptr || !anim->IsLoaded())
        return Value::Null;

    float pos, prevPos;
    GetAnimPos(loop, length, speed, startTimePos, prevTimePos, newTimePos, pos, prevPos);

    const auto nodes = node->GetNodes(this);
    InitNodes(nodes);
    nodes->Position = pos;
    nodes->Length = length;
    ProcessAnimation(nodes, node, loop, length, pos, prevPos, anim, speed);
    NormalizeRotations(nodes, _rootMotionMode);

    return nodes;
}

Variant AnimGraphExecutor::SampleAnimation(AnimGraphNode* node, bool loop, float startTimePos, AnimSampleData& sample)
{
    return SampleAnimation(node, loop, sample.Length, startTimePos, sample.PrevTimePos, sample.TimePos, sample.Anim, sample.Speed);
}

Variant AnimGraphExecutor::SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float startTimePos, AnimSampleData& a, AnimSampleData& b, float alpha)
{
    // Skip if any animation is not ready to use
    if (a.Anim == nullptr || !a.Anim->IsLoaded() ||
        b.Anim == nullptr || !b.Anim->IsLoaded())
        return Value::Null;

    // Get actual animation position (includes looping and start offset)
    float posA, prevPosA, posB, prevPosB;
    GetAnimPos(loop, a.Length, a.Speed, startTimePos, a.PrevTimePos, a.TimePos, posA, prevPosA);
    GetAnimPos(loop, b.Length, b.Speed, startTimePos, b.PrevTimePos, b.TimePos, posB, prevPosB);

    // Sample the animations with blending
    const auto nodes = node->GetNodes(this);
    InitNodes(nodes);
    nodes->Position = (a.TimePos + b.TimePos) / 2.0f;
    nodes->Length = Math::Max(a.Length, b.Length);
    ProcessAnimation(nodes, node, loop, a.Length, posA, prevPosA, a.Anim, a.Speed, 1.0f - alpha, ProcessAnimationMode::Override);
    ProcessAnimation(nodes, node, loop, b.Length, posB, prevPosB, b.Anim, b.Speed, alpha, ProcessAnimationMode::BlendAdditive);
    NormalizeRotations(nodes, _rootMotionMode);

    return nodes;
}

Variant AnimGraphExecutor::SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float startTimePos, AnimSampleData& a, AnimSampleData& b, AnimSampleData& c, float alphaA, float alphaB, float alphaC)
{
    // Skip if any animation is not ready to use
    if (a.Anim == nullptr || !a.Anim->IsLoaded() ||
        b.Anim == nullptr || !b.Anim->IsLoaded() ||
        c.Anim == nullptr || !c.Anim->IsLoaded())
        return Value::Null;

    // Get actual animation position (includes looping and start offset)
    float posA, prevPosA, posB, prevPosB, posC, prevPosC;
    GetAnimPos(loop, a.Length, a.Speed, startTimePos, a.PrevTimePos, a.TimePos, posA, prevPosA);
    GetAnimPos(loop, b.Length, b.Speed, startTimePos, b.PrevTimePos, b.TimePos, posB, prevPosB);
    GetAnimPos(loop, c.Length, c.Speed, startTimePos, c.PrevTimePos, c.TimePos, posC, prevPosC);

    // Sample the animations with blending
    const auto nodes = node->GetNodes(this);
    InitNodes(nodes);
    nodes->Position = (a.TimePos + b.TimePos + c.TimePos) / 3.0f;
    nodes->Length = Math::Max(a.Length, b.Length, c.Length);
    ASSERT(Math::Abs(alphaA + alphaB + alphaC - 1.0f) <= ANIM_GRAPH_BLEND_THRESHOLD); // Assumes weights are normalized
    ProcessAnimation(nodes, node, loop, a.Length, posA, prevPosA, a.Anim, a.Speed, alphaA, ProcessAnimationMode::Override);
    ProcessAnimation(nodes, node, loop, b.Length, posB, prevPosB, b.Anim, b.Speed, alphaB, ProcessAnimationMode::BlendAdditive);
    ProcessAnimation(nodes, node, loop, c.Length, posC, prevPosC, c.Anim, c.Speed, alphaC, ProcessAnimationMode::BlendAdditive);
    NormalizeRotations(nodes, _rootMotionMode);

    return nodes;
}

Variant AnimGraphExecutor::Blend(AnimGraphNode* node, const Value& poseA, const Value& poseB, float alpha, AlphaBlendMode alphaMode)
{
    ANIM_GRAPH_PROFILE_EVENT("Blend Pose");

    if (isnan(alpha) || isinf(alpha))
        alpha = 0;
    alpha = Math::Saturate(alpha);
    alpha = AlphaBlend::Process(alpha, alphaMode);

    const auto nodes = node->GetNodes(this);
    auto nodesA = static_cast<AnimGraphImpulse*>(poseA.AsPointer);
    auto nodesB = static_cast<AnimGraphImpulse*>(poseB.AsPointer);
    if (!ANIM_GRAPH_IS_VALID_PTR(poseA))
        nodesA = GetEmptyNodes();
    if (!ANIM_GRAPH_IS_VALID_PTR(poseB))
        nodesB = GetEmptyNodes();

    for (int32 i = 0; i < nodes->Nodes.Count(); i++)
    {
        Transform::Lerp(nodesA->Nodes[i], nodesB->Nodes[i], alpha, nodes->Nodes[i]);
    }
    Transform::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);
    nodes->Position = Math::Lerp(nodesA->Position, nodesB->Position, alpha);
    nodes->Length = Math::Lerp(nodesA->Length, nodesB->Length, alpha);

    return nodes;
}

Variant AnimGraphExecutor::SampleState(AnimGraphContext& context, const AnimGraphNode* state)
{
    auto& data = state->Data.State;
    if (data.Graph == nullptr || data.Graph->GetRootNode() == nullptr)
        return Value::Null;

    // Add to trace
    if (context.Data->EnableTracing)
    {
        auto& trace = context.AddTraceEvent(state);
    }

    ANIM_GRAPH_PROFILE_EVENT("Evaluate State");
    context.NodePath.Add(state->ID);
    auto rootNode = data.Graph->GetRootNode();
    auto result = eatBox((Node*)rootNode, &rootNode->Boxes[0]);
    context.NodePath.Pop();

    return result;
}

void AnimGraphExecutor::InitStateTransition(AnimGraphContext& context, AnimGraphInstanceData::StateMachineBucket& stateMachineBucket, AnimGraphStateTransition* transition)
{
    // Reset transition
    stateMachineBucket.ActiveTransition = transition;
    stateMachineBucket.TransitionPosition = 0.0f;

    // End base transition
    if (stateMachineBucket.BaseTransition)
    {
        ResetBuckets(context, stateMachineBucket.BaseTransitionState->Data.State.Graph);
        stateMachineBucket.BaseTransition = nullptr;
        stateMachineBucket.BaseTransitionState = nullptr;
        stateMachineBucket.BaseTransitionPosition = 0.0f;
    }
}

AnimGraphStateTransition* AnimGraphExecutor::UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, AnimGraphNode* state, AnimGraphNode* ignoreState)
{
    return UpdateStateTransitions(context, stateMachineData, state->Data.State, state, ignoreState);
}

AnimGraphStateTransition* AnimGraphExecutor::UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, const AnimGraphNode::StateBaseData& stateData, AnimGraphNode* state, AnimGraphNode* ignoreState)
{
    int32 transitionIndex = 0;
    while (stateData.Transitions && stateData.Transitions[transitionIndex] != AnimGraphNode::StateData::InvalidTransitionIndex)
    {
        const uint16 idx = stateData.Transitions[transitionIndex];
        ASSERT(idx < stateMachineData.Graph->StateTransitions.Count());
        auto& transition = stateMachineData.Graph->StateTransitions[idx];
        if (transition.Destination == state || transition.Destination == ignoreState)
        {
            // Ignore transition to the current state
            transitionIndex++;
            continue;
        }

        // Evaluate source state transition data (position, length, etc.)
        const Value sourceStatePtr = SampleState(context, state);
        auto& transitionData = context.TransitionData; // Note: this could support nested transitions but who uses state machine inside transition rule?
        if (ANIM_GRAPH_IS_VALID_PTR(sourceStatePtr))
        {
            // Use source state as data provider
            const auto sourceState = (AnimGraphImpulse*)sourceStatePtr.AsPointer;
            auto sourceLength = Math::Max(sourceState->Length, 0.0f);
            transitionData.Position = Math::Clamp(sourceState->Position, 0.0f, sourceLength);
            transitionData.Length = sourceLength;
        }
        else
        {
            // Reset
            transitionData.Position = 0;
            transitionData.Length = ZeroTolerance;
        }

        const bool useDefaultRule = EnumHasAnyFlags(transition.Flags, AnimGraphStateTransition::FlagTypes::UseDefaultRule);
        if (transition.RuleGraph && !useDefaultRule)
        {
            // Execute transition rule
            ANIM_GRAPH_PROFILE_EVENT("Rule");
            auto rootNode = transition.RuleGraph->GetRootNode();
            ASSERT(rootNode);
            if (!(bool)eatBox((Node*)rootNode, &rootNode->Boxes[0]))
            {
                transitionIndex++;
                continue;
            }
        }

        // Check if can trigger the transition
        bool canEnter = false;
        if (useDefaultRule)
        {
            // Start transition when the current state animation is about to end (split blend duration evenly into two states)
            const auto transitionDurationHalf = transition.BlendDuration * 0.5f + ZeroTolerance;
            const auto endPos = transitionData.Length - transitionDurationHalf;
            canEnter = transitionData.Position >= endPos;
        }
        else if (transition.RuleGraph)
            canEnter = true;
        if (canEnter)
        {
            return &transition;
        }

        // Skip after Solo transition
        // TODO: don't load transitions after first enabled Solo transition and remove this check here
        if (EnumHasAnyFlags(transition.Flags, AnimGraphStateTransition::FlagTypes::Solo))
            break;

        transitionIndex++;
    }

    // No transition
    return nullptr;
}

void AnimGraphExecutor::UpdateStateTransitions(AnimGraphContext& context, const AnimGraphNode::StateMachineData& stateMachineData, AnimGraphInstanceData::StateMachineBucket& stateMachineBucket, const AnimGraphNode::StateBaseData& stateData)
{
    AnimGraphStateTransition* transition = UpdateStateTransitions(context, stateMachineData, stateData, stateMachineBucket.CurrentState);
    if (transition)
    {
        InitStateTransition(context, stateMachineBucket, transition);
    }
}

void ComputeMultiBlendLength(float& length, AnimGraphNode* node)
{
    ANIM_GRAPH_PROFILE_EVENT("Setup Multi Blend Length");

    // TODO: lock graph or graph asset here? make it thread safe

    length = 0.0f;
    for (int32 i = 0; i < node->Assets.Count(); i++)
    {
        auto& asset = node->Assets[i];
        if (asset)
        {
            // TODO: maybe don't update if not all anims are loaded? just skip the node with the bind pose?
            if (asset->WaitForLoaded())
            {
                asset = nullptr;
                LOG(Warning, "Failed to load one of the animations.");
            }
            else
            {
                const auto anim = asset.As<Animation>();
                const auto aData = node->Values[4 + i * 2].AsFloat4();
                length = Math::Max(length, anim->GetLength() * Math::Abs(aData.W));
            }
        }
    }
}

void AnimGraphExecutor::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    auto& context = *Context.Get();
    switch (node->TypeID)
    {
    // Get
    case 1:
    {
        // Get parameter
        int32 paramIndex;
        const auto param = _graph.GetParameter((Guid)node->Values[0], paramIndex);
        if (param)
        {
            value = context.Data->Parameters[paramIndex].Value;
            switch (param->Type.Type)
            {
            case VariantType::Float2:
                switch (box->ID)
                {
                case 1:
                case 2:
                    value = value.AsFloat2().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Float3:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                    value = value.AsFloat3().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Float4:
            case VariantType::Color:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                case 4:
                    value = value.AsFloat4().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double2:
                switch (box->ID)
                {
                case 1:
                case 2:
                    value = value.AsDouble2().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double3:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                    value = value.AsDouble3().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Double4:
                switch (box->ID)
                {
                case 1:
                case 2:
                case 3:
                case 4:
                    value = value.AsDouble4().Raw[box->ID - 1];
                    break;
                }
                break;
            case VariantType::Matrix:
            {
                auto& matrix = value.Type.Type == VariantType::Matrix && value.AsBlob.Data ? *(Matrix*)value.AsBlob.Data : Matrix::Identity;
                switch (box->ID)
                {
                case 0:
                    value = matrix.GetRow1();
                    break;
                case 1:
                    value = matrix.GetRow2();
                    break;
                case 2:
                    value = matrix.GetRow3();
                    break;
                case 3:
                    value = matrix.GetRow4();
                    break;
                }
                break;
            }
            }
        }
        else
        {
            // TODO: add warning that no parameter selected
            value = Value::Zero;
        }
        break;
    }
    default:
        break;
    }
}

void AnimGraphExecutor::ProcessGroupTools(Box* box, Node* nodeBase, Value& value)
{
    auto& context = *Context.Get();
    auto node = (AnimGraphNode*)nodeBase;
    switch (node->TypeID)
    {
    // Time
    case 5:
    {
        auto& bucket = context.Data->State[node->BucketIndex].Animation;
        if (bucket.LastUpdateFrame != context.CurrentFrameIndex)
        {
            bucket.TimePosition += context.DeltaTime;
            bucket.LastUpdateFrame = context.CurrentFrameIndex;
        }
        value = box->ID == 0 ? bucket.TimePosition : context.DeltaTime;
        break;
    }
    default:
        VisjectExecutor::ProcessGroupTools(box, nodeBase, value);
        break;
    }
}

void AnimGraphExecutor::ProcessGroupAnimation(Box* boxBase, Node* nodeBase, Value& value)
{
    auto& context = *Context.Get();
    if (context.ValueCache.TryGet(boxBase, value))
        return;
    auto box = (AnimGraphBox*)boxBase;
    auto node = (AnimGraphNode*)nodeBase;
    switch (node->TypeID)
    {
    // Animation Output
    case 1:
        value = tryGetValue(box, Value::Null);
        break;
    // Animation
    case 2:
    {
        auto anim = node->Assets[0].As<Animation>();
        auto& bucket = context.Data->State[node->BucketIndex].Animation;

        // Override animation when animation reference box is connected
        auto animationAssetBox = node->TryGetBox(8);
        if (animationAssetBox && animationAssetBox->HasConnection())
        {
            anim = TVariantValueCast<Animation*>::Cast(tryGetValue(animationAssetBox, Value::Null));
        }

        switch (box->ID)
        {
        // Animation
        case 0:
        {
            ANIM_GRAPH_PROFILE_EVENT("Animation");
            const float speed = (float)tryGetValue(node->GetBox(5), node->Values[1]);
            const bool loop = (bool)tryGetValue(node->GetBox(6), node->Values[2]);
            const float startTimePos = (float)tryGetValue(node->GetBox(7), node->Values[3]);
            const float length = anim ? anim->GetLength() : 0.0f;

            // Calculate new time position
            if (speed < 0.0f && bucket.LastUpdateFrame < context.CurrentFrameIndex - 1)
            {
                // If speed is negative and it's the first node update then start playing from end
                bucket.TimePosition = length;
            }
            float newTimePos = bucket.TimePosition + context.DeltaTime * speed;

            value = SampleAnimation(node, loop, length, startTimePos, bucket.TimePosition, newTimePos, anim, 1.0f);

            bucket.TimePosition = newTimePos;
            bucket.LastUpdateFrame = context.CurrentFrameIndex;

            break;
        }
        // Normalized Time
        case 1:
        {
            const float startTimePos = (float)tryGetValue(node->GetBox(7), node->Values[3]);
            value = startTimePos + bucket.TimePosition;
            if (anim && anim->IsLoaded())
                value.AsFloat /= anim->GetLength();
            break;
        }
        // Time
        case 2:
        {
            const float startTimePos = (float)tryGetValue(node->GetBox(7), node->Values[3]);
            value = startTimePos + bucket.TimePosition;
            break;
        }
        // Length
        case 3:
            value = anim ? anim->GetLength() : 0.0f;
            break;
        // Is Playing
        case 4:
            // If anim was updated during this or a previous frame
            value = bucket.LastUpdateFrame >= context.CurrentFrameIndex - 1;
            break;
        }
        break;
    }
    // Transform Bone (local/model space)
    case 3:
    case 4:
    {
        // [Deprecated on 13.05.2020, expires on 13.05.2021]
        const auto inputBox = node->GetBox(1);
        const auto boneIndex = (int32)node->Values[0];
        const auto transformMode = static_cast<BoneTransformMode>((int32)node->Values[1]);

        // Get the transformation
        Transform transform;
        transform.Translation = (Vector3)tryGetValue(node->GetBox(2), Vector3::Zero);
        transform.Orientation = (Quaternion)tryGetValue(node->GetBox(3), Quaternion::Identity);
        transform.Scale = (Float3)tryGetValue(node->GetBox(4), Float3::One);

        // Skip if no change will be performed
        auto& skeleton = _graph.BaseModel->Skeleton;
        if (boneIndex < 0 || boneIndex >= skeleton.Bones.Count() || transformMode == BoneTransformMode::None || (transformMode == BoneTransformMode::Add && transform.IsIdentity()))
        {
            // Pass through the input
            value = Value::Null;
            if (inputBox->HasConnection())
                value = eatBox(nodeBase, inputBox->FirstConnection());
            context.ValueCache.Add(boxBase, value);
            return;
        }
        const auto nodeIndex = _graph.BaseModel->Skeleton.Bones[boneIndex].NodeIndex;
        const auto nodes = node->GetNodes(this);

        // Prepare the input nodes
        bool hasValidInput = false;
        if (inputBox->HasConnection())
        {
            const auto input = eatBox(nodeBase, inputBox->FirstConnection());
            hasValidInput = ANIM_GRAPH_IS_VALID_PTR(input);
            if (hasValidInput)
                CopyNodes(nodes, input);
        }
        if (!hasValidInput)
            InitNodes(nodes);

        // Apply the transformation
        if (transformMode == BoneTransformMode::Add)
            nodes->Nodes[nodeIndex] = nodes->Nodes[nodeIndex] * transform;
        else
            nodes->Nodes[nodeIndex] = transform;

        value = nodes;
        break;
    }
    // Local To Model
    case 5:
    {
        // [Deprecated on 15.05.2020, expires on 15.05.2021]
        value = tryGetValue(node->GetBox(1), Value::Null);
        /*const AnimGraphImpulse* src;
        AnimGraphImpulse* dst = node->GetNodes(this);
        if (ANIM_GRAPH_IS_VALID_PTR(input))
        {
            src = (AnimGraphImpulse*)input.AsPointer;
        }
        else
        {
            src = GetEmptyNodes();
        }
        const auto srcNodes = src->Nodes.Get();
        const auto dstNodes = dst->Nodes.Get();

        // Transform every node
        const auto& skeleton = BaseModel->Skeleton;
        for (int32 i = 0; i < nodes->Nodes.Count(); i++)
        {
            const int32 parentIndex = skeleton.Nodes[i].ParentIndex;
            if (parentIndex != -1)
                dstNodes[parentIndex].LocalToWorld(srcNodes[i], dstNodes[i]);
            else
                dstNodes[i] = srcNodes[i];
        }

        value = dst;*/
        break;
    }
    // Model To Local
    case 6:
    {
        // [Deprecated on 15.05.2020, expires on 15.05.2021]
        value = tryGetValue(node->GetBox(1), Value::Null);
        /*// Get input and output
        const auto input = tryGetValue(node->GetBox(1), Value::Null);
        if (!ANIM_GRAPH_IS_VALID_PTR(input))
        {
            // Skip
            value = Value::Null;
            break;
        }
        const AnimGraphImpulse* src = (AnimGraphImpulse*)input.AsPointer;
        if (src->PerNodeTransformationSpace.IsEmpty() && src->TransformationSpace == AnimGraphTransformationSpace::Local)
        {
            // Input is already in local space (eg. not modified)
            value = input;
            break;
        }
        const auto nodes = node->GetNodes(this);
        AnimGraphImpulse* dst = nodes;
        const auto srcNodes = src->Nodes.Get();
        const auto dstNodes = dst->Nodes.Get();

        // Inv transform every node
        const auto& skeleton = BaseModel->Skeleton;
        for (int32 i = nodes->Nodes.Count() - 1; i >= 0; i--)
        {
            const int32 parentIndex = skeleton.Nodes[i].ParentIndex;
            if (parentIndex != -1)
                dstNodes[parentIndex].WorldToLocal(srcNodes[i], dstNodes[i]);
            else
                dstNodes[i] = srcNodes[i];
        }

        value = dst;*/
        break;
    }
    // Copy Bone
    case 7:
    {
        // [Deprecated on 13.05.2020, expires on 13.05.2021]
        // Get input
        auto input = tryGetValue(node->GetBox(1), Value::Null);
        const auto nodes = node->GetNodes(this);
        if (ANIM_GRAPH_IS_VALID_PTR(input))
        {
            // Use input nodes
            CopyNodes(nodes, input);
        }
        else
        {
            // Use default nodes
            InitNodes(nodes);
            input = nodes;
        }

        // Fetch the settings
        const auto srcBoneIndex = (int32)node->Values[0];
        const auto dstBoneIndex = (int32)node->Values[1];
        const auto copyTranslation = (bool)node->Values[2];
        const auto copyRotation = (bool)node->Values[3];
        const auto copyScale = (bool)node->Values[4];

        // Skip if no change will be performed
        const auto& skeleton = _graph.BaseModel->Skeleton;
        if (srcBoneIndex < 0 || srcBoneIndex >= skeleton.Bones.Count() ||
            dstBoneIndex < 0 || dstBoneIndex >= skeleton.Bones.Count() ||
            !(copyTranslation || copyRotation || copyScale))
        {
            // Pass through the input
            value = input;
            context.ValueCache.Add(boxBase, value);
            return;
        }

        // Copy bone data
        Transform srcTransform = nodes->Nodes[skeleton.Bones[srcBoneIndex].NodeIndex];
        Transform dstTransform = nodes->Nodes[skeleton.Bones[dstBoneIndex].NodeIndex];
        if (copyTranslation)
            dstTransform.Translation = srcTransform.Translation;
        if (copyRotation)
            dstTransform.Orientation = srcTransform.Orientation;
        if (copyScale)
            dstTransform.Scale = srcTransform.Scale;
        nodes->Nodes[skeleton.Bones[dstBoneIndex].NodeIndex] = dstTransform;

        value = nodes;
        break;
    }
    // Get Bone Transform
    case 8:
    {
        // [Deprecated on 13.05.2020, expires on 13.05.2021]
        // Get input
        const auto boneIndex = (int32)node->Values[0];
        const auto& skeleton = _graph.BaseModel->Skeleton;
        const auto input = tryGetValue(node->GetBox(0), Value::Null);
        if (ANIM_GRAPH_IS_VALID_PTR(input) && boneIndex >= 0 && boneIndex < skeleton.Bones.Count())
            value = Variant(((AnimGraphImpulse*)input.AsPointer)->Nodes[skeleton.Bones[boneIndex].NodeIndex]);
        else
            value = Variant(Transform::Identity);
        break;
    }
    // Blend
    case 9:
    {
        const float alpha = Math::Saturate((float)tryGetValue(node->GetBox(3), node->Values[0]));

        // Only A
        if (Math::NearEqual(alpha, 0.0f, ANIM_GRAPH_BLEND_THRESHOLD))
        {
            value = tryGetValue(node->GetBox(1), Value::Null);
        }
        // Only B
        else if (Math::NearEqual(alpha, 1.0f, ANIM_GRAPH_BLEND_THRESHOLD))
        {
            value = tryGetValue(node->GetBox(2), Value::Null);
        }
        // Blend A and B
        else
        {
            const auto valueA = tryGetValue(node->GetBox(1), Value::Null);
            const auto valueB = tryGetValue(node->GetBox(2), Value::Null);
            const auto nodes = node->GetNodes(this);

            auto nodesA = static_cast<AnimGraphImpulse*>(valueA.AsPointer);
            auto nodesB = static_cast<AnimGraphImpulse*>(valueB.AsPointer);
            if (!ANIM_GRAPH_IS_VALID_PTR(valueA))
                nodesA = GetEmptyNodes();
            if (!ANIM_GRAPH_IS_VALID_PTR(valueB))
                nodesB = GetEmptyNodes();

            for (int32 i = 0; i < nodes->Nodes.Count(); i++)
            {
                Transform::Lerp(nodesA->Nodes[i], nodesB->Nodes[i], alpha, nodes->Nodes[i]);
            }
            Transform::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);
            value = nodes;
        }

        break;
    }
    // Blend Additive
    case 10:
    {
        const float alpha = Math::Saturate((float)tryGetValue(node->GetBox(3), node->Values[0]));

        // Only A
        if (Math::NearEqual(alpha, 0.0f, ANIM_GRAPH_BLEND_THRESHOLD))
        {
            value = tryGetValue(node->GetBox(1), Value::Null);
        }
        // Blend A and B
        else
        {
            const auto valueA = tryGetValue(node->GetBox(1), Value::Null);
            const auto valueB = tryGetValue(node->GetBox(2), Value::Null);

            if (!ANIM_GRAPH_IS_VALID_PTR(valueA))
            {
                value = Value::Null;
            }
            else if (!ANIM_GRAPH_IS_VALID_PTR(valueB))
            {
                value = valueA;
            }
            else
            {
                const auto nodes = node->GetNodes(this);
                const auto basePoseNodes = static_cast<AnimGraphImpulse*>(valueA.AsPointer);
                const auto blendPoseNodes = static_cast<AnimGraphImpulse*>(valueB.AsPointer);
                const auto& refNodes = _graph.BaseModel.Get()->GetNodes();
                Transform t, basePoseTransform, blendPoseTransform, refTransform;
                for (int32 i = 0; i < nodes->Nodes.Count(); i++)
                {
                    basePoseTransform = basePoseNodes->Nodes[i];
                    blendPoseTransform = blendPoseNodes->Nodes[i];
                    refTransform = refNodes[i].LocalTransform;

                    // base + (blend - reference)
                    t.Translation = basePoseTransform.Translation + (blendPoseTransform.Translation - refTransform.Translation);
                    auto diff = Quaternion::Invert(refTransform.Orientation) * blendPoseTransform.Orientation;
                    t.Orientation = basePoseTransform.Orientation * diff;
                    t.Scale = basePoseTransform.Scale + (blendPoseTransform.Scale - refTransform.Scale);

                    // Lerp base and transform
                    Transform::Lerp(basePoseTransform, t, alpha, nodes->Nodes[i]);
                }
                Transform::Lerp(basePoseNodes->RootMotion, basePoseNodes->RootMotion + blendPoseNodes->RootMotion, alpha, nodes->RootMotion);
                value = nodes;
            }
        }

        break;
    }
    // Blend with Mask
    case 11:
    {
        const float alpha = Math::Saturate((float)tryGetValue(node->GetBox(3), node->Values[0]));
        auto mask = node->Assets[0].As<SkeletonMask>();

        // Use the mask connected with this node instead of default mask asset
        auto maskAssetBox = node->TryGetBox(4);
        if (maskAssetBox && maskAssetBox->HasConnection())
        {
            const Value assetBoxValue = tryGetValue(maskAssetBox, Value::Null);
            if (assetBoxValue != Value::Null)
                mask = (SkeletonMask*)assetBoxValue.AsAsset;
        }

        // Only A or missing/invalid mask
        if (Math::NearEqual(alpha, 0.0f, ANIM_GRAPH_BLEND_THRESHOLD) || mask == nullptr || mask->WaitForLoaded())
        {
            value = tryGetValue(node->GetBox(1), Value::Null);
        }
        // Blend A and B with mask
        else
        {
            auto valueA = tryGetValue(node->GetBox(1), Value::Null);
            auto valueB = tryGetValue(node->GetBox(2), Value::Null);
            const auto nodes = node->GetNodes(this);

            if (!ANIM_GRAPH_IS_VALID_PTR(valueA))
                valueA = GetEmptyNodes();
            if (!ANIM_GRAPH_IS_VALID_PTR(valueB))
                valueB = GetEmptyNodes();
            const auto nodesA = static_cast<AnimGraphImpulse*>(valueA.AsPointer);
            const auto nodesB = static_cast<AnimGraphImpulse*>(valueB.AsPointer);

            // Blend all nodes masked by the user
            Transform tA, tB;
            auto& nodesMask = mask->GetNodesMask();
            for (int32 nodeIndex = 0; nodeIndex < nodes->Nodes.Count(); nodeIndex++)
            {
                tA = nodesA->Nodes[nodeIndex];
                if (nodesMask[nodeIndex])
                {
                    tB = nodesB->Nodes[nodeIndex];
                    Transform::Lerp(tA, tB, alpha, nodes->Nodes[nodeIndex]);
                }
                else
                {
                    nodes->Nodes[nodeIndex] = tA;
                }
            }
            Transform::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);

            value = nodes;
        }

        break;
    }
    // Multi Blend 1D
    case 12:
    {
        ANIM_GRAPH_PROFILE_EVENT("Multi Blend 1D");
        ASSERT(box->ID == 0);
        value = Value::Null;

        // Note data layout:
        // [0]: Float4 Range (minX, maxX, 0, 0)
        // [1]: float Speed
        // [2]: bool Loop
        // [3]: float StartPosition
        // Per Blend Sample data layout:
        // [0]: Float4 Info (x=posX, y=0, z=0, w=Speed)
        // [1]: Guid Animation

        // Prepare
        auto& bucket = context.Data->State[node->BucketIndex].MultiBlend;
        const auto range = node->Values[0].AsFloat4();
        const auto speed = (float)tryGetValue(node->GetBox(1), node->Values[1]);
        const auto loop = (bool)tryGetValue(node->GetBox(2), node->Values[2]);
        const auto startTimePos = (float)tryGetValue(node->GetBox(3), node->Values[3]);
        const auto syncLength = false; // TODO: make it configurable via node settings? (change node->Values[2] to contain flags)
        auto& data = node->Data.MultiBlend1D;
        if (data.Count == 0)
            break; // Skip if no valid animations added

        // Get axis X
        float x = (float)tryGetValue(node->GetBox(4), Value::Zero);
        x = Math::Clamp(x, range.X, range.Y);

        // Add to trace
        if (context.Data->EnableTracing)
        {
            auto& trace = context.AddTraceEvent(node);
            trace.Value = x;
        }

        // Check if need to evaluate multi blend length
        if (data.Length < 0)
            ComputeMultiBlendLength(data.Length, node);
        if (data.Length <= ZeroTolerance)
            break;

        MultiBlendAnimData::List prevList, newList;
        MultiBlendAnimData::GetList(bucket, prevList);

        // Find 2 animations to blend (line)
        for (int32 i = 0; i < data.Count - 1; i++)
        {
            const auto aIndex = data.IndicesSorted[i];
            const auto bIndex = data.IndicesSorted[i + 1];
            const auto aData = node->Values[4 + aIndex * 2].AsFloat4();
            AnimSampleData a(node->Assets[aIndex].As<Animation>(), aData.W, aIndex);

            // Check single A case
            if (x <= aData.X + ANIM_GRAPH_BLEND_THRESHOLD)
            {
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
                value = SampleAnimation(node, loop, startTimePos, a);
                MultiBlendAnimData::AfterSample(newList, a);
                break;
            }

            // Get B animation data
            auto bData = node->Values[4 + bIndex * 2].AsFloat4();
            AnimSampleData b(node->Assets[bIndex].As<Animation>(), bData.W, bIndex);

            // Check single B edge case
            if (Math::NearEqual(bData.X, x, ANIM_GRAPH_BLEND_THRESHOLD))
            {
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, b, speed);
                value = SampleAnimation(node, loop, startTimePos, b);
                MultiBlendAnimData::AfterSample(newList, b);
                break;
            }

            // Blend A and B
            const float alpha = (x - aData.X) / (bData.X - aData.X);
            if (alpha > 1.0f)
                continue;
            MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
            MultiBlendAnimData::BeforeSample(context, bucket, prevList, b, speed);
            value = SampleAnimationsWithBlend(node, loop, startTimePos, a, b, alpha);
            MultiBlendAnimData::AfterSample(newList, a);
            MultiBlendAnimData::AfterSample(newList, b);
            break;
        }
        if (newList.IsEmpty())
        {
            // Sample the last animation if had no result
            const auto aIndex = data.IndicesSorted[data.Count - 1];
            const auto aData = node->Values[4 + aIndex * 2].AsFloat4();
            AnimSampleData a(node->Assets[aIndex].As<Animation>(), aData.W, aIndex);
            MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
            value = SampleAnimation(node, loop, startTimePos, a);
            MultiBlendAnimData::AfterSample(newList, a);
        }

        MultiBlendAnimData::SetList(bucket, newList);
        bucket.LastUpdateFrame = context.CurrentFrameIndex;

        break;
    }
    // Multi Blend 2D
    case 13:
    {
        ANIM_GRAPH_PROFILE_EVENT("Multi Blend 2D");
        ASSERT(box->ID == 0);
        value = Value::Null;

        // Note data layout:
        // [0]: Float4 Range (minX, maxX, minY, maxY)
        // [1]: float Speed
        // [2]: bool Loop
        // [3]: float StartPosition
        // Per Blend Sample data layout:
        // [0]: Float4 Info (x=posX, y=posY, z=0, w=Speed)
        // [1]: Guid Animation

        // Prepare
        auto& bucket = context.Data->State[node->BucketIndex].MultiBlend;
        const auto range = node->Values[0].AsFloat4();
        const auto speed = (float)tryGetValue(node->GetBox(1), node->Values[1]);
        const auto loop = (bool)tryGetValue(node->GetBox(2), node->Values[2]);
        const auto startTimePos = (float)tryGetValue(node->GetBox(3), node->Values[3]);
        const auto syncLength = false; // TODO: make it configurable via node settings? (change node->Values[2] to contain flags)
        auto& data = node->Data.MultiBlend2D;
        if (data.TrianglesCount == 0)
            break; // Skip if no valid animations added

        // Get axis X
        float x = (float)tryGetValue(node->GetBox(4), Value::Zero);
        x = Math::Clamp(x, range.X, range.Y);

        // Get axis Y
        float y = (float)tryGetValue(node->GetBox(5), Value::Zero);
        y = Math::Clamp(y, range.Z, range.W);

        // Add to trace
        if (context.Data->EnableTracing)
        {
            auto& trace = context.AddTraceEvent(node);
            const Half2 packed(x, y); // Pack xy into 32-bits
            *(uint32*)&trace.Value = *(uint32*)&packed;
        }

        // Check if need to evaluate multi blend length
        if (data.Length < 0)
            ComputeMultiBlendLength(data.Length, node);
        if (data.Length <= ZeroTolerance)
            break;

        MultiBlendAnimData::List prevList, newList;
        MultiBlendAnimData::GetList(bucket, prevList);

        // Find 3 animations to blend (triangle)
        Float2 p(x, y);
        bool hasBest = false;
        Float2 bestPoint;
        float bestWeight = 0.0f;
        byte bestAnims[2];
        for (int32 i = 0, t = 0; i < data.TrianglesCount; i++)
        {
            // Get animations data at vertices
            const auto aIndex = data.Triangles[t++];
            const auto bIndex = data.Triangles[t++];
            const auto cIndex = data.Triangles[t++];
            const auto aData = node->Values[4 + aIndex * 2].AsFloat4();
            const auto bData = node->Values[4 + bIndex * 2].AsFloat4();
            const auto cData = node->Values[4 + cIndex * 2].AsFloat4();
            AnimSampleData a(node->Assets[aIndex].As<Animation>(), aData.W, aIndex);
            AnimSampleData b(node->Assets[bIndex].As<Animation>(), bData.W, bIndex);
            AnimSampleData c(node->Assets[cIndex].As<Animation>(), cData.W, cIndex);
            if (syncLength)
                a.Length = b.Length = c.Length = data.Length;

            // Get triangle coords
            byte anims[3] = { aIndex, bIndex, cIndex };
            Float2 points[3] = {
                Float2(aData.X, aData.Y),
                Float2(bData.X, bData.Y),
                Float2(cData.X, cData.Y)
            };

            // Check if blend using this triangle
            if (CollisionsHelper::IsPointInTriangle(p, points[0], points[1], points[2]))
            {
                if (Float2::DistanceSquared(p, points[0]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex A
                    MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
                    value = SampleAnimation(node, loop, startTimePos, a);
                    MultiBlendAnimData::AfterSample(newList, a);
                    break;
                }
                if (Float2::DistanceSquared(p, points[1]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex B
                    MultiBlendAnimData::BeforeSample(context, bucket, prevList, b, speed);
                    value = SampleAnimation(node, loop, startTimePos, b);
                    MultiBlendAnimData::AfterSample(newList, b);
                    break;
                }
                if (Float2::DistanceSquared(p, points[2]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex C
                    MultiBlendAnimData::BeforeSample(context, bucket, prevList, c, speed);
                    value = SampleAnimation(node, loop, startTimePos, c);
                    MultiBlendAnimData::AfterSample(newList, c);
                    break;
                }

                auto v0 = points[1] - points[0];
                auto v1 = points[2] - points[0];
                auto v2 = p - points[0];

                const float d00 = Float2::Dot(v0, v0);
                const float d01 = Float2::Dot(v0, v1);
                const float d11 = Float2::Dot(v1, v1);
                const float d20 = Float2::Dot(v2, v0);
                const float d21 = Float2::Dot(v2, v1);
                const float coeff = (d00 * d11 - d01 * d01);
                if (Math::IsZero(coeff))
                {
                    const bool xAxis = Math::IsZero(v0.X) && Math::IsZero(v1.X);
                    const bool yAxis = Math::IsZero(v0.Y) && Math::IsZero(v1.Y);
                    if (xAxis && yAxis)
                    {
                        // Single animation
                        MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
                        value = SampleAnimation(node, loop, startTimePos, a);
                        MultiBlendAnimData::AfterSample(newList, a);
                    }
                    else if (xAxis || yAxis)
                    {
                        if (yAxis)
                        {
                            // Use code for X-axis case so swap coordinates
                            Swap(v0.X, v0.Y);
                            Swap(v1.X, v1.Y);
                            Swap(v2.X, v2.Y);
                            Swap(p.X, p.Y);
                        }

                        // Use 1D blend if points are on the same line (degenerated triangle)
                        struct BlendData
                        {
                            float AlphaX, AlphaY;
                            AnimSampleData *SampleA, *SampleB;
                        };
                        BlendData blendData;
                        if (v1.Y >= v0.Y)
                        {
                            if (p.Y < v0.Y && v1.Y >= v0.Y)
                                blendData = { p.Y, v0.Y, &a, &b };
                            else
                                blendData = { p.Y - v0.Y, v1.Y - v0.Y, &b, &c };
                        }
                        else
                        {
                            if (p.Y < v1.Y)
                                blendData = { p.Y, v1.Y, &a, &c };
                            else
                                blendData = { p.Y - v1.Y, v0.Y - v1.Y, &c, &b };
                        }
                        const float alpha = Math::IsZero(blendData.AlphaY) ? 0.0f : blendData.AlphaX / blendData.AlphaY;
                        MultiBlendAnimData::BeforeSample(context, bucket, prevList, *blendData.SampleA, speed);
                        MultiBlendAnimData::BeforeSample(context, bucket, prevList, *blendData.SampleB, speed);
                        value = SampleAnimationsWithBlend(node, loop, startTimePos, *blendData.SampleA, *blendData.SampleB, alpha);
                        MultiBlendAnimData::AfterSample(newList, *blendData.SampleA);
                        MultiBlendAnimData::AfterSample(newList, *blendData.SampleB);
                    }
                    else
                    {
                        // Use only vertex A for invalid triangle
                        MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
                        value = SampleAnimation(node, loop, startTimePos, a);
                        MultiBlendAnimData::AfterSample(newList, a);
                    }
                    break;
                }
                const float v = (d11 * d20 - d01 * d21) / coeff;
                const float w = (d00 * d21 - d01 * d20) / coeff;
                const float u = 1.0f - v - w;

                // Blend A and B and C
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, a, speed);
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, b, speed);
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, c, speed);
                value = SampleAnimationsWithBlend(node, loop, startTimePos, a, b, c, u, v, w);
                MultiBlendAnimData::AfterSample(newList, a);
                MultiBlendAnimData::AfterSample(newList, b);
                MultiBlendAnimData::AfterSample(newList, c);
                break;
            }

            // Try to find the best blend weights for blend position being outside the all triangles (edge case)
            for (int j = 0; j < 3; j++)
            {
                Float2 s[2] = {
                    points[j],
                    points[(j + 1) % 3]
                };
                Float2 closest;
                CollisionsHelper::ClosestPointPointLine(p, s[0], s[1], closest);
                if (!hasBest || Float2::DistanceSquared(closest, p) < Float2::DistanceSquared(bestPoint, p))
                {
                    bestPoint = closest;
                    hasBest = true;

                    const float d = Float2::Distance(s[0], s[1]);
                    bestWeight = d < ANIM_GRAPH_BLEND_THRESHOLD ? 0 : Float2::Distance(s[0], closest) / d;

                    bestAnims[0] = anims[j];
                    bestAnims[1] = anims[(j + 1) % 3];
                }
            }
        }

        // Check if use the closest sample
        if ((void*)value == nullptr && hasBest)
        {
            const auto best0Index = bestAnims[0];
            const auto best1Index = bestAnims[1];
            const auto best0Data = node->Values[4 + best0Index * 2].AsFloat4();
            const auto best1Data = node->Values[4 + best1Index * 2].AsFloat4();
            AnimSampleData best0(node->Assets[best0Index].As<Animation>(), best0Data.W, best0Index);
            AnimSampleData best1(node->Assets[best1Index].As<Animation>(), best1Data.W, best1Index);
            if (syncLength)
                best0.Length = best1.Length = data.Length;

            // Check if use only one sample
            MultiBlendAnimData::BeforeSample(context, bucket, prevList, best0, speed);
            if (bestWeight < ANIM_GRAPH_BLEND_THRESHOLD)
            {
                value = SampleAnimation(node, loop, startTimePos, best0);
            }
            else
            {
                MultiBlendAnimData::BeforeSample(context, bucket, prevList, best1, speed);
                value = SampleAnimationsWithBlend(node, loop, startTimePos, best0, best1, bestWeight);
                MultiBlendAnimData::AfterSample(newList, best1);
            }
            MultiBlendAnimData::AfterSample(newList, best0);
        }

        MultiBlendAnimData::SetList(bucket, newList);
        bucket.LastUpdateFrame = context.CurrentFrameIndex;

        break;
    }
    // Blend Poses
    case 14:
    {
        ASSERT(box->ID == 0);
        const int32 FirstBlendPoseBoxIndex = 3;
        const int32 MaxBlendPoses = 8;
        value = Value::Null;

        // Note data layout:
        // [0]: int Pose Index
        // [1]: float Blend Duration
        // [2]: int Pose Count
        // [3]: AlphaBlendMode Mode

        // Prepare
        auto& bucket = context.Data->State[node->BucketIndex].BlendPose;
        const int32 poseIndex = (int32)tryGetValue(node->GetBox(1), node->Values[0]);
        const float blendDuration = (float)tryGetValue(node->GetBox(2), node->Values[1]);
        const int32 poseCount = Math::Clamp(node->Values[2].AsInt, 0, MaxBlendPoses);
        const AlphaBlendMode mode = (AlphaBlendMode)node->Values[3].AsInt;

        // Skip if nothing to blend
        if (poseCount == 0 || poseIndex < 0 || poseIndex >= poseCount)
        {
            break;
        }

        // Check if transition is not active (first update, pose not changing or transition ended)
        bucket.TransitionPosition += context.DeltaTime;
        if (bucket.PreviousBlendPoseIndex == -1 || bucket.PreviousBlendPoseIndex == poseIndex || bucket.TransitionPosition >= blendDuration || blendDuration <= ANIM_GRAPH_BLEND_THRESHOLD)
        {
            bucket.TransitionPosition = 0.0f;
            bucket.PreviousBlendPoseIndex = poseIndex;
            value = tryGetValue(node->GetBox(FirstBlendPoseBoxIndex + poseIndex), Value::Null);
            break;
        }
        ASSERT(bucket.PreviousBlendPoseIndex >= 0 && bucket.PreviousBlendPoseIndex < poseCount);

        // Blend two animations
        {
            const float alpha = bucket.TransitionPosition / blendDuration;
            const auto valueA = tryGetValue(node->GetBox(FirstBlendPoseBoxIndex + bucket.PreviousBlendPoseIndex), Value::Null);
            const auto valueB = tryGetValue(node->GetBox(FirstBlendPoseBoxIndex + poseIndex), Value::Null);
            value = Blend(node, valueA, valueB, alpha, mode);
        }

        break;
    }
    // Get Root Motion
    case 15:
    {
        auto pose = tryGetValue(node->GetBox(2), Value::Null);
        if (ANIM_GRAPH_IS_VALID_PTR(pose))
        {
            const AnimGraphImpulse* poseData = (AnimGraphImpulse*)pose.AsPointer;
            switch (box->ID)
            {
            case 0:
                value = poseData->RootMotion.Translation;
                break;
            case 1:
                value = poseData->RootMotion.Orientation;
                break;
            }
        }
        else
        {
            switch (box->ID)
            {
            case 0:
                value = Vector3::Zero;
                break;
            case 1:
                value = Quaternion::Identity;
                break;
            }
        }
        break;
    }
    // Set Root Motion
    case 16:
    {
        auto pose = tryGetValue(node->GetBox(1), Value::Null);
        if (!ANIM_GRAPH_IS_VALID_PTR(pose))
        {
            value = pose;
            break;
        }
        const AnimGraphImpulse* poseData = (AnimGraphImpulse*)pose.AsPointer;

        auto nodes = node->GetNodes(this);
        *nodes = *poseData;
        nodes->RootMotion.Translation = (Vector3)tryGetValue(node->GetBox(2), Value::Zero);
        nodes->RootMotion.Orientation = (Quaternion)tryGetValue(node->GetBox(3), Value::Zero);
        value = nodes;
        break;
    }
    // Add Root Motion
    case 17:
    {
        auto pose = tryGetValue(node->GetBox(1), Value::Null);
        if (!ANIM_GRAPH_IS_VALID_PTR(pose))
        {
            value = pose;
            break;
        }
        const AnimGraphImpulse* poseData = (AnimGraphImpulse*)pose.AsPointer;

        auto nodes = node->GetNodes(this);
        *nodes = *poseData;
        nodes->RootMotion.Translation = poseData->RootMotion.Translation + (Vector3)tryGetValue(node->GetBox(2), Value::Zero);
        nodes->RootMotion.Orientation = poseData->RootMotion.Orientation * (Quaternion)tryGetValue(node->GetBox(3), Value::Zero);
        value = nodes;
        break;
    }
    // State Machine
    case 18:
    {
        ANIM_GRAPH_PROFILE_EVENT("State Machine");
        const int32 maxTransitionsPerUpdate = node->Values[2].AsInt;
        const bool reinitializeOnBecomingRelevant = node->Values[3].AsBool;
        const bool skipFirstUpdateTransition = node->Values[4].AsBool;

        // Prepare
        auto& bucket = context.Data->State[node->BucketIndex].StateMachine;
        auto& data = node->Data.StateMachine;
        int32 transitionsLeft = maxTransitionsPerUpdate == 0 ? MAX_uint16 : maxTransitionsPerUpdate;
        bool isFirstUpdate = bucket.LastUpdateFrame == 0 || bucket.CurrentState == nullptr;
        if (bucket.LastUpdateFrame != context.CurrentFrameIndex - 1 && reinitializeOnBecomingRelevant)
        {
            // Reset on becoming relevant
            isFirstUpdate = true;
        }
        if (isFirstUpdate && skipFirstUpdateTransition)
            transitionsLeft = 0;

        // Initialize on the first update
        if (isFirstUpdate)
        {
            // Ensure to have valid state machine graph
            if (data.Graph == nullptr || data.Graph->GetRootNode() == nullptr)
            {
                value = Value::Null;
                break;
            }

            // Enter to the first state pointed by the Entry node (without transitions)
            bucket.CurrentState = data.Graph->GetRootNode();
            InitStateTransition(context, bucket);

            // Reset all state buckets of the graphs and nodes included inside the state machine
            ResetBuckets(context, data.Graph);
        }
#define END_TRANSITION() \
    ResetBuckets(context, bucket.CurrentState->Data.State.Graph); \
    bucket.CurrentState = bucket.ActiveTransition->Destination; \
    InitStateTransition(context, bucket)

        context.NodePath.Push(node->ID);

        // Update the active transition
        if (bucket.ActiveTransition)
        {
            bucket.TransitionPosition += context.DeltaTime;
            ASSERT(bucket.CurrentState);

            // Check for transition end
            if (bucket.TransitionPosition >= bucket.ActiveTransition->BlendDuration)
            {
                END_TRANSITION();
            }
            // Check for transition interruption
            else if (EnumHasAnyFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionRuleRechecking) &&
                EnumHasNoneFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::UseDefaultRule) &&
                bucket.ActiveTransition->RuleGraph)
            {
                // Execute transition rule
                auto rootNode = bucket.ActiveTransition->RuleGraph->GetRootNode();
                if (!(bool)eatBox((Node*)rootNode, &rootNode->Boxes[0]))
                {
                    bool cancelTransition = false;
                    if (EnumHasAnyFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionInstant))
                    {
                        cancelTransition = true;
                    }
                    else
                    {
                        // Blend back to the source state (remove currently applied delta and rewind transition)
                        bucket.TransitionPosition -= context.DeltaTime;
                        bucket.TransitionPosition -= context.DeltaTime;
                        if (bucket.TransitionPosition <= ZeroTolerance)
                        {
                            cancelTransition = true;
                        }
                    }
                    if (cancelTransition)
                    {
                        // Go back to the source state
                        ResetBuckets(context, bucket.CurrentState->Data.State.Graph);
                        InitStateTransition(context, bucket);
                    }
                }
            }
            if (bucket.ActiveTransition && !bucket.BaseTransition && EnumHasAnyFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionSourceState))
            {
                // Try to interrupt with any other transition in the source state (except the current transition)
                if (AnimGraphStateTransition* transition = UpdateStateTransitions(context, data, bucket.CurrentState, bucket.ActiveTransition->Destination))
                {
                    // Change active transition to the interrupted one
                    if (EnumHasNoneFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionInstant))
                    {
                        // Cache the current blending state to be used as a base when blending towards new destination state (seamless blending after interruption)
                        bucket.BaseTransition = bucket.ActiveTransition;
                        bucket.BaseTransitionState = bucket.CurrentState;
                        bucket.BaseTransitionPosition = bucket.TransitionPosition;
                    }
                    bucket.ActiveTransition = transition;
                    bucket.TransitionPosition = 0.0f;
                }
            }
            if (bucket.ActiveTransition && !bucket.BaseTransition && EnumHasAnyFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionDestinationState))
            {
                // Try to interrupt with any other transition in the destination state (except the transition back to the current state if exists)
                if (AnimGraphStateTransition* transition = UpdateStateTransitions(context, data, bucket.ActiveTransition->Destination, bucket.CurrentState))
                {
                    // Change active transition to the interrupted one
                    if (EnumHasNoneFlags(bucket.ActiveTransition->Flags, AnimGraphStateTransition::FlagTypes::InterruptionInstant))
                    {
                        // Cache the current blending state to be used as a base when blending towards new destination state (seamless blending after interruption)
                        bucket.BaseTransition = bucket.ActiveTransition;
                        bucket.BaseTransitionState = bucket.CurrentState;
                        bucket.BaseTransitionPosition = bucket.TransitionPosition;
                    }
                    bucket.CurrentState = bucket.ActiveTransition->Destination;
                    bucket.ActiveTransition = transition;
                    bucket.TransitionPosition = 0.0f;
                }
            }
        }

        ASSERT(bucket.CurrentState && bucket.CurrentState->Type == GRAPH_NODE_MAKE_TYPE(9, 20));

        // Update transitions
        // Note: this logic assumes that all transitions are sorted by Order property and Enabled (by Editor when saving Anim Graph asset)
        while (!bucket.ActiveTransition && transitionsLeft-- > 0)
        {
            // State transitions
            UpdateStateTransitions(context, data, bucket, bucket.CurrentState->Data.State);

            // Any state transitions
            // TODO: cache Any state nodes inside State Machine to optimize the loop below
            for (const AnimGraphNode& anyStateNode : data.Graph->Nodes)
            {
                if (anyStateNode.Type == GRAPH_NODE_MAKE_TYPE(9, 34))
                    UpdateStateTransitions(context, data, bucket, anyStateNode.Data.AnyState);
            }

            // Check for instant transitions
            if (bucket.ActiveTransition && bucket.ActiveTransition->BlendDuration <= ZeroTolerance)
            {
                END_TRANSITION();
            }
        }

        if (bucket.BaseTransitionState)
        {
            // Sample the other state (eg. when blending from interrupted state to the another state from the old destination)
            value = SampleState(context, bucket.BaseTransitionState);
            if (bucket.BaseTransition)
            {
                // Evaluate the base pose from the time when transition was interrupted
                const auto destinationState = SampleState(context, bucket.BaseTransition->Destination);
                const float alpha = bucket.BaseTransitionPosition / bucket.BaseTransition->BlendDuration;
                value = Blend(node, value, destinationState, alpha, bucket.BaseTransition->BlendMode);
            }
        }
        else
        {
            // Sample the current state
            value = SampleState(context, bucket.CurrentState);
        }

        // Handle active transition blending
        if (bucket.ActiveTransition)
        {
            // Sample the active transition destination state
            const auto destinationState = SampleState(context, bucket.ActiveTransition->Destination);

            // Perform blending
            const float alpha = bucket.TransitionPosition / bucket.ActiveTransition->BlendDuration;
            value = Blend(node, value, destinationState, alpha, bucket.ActiveTransition->BlendMode);
        }

        bucket.LastUpdateFrame = context.CurrentFrameIndex;
        context.NodePath.Pop();
#undef END_TRANSITION
        break;
    }
    // Entry
    case 19:
    // State
    case 20:
    // Any State
    case 34:
    {
        // Not used
        CRASH;
        break;
    }
    // State Output
    case 21:
    // Rule Output
    case 22:
        value = box->HasConnection() ? eatBox(nodeBase, box->FirstConnection()) : Value::Null;
        break;
    // Transition Source State Anim
    case 23:
    {
        const AnimGraphTransitionData& transitionsData = context.TransitionData;
        switch (box->ID)
        {
        // Length
        case 0:
            value = transitionsData.Length;
            break;
        // Time
        case 1:
            value = transitionsData.Position;
            break;
        // Normalized Time
        case 2:
            value = transitionsData.Position / transitionsData.Length;
            break;
        // Remaining Time
        case 3:
            value = transitionsData.Length - transitionsData.Position;
            break;
        // Remaining Normalized Time
        case 4:
            value = 1.0f - (transitionsData.Position / transitionsData.Length);
            break;
        default: CRASH;
            break;
        }
        break;
    }
    // Animation Graph Function
    case 24:
    {
        // Load function graph
        auto function = node->Assets[0].As<AnimationGraphFunction>();
        auto& data = node->Data.AnimationGraphFunction;
        if (data.Graph == nullptr)
        {
            value = Value::Zero;
            break;
        }

#if 0
        // Prevent recursive calls
        for (int32 i = context.CallStack.Count() - 1; i >= 0; i--)
        {
            if (context.CallStack[i]->Type == GRAPH_NODE_MAKE_TYPE(9, 24))
            {
                const auto callFunc = context.CallStack[i]->Assets[0].Get();
                if (callFunc == function)
                {
                    value = Value::Zero;
                    context.ValueCache.Add(boxBase, value);
                    return;
                }
            }
        }
#endif

        // Peek the function output (function->Outputs maps the functions outputs to output nodes indices)
        // This assumes that Function Output nodes are allowed to be only in the root graph (not in state machine sub-graphs)
        const int32 outputIndex = box->ID - 16;
        if (outputIndex < 0 || outputIndex >= function->Outputs.Count())
        {
            value = Value::Zero;
            break;
        }
        Node* functionOutputNode = (Node*)&data.Graph->Nodes[function->Outputs[outputIndex].NodeIndex];
        Box* functionOutputBox = functionOutputNode->TryGetBox(0);

        // Cache relation between current node in the call stack to the actual function graph
        context.Functions[nodeBase] = (Graph*)data.Graph;

        // Evaluate the function output
        context.GraphStack.Push((Graph*)data.Graph);
        value = functionOutputBox && functionOutputBox->HasConnection() ? eatBox(nodeBase, functionOutputBox->FirstConnection()) : Value::Zero;
        context.GraphStack.Pop();
        break;
    }
    // Transform Bone (local/model space)
    case 25:
    case 26:
    {
        const auto inputBox = node->GetBox(1);
        const auto nodeIndex = node->Data.TransformNode.NodeIndex;
        const auto transformMode = static_cast<BoneTransformMode>((int32)node->Values[1]);

        // Get the transformation
        Transform transform;
        transform.Translation = (Vector3)tryGetValue(node->GetBox(2), Vector3::Zero);
        transform.Orientation = (Quaternion)tryGetValue(node->GetBox(3), Quaternion::Identity);
        transform.Scale = (Float3)tryGetValue(node->GetBox(4), Float3::One);

        // Skip if no change will be performed
        if (nodeIndex < 0 || nodeIndex >= _skeletonNodesCount || transformMode == BoneTransformMode::None || (transformMode == BoneTransformMode::Add && transform.IsIdentity()))
        {
            // Pass through the input
            value = Value::Null;
            if (inputBox->HasConnection())
                value = eatBox(nodeBase, inputBox->FirstConnection());
            context.ValueCache.Add(boxBase, value);
            return;
        }
        const auto nodes = node->GetNodes(this);

        // Prepare the input nodes
        bool hasValidInput = false;
        if (inputBox->HasConnection())
        {
            const auto input = eatBox(nodeBase, inputBox->FirstConnection());
            hasValidInput = ANIM_GRAPH_IS_VALID_PTR(input);
            if (hasValidInput)
                CopyNodes(nodes, input);
        }
        if (!hasValidInput)
            InitNodes(nodes);

        if (node->TypeID == 25)
        {
            // Local space
            if (transformMode == BoneTransformMode::Add)
                nodes->Nodes[nodeIndex] = nodes->Nodes[nodeIndex] + transform;
            else
                nodes->Nodes[nodeIndex] = transform;
        }
        else
        {
            // Global space
            if (transformMode == BoneTransformMode::Add)
                nodes->SetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex, nodes->GetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex) + transform);
            else
                nodes->SetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex, transform);
        }

        value = nodes;
        break;
    }
    // Copy Node
    case 27:
    {
        // Get input
        auto input = tryGetValue(node->GetBox(1), Value::Null);
        const auto nodes = node->GetNodes(this);
        if (ANIM_GRAPH_IS_VALID_PTR(input))
        {
            // Use input nodes
            CopyNodes(nodes, input);
        }
        else
        {
            // Use default nodes
            InitNodes(nodes);
            input = nodes;
        }

        // Fetch the settings
        const auto srcNodeIndex = node->Data.CopyNode.SrcNodeIndex;
        const auto dstNodeIndex = node->Data.CopyNode.DstNodeIndex;
        const auto copyTranslation = (bool)node->Values[2];
        const auto copyRotation = (bool)node->Values[3];
        const auto copyScale = (bool)node->Values[4];

        // Skip if no change will be performed
        if (srcNodeIndex < 0 || srcNodeIndex >= _skeletonNodesCount ||
            dstNodeIndex < 0 || dstNodeIndex >= _skeletonNodesCount ||
            !(copyTranslation || copyRotation || copyScale))
        {
            // Pass through the input
            value = input;
            context.ValueCache.Add(boxBase, value);
            return;
        }

        // Copy bone data
        Transform& srcTransform = nodes->Nodes[srcNodeIndex];
        Transform& dstTransform = nodes->Nodes[dstNodeIndex];
        if (copyTranslation)
            dstTransform.Translation = srcTransform.Translation;
        if (copyRotation)
            dstTransform.Orientation = srcTransform.Orientation;
        if (copyScale)
            dstTransform.Scale = srcTransform.Scale;

        value = nodes;
        break;
    }
    // Get Node Transform (model space)
    case 28:
    {
        // Get input
        const auto nodeIndex = node->Data.TransformNode.NodeIndex;
        const auto input = tryGetValue(node->GetBox(0), Value::Null);
        if (ANIM_GRAPH_IS_VALID_PTR(input) && nodeIndex >= 0 && nodeIndex < _skeletonNodesCount)
            value = Variant(((AnimGraphImpulse*)input.AsPointer)->GetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex));
        else
            value = Variant(Transform::Identity);
        break;
    }
    // Aim IK
    case 29:
    {
        // Get input
        auto input = tryGetValue(node->GetBox(1), Value::Null);
        const auto nodeIndex = node->Data.TransformNode.NodeIndex;
        float weight = (float)tryGetValue(node->GetBox(3), node->Values[1]);
        if (nodeIndex < 0 || nodeIndex >= _skeletonNodesCount || weight < ANIM_GRAPH_BLEND_THRESHOLD)
        {
            // Pass through the input
            value = input;
            break;
        }
        const auto nodes = node->GetNodes(this);
        if (ANIM_GRAPH_IS_VALID_PTR(input))
        {
            // Use input nodes
            CopyNodes(nodes, input);
        }
        else
        {
            // Use default nodes
            InitNodes(nodes);
            input = nodes;
        }
        const Vector3 target = (Vector3)tryGetValue(node->GetBox(2), Vector3::Zero);
        weight = Math::Saturate(weight);

        // Solve IK
        Transform nodeTransformModelSpace = nodes->GetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex);
        Quaternion nodeCorrection;
        InverseKinematics::SolveAimIK(nodeTransformModelSpace, target, nodeCorrection);

        // Apply IK
        auto bindPoseNodeTransformation = GetEmptyNodes()->GetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex);
        Quaternion newRotation = nodeCorrection * bindPoseNodeTransformation.Orientation;
        if (weight < 1.0f)
            Quaternion::Slerp(nodeTransformModelSpace.Orientation, newRotation, weight, nodeTransformModelSpace.Orientation);
        else
            nodeTransformModelSpace.Orientation = newRotation;
        nodes->SetNodeModelTransformation(_graph.BaseModel->Skeleton, nodeIndex, nodeTransformModelSpace);

        value = nodes;
        break;
    }
    // Get Node Transform (local space)
    case 30:
    {
        // Get input
        const auto nodeIndex = node->Data.TransformNode.NodeIndex;
        const auto input = tryGetValue(node->GetBox(0), Value::Null);
        if (ANIM_GRAPH_IS_VALID_PTR(input) && nodeIndex >= 0 && nodeIndex < _skeletonNodesCount)
            value = Variant(((AnimGraphImpulse*)input.AsPointer)->GetNodeLocalTransformation(_graph.BaseModel->Skeleton, nodeIndex));
        else
            value = Variant(Transform::Identity);
        break;
    }
    // Two Bone IK
    case 31:
    {
        // Get input
        auto input = tryGetValue(node->GetBox(1), Value::Null);
        const auto nodeIndex = node->Data.TransformNode.NodeIndex;
        float weight = (float)tryGetValue(node->GetBox(4), node->Values[1]);
        if (nodeIndex < 0 || nodeIndex >= _skeletonNodesCount || weight < ANIM_GRAPH_BLEND_THRESHOLD)
        {
            // Pass through the input
            value = input;
            break;
        }
        const auto nodes = node->GetNodes(this);
        if (ANIM_GRAPH_IS_VALID_PTR(input))
        {
            // Use input nodes
            CopyNodes(nodes, input);
        }
        else
        {
            // Use default nodes
            InitNodes(nodes);
            input = nodes;
        }
        const Vector3 target = (Vector3)tryGetValue(node->GetBox(2), Vector3::Zero);
        const Vector3 jointTarget = (Vector3)tryGetValue(node->GetBox(3), Vector3::Zero);
        const bool allowStretching = (bool)tryGetValue(node->GetBox(5), node->Values[2]);
        const float maxStretchScale = (float)tryGetValue(node->GetBox(6), node->Values[3]);
        weight = Math::Saturate(weight);

        // Solve IK
        int32 jointNodeIndex = _graph.BaseModel->Skeleton.Nodes[nodeIndex].ParentIndex;
        if (jointNodeIndex == -1)
        {
            value = input;
            break;
        }
        int32 rootNodeIndex = _graph.BaseModel->Skeleton.Nodes[jointNodeIndex].ParentIndex;
        if (rootNodeIndex == -1)
        {
            value = input;
            break;
        }
        Transform rootTransformLocalSpace = nodes->Nodes[rootNodeIndex];
        Transform jointTransformLocalSpace = nodes->Nodes[jointNodeIndex];
        Transform nodeTransformLocalSpace = nodes->Nodes[nodeIndex];
        Transform rootTransformModelSpace = nodes->GetNodeModelTransformation(_graph.BaseModel->Skeleton, rootNodeIndex);
        Transform jointTransformModelSpace = rootTransformModelSpace.LocalToWorld(jointTransformLocalSpace);
        Transform targetTransformModelSpace = jointTransformModelSpace.LocalToWorld(nodeTransformLocalSpace);
        InverseKinematics::SolveTwoBoneIK(rootTransformModelSpace, jointTransformModelSpace, targetTransformModelSpace, target, jointTarget, allowStretching, maxStretchScale);

        // Apply IK
        nodes->SetNodeModelTransformation(_graph.BaseModel->Skeleton, rootNodeIndex, rootTransformModelSpace);
        rootTransformModelSpace.WorldToLocal(jointTransformModelSpace, nodes->Nodes[jointNodeIndex]);
        jointTransformModelSpace.WorldToLocal(targetTransformModelSpace, nodes->Nodes[nodeIndex]);
        if (weight < 1.0f)
        {
            Transform::Lerp(rootTransformLocalSpace, nodes->Nodes[rootNodeIndex], weight, nodes->Nodes[rootNodeIndex]);
            Transform::Lerp(jointTransformLocalSpace, nodes->Nodes[jointNodeIndex], weight, nodes->Nodes[jointNodeIndex]);
            Transform::Lerp(nodeTransformLocalSpace, nodes->Nodes[nodeIndex], weight, nodes->Nodes[nodeIndex]);
        }

        value = nodes;
        break;
    }
    // Animation Slot
    case 32:
    {
        auto& slots = context.Data->Slots;
        if (slots.Count() == 0)
        {
            value = tryGetValue(node->GetBox(1), Value::Null);
            return;
        }
        const StringView slotName(node->Values[0]);
        auto& bucket = context.Data->State[node->BucketIndex].Slot;
        if (bucket.Index != -1 && (slots.Count() <= bucket.Index || slots[bucket.Index].Animation == nullptr))
        {
            // Current slot animation ended
            bucket.Index = -1;
        }
        if (bucket.Index == -1)
        {
            // Pick the animation to play
            for (int32 i = 0; i < slots.Count(); i++)
            {
                auto& slot = slots[i];
                if (slot.Animation && slot.Name == slotName)
                {
                    // Start playing animation
                    bucket.Index = i;
                    // Keep bucket time position and blend in time for if blending between two anims in the same slot.
                    bucket.TimePosition = bucket.TimePosition;
                    bucket.BlendInPosition = bucket.BlendInPosition;
                    bucket.BlendOutPosition = 0.0f;
                    bucket.LoopsDone = 0;
                    bucket.LoopsLeft = slot.LoopCount;
                    break;
                }
            }
            if (bucket.Index == -1 || !slots[bucket.Index].Animation->IsLoaded())
            {
                value = tryGetValue(node->GetBox(1), Value::Null);
                // Reset times if time is left over from playing between different anims in the same slot.
                if (bucket.BlendInPosition > 0)
                {
                    bucket.TimePosition = 0;
                    bucket.BlendInPosition = 0;
                }
                return;
            }
        }

        // Play the animation
        auto& slot = slots[bucket.Index];
        Animation* anim = slot.Animation;
        ASSERT(slot.Animation && slot.Animation->IsLoaded());
        const float deltaTime = slot.Pause ? 0.0f : context.DeltaTime * slot.Speed;
        const float length = anim->GetLength();
        const bool loop = bucket.LoopsLeft != 0;
        float newTimePos = bucket.TimePosition + deltaTime;
        if (newTimePos >= length)
        {
            if (bucket.LoopsLeft == 0)
            {
                // End playing animation
                value = tryGetValue(node->GetBox(1), Value::Null);
                bucket.Index = -1;
                slot.Animation = nullptr;
                return;
            }

            // Loop animation
            if (bucket.LoopsLeft > 0)
                bucket.LoopsLeft--;
            bucket.LoopsDone++;
        }
        // Speed is accounted for in the new time pos, so keep sample speed at 1
        value = SampleAnimation(node, loop, length, 0.0f, bucket.TimePosition, newTimePos, anim, 1);
        bucket.TimePosition = newTimePos;

        // On animation slot stop
        if (slot.Reset)
        {
            // Blend between last anim and new anim if found, otherwise blend back to input.
            Animation* sAnim = nullptr;
            for (int32 i = 0; i < slots.Count(); i++)
            {
                if (bucket.Index == i)
                    continue;

                auto& s = slots[i];
                if (s.Animation && s.Name == slotName)
                {
                    sAnim = s.Animation;
                }
            }
            float oldTimePos = bucket.BlendOutPosition;
            bucket.BlendOutPosition += deltaTime;
            bucket.BlendInPosition = bucket.BlendOutPosition;
            const float alpha = bucket.BlendOutPosition / slot.BlendOutTime;
            if (sAnim != nullptr)
            {
                auto sValue = SampleAnimation(node, false, sAnim->GetLength(), 0.0f, oldTimePos, bucket.BlendInPosition, sAnim, 1);
                //value = SampleAnimationsWithBlend(node, false, length, 0.0f, bucket.TimePosition, newTimePos, anim, sAnim, 1, 1, alpha);
                value = Blend(node, value, sValue, alpha, AlphaBlendMode::HermiteCubic);
            }
            else
            {
                auto input = tryGetValue(node->GetBox(1), Value::Null);
                value = Blend(node, value, input, alpha, AlphaBlendMode::HermiteCubic);
            }

            if (bucket.BlendOutPosition >= slot.BlendOutTime)
            {
                // Start from the beginning or the blend in position if next anim found.
                slot.Animation = nullptr;
                slot.Reset = false;
                if (!sAnim)
                {
                    bucket.TimePosition = 0;
                    bucket.BlendInPosition = 0;
                }
                else
                {
                    bucket.TimePosition = bucket.BlendInPosition;
                }
            }
            break;
        }

        if (bucket.LoopsLeft == 0 && slot.BlendOutTime > 0.0f && length - slot.BlendOutTime < bucket.TimePosition)
        {
            // Blend out
            auto input = tryGetValue(node->GetBox(1), Value::Null);
            bucket.BlendOutPosition += deltaTime;
            const float alpha = bucket.BlendOutPosition / slot.BlendOutTime;
            value = Blend(node, value, input, alpha, AlphaBlendMode::HermiteCubic);
        }
        else if (bucket.LoopsDone == 0 && slot.BlendInTime > 0.0f && bucket.BlendInPosition < slot.BlendInTime)
        {
            // Blend in
            auto input = tryGetValue(node->GetBox(1), Value::Null);
            bucket.BlendInPosition += deltaTime;
            const float alpha = bucket.BlendInPosition / slot.BlendInTime;
            value = Blend(node, input, value, alpha, AlphaBlendMode::HermiteCubic);
        }
        break;
    }
    // Animation Instance Data
    case 33:
    {
        auto& bucket = context.Data->State[node->BucketIndex].InstanceData;
        if (bucket.Init)
        {
            bucket.Init = false;
            *(Float4*)bucket.Data = (Float4)tryGetValue(node->GetBox(1), Value::Zero);
        }
        value = *(Float4*)bucket.Data;
        break;
    }
    default:
        break;
    }
    context.ValueCache[boxBase] = value;
}

void AnimGraphExecutor::ProcessGroupFunction(Box* boxBase, Node* node, Value& value)
{
    auto& context = *Context.Get();
    if (context.ValueCache.TryGet(boxBase, value))
        return;
    switch (node->TypeID)
    {
    // Function Input
    case 1:
    {
        // Find the function call
        AnimGraphNode* functionCallNode = nullptr;
        ASSERT(context.GraphStack.Count() >= 2);
        Graph* graph;
        for (int32 i = context.CallStack.Count() - 1; i >= 0; i--)
        {
            if (context.CallStack[i]->Type == GRAPH_NODE_MAKE_TYPE(9, 24) && context.Functions.TryGet(context.CallStack[i], graph) && context.GraphStack.Last() == graph)
            {
                functionCallNode = (AnimGraphNode*)context.CallStack[i];
                break;
            }
        }
        if (!functionCallNode)
        {
            value = Value::Zero;
            break;
        }
        const auto function = functionCallNode->Assets[0].As<AnimationGraphFunction>();
        auto& data = functionCallNode->Data.AnimationGraphFunction;
        if (data.Graph == nullptr)
        {
            value = Value::Zero;
            break;
        }

        // Peek the input box to use
        int32 inputIndex = -1;
        const auto name = (StringView)node->Values[1];
        for (int32 i = 0; i < function->Inputs.Count(); i++)
        {
            auto& input = function->Inputs[i];
            if (input.Name == name)
            {
                inputIndex = input.InputIndex;
                break;
            }
        }
        if (inputIndex < 0 || inputIndex >= function->Inputs.Count())
        {
            value = Value::Zero;
            break;
        }
        Box* functionCallBox = functionCallNode->TryGetBox(inputIndex);
        if (functionCallBox && functionCallBox->HasConnection())
        {
            // Use provided input value from the function call
            context.GraphStack.Pop();
            value = eatBox(node, functionCallBox->FirstConnection());
            context.GraphStack.Push(graph);
        }
        else
        {
            // Use the default value from the function graph
            value = tryGetValue(node->TryGetBox(1), Value::Zero);
        }
        context.ValueCache.Add(boxBase, value);
        break;
    }
    default:
        break;
    }
}
