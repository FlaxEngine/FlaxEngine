// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// Implementation of the rectangles packing into 2D atlas with padding. Uses simple space division.
/// </summary>
template<typename NodeType, typename SizeType = uint32>
struct RectPack
{
    // Left and Right slots allow to easily move around the atlas like in a tree structure.
    NodeType* Left;
    NodeType* Right;

    // Position of the entry in the atlas.
    SizeType X;
    SizeType Y;

    // Size of the entry.
    SizeType Width;
    SizeType Height;

    // The remaining space amount inside this slot (updated on every insertion, initial it equal to width*height).
    SizeType SpaceLeft;

    // True, if slot has been allocated, otherwise it's free.
    bool IsUsed;

    /// <summary>
    /// Initializes a new instance of the <see cref="RectPack"/> struct.
    /// </summary>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    RectPack(SizeType x, SizeType y, SizeType width, SizeType height)
        : Left(nullptr)
        , Right(nullptr)
        , X(x)
        , Y(y)
        , Width(width)
        , Height(height)
        , SpaceLeft(width * height)
        , IsUsed(false)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="RectPack"/> class.
    /// </summary>
    ~RectPack()
    {
        if (Left)
            Delete(Left);
        if (Right)
            Delete(Right);
    }

    /// <summary>
    /// Tries to insert an item into this node using rectangle pack algorithm.
    /// </summary>
    /// <param name="itemWidth">The item width (in pixels).</param>
    /// <param name="itemHeight">The item height (in pixels).</param>
    /// <param name="itemPadding">The item padding margin (in pixels).</param>
    /// <param name="args">The additional arguments.</param>
    /// <returns>The node that contains inserted an item or null if failed to find a free space.</returns>
    template<class... Args>
    NodeType* Insert(SizeType itemWidth, SizeType itemHeight, SizeType itemPadding, Args&&...args)
    {
        NodeType* result;
        const SizeType paddedWidth = itemWidth + itemPadding;
        const SizeType paddedHeight = itemHeight + itemPadding;
        const SizeType paddedSize = paddedWidth * paddedHeight;

        // Check if there is enough space to fix that item within this slot
        if (SpaceLeft < paddedSize)
        {
            // Not enough space
            return nullptr;
        }

        // If there are left and right slots there are empty regions around this slot (it also means this slot is occupied)
        if (Left || Right)
        {
            if (Left)
            {
                result = Left->Insert(itemWidth, itemHeight, itemPadding, Forward<Args>(args)...);
                if (result)
                {
                    SpaceLeft -= paddedSize;
                    return result;
                }
            }
            if (Right)
            {
                result = Right->Insert(itemWidth, itemHeight, itemPadding, Forward<Args>(args)...);
                if (result)
                {
                    SpaceLeft -= paddedSize;
                    return result;
                }
            }

            // Not enough space
            return nullptr;
        }

        // This slot can't fit or has been already occupied
        if (IsUsed || paddedWidth > Width || paddedHeight > Height)
        {
            // Not enough space
            return nullptr;
        }

        // Check if we're just right size
        if (Width == paddedWidth && Height == paddedHeight)
        {
            // Insert into this slot
            IsUsed = true;
            SpaceLeft -= paddedSize;
            result = (NodeType*)this;
            result->OnInsert(Forward<Args>(args)...);
            return result;
        }

        // The width and height of the new child node
        const SizeType remainingWidth = Math::Max<SizeType>(0, Width - paddedWidth);
        const SizeType remainingHeight = Math::Max<SizeType>(0, Height - paddedHeight);

        // Split the remaining area around this slot into two children
        if (remainingHeight <= remainingWidth)
        {
            // Split vertically
            Left = New<NodeType>(X, Y + paddedHeight, paddedWidth, remainingHeight);
            Right = New<NodeType>(X + paddedWidth, Y, remainingWidth, Height);
        }
        else
        {
            // Split horizontally
            Left = New<NodeType>(X + paddedWidth, Y, remainingWidth, paddedHeight);
            Right = New<NodeType>(X, Y + paddedHeight, Width, remainingHeight);
        }

        // Shrink the slot to the actual area
        Width = paddedWidth;
        Height = paddedHeight;

        // Insert into this slot
        IsUsed = true;
        SpaceLeft -= paddedSize;
        result = (NodeType*)this;
        result->OnInsert(Forward<Args>(args)...);
        return result;
    }
};
