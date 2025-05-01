// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Visject/VisjectMeta.h"

template<class BoxType>
class GraphNode;

#define GRAPH_NODE_MAKE_TYPE(groupID, typeID) (uint32)((groupID) << 16 | (typeID))

/// <summary>
/// Represents single box of the graph node
/// </summary>
class GraphBox
{
public:
    /// <summary>
    /// The parent node
    /// </summary>
    void* Parent;

    /// <summary>
    /// Unique box ID within single node
    /// </summary>
    byte ID;

    /// <summary>
    /// The box type.
    /// </summary>
    VariantType Type;

    /// <summary>
    /// List with all connections to other boxes
    /// </summary>
    Array<GraphBox*, InlinedAllocation<4>> Connections;

public:
    GraphBox()
        : Parent(nullptr)
        , ID(0)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="parent">Parent node</param>
    /// <param name="id">Unique box id</param>
    /// <param name="type">Connections type</param>
    GraphBox(void* parent, byte id, const VariantType::Types type)
        : Parent(parent)
        , ID(id)
        , Type(type)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="parent">Parent node</param>
    /// <param name="id">Unique box id</param>
    /// <param name="type">Connections type</param>
    GraphBox(void* parent, byte id, const VariantType& type)
        : Parent(parent)
        , ID(id)
        , Type(type)
    {
    }

public:
    /// <summary>
    /// Gets the parent node.
    /// </summary>
    template<class NodeType>
    FORCE_INLINE NodeType* GetParent() const
    {
        return static_cast<NodeType*>(Parent);
    }

    /// <summary>
    /// Returns true if box has one or more connections.
    /// </summary>
    FORCE_INLINE bool HasConnection() const
    {
        return Connections.HasItems();
    }
};

/// <summary>
/// Visject graph node base
/// </summary>
template<class BoxType>
class GraphNode
{
public:
    typedef BoxType Box;
    typedef GraphNode<BoxType> Node;

public:
    /// <summary>
    /// Unique node ID (within a graph).
    /// </summary>
    uint32 ID;

    union
    {
        struct
        {
            uint16 TypeID;
            uint16 GroupID;
        };
        uint32 Type;
    };

    /// <summary>
    /// List of all node values. Array size and value types are constant over time. Only value data can change.
    /// </summary>
    Array<Variant, InlinedAllocation<8>> Values;

    /// <summary>
    /// Node boxes cache. Array index matches the box ID (for fast O(1) lookups).
    /// </summary>
    Array<Box> Boxes;

    /// <summary>
    /// Additional metadata.
    /// </summary>
    VisjectMeta Meta;

public:
    GraphNode()
        : ID(0)
        , Type(0)
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~GraphNode()
    {
    }

public:
    /// <summary>
    /// Gets all the valid boxes.
    /// </summary>
    /// <param name="result">The result container.</param>
    template<typename AllocationType = HeapAllocation>
    void GetBoxes(Array<Box*, AllocationType>& result)
    {
        result.Clear();
        for (int32 i = 0; i < Boxes.Count(); i++)
        {
            if (Boxes[i].Parent == this)
            {
                result.Add(&Boxes[i]);
            }
        }
    }

    /// <summary>
    /// Gets all the valid boxes.
    /// </summary>
    /// <param name="result">The result container.</param>
    template<typename AllocationType = HeapAllocation>
    void GetBoxes(Array<const Box*, AllocationType>& result) const
    {
        result.Clear();
        for (int32 i = 0; i < Boxes.Count(); i++)
        {
            if (Boxes[i].Parent == this)
            {
                result.Add(&Boxes[i]);
            }
        }
    }

    /// <summary>
    /// Get box by ID
    /// </summary>
    /// <param name="id">Box ID</param>
    /// <returns>Box</returns>
    Box* GetBox(int32 id)
    {
        ASSERT(Boxes.HasItems() && Boxes.Count() > id && Boxes[id].ID == id && Boxes[id].Parent == this);
        return &Boxes[id];
    }

    /// <summary>
    /// Get box by ID, returns null if missing.
    /// </summary>
    /// <param name="id">Box ID</param>
    /// <returns>Box</returns>
    Box* TryGetBox(int32 id)
    {
        if (id >= 0 && id < Boxes.Count() && Boxes[id].ID == id && Boxes[id].Parent == this)
            return &Boxes[id];
        return nullptr;
    }

    /// <summary>
    /// Get box by ID
    /// </summary>
    /// <param name="id">Box ID</param>
    /// <returns>Box</returns>
    const Box* GetBox(int32 id) const
    {
        ASSERT(Boxes.HasItems() && Boxes.Count() > id && Boxes[id].ID == id && Boxes[id].Parent == this);
        return &Boxes[id];
    }
};
