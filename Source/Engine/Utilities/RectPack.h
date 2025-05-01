// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Memory/Memory.h"

/// <summary>
/// Implementation of the rectangles packing node into 2D atlas with padding. Uses simple space division via Binary Tree.
/// </summary>
template<typename SizeType = uint32>
struct RectPackNode
{
    typedef SizeType Size;

    // Position of the node in the atlas.
    Size X;
    Size Y;

    // Size of the node.
    Size Width;
    Size Height;

    RectPackNode(Size x, Size y, Size width, Size height)
        : X(x)
        , Y(y)
        , Width(width)
        , Height(height)
    {
    }
};

/// <summary>
/// Implementation of the rectangles packing atlas into 2D atlas with padding. Uses simple space division via Binary Tree but stored in a linear memory allocation.
/// </summary>
/// <remarks>Implementation based on https://github.com/TeamHypersomnia/rectpack2D?tab=readme-ov-file#algorithm and https://blackpawn.com/texts/lightmaps/default.html.</remarks>
template<typename NodeType, uint32 NodesMemoryChunkSize = 1024>
struct RectPackAtlas
{
    typedef typename NodeType::Size Size;

    // Atlas width (in pixels).
    Size Width = 0;
    // Atlas height (in pixels).
    Size Height = 0;
    // Atlas borders padding (in pixels).
    Size BordersPadding = 0;
    // List with all allocated nodes (chunked array to ensure node pointers are always valid).
    ChunkedArray<NodeType, NodesMemoryChunkSize> Nodes;

private:
    Array<NodeType*> FreeNodes;

    struct SizeRect
    {
        Size X, Y, W, H;

        FORCE_INLINE SizeRect()
        {
        }

        FORCE_INLINE SizeRect(Size x, Size y, Size w, Size h)
            : X(x)
            , Y(y)
            , W(w)
            , H(h)
        {
        }
    };

    void AddFreeNode(NodeType* node)
    {
        // Use binary search to find the insert location (assumes that FreeNodes are always sorted)
        int32 left = 0, right = FreeNodes.Count() - 1;
        const uint32 nodeSize = (uint32)node->Width * (uint32)node->Height;
        while (left <= right)
        {
            int32 mid = left + (right - left) / 2;
            const NodeType* midNode = FreeNodes.Get()[mid];
            const uint32 midSize = (uint32)midNode->Width * (uint32)midNode->Height;
            if (nodeSize == midSize)
            {
                // Insert right after node of the same size
                left = mid;
                break;
            }
            if (nodeSize > midSize)
            {
                // Go to the left half (contains nodes with higher sizes)
                right = mid - 1;
            }
            else
            {
                // Go to the right half (contains nodes with lower sizes)
                left = mid + 1;
            }
        }
        FreeNodes.Insert(left, node);
    }

public:
    FORCE_INLINE bool IsInitialized()
    {
        return Width != 0;
    }

    /// <summary>
    /// Initializes the atlas of a given size. Clears any previously added nodes. This won't invoke OnFree for atlas tiles.
    /// </summary>
    /// <param name="atlasWidth">The atlas width (in pixels).</param>
    /// <param name="atlasHeight">The atlas height (in pixels).</param>
    /// <param name="bordersPadding">The nodes padding (in pixels). Distance from node contents to atlas borders or other nodes.</param>
    void Init(Size atlasWidth, Size atlasHeight, Size bordersPadding = 0)
    {
        Width = atlasWidth;
        Height = atlasHeight;
        BordersPadding = bordersPadding;
        Nodes.Clear();
        FreeNodes.Clear();
        Nodes.Add(NodeType(bordersPadding, bordersPadding, atlasWidth - bordersPadding, atlasHeight - bordersPadding));
        FreeNodes.Add(&Nodes[0]);
    }

    /// <summary>
    /// Clears the atlas. This won't invoke OnFree for atlas tiles.
    /// </summary>
    void Clear()
    {
        if (Width == 0)
            return;
        Init(Width, Height, BordersPadding);
    }

    /// <summary>
    /// Clears and resets atlas back to the initial state.
    /// </summary>
    void Reset()
    {
        Width = 0;
        Height = 0;
        BordersPadding = 0;
        Nodes.Clear();
        FreeNodes.Clear();
    }

    /// <summary>
    /// Tries to insert a node into the atlas using rectangle pack algorithm.
    /// </summary>
    /// <param name="width">The node width (in pixels).</param>
    /// <param name="height">The node height (in pixels).</param>
    /// <param name="args">The additional arguments.</param>
    /// <returns>The node that contains inserted an item or null if failed to find a free space.</returns>
    template<class... Args>
    NodeType* Insert(Size width, Size height, Args&&... args)
    {
        NodeType* result = nullptr;
        const Size paddedWidth = width + BordersPadding;
        const Size paddedHeight = height + BordersPadding;

        // Search free nodes from back to front and find the one that fits requested item size
        // TODO: FreeNodes are sorted so use Binary Search to quickly find the first tile that might have enough space for insert
        for (int32 i = FreeNodes.Count() - 1; i >= 0; i--)
        {
            NodeType* freeNode = FreeNodes.Get()[i];
            if (paddedWidth > freeNode->Width || paddedHeight > freeNode->Height)
            {
                // Not enough space
                continue;
            }

            // Check if there will be some remaining space left in this node
            if (freeNode->Width != width || freeNode->Height != height)
            {
                // Subdivide this node into up to 2 additional nodes
                const Size remainingWidth = freeNode->Width - paddedWidth;
                const Size remainingHeight = freeNode->Height - paddedHeight;

                // Split the remaining area around this node into two children
                SizeRect bigger, smaller;
                if (remainingHeight <= remainingWidth)
                {
                    // Split vertically
                    smaller = SizeRect(freeNode->X, freeNode->Y + paddedHeight, width, remainingHeight);
                    bigger = SizeRect(freeNode->X + paddedWidth, freeNode->Y, remainingWidth, freeNode->Height);
                }
                else
                {
                    // Split horizontally
                    smaller = SizeRect(freeNode->X + paddedWidth, freeNode->Y, remainingWidth, height);
                    bigger = SizeRect(freeNode->X, freeNode->Y + paddedHeight, freeNode->Width, remainingHeight);
                }
                if (smaller.W * smaller.H > bigger.W * bigger.H)
                    Swap(bigger, smaller);
                if (bigger.W * bigger.H > BordersPadding)
                    AddFreeNode(Nodes.Add(NodeType(bigger.X, bigger.Y, bigger.W, bigger.H)));
                if (smaller.W * smaller.H > BordersPadding)
                    AddFreeNode(Nodes.Add(NodeType(smaller.X, smaller.Y, smaller.W, smaller.H)));

                // Shrink to the actual area
                freeNode->Width = width;
                freeNode->Height = height;
            }

            // Insert into this node
            result = freeNode;
            FreeNodes.RemoveAtKeepOrder(i);
            result->OnInsert(Forward<Args>(args)...);
            break;
        }

        return result;
    }

    /// <summary>
    /// Frees the node.
    /// </summary>
    /// <param name="node">The node to remove from atlas.</param>
    template<class... Args>
    void Free(NodeType* node, Args&&... args)
    {
        ASSERT_LOW_LAYER(node);
        node->OnFree(Forward<Args>(args)...);
        AddFreeNode(node);
    }
};

/// <summary>
/// Implementation of the rectangles packing node into 2D atlas with padding. Uses simple space division via Binary Tree.
/// [Deprecated on 19.06.2024 expires on 19.06.2025] Use RectPackNode and RectPackAtlas instead.
/// </summary>
/// <remarks>Implementation based on https://blackpawn.com/texts/lightmaps/default.html.</remarks>
template<typename NodeType, typename SizeType = uint32>
struct DEPRECATED("Use RectPackNode and RectPackAtlas instead.") RectPack
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
    NodeType* Insert(SizeType itemWidth, SizeType itemHeight, SizeType itemPadding, Args&&... args)
    {
        NodeType* result;
        const SizeType paddedWidth = itemWidth + itemPadding;
        const SizeType paddedHeight = itemHeight + itemPadding;

        // Check if we're free and just the right size
        if (!IsUsed && Width == paddedWidth && Height == paddedHeight)
        {
            // Insert into this slot
            IsUsed = true;
            result = (NodeType*)this;
            result->OnInsert(Forward<Args>(args)...);
            return result;
        }

        // If there are left and right slots there are empty regions around this slot (it also means this slot is occupied)
        if (Left || Right)
        {
            if (Left)
            {
                result = Left->Insert(itemWidth, itemHeight, itemPadding, Forward<Args>(args)...);
                if (result)
                    return result;
            }
            if (Right)
            {
                result = Right->Insert(itemWidth, itemHeight, itemPadding, Forward<Args>(args)...);
                if (result)
                    return result;
            }
        }

        // This slot can't fit or has been already occupied
        if (IsUsed || paddedWidth > Width || paddedHeight > Height)
        {
            // Not enough space
            return nullptr;
        }

        // The width and height of the new child node
        const SizeType remainingWidth = Width - paddedWidth;
        const SizeType remainingHeight = Height - paddedHeight;

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
        result = (NodeType*)this;
        result->OnInsert(Forward<Args>(args)...);
        return result;
    }

    /// <summary>
    /// Frees the node.
    /// </summary>
    /// <returns>The node that contains inserted an item or null if failed to find a free space.</returns>
    template<class... Args>
    void Free(Args&&... args)
    {
        if (!IsUsed)
            return;
        IsUsed = false;
        ((NodeType*)this)->OnFree(Forward<Args>(args)...);
    }
};
