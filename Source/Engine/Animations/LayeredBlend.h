// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/BitArray.h"

struct SkeletonNode;

/// <summary>
/// Layered pose blending utilities: blends a layer pose over a base pose with a weight per skeleton node, optionally keeping the layered nodes' rotation in model space.
/// </summary>
class FLAXENGINE_API LayeredBlend
{
public:
    /// <summary>
    /// Computes the per-node layer weights.
    /// When a branch root node is given, the root and all of its descendants are layered. Their weight ramps up linearly over the first rampLength nodes of each chain (the root gets 1/rampLength, the next node 2/rampLength, and so on up to 1). A rampLength of 1 or less gives every node of the branch a full weight.
    /// When a mask is given, nodes outside the mask get no weight. With a mask and no branch root, nodes in the mask get a full weight.
    /// With neither, every node gets a full weight.
    /// </summary>
    /// <param name="nodes">The skeleton nodes (sorted so parents come before their children).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="branchRoot">The index of the branch root node, or -1 to not use one.</param>
    /// <param name="rampLength">The amount of nodes (counted from the branch root) over which the weight ramps up to 1.</param>
    /// <param name="mask">The optional nodes mask (one bit per node). Can be null.</param>
    /// <param name="outWeights">The output weights (one per node, in range 0-1).</param>
    static void ComputeWeights(const SkeletonNode* nodes, int32 nodesCount, int32 branchRoot, int32 rampLength, const BitArray<>* mask, float* outWeights);

    /// <summary>
    /// Reads per-node weights from text: node name and weight pairs (eg. "spine_01 0.1, spine_02 0.2" or one pair per line; a name and its weight are split by spaces, a tab, ':' or '='; pairs by ',', ';' or new lines). Nodes not listed get no weight; names the skeleton doesn't have are skipped.
    /// </summary>
    /// <param name="nodes">The skeleton nodes.</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="text">The node weights text.</param>
    /// <param name="outWeights">The output weights (one per node, in range 0-1).</param>
    /// <returns>True if any node got a weight from the text, otherwise false (outWeights is left all zero).</returns>
    static bool ParseNodeWeights(const SkeletonNode* nodes, int32 nodesCount, const StringView& text, float* outWeights);

    /// <summary>
    /// Blends the layer pose over the base pose using per-node weights scaled by alpha. Translation and scale always blend in the node local space.
    /// With meshSpaceRotation off, rotations blend in local space too (a layered node also inherits the base pose rotation of the non-layered nodes above it).
    /// With meshSpaceRotation on, rotations blend in model space, so a fully layered node keeps the layer's model space rotation whatever the base pose does to the nodes above it.
    /// </summary>
    /// <param name="nodes">The skeleton nodes (sorted so parents come before their children).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="basePose">The base pose node transformations (in local space).</param>
    /// <param name="layerPose">The layer pose node transformations (in local space).</param>
    /// <param name="weights">The per-node weights (in range 0-1).</param>
    /// <param name="alpha">The overall blend amount (in range 0-1).</param>
    /// <param name="meshSpaceRotation">True to blend rotations in model space, otherwise they are blended in local space.</param>
    /// <param name="outPose">The output pose node transformations (in local space). Can't alias the inputs.</param>
    static void Blend(const SkeletonNode* nodes, int32 nodesCount, const Transform* basePose, const Transform* layerPose, const float* weights, float alpha, bool meshSpaceRotation, Transform* outPose);
};
