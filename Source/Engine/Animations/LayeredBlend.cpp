// Copyright (c) Wojciech Figat. All rights reserved.

#include "LayeredBlend.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Types/String.h"
#include "Config.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Models/SkeletonData.h"

void LayeredBlend::ComputeWeights(const SkeletonNode* nodes, int32 nodesCount, int32 branchRoot, int32 rampLength, const BitArray<>* mask, float* outWeights)
{
    if (mask && mask->Count() < nodesCount)
        mask = nullptr;
    if (branchRoot < 0 || branchRoot >= nodesCount)
    {
        for (int32 i = 0; i < nodesCount; i++)
            outWeights[i] = !mask || mask->Get(i) ? 1.0f : 0.0f;
        return;
    }

    // Count how far each node sits below the branch root (-1 for nodes outside the branch)
    // Note: skeletons with more than 256 nodes fall back to a heap allocation here
    Array<int32, InlinedAllocation<256>> distance;
    distance.Resize(nodesCount);
    const float rampStep = rampLength > 1 ? 1.0f / (float)rampLength : 1.0f;
    for (int32 i = 0; i < nodesCount; i++)
    {
        const int32 parent = nodes[i].ParentIndex;
        if (i == branchRoot)
            distance[i] = 0;
        else if (parent >= 0 && parent < i && distance[parent] >= 0)
            distance[i] = distance[parent] + 1;
        else
            distance[i] = -1;
        float weight = 0.0f;
        if (distance[i] >= 0 && (!mask || mask->Get(i)))
            weight = Math::Min((float)(distance[i] + 1) * rampStep, 1.0f);
        outWeights[i] = weight;
    }
}

void LayeredBlend::Blend(const SkeletonNode* nodes, int32 nodesCount, const Transform* basePose, const Transform* layerPose, const float* weights, float alpha, bool meshSpaceRotation, Transform* outPose)
{
    alpha = Math::Saturate(alpha);
    if (!meshSpaceRotation)
    {
        for (int32 i = 0; i < nodesCount; i++)
        {
            const float w = weights[i] * alpha;
            if (w <= ANIM_GRAPH_BLEND_THRESHOLD)
                outPose[i] = basePose[i];
            else
                Transform::Lerp(basePose[i], layerPose[i], w, outPose[i]);
        }
        return;
    }

    // Model space rotations of the base, layer and output poses (parents are always processed before their children)
    // Note: skeletons with more than 256 nodes fall back to a heap allocation here
    Array<Quaternion, InlinedAllocation<768>> rotations;
    rotations.Resize(nodesCount * 3);
    Quaternion* baseRot = rotations.Get();
    Quaternion* layerRot = baseRot + nodesCount;
    Quaternion* outRot = layerRot + nodesCount;
    for (int32 i = 0; i < nodesCount; i++)
    {
        const int32 parent = nodes[i].ParentIndex;
        const bool hasParent = parent >= 0 && parent < i;
        const Transform& base = basePose[i];
        const Transform& layer = layerPose[i];
        baseRot[i] = hasParent ? baseRot[parent] * base.Orientation : base.Orientation;
        layerRot[i] = hasParent ? layerRot[parent] * layer.Orientation : layer.Orientation;
        baseRot[i].Normalize();
        layerRot[i].Normalize();

        Transform& out = outPose[i];
        const float w = weights[i] * alpha;
        if (w <= ANIM_GRAPH_BLEND_THRESHOLD)
        {
            // Keep the base local transform (inherits whatever the output did to the parent)
            out = base;
            outRot[i] = hasParent ? outRot[parent] * base.Orientation : base.Orientation;
            outRot[i].Normalize();
            continue;
        }

        // Blend the model space rotation, then bring it back to the output parent's space
        Quaternion target;
        Quaternion::Slerp(baseRot[i], layerRot[i], w, target);
        target.Normalize();
        outRot[i] = target;
        if (hasParent)
        {
            out.Orientation = Quaternion::Invert(outRot[parent]) * target;
            out.Orientation.Normalize();
        }
        else
        {
            out.Orientation = target;
        }
        Vector3::Lerp(base.Translation, layer.Translation, w, out.Translation);
        Float3::Lerp(base.Scale, layer.Scale, w, out.Scale);
    }
}

bool LayeredBlend::ParseNodeWeights(const SkeletonNode* nodes, int32 nodesCount, const StringView& text, float* outWeights)
{
    for (int32 i = 0; i < nodesCount; i++)
        outWeights[i] = 0.0f;
    bool any = false;
    const Char* at = text.Get();
    const Char* end = at + text.Length();
    auto isPairSeparator = [](Char c) { return c == ',' || c == ';' || c == '\n' || c == '\r'; };
    auto isSpace = [](Char c) { return c == ' ' || c == '\t' || c == ':' || c == '='; };
    while (at < end)
    {
        // One pair: a name, then a weight
        while (at < end && (isPairSeparator(*at) || isSpace(*at)))
            at++;
        const Char* nameStart = at;
        while (at < end && !isPairSeparator(*at) && !isSpace(*at))
            at++;
        const StringView name(nameStart, (int32)(at - nameStart));
        while (at < end && isSpace(*at))
            at++;
        const Char* valueStart = at;
        while (at < end && !isPairSeparator(*at) && !isSpace(*at))
            at++;
        const StringView value(valueStart, (int32)(at - valueStart));
        if (name.IsEmpty() || value.IsEmpty())
            continue;
        float weight;
        const String valueText(value);
        if (StringUtils::Parse(valueText.Get(), &weight))
            continue;
        for (int32 i = 0; i < nodesCount; i++)
        {
            if (nodes[i].Name == name)
            {
                outWeights[i] = Math::Saturate(weight);
                any = true;
                break;
            }
        }
    }
    return any;
}
