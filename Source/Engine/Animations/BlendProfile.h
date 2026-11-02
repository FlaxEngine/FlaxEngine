// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Animations/AlphaBlend.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Types/Guid.h"

class ReadStream;

/// <summary>
/// Blend profile utilities: lets a set of skeleton nodes (picked by a skeleton mask) finish a pose blend faster than the rest of the skeleton.
/// The blend scale is a multiplier of the blend time for the masked nodes. For example, with a 0.2s transition and a scale of 0.5, the masked nodes are fully on the new pose after 0.1s while the other nodes take the whole 0.2s.
/// </summary>
class FLAXENGINE_API BlendProfile
{
public:
    /// <summary>
    /// The default blend scale of the masked nodes.
    /// </summary>
    static constexpr float DefaultScale = 0.5f;

    /// <summary>
    /// Gets the linear blend alpha of a node that blends over the given fraction of the blend time.
    /// </summary>
    /// <param name="alpha">The linear blend alpha of the whole blend (in range 0-1).</param>
    /// <param name="scale">The blend time scale of the node. Values of 1 or more blend like the rest of the skeleton (the blend can't outlast its own duration). 0 makes the node switch to the new pose as soon as the blend starts.</param>
    /// <returns>The linear blend alpha of the node (in range 0-1).</returns>
    static float ScaleAlpha(float alpha, float scale);

    /// <summary>
    /// Computes the per-node blend weights for a blend.
    /// Nodes in the mask blend over scale times the blend duration, the other nodes over the whole duration. The blend mode curve is applied to each node's own progress.
    /// </summary>
    /// <param name="alpha">The linear blend alpha of the whole blend (in range 0-1).</param>
    /// <param name="mode">The blend mode applied to the alpha.</param>
    /// <param name="mask">The nodes mask (one bit per node). Can be null, then all nodes use the whole duration.</param>
    /// <param name="scale">The blend time scale of the masked nodes.</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="outWeights">The output weights (one per node, in range 0-1).</param>
    static void ComputeWeights(float alpha, AlphaBlendMode mode, const BitArray<>* mask, float scale, int32 nodesCount, float* outWeights);

    /// <summary>
    /// Blends two poses with a weight per node.
    /// </summary>
    /// <param name="poseA">The source pose node transformations.</param>
    /// <param name="poseB">The destination pose node transformations.</param>
    /// <param name="weights">The per-node weights (in range 0-1).</param>
    /// <param name="nodesCount">The amount of skeleton nodes.</param>
    /// <param name="outPose">The output pose node transformations.</param>
    static void Blend(const Transform* poseA, const Transform* poseB, const float* weights, int32 nodesCount, Transform* outPose);

    /// <summary>
    /// Reads the blend profile of a state machine transition from the Anim Graph transitions data. Version 1 data has no profile (nothing is read and the defaults are returned). Version 2 stores the mask asset ID and the scale after the fixed transition data.
    /// </summary>
    /// <param name="stream">The transitions data stream.</param>
    /// <param name="version">The transitions data version.</param>
    /// <param name="maskId">The output blend mask asset ID (empty if none).</param>
    /// <param name="scale">The output blend scale.</param>
    static void ReadTransitionData(ReadStream& stream, int32 version, Guid& maskId, float& scale);
};
