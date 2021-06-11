// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Describes a single skeleton node data. Used by the runtime.
/// </summary>
API_STRUCT() struct SkeletonNode
{
DECLARE_SCRIPTING_TYPE_MINIMAL(SkeletonNode);

    /// <summary>
    /// The parent node index. The root node uses value -1.
    /// </summary>
    API_FIELD() int32 ParentIndex;

    /// <summary>
    /// The local transformation of the node, relative to the parent node.
    /// </summary>
    API_FIELD() Transform LocalTransform;

    /// <summary>
    /// The name of this node.
    /// </summary>
    API_FIELD() String Name;
};

/// <summary>
/// Describes a single skeleton bone data. Used by the runtime. Skeleton bones are subset of the skeleton nodes collection that are actually used by the skinned model meshes.
/// </summary>
API_STRUCT() struct SkeletonBone
{
DECLARE_SCRIPTING_TYPE_MINIMAL(SkeletonBone);

    /// <summary>
    /// The parent bone index. The root bone uses value -1.
    /// </summary>
    API_FIELD() int32 ParentIndex;

    /// <summary>
    /// The index of the skeleton node where bone is 'attached'. Used as a animation transformation source.
    /// </summary>
    API_FIELD() int32 NodeIndex;

    /// <summary>
    /// The local transformation of the bone, relative to the parent bone (in bind pose).
    /// </summary>
    API_FIELD() Transform LocalTransform;

    /// <summary>
    /// The matrix that transforms from mesh space to bone space in bind pose (inverse bind pose).
    /// </summary>
    API_FIELD() Matrix OffsetMatrix;
};

template<>
struct TIsPODType<SkeletonBone>
{
    enum { Value = true };
};

/// <summary>
/// Describes hierarchical bones in a flattened array.
/// </summary>
/// <remarks>
/// Bones are ordered so that parents always come first, allowing for hierarchical updates in a simple loop.
/// </remarks>
class SkeletonData
{
public:

    /// <summary>
    /// The nodes in this hierarchy. The root node is always at the index 0.
    /// </summary>
    Array<SkeletonNode> Nodes;

    /// <summary>
    /// The bones in this hierarchy.
    /// </summary>
    Array<SkeletonBone> Bones;

public:

    /// <summary>
    /// Gets the root node reference.
    /// </summary>
    /// <returns>The root node.</returns>
    FORCE_INLINE SkeletonNode& RootNode()
    {
        ASSERT(Nodes.HasItems());
        return Nodes[0];
    }

    /// <summary>
    /// Gets the root node reference.
    /// </summary>
    /// <returns>The root node.</returns>
    FORCE_INLINE const SkeletonNode& RootNode() const
    {
        ASSERT(Nodes.HasItems());
        return Nodes[0];
    }

    /// <summary>
    /// Swaps the contents of object with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other object.</param>
    void Swap(SkeletonData& other)
    {
        Nodes.Swap(other.Nodes);
        Bones.Swap(other.Bones);
    }

    int32 FindNode(const StringView& name)
    {
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            if (Nodes[i].Name == name)
                return i;
        }
        return -1;
    }

    int32 FindBone(int32 nodeIndex)
    {
        for (int32 i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].NodeIndex == nodeIndex)
                return i;
        }
        return -1;
    }

    /// <summary>
    /// Releases data.
    /// </summary>
    void Dispose()
    {
        Nodes.Resize(0);
        Bones.Resize(0);
    }
};
