// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "AnimGraph.h"
#include "Engine/Content/Assets/Animation.h"
#include "Engine/Content/Assets/SkeletonMask.h"
#include "Engine/Content/Assets/AnimationGraphFunction.h"
#include "Engine/Animations/AlphaBlend.h"
#include "Engine/Animations/InverseKinematics.h"

int32 AnimGraphExecutor::GetRootNodeIndex(Animation* anim)
{
    // TODO: cache the root node index (use dictionary with Animation* -> int32 for fast lookups)
    int32 rootNodeIndex = 0;
    if (anim->Data.RootNodeName.HasChars())
    {
        auto& skeleton = _graph.BaseModel->Skeleton;
        for (int32 i = 0; i < _skeletonNodesCount; i++)
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

void AnimGraphExecutor::UpdateRootMotion(const Animation::NodeToChannel* mapping, Animation* anim, float pos, float prevPos, Transform& rootNode, RootMotionData& rootMotion)
{
    // Pick node
    const int32 rootNodeIndex = GetRootNodeIndex(anim);

    // Extract it's current motion
    const Transform rootNow = rootNode;
    const Transform refPose = GetEmptyNodes()->Nodes[rootNodeIndex];
    rootNode = refPose;

    // Apply
    const int32 nodeToChannel = mapping->At(rootNodeIndex);
    if (_rootMotionMode == RootMotionMode::Enable && nodeToChannel != -1)
    {
        const NodeAnimationData& rootChannel = anim->Data.Channels[nodeToChannel];

        // Get the root bone transformation in the previous update
        Transform rootBefore = refPose;
        rootChannel.Evaluate(prevPos, &rootBefore, false);

        // Check if animation looped
        if (pos < prevPos)
        {
            const float length = anim->GetLength();
            const float endPos = length * static_cast<float>(anim->Data.FramesPerSecond);
            const float timeToEnd = endPos - prevPos;

            Transform rootBegin = refPose;
            rootChannel.Evaluate(0, &rootBegin, false);

            Transform rootEnd = refPose;
            rootChannel.Evaluate(endPos, &rootEnd, false);

            //rootChannel.Evaluate(pos - timeToEnd, &rootNow, true);

            // Complex motion calculation to preserve the looped movement
            // (end - before + now - begin)
            // It sums the motion since the last update to anim end and since the start to now
            rootMotion.Translation = rootEnd.Translation - rootBefore.Translation + rootNow.Translation - rootBegin.Translation;
            rootMotion.Rotation = rootEnd.Orientation * rootBefore.Orientation.Conjugated() * (rootNow.Orientation * rootBegin.Orientation.Conjugated());
            //rootMotion.Rotation = Quaternion::Identity;
        }
        else
        {
            // Simple motion delta
            // (now - before)
            rootMotion.Translation = rootNow.Translation - rootBefore.Translation;
            rootMotion.Rotation = rootBefore.Orientation.Conjugated() * rootNow.Orientation;
        }
    }
}

float GetAnimPos(float& timePos, float startTimePos, bool loop, float length)
{
    // Apply animation offset and looping to calculate the animation sampling position within [0;length]
    float result = startTimePos + timePos;
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
        }
        else
        {
            // Animation ended
            result = length;
        }
        timePos = result;
    }
    return result;
}

float GetAnimSamplePos(float length, Animation* anim, float pos, float speed)
{
    // Convert into animation local time (track length may be bigger so fill the gaps with animation clip and include playback speed)
    // Also, scale the animation to fit the total animation node length without cut in a middle
    const auto animLength = anim->GetLength();
    const int32 cyclesCount = Math::FloorToInt(length / animLength);
    const float cycleLength = animLength * cyclesCount;
    const float adjustRateScale = length / cycleLength;
    auto animPos = pos * speed * adjustRateScale;
    while (animPos > animLength)
    {
        animPos -= animLength;
    }
    if (animPos < 0)
        animPos = animLength + animPos;
    animPos *= static_cast<float>(anim->Data.FramesPerSecond);
    return animPos;
}

Variant AnimGraphExecutor::SampleAnimation(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* anim, float speed)
{
    // Skip if animation is not ready to use
    if (anim == nullptr || !anim->IsLoaded())
        return Value::Null;

    // Calculate actual time position within the animation node (defined by length and loop mode)
    const float pos = GetAnimPos(newTimePos, startTimePos, loop, length);
    const float prevPos = GetAnimPos(prevTimePos, startTimePos, loop, length);

    // Get animation position (animation track position for channels sampling)
    const float animPos = GetAnimSamplePos(length, anim, pos, speed);
    const float animPrevPos = GetAnimSamplePos(length, anim, prevPos, speed);

    // Sample the animation
    const auto nodes = node->GetNodes(this);
    nodes->RootMotion = RootMotionData::Identity;
    nodes->Position = pos;
    nodes->Length = length;
    const auto mapping = anim->GetMapping(_graph.BaseModel);
    for (int32 i = 0; i < _skeletonNodesCount; i++)
    {
        const int32 nodeToChannel = mapping->At(i);
        InitNode(nodes, i);
        if (nodeToChannel != -1)
        {
            // Calculate the animated node transformation
            anim->Data.Channels[nodeToChannel].Evaluate(animPos, &nodes->Nodes[i], false);
        }
    }

    // Handle root motion
    if (anim->Data.EnableRootMotion && _rootMotionMode != RootMotionMode::NoExtraction)
    {
        const int32 rootNodeIndex = GetRootNodeIndex(anim);
        UpdateRootMotion(mapping, anim, animPos, animPrevPos, nodes->Nodes[rootNodeIndex], nodes->RootMotion);
    }

    return nodes;
}

Variant AnimGraphExecutor::SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* animA, Animation* animB, float speedA, float speedB, float alpha)
{
    // Skip if any animation is not ready to use
    if (animA == nullptr || !animA->IsLoaded() ||
        animB == nullptr || !animB->IsLoaded())
        return Value::Null;

    // Calculate actual time position within the animation node (defined by length and loop mode)
    const float pos = GetAnimPos(newTimePos, startTimePos, loop, length);
    const float prevPos = GetAnimPos(prevTimePos, startTimePos, loop, length);

    // Get animation position (animation track position for channels sampling)
    const float animPosA = GetAnimSamplePos(length, animA, pos, speedA);
    const float animPrevPosA = GetAnimSamplePos(length, animA, prevPos, speedA);
    const float animPosB = GetAnimSamplePos(length, animB, pos, speedB);
    const float animPrevPosB = GetAnimSamplePos(length, animB, prevPos, speedB);

    // Sample the animations with blending
    const auto nodes = node->GetNodes(this);
    nodes->RootMotion = RootMotionData::Identity;
    nodes->Position = pos;
    nodes->Length = length;
    const auto mappingA = animA->GetMapping(_graph.BaseModel);
    const auto mappingB = animB->GetMapping(_graph.BaseModel);
    for (int32 i = 0; i < _skeletonNodesCount; i++)
    {
        const int32 nodeToChannelA = mappingA->At(i);
        const int32 nodeToChannelB = mappingB->At(i);
        Transform nodeA = GetEmptyNodes()->Nodes[i];
        Transform nodeB = nodeA;

        // Calculate the animated node transformations
        if (nodeToChannelA != -1)
        {
            animA->Data.Channels[nodeToChannelA].Evaluate(animPosA, &nodeA, false);
        }
        if (nodeToChannelB != -1)
        {
            animB->Data.Channels[nodeToChannelB].Evaluate(animPosB, &nodeB, false);
        }

        // Blend
        Transform::Lerp(nodeA, nodeB, alpha, nodes->Nodes[i]);
    }

    // Handle root motion
    if (_rootMotionMode != RootMotionMode::NoExtraction)
    {
        // Extract root motion from animation A
        if (animA->Data.EnableRootMotion)
        {
            RootMotionData rootMotion = RootMotionData::Identity;
            const int32 rootNodeIndex = GetRootNodeIndex(animA);
            const int32 nodeToChannel = mappingA->At(rootNodeIndex);
            Transform rootNode = Transform::Identity;
            if (nodeToChannel != -1)
            {
                animA->Data.Channels[nodeToChannel].Evaluate(animPosA, &rootNode, false);
            }
            UpdateRootMotion(mappingA, animA, animPosA, animPrevPosA, rootNode, rootMotion);
            RootMotionData::Lerp(nodes->RootMotion, rootMotion, 1.0f - alpha, nodes->RootMotion);
            Transform::Lerp(nodes->Nodes[rootNodeIndex], rootNode, 1.0f - alpha, nodes->Nodes[rootNodeIndex]);
        }

        // Extract root motion from animation B
        if (animB->Data.EnableRootMotion)
        {
            RootMotionData rootMotion = RootMotionData::Identity;
            const int32 rootNodeIndex = GetRootNodeIndex(animA);
            const int32 nodeToChannel = mappingB->At(rootNodeIndex);
            Transform rootNode = Transform::Identity;
            if (nodeToChannel != -1)
            {
                animB->Data.Channels[nodeToChannel].Evaluate(animPosB, &rootNode, false);
            }
            UpdateRootMotion(mappingB, animB, animPosB, animPrevPosB, rootNode, rootMotion);
            RootMotionData::Lerp(nodes->RootMotion, rootMotion, alpha, nodes->RootMotion);
            Transform::Lerp(nodes->Nodes[rootNodeIndex], rootNode, alpha, nodes->Nodes[rootNodeIndex]);
        }
    }

    return nodes;
}

Variant AnimGraphExecutor::SampleAnimationsWithBlend(AnimGraphNode* node, bool loop, float length, float startTimePos, float prevTimePos, float& newTimePos, Animation* animA, Animation* animB, Animation* animC, float speedA, float speedB, float speedC, float alphaA, float alphaB, float alphaC)
{
    // Skip if any animation is not ready to use
    if (animA == nullptr || !animA->IsLoaded() ||
        animB == nullptr || !animB->IsLoaded() ||
        animC == nullptr || !animC->IsLoaded())
        return Value::Null;

    // Calculate actual time position within the animation node (defined by length and loop mode)
    const float pos = GetAnimPos(newTimePos, startTimePos, loop, length);
    const float prevPos = GetAnimPos(prevTimePos, startTimePos, loop, length);

    // Get animation position (animation track position for channels sampling)
    const float animPosA = GetAnimSamplePos(length, animA, pos, speedA);
    const float animPrevPosA = GetAnimSamplePos(length, animA, prevPos, speedA);
    const float animPosB = GetAnimSamplePos(length, animB, pos, speedB);
    const float animPrevPosB = GetAnimSamplePos(length, animB, prevPos, speedB);
    const float animPosC = GetAnimSamplePos(length, animC, pos, speedC);
    const float animPrevPosC = GetAnimSamplePos(length, animC, prevPos, speedC);

    // Sample the animations with blending
    const auto nodes = node->GetNodes(this);
    nodes->RootMotion = RootMotionData::Identity;
    nodes->Position = pos;
    nodes->Length = length;
    const auto mappingA = animA->GetMapping(_graph.BaseModel);
    const auto mappingB = animB->GetMapping(_graph.BaseModel);
    const auto mappingC = animC->GetMapping(_graph.BaseModel);
    Transform tmp, t;
    for (int32 i = 0; i < _skeletonNodesCount; i++)
    {
        const int32 nodeToChannelA = mappingA->At(i);
        const int32 nodeToChannelB = mappingB->At(i);
        const int32 nodeToChannelC = mappingC->At(i);
        tmp = t = GetEmptyNodes()->Nodes[i];

        // Calculate the animated node transformations
        if (nodeToChannelA != -1)
        {
            animA->Data.Channels[nodeToChannelA].Evaluate(animPosA, &tmp, false);
            Transform::Lerp(t, tmp, alphaA, t);
        }
        if (nodeToChannelB != -1)
        {
            animB->Data.Channels[nodeToChannelB].Evaluate(animPosB, &tmp, false);
            Transform::Lerp(t, tmp, alphaB, t);
        }
        if (nodeToChannelC != -1)
        {
            animC->Data.Channels[nodeToChannelC].Evaluate(animPosC, &tmp, false);
            Transform::Lerp(t, tmp, alphaC, t);
        }

        // Write blended transformation
        nodes->Nodes[i] = t;
    }

    // Handle root motion
    if (_rootMotionMode != RootMotionMode::NoExtraction)
    {
        // Extract root motion from animation A
        if (animA->Data.EnableRootMotion)
        {
            RootMotionData rootMotion = RootMotionData::Identity;
            const int32 rootNodeIndex = GetRootNodeIndex(animA);
            const int32 nodeToChannel = mappingA->At(rootNodeIndex);
            Transform rootNode = Transform::Identity;
            if (nodeToChannel != -1)
            {
                animA->Data.Channels[nodeToChannel].Evaluate(animPosA, &rootNode, false);
            }
            UpdateRootMotion(mappingA, animA, animPosA, animPrevPosA, rootNode, rootMotion);
            RootMotionData::Lerp(nodes->RootMotion, rootMotion, alphaA, nodes->RootMotion);
            Transform::Lerp(nodes->Nodes[rootNodeIndex], rootNode, alphaA, nodes->Nodes[rootNodeIndex]);
        }

        // Extract root motion from animation B
        if (animB->Data.EnableRootMotion)
        {
            RootMotionData rootMotion = RootMotionData::Identity;
            const int32 rootNodeIndex = GetRootNodeIndex(animA);
            const int32 nodeToChannel = mappingB->At(rootNodeIndex);
            Transform rootNode = Transform::Identity;
            if (nodeToChannel != -1)
            {
                animB->Data.Channels[nodeToChannel].Evaluate(animPosB, &rootNode, false);
            }
            UpdateRootMotion(mappingB, animB, animPosB, animPrevPosB, rootNode, rootMotion);
            RootMotionData::Lerp(nodes->RootMotion, rootMotion, alphaB, nodes->RootMotion);
            Transform::Lerp(nodes->Nodes[rootNodeIndex], rootNode, alphaB, nodes->Nodes[rootNodeIndex]);
        }

        // Extract root motion from animation C
        if (animC->Data.EnableRootMotion)
        {
            RootMotionData rootMotion = RootMotionData::Identity;
            const int32 rootNodeIndex = GetRootNodeIndex(animA);
            const int32 nodeToChannel = mappingC->At(rootNodeIndex);
            Transform rootNode = Transform::Identity;
            if (nodeToChannel != -1)
            {
                animC->Data.Channels[nodeToChannel].Evaluate(animPosC, &rootNode, false);
            }
            UpdateRootMotion(mappingC, animC, animPosC, animPrevPosC, rootNode, rootMotion);
            RootMotionData::Lerp(nodes->RootMotion, rootMotion, alphaC, nodes->RootMotion);
            Transform::Lerp(nodes->Nodes[rootNodeIndex], rootNode, alphaC, nodes->Nodes[rootNodeIndex]);
        }
    }

    return nodes;
}

Variant AnimGraphExecutor::Blend(AnimGraphNode* node, const Value& poseA, const Value& poseB, float alpha, AlphaBlendMode alphaMode)
{
    ANIM_GRAPH_PROFILE_EVENT("Blend Pose");

    alpha = AlphaBlend::Process(alpha, alphaMode);

    const auto nodes = node->GetNodes(this);

    auto nodesA = static_cast<AnimGraphImpulse*>(poseA.AsPointer);
    auto nodesB = static_cast<AnimGraphImpulse*>(poseB.AsPointer);
    if (!ANIM_GRAPH_IS_VALID_PTR(poseA))
        nodesA = GetEmptyNodes();
    if (!ANIM_GRAPH_IS_VALID_PTR(poseB))
        nodesB = GetEmptyNodes();

    for (int32 i = 0; i < _skeletonNodesCount; i++)
    {
        Transform::Lerp(nodesA->Nodes[i], nodesB->Nodes[i], alpha, nodes->Nodes[i]);
    }
    RootMotionData::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);
    nodes->Position = Math::Lerp(nodesA->Position, nodesB->Position, alpha);
    nodes->Length = Math::Lerp(nodesA->Length, nodesB->Length, alpha);

    return nodes;
}

Variant AnimGraphExecutor::SampleState(AnimGraphNode* state)
{
    // Prepare
    auto& data = state->Data.State;
    if (data.Graph == nullptr || data.Graph->GetRootNode() == nullptr)
    {
        // Invalid state graph
        return Value::Null;
    }

    ANIM_GRAPH_PROFILE_EVENT("Evaluate State");

    // Evaluate state
    auto rootNode = data.Graph->GetRootNode();
    auto result = eatBox((Node*)rootNode, &rootNode->Boxes[0]);

    return result;
}

void ComputeMultiBlendLength(float& length, AnimGraphNode* node)
{
    ANIM_GRAPH_PROFILE_EVENT("Setup Mutli Blend Length");

    // TODO: lock graph or graph asset here? make it thread safe

    length = 0.0f;
    for (int32 i = 0; i < ARRAY_COUNT(node->Assets); i++)
    {
        if (node->Assets[i])
        {
            // TODO: maybe don't update if not all anims are loaded? just skip the node with the bind pose?
            if (node->Assets[i]->WaitForLoaded())
            {
                node->Assets[i] = nullptr;
                LOG(Warning, "Failed to load one of the animations.");
            }
            else
            {
                const auto anim = node->Assets[i].As<Animation>();
                const auto aData = node->Values[4 + i * 2].AsVector4();
                length = Math::Max(length, anim->GetLength() * Math::Abs(aData.W));
            }
        }
    }
}

void AnimGraphExecutor::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Get
    case 1:
    {
        // Get parameter
        int32 paramIndex;
        const auto param = _graph.GetParameter((Guid)node->Values[0], paramIndex);
        value = param ? _data->Parameters[paramIndex].Value : Value::Null;
        break;
    }
    default:
        break;
    }
}

void AnimGraphExecutor::ProcessGroupTools(Box* box, Node* nodeBase, Value& value)
{
    auto node = (AnimGraphNode*)nodeBase;
    switch (node->TypeID)
    {
        // Time
    case 5:
    {
        auto& bucket = _data->State[node->BucketIndex].Animation;
        if (bucket.LastUpdateFrame != _currentFrameIndex)
        {
            bucket.TimePosition += _deltaTime;
            bucket.LastUpdateFrame = _currentFrameIndex;
        }
        value = box->ID == 0 ? bucket.TimePosition : _deltaTime;
        break;
    }
    default:
        VisjectExecutor::ProcessGroupTools(box, nodeBase, value);
        break;
    }
}

void AnimGraphExecutor::ProcessGroupAnimation(Box* boxBase, Node* nodeBase, Value& value)
{
    auto box = (AnimGraphBox*)boxBase;
    if (box->IsCacheValid())
    {
        // Return cache
        value = box->Cache;
        return;
    }
    auto node = (AnimGraphNode*)nodeBase;
    switch (node->TypeID)
    {
        // Animation Output
    case 1:
    {
        if (box->HasConnection())
            value = eatBox(nodeBase, box->FirstConnection());
        else
            value = Value::Null;
        break;
    }
        // Animation
    case 2:
    {
        const auto anim = node->Assets[0].As<Animation>();
        auto& bucket = _data->State[node->BucketIndex].Animation;
        const float speed = (float)tryGetValue(node->GetBox(5), node->Values[1]);
        const bool loop = (bool)tryGetValue(node->GetBox(6), node->Values[2]);
        const float startTimePos = (float)tryGetValue(node->GetBox(7), node->Values[3]);

        switch (box->ID)
        {
            // Animation
        case 0:
        {
            ANIM_GRAPH_PROFILE_EVENT("Sample");

            const float length = anim ? anim->GetLength() : 0.0f;

            // Calculate new time position
            if (speed < 0.0f && bucket.LastUpdateFrame < _currentFrameIndex - 1)
            {
                // If speed is negative and it's the first node update then start playing from end
                bucket.TimePosition = length;
            }
            float newTimePos = bucket.TimePosition + _deltaTime * speed;

            value = SampleAnimation(node, loop, length, startTimePos, bucket.TimePosition, newTimePos, anim, 1.0f);

            bucket.TimePosition = newTimePos;
            bucket.LastUpdateFrame = _currentFrameIndex;

            break;
        }
            // Normalized Time
        case 1:
            value = startTimePos + bucket.TimePosition;
            if (anim && anim->IsLoaded())
                value.AsFloat /= anim->GetLength();
            break;
            // Time
        case 2:
            value = startTimePos + bucket.TimePosition;
            break;
            // Length
        case 3:
            value = anim ? anim->GetLength() : 0.0f;
            break;
            // Is Playing
        case 4:
            // If anim was updated during this or a previous frame
            value = bucket.LastUpdateFrame >= _currentFrameIndex - 1;
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
        transform.Scale = (Vector3)tryGetValue(node->GetBox(4), Vector3::One);

        // Skip if no change will be performed
        if (boneIndex < 0 || boneIndex >= _skeletonBonesCount || transformMode == BoneTransformMode::None || transform.IsIdentity())
        {
            // Pass through the input
            value = Value::Null;
            if (inputBox->HasConnection())
                value = eatBox(nodeBase, inputBox->FirstConnection());
            box->Cache = value;
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
        for (int32 i = 0; i < _skeletonNodesCount; i++)
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
        for (int32 i = _skeletonNodesCount - 1; i >= 0; i--)
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
        if (srcBoneIndex < 0 || srcBoneIndex >= _skeletonBonesCount ||
            dstBoneIndex < 0 || dstBoneIndex >= _skeletonBonesCount ||
            !(copyTranslation || copyRotation || copyScale))
        {
            // Pass through the input
            value = input;
            box->Cache = value;
            return;
        }
        const auto& skeleton = _graph.BaseModel->Skeleton;

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
        if (ANIM_GRAPH_IS_VALID_PTR(input) && boneIndex >= 0 && boneIndex < _skeletonBonesCount)
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

            for (int32 i = 0; i < _skeletonNodesCount; i++)
            {
                Transform::Lerp(nodesA->Nodes[i], nodesB->Nodes[i], alpha, nodes->Nodes[i]);
            }
            RootMotionData::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);
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
                const auto nodesA = static_cast<AnimGraphImpulse*>(valueA.AsPointer);
                const auto nodesB = static_cast<AnimGraphImpulse*>(valueB.AsPointer);
                Transform t, tA, tB;
                for (int32 i = 0; i < _skeletonNodesCount; i++)
                {
                    tA = nodesA->Nodes[i];
                    tB = nodesB->Nodes[i];
                    t.Translation = tA.Translation + tB.Translation;
                    t.Orientation = tA.Orientation * tB.Orientation;
                    t.Scale = tA.Scale * tB.Scale;
                    t.Orientation.Normalize();
                    Transform::Lerp(tA, t, alpha, nodes->Nodes[i]);
                }
                RootMotionData::Lerp(nodesA->RootMotion, nodesA->RootMotion + nodesB->RootMotion, alpha, nodes->RootMotion);
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
            for (int32 nodeIndex = 0; nodeIndex < _skeletonNodesCount; nodeIndex++)
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
            RootMotionData::Lerp(nodesA->RootMotion, nodesB->RootMotion, alpha, nodes->RootMotion);

            value = nodes;
        }

        break;
    }
        // Multi Blend 1D
    case 12:
    {
        ASSERT(box->ID == 0);

        // Note data layout:
        // [0]: Vector4 Range (minX, maxX, 0, 0)
        // [1]: float Speed
        // [2]: bool Loop
        // [3]: float StartPosition
        // Per Blend Sample data layout:
        // [0]: Vector4 Info (x=posX, y=0, z=0, w=Speed)
        // [1]: Guid Animation

        // Prepare
        auto& bucket = _data->State[node->BucketIndex].MultiBlend;
        const auto range = node->Values[0].AsVector4();
        const auto speed = (float)tryGetValue(node->GetBox(1), node->Values[1]);
        const auto loop = (bool)tryGetValue(node->GetBox(2), node->Values[2]);
        const auto startTimePos = (float)tryGetValue(node->GetBox(3), node->Values[3]);
        auto& data = node->Data.MultiBlend1D;

        // Check if not valid animation binded
        if (data.IndicesSorted[0] == ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS)
        {
            // Nothing to sample
            value = Value::Null;
            break;
        }

        // Get axis X
        float x = (float)tryGetValue(node->GetBox(4), Value::Zero);
        x = Math::Clamp(x, range.X, range.Y);

        // Check if need to evaluate multi blend length
        if (data.Length < 0)
        {
            ComputeMultiBlendLength(data.Length, node);
        }
        if (data.Length <= ZeroTolerance)
        {
            // Nothing to sample
            value = Value::Null;
            break;
        }

        // Calculate new time position
        if (speed < 0.0f && bucket.LastUpdateFrame < _currentFrameIndex - 1)
        {
            // If speed is negative and it's the first node update then start playing from end
            bucket.TimePosition = data.Length;
        }
        float newTimePos = bucket.TimePosition + _deltaTime * speed;

        ANIM_GRAPH_PROFILE_EVENT("Multi Blend 1D");

        // Find 2 animations to blend (line)
        for (int32 i = 0; i < ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS - 1; i++)
        {
            const auto a = data.IndicesSorted[i];
            const auto b = data.IndicesSorted[i + 1];

            // Get A animation data
            const auto aAnim = node->Assets[a].As<Animation>();
            auto aData = node->Values[4 + a * 2].AsVector4();

            // Check single A case or the last valid animation
            if (x <= aData.X + ANIM_GRAPH_BLEND_THRESHOLD || b == ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS)
            {
                value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, aData.W);
                break;
            }

            // Get B animation data
            ASSERT(b != ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS);
            const auto bAnim = node->Assets[b].As<Animation>();
            auto bData = node->Values[4 + b * 2].AsVector4();

            // Check single B edge case
            if (Math::NearEqual(bData.X, x, ANIM_GRAPH_BLEND_THRESHOLD))
            {
                value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, bAnim, bData.W);
                break;
            }

            // Blend A and B
            const float alpha = (x - aData.X) / (bData.X - aData.X);
            if (alpha > 1.0f)
                continue;
            value = SampleAnimationsWithBlend(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, bAnim, aData.W, bData.W, alpha);
            break;
        }

        bucket.TimePosition = newTimePos;
        bucket.LastUpdateFrame = _currentFrameIndex;

        break;
    }
        // Multi Blend 2D
    case 13:
    {
        ASSERT(box->ID == 0);

        // Note data layout:
        // [0]: Vector4 Range (minX, maxX, minY, maxY)
        // [1]: float Speed
        // [2]: bool Loop
        // [3]: float StartPosition
        // Per Blend Sample data layout:
        // [0]: Vector4 Info (x=posX, y=posY, z=0, w=Speed)
        // [1]: Guid Animation

        // Prepare
        auto& bucket = _data->State[node->BucketIndex].MultiBlend;
        const auto range = node->Values[0].AsVector4();
        const auto speed = (float)tryGetValue(node->GetBox(1), node->Values[1]);
        const auto loop = (bool)tryGetValue(node->GetBox(2), node->Values[2]);
        const auto startTimePos = (float)tryGetValue(node->GetBox(3), node->Values[3]);
        auto& data = node->Data.MultiBlend2D;

        // Check if not valid animation binded
        if (data.TrianglesP0[0] == ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS)
        {
            // Nothing to sample
            value = Value::Null;
            break;
        }

        // Get axis X
        float x = (float)tryGetValue(node->GetBox(4), Value::Zero);
        x = Math::Clamp(x, range.X, range.Y);

        // Get axis Y
        float y = (float)tryGetValue(node->GetBox(5), Value::Zero);
        y = Math::Clamp(y, range.Z, range.W);

        // Check if need to evaluate multi blend length
        if (data.Length < 0)
        {
            ComputeMultiBlendLength(data.Length, node);
        }
        if (data.Length <= ZeroTolerance)
        {
            // Nothing to sample
            value = Value::Null;
            break;
        }

        // Calculate new time position
        if (speed < 0.0f && bucket.LastUpdateFrame < _currentFrameIndex - 1)
        {
            // If speed is negative and it's the first node update then start playing from end
            bucket.TimePosition = data.Length;
        }
        float newTimePos = bucket.TimePosition + _deltaTime * speed;

        ANIM_GRAPH_PROFILE_EVENT("Multi Blend 2D");

        // Find 3 animations to blend (triangle)
        value = Value::Null;
        Vector2 p(x, y);
        bool hasBest = false;
        Vector2 bestPoint;
        float bestWeight = 0.0f;
        byte bestAnims[2];
        for (int32 i = 0; i < ANIM_GRAPH_MULTI_BLEND_2D_MAX_TRIS && data.TrianglesP0[i] != ANIM_GRAPH_MULTI_BLEND_MAX_ANIMS; i++)
        {
            // Get A animation data
            const auto a = data.TrianglesP0[i];
            const auto aAnim = node->Assets[a].As<Animation>();
            const auto aData = node->Values[4 + a * 2].AsVector4();

            // Get B animation data
            const auto b = data.TrianglesP1[i];
            const auto bAnim = node->Assets[b].As<Animation>();
            const auto bData = node->Values[4 + b * 2].AsVector4();

            // Get C animation data
            const auto c = data.TrianglesP2[i];
            const auto cAnim = node->Assets[c].As<Animation>();
            const auto cData = node->Values[4 + c * 2].AsVector4();

            // Get triangle coords
            Vector2 points[3] = {
                Vector2(aData.X, aData.Y),
                Vector2(bData.X, bData.Y),
                Vector2(cData.X, cData.Y)
            };

            // Check if blend using this triangle
            if (CollisionsHelper::IsPointInTriangle(p, points[0], points[1], points[2]))
            {
                if (Vector2::DistanceSquared(p, points[0]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex A
                    value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, aData.W);
                    break;
                }
                if (Vector2::DistanceSquared(p, points[1]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex B
                    value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, bAnim, bData.W);
                    break;
                }
                if (Vector2::DistanceSquared(p, points[2]) < ANIM_GRAPH_BLEND_THRESHOLD2)
                {
                    // Use only vertex C
                    value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, cAnim, cData.W);
                    break;
                }

                const Vector2 v0 = points[1] - points[0];
                const Vector2 v1 = points[2] - points[0];
                const Vector2 v2 = p - points[0];

                const float d00 = Vector2::Dot(v0, v0);
                const float d01 = Vector2::Dot(v0, v1);
                const float d11 = Vector2::Dot(v1, v1);
                const float d20 = Vector2::Dot(v2, v0);
                const float d21 = Vector2::Dot(v2, v1);
                const float coeff = (d00 * d11 - d01 * d01);
                if (Math::IsZero(coeff))
                {
                    // Use only vertex A for invalid triangle
                    value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, aData.W);
                    break;
                }
                const float v = (d11 * d20 - d01 * d21) / coeff;
                const float w = (d00 * d21 - d01 * d20) / coeff;
                const float u = 1.0f - v - w;

                // Blend A and B and C
                value = SampleAnimationsWithBlend(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, bAnim, cAnim, aData.W, bData.W, cData.W, u, v, w);
                break;
            }

            // Try to find the best blend weights for blend position being outside the all triangles (edge case)
            for (int j = 0; j < 3; j++)
            {
                Vector2 s[2] = {
                    points[j],
                    points[(j + 1) % 3]
                };
                Vector2 closest;
                CollisionsHelper::ClosestPointPointLine(p, s[0], s[1], closest);
                if (!hasBest || Vector2::DistanceSquared(closest, p) < Vector2::DistanceSquared(bestPoint, p))
                {
                    bestPoint = closest;
                    hasBest = true;

                    float d = Vector2::Distance(s[0], s[1]);
                    if (Math::IsZero(d))
                    {
                        bestWeight = 0;
                    }
                    else
                    {
                        bestWeight = Vector2::Distance(s[0], closest) / d;
                    }

                    bestAnims[0] = j;
                    bestAnims[1] = (j + 1) % 3;
                }
            }
        }

        // Check if use the closes sample
        if (value.AsPointer == nullptr && hasBest)
        {
            const auto aAnim = node->Assets[bestAnims[0]].As<Animation>();
            const auto aData = node->Values[4 + bestAnims[0] * 2].AsVector4();

            // Check if use only one sample
            if (bestWeight < ANIM_GRAPH_BLEND_THRESHOLD)
            {
                value = SampleAnimation(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, aData.W);
            }
            else
            {
                const auto bAnim = node->Assets[bestAnims[1]].As<Animation>();
                const auto bData = node->Values[4 + bestAnims[1] * 2].AsVector4();
                value = SampleAnimationsWithBlend(node, loop, data.Length, startTimePos, bucket.TimePosition, newTimePos, aAnim, bAnim, aData.W, bData.W, bestWeight);
            }
        }

        bucket.TimePosition = newTimePos;
        bucket.LastUpdateFrame = _currentFrameIndex;

        break;
    }
        // Blend Pose
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
        auto& bucket = _data->State[node->BucketIndex].BlendPose;
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
        bucket.TransitionPosition += _deltaTime;
        if (bucket.PreviousBlendPoseIndex == -1 || bucket.PreviousBlendPoseIndex == poseIndex || bucket.TransitionPosition >= blendDuration || blendDuration <= ANIM_GRAPH_BLEND_THRESHOLD)
        {
            bucket.TransitionPosition = 0.0f;
            bucket.PreviousBlendPoseIndex = poseIndex;
            value = tryGetValue(node->GetBox(FirstBlendPoseBoxIndex + poseIndex), Value::Null);
            break;
        }

        // Blend two animations
        {
            const float alpha = Math::Saturate(bucket.TransitionPosition / blendDuration);
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
                value = poseData->RootMotion.Rotation;
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
        nodes->Nodes = poseData->Nodes;
        nodes->RootMotion.Translation = (Vector3)tryGetValue(node->GetBox(2), Value::Zero);
        nodes->RootMotion.Rotation = (Quaternion)tryGetValue(node->GetBox(3), Value::Zero);
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
        nodes->Nodes = poseData->Nodes;
        nodes->RootMotion.Translation = poseData->RootMotion.Translation + (Vector3)tryGetValue(node->GetBox(2), Value::Zero);
        nodes->RootMotion.Rotation = poseData->RootMotion.Rotation * (Quaternion)tryGetValue(node->GetBox(3), Value::Zero);
        value = nodes;
        break;
    }
        // State Machine
    case 18:
    {
        const int32 maxTransitionsPerUpdate = node->Values[2].AsInt;
        const bool reinitializeOnBecomingRelevant = node->Values[3].AsBool;
        const bool skipFirstUpdateTransition = node->Values[4].AsBool;

        ANIM_GRAPH_PROFILE_EVENT("State Machine");

        // Prepare
        auto& bucket = _data->State[node->BucketIndex].StateMachine;
        auto& data = node->Data.StateMachine;
        int32 transitionsLeft = maxTransitionsPerUpdate == 0 ? MAX_uint16 : maxTransitionsPerUpdate;
        bool isFirstUpdate = bucket.LastUpdateFrame == 0 || bucket.CurrentState == nullptr;
        if (bucket.LastUpdateFrame != _currentFrameIndex - 1 && reinitializeOnBecomingRelevant)
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
            bucket.ActiveTransition = nullptr;
            bucket.TransitionPosition = 0.0f;

            // Reset all state buckets pof the graphs and nodes included inside the state machine
            ResetBuckets(data.Graph);
        }

        // Update the active transition
        if (bucket.ActiveTransition)
        {
            bucket.TransitionPosition += _deltaTime;

            // Check ofr transition end
            if (bucket.TransitionPosition >= bucket.ActiveTransition->BlendDuration)
            {
                // End transition
                ResetBuckets(bucket.CurrentState->Data.State.Graph);
                bucket.CurrentState = bucket.ActiveTransition->Destination;
                bucket.ActiveTransition = nullptr;
                bucket.TransitionPosition = 0.0f;
            }
        }

        ASSERT(bucket.CurrentState && bucket.CurrentState->GroupID == 9 && bucket.CurrentState->TypeID == 20);

        // Update transitions
        // Note: this logic assumes that all transitions are sorted by Order property and Enabled
        while (!bucket.ActiveTransition && transitionsLeft-- > 0)
        {
            // Check if can change the current state
            const auto& stateData = bucket.CurrentState->Data.State;
            int32 transitionIndex = 0;
            while (stateData.Transitions[transitionIndex] != AnimGraphNode::StateData::InvalidTransitionIndex
                && transitionIndex < ANIM_GRAPH_MAX_STATE_TRANSITIONS)
            {
                const auto idx = stateData.Transitions[transitionIndex];
                ASSERT(idx >= 0 && idx < data.Graph->StateTransitions.Count());
                auto& transition = data.Graph->StateTransitions[idx];
                const bool useDefaultRule = static_cast<int32>(transition.Flags & AnimGraphStateTransition::FlagTypes::UseDefaultRule) != 0;

                // Evaluate source state transition data (position, length, etc.)
                const Value sourceStatePtr = SampleState(bucket.CurrentState);
                auto& transitionData = _transitionData; // Note: this could support nested transitions but who uses state machine inside transition rule?
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
                {
                    ASSERT(transition.RuleGraph->GetRootNode());

                    // Execute transition rule
                    auto rootNode = transition.RuleGraph->GetRootNode();
                    canEnter = (bool)eatBox((Node*)rootNode, &rootNode->Boxes[0]);
                }
                if (canEnter)
                {
                    // Start transition
                    bucket.ActiveTransition = &transition;
                    bucket.TransitionPosition = 0.0f;
                    break;
                }

                // Skip after Solo transition
                // TODO: don't load transitions after first enabled Solo transition and remove this check here
                if (static_cast<int32>(transition.Flags & AnimGraphStateTransition::FlagTypes::Solo) != 0)
                    break;

                transitionIndex++;
            }

            // Check for instant transitions
            if (bucket.ActiveTransition && bucket.ActiveTransition->BlendDuration <= ZeroTolerance)
            {
                // End transition
                ResetBuckets(bucket.CurrentState->Data.State.Graph);
                bucket.CurrentState = bucket.ActiveTransition->Destination;
                bucket.ActiveTransition = nullptr;
                bucket.TransitionPosition = 0.0f;
            }
        }

        // Sample the current state
        const auto currentState = SampleState(bucket.CurrentState);
        value = currentState;

        // Handle active transition blending
        if (bucket.ActiveTransition)
        {
            // Sample the active transition destination state
            const auto destinationState = SampleState(bucket.ActiveTransition->Destination);

            // Perform blending
            const float alpha = Math::Saturate(bucket.TransitionPosition / bucket.ActiveTransition->BlendDuration);
            value = Blend(node, currentState, destinationState, alpha, bucket.ActiveTransition->BlendMode);
        }

        // Update bucket
        bucket.LastUpdateFrame = _currentFrameIndex;

        break;
    }
        // Entry
    case 19:
    {
        // Not used
        CRASH;
        break;
    }
        // State
    case 20:
    {
        // Not used
        CRASH;
        break;
    }
        // State Output
    case 21:
    {
        if (box->HasConnection())
            value = eatBox(nodeBase, box->FirstConnection());
        else
            value = Value::Null;
        break;
    }
        // Rule Output
    case 22:
    {
        if (box->HasConnection())
            value = eatBox(nodeBase, box->FirstConnection());
        else
            value = Value::False;
        break;
    }
        // Transition Source State Anim
    case 23:
    {
        const AnimGraphTransitionData& transitionsData = _transitionData;
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
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(9, 24))
            {
                const auto callFunc = _callStack[i]->Assets[0].Get();
                if (callFunc == function)
                {
                    value = Value::Zero;
                    box->Cache = value;
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
        _functions[nodeBase] = (Graph*)data.Graph;

        // Evaluate the function output
        _graphStack.Push((Graph*)data.Graph);
        value = functionOutputBox && functionOutputBox->HasConnection() ? eatBox(nodeBase, functionOutputBox->FirstConnection()) : Value::Zero;
        _graphStack.Pop();
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
        transform.Scale = (Vector3)tryGetValue(node->GetBox(4), Vector3::One);

        // Skip if no change will be performed
        if (nodeIndex < 0 || nodeIndex >= _skeletonNodesCount || transformMode == BoneTransformMode::None || transform.IsIdentity())
        {
            // Pass through the input
            value = Value::Null;
            if (inputBox->HasConnection())
                value = eatBox(nodeBase, inputBox->FirstConnection());
            box->Cache = value;
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
            box->Cache = value;
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
    default:
        break;
    }
    box->Cache = value;
}

void AnimGraphExecutor::ProcessGroupFunction(Box* boxBase, Node* node, Value& value)
{
    auto box = (AnimGraphBox*)boxBase;
    if (box->IsCacheValid())
    {
        // Return cache
        value = box->Cache;
        return;
    }
    switch (node->TypeID)
    {
        // Function Input
    case 1:
    {
        // Find the function call
        AnimGraphNode* functionCallNode = nullptr;
        ASSERT(_graphStack.Count() >= 2);
        Graph* graph;
        for (int32 i = _callStack.Count() - 1; i >= 0; i--)
        {
            if (_callStack[i]->Type == GRAPH_NODE_MAKE_TYPE(9, 24) && _functions.TryGet(_callStack[i], graph) && _graphStack[_graphStack.Count() - 1] == (Graph*)graph)
            {
                functionCallNode = (AnimGraphNode*)_callStack[i];
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
        for (int32 i = 0; i < function->Inputs.Count(); i++)
        {
            auto& input = function->Inputs[i];

            // Pick the any nested graph that uses this input
            AnimSubGraph* subGraph = data.Graph;
            for (auto& graphIndex : input.GraphIndices)
                subGraph = subGraph->SubGraphs[graphIndex];
            if (node->ID == subGraph->Nodes[input.NodeIndex].ID)
            {
                inputIndex = i;
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
            _graphStack.Pop();
            value = eatBox(node, functionCallBox->FirstConnection());
            _graphStack.Push(graph);
        }
        else
        {
            // Use the default value from the function graph
            value = tryGetValue(node->TryGetBox(1), Value::Zero);
        }
        break;
    }
    default:
        break;
    }
    box->Cache = value;
}
