// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "SkeletonData.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Performs hierarchical updates for skeleton nodes.
/// </summary>
template<typename T>
class SkeletonUpdater
{
public:
    typedef Array<T> Items;

    /// <summary>
    /// Represents skeleton node transformation data.
    /// </summary>
    struct Node
    {
        /// <summary>
        /// The parent node index. The root node uses value -1.
        /// </summary>
        int32 ParentIndex;

        /// <summary>
        /// The local transform.
        /// </summary>
        Transform Transform;

        /// <summary>
        /// The local transformation matrix (from parent local space to node local space).
        /// </summary>
        Matrix LocalMatrix;

        /// <summary>
        /// The absolute world transformation matrix (from world space to node local space).
        /// </summary>
        Matrix WorldMatrix;
    };

public:
    /// <summary>
    /// The cached node transformations.
    /// </summary>
    Array<Node> NodeTransformations;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkeletonUpdater" /> class.
    /// </summary>
    /// <param name="skeleton">The skeleton.</param>
    SkeletonUpdater(const Items& skeleton)
    {
        Initialize(skeleton);
    }

    /// <summary>
    /// Initializes the updater using the specified skeleton.
    /// </summary>
    /// <param name="skeleton">The skeleton.</param>
    void Initialize(const Items& skeleton)
    {
        const int32 count = skeleton.Count();
        NodeTransformations.Resize(count, false);
        for (int32 i = 0; i < count; i++)
        {
            auto& n1 = NodeTransformations[i];
            auto& n2 = skeleton[i];

            n1.ParentIndex = n2.ParentIndex;
            n1.Transform = n2.LocalTransform;
            n1.WorldMatrix = Matrix::Identity;
            n1.Transform.GetWorld(n1.LocalMatrix);
        }
    }

    /// <summary>
    /// For each node, updates the world matrices from local matrices.
    /// </summary>
    void UpdateMatrices()
    {
        for (int32 i = 0; i < NodeTransformations.Count(); i++)
        {
            UpdateNode(NodeTransformations[i]);
        }
    }

    /// <summary>
    /// Gets the transformation matrix to go from rootIndex to index.
    /// </summary>
    /// <param name="rootIndex">The root index.</param>
    /// <param name="index">The current index.</param>
    /// <returns>The matrix at this index.</returns>
    Matrix CombineMatricesFromNodeIndices(int32 rootIndex, int32 index)
    {
        if (index == -1)
            return Matrix::Identity;

        auto result = NodeTransformations[index].LocalMatrix;
        if (index != rootIndex)
        {
            const auto topMatrix = CombineMatricesFromNodeIndices(rootIndex, NodeTransformations[index].ParentIndex);
            result = Matrix::Multiply(result, topMatrix);
        }

        return result;
    }

    /// <summary>
    /// Gets the world matrix of the node.
    /// </summary>
    /// <param name="index">The node index.</param>
    /// <param name="matrix">The result matrix.</param>
    FORCE_INLINE void GetWorldMatrix(int32 index, Matrix* matrix)
    {
        *matrix = NodeTransformations[index].WorldMatrix;
    }

    /// <summary>
    /// Gets the local matrix of the node.
    /// </summary>
    /// <param name="index">The node index.</param>
    /// <param name="matrix">The result matrix.</param>
    FORCE_INLINE void GetLocalMatrix(int32 index, Matrix* matrix)
    {
        *matrix = NodeTransformations[index].LocalMatrix;
    }

public:
    /// <summary>
    /// Gets the default root node.
    /// </summary>
    /// <returns>The node.</returns>
    static Node GetDefaultNode()
    {
        Node node;
        node.ParentIndex = -1;
        node.Transform = Transform::Identity;
        node.LocalMatrix = Matrix::Identity;
        node.WorldMatrix = Matrix::Identity;
        return node;
    }

private:
    void UpdateNode(Node& node)
    {
        // Compute local matrix
        node.Transform.GetWorld(node.LocalMatrix);

        // Compute world matrix
        if (node.ParentIndex != -1)
        {
            Matrix::Multiply(node.LocalMatrix, NodeTransformations[node.ParentIndex].WorldMatrix, node.WorldMatrix);
        }
        else
        {
            node.WorldMatrix = node.LocalMatrix;
        }
    }
};
