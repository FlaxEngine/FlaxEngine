// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Animations/AlphaBlend.h"
#include "Engine/Core/Collections/BitArray.h"

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
};
