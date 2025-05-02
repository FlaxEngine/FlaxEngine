// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/ArrayExtensions.h"
#include "Engine/Core/Types/String.h"

/// <summary>
/// Helper class used to map model nodes/bones from one skeleton into another. Useful for animation retargeting.
/// </summary>
template<typename T>
class SkeletonMapping
{
public:
    typedef Array<T> Items;

public:
    /// <summary>
    /// The amount of the nodes (from the source skeleton).
    /// </summary>
    int32 Size;

    /// <summary>
    /// The node mapping from source to target skeletons.
    /// </summary>
    Array<int32> SourceToTarget;

    /// <summary>
    /// A round-trip through TargetToSource[SourceToTarget[i]] so that we know easily what nodes are remapped in source skeleton side.
    /// </summary>
    Array<int32> SourceToSource;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkeletonMapping"/> class.
    /// </summary>
    /// <param name="sourceSkeleton">The source model skeleton.</param>
    /// <param name="targetSkeleton">The target skeleton. May be null to disable nodes mapping.</param>
    SkeletonMapping(const Items& sourceSkeleton, const Items* targetSkeleton)
    {
        Size = sourceSkeleton.Count();
        SourceToTarget.Resize(Size); // model => skeleton mapping
        SourceToSource.Resize(Size); // model => model mapping

        if (targetSkeleton == nullptr)
        {
            // No skeleton, we can compact everything
            for (int i = 0; i < Size; i++)
            {
                // Map everything to the root node
                SourceToTarget[i] = 0;
                SourceToSource[i] = 0;
            }
            return;
        }

        Array<int32> targetToSource;
        targetToSource.Resize(Size); // skeleton => model mapping
        for (int32 i = 0; i < Size; i++)
            targetToSource[i] = -1;

        // Build mapping from model to actual skeleton
        for (int32 modelIndex = 0; modelIndex < Size; modelIndex++)
        {
            auto node = sourceSkeleton[modelIndex];
            const auto parentModelIndex = node.ParentIndex;

            // Find matching node in skeleton (or map to best parent)
            const Function<bool(const T&)> f = [node](const T& x) -> bool
            {
                return x.Name == node.Name;
            };
            const auto skeletonIndex = ArrayExtensions::IndexOf(*targetSkeleton, f);
            if (skeletonIndex == -1)
            {
                // Nothing match, remap to parent node
                SourceToTarget[modelIndex] = parentModelIndex != -1 ? SourceToTarget[parentModelIndex] : 0;
                continue;
            }

            // Name match
            SourceToTarget[modelIndex] = skeletonIndex;
            targetToSource[skeletonIndex] = modelIndex;
        }
        for (int32 modelIndex = 0; modelIndex < Size; modelIndex++)
        {
            SourceToSource[modelIndex] = targetToSource[SourceToTarget[modelIndex]];
        }
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SkeletonMapping"/> class.
    /// </summary>
    ~SkeletonMapping()
    {
    }
};
