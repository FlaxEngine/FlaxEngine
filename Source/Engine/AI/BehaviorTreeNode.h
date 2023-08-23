// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/SerializableScriptingObject.h"
#include "BehaviorTypes.h"

/// <summary>
/// Base class for Behavior Tree nodes.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeNode : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeNode, SerializableScriptingObject);
    friend class BehaviorTreeGraph;
    friend class BehaviorKnowledge;
    friend class BehaviorTreeSubTreeNode;
    friend class BehaviorTreeCompoundNode;

protected:
    // Raw memory byte offset from the start of the behavior memory block.
    API_FIELD(ReadOnly) int32 _memoryOffset = 0;
    // Execution index of the node within tree.
    API_FIELD(ReadOnly) int32 _executionIndex = -1;

public:
    /// <summary>
    /// Node user name (eg. Follow Enemy, or Pick up Weapon).
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// Initializes node state. Called after whole tree is loaded and nodes hierarchy is setup.
    /// </summary>
    /// <param name="tree">Node owner asset.</param>
    API_FUNCTION() virtual void Init(BehaviorTree* tree)
    {
    }

    /// <summary>
    /// Gets the node instance state size. A chunk of the valid memory is passed via InitState to setup that memory chunk (one per-behavior).
    /// </summary>
    API_FUNCTION() virtual int32 GetStateSize() const
    {
        return 0;
    }

    /// <summary>
    /// Initializes node instance state. Called when starting logic simulation for a given behavior.
    /// </summary>
    /// <param name="behavior">Behavior to simulate.</param>
    /// <param name="memory">Pointer to pre-allocated memory for this node to use (call constructor of the state container).</param>
    API_FUNCTION() virtual void InitState(Behavior* behavior, void* memory)
    {
    }

    /// <summary>
    /// Cleanups node instance state. Called when stopping logic simulation for a given behavior.
    /// </summary>
    /// <param name="behavior">Behavior to simulate.</param>
    /// <param name="memory">Pointer to pre-allocated memory for this node to use (call destructor of the state container).</param>
    API_FUNCTION() virtual void ReleaseState(Behavior* behavior, void* memory)
    {
    }

    /// <summary>
    /// Updates node logic.
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    /// <returns>Operation result enum.</returns>
    API_FUNCTION() virtual BehaviorUpdateResult Update(BehaviorUpdateContext context)
    {
        return BehaviorUpdateResult::Success;
    }

    // Helper utility to update node with state creation/cleanup depending on node relevancy.
    BehaviorUpdateResult InvokeUpdate(const BehaviorUpdateContext& context);

    // [SerializableScriptingObject]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

public:
    // Returns the typed node state at the given memory address.
    template<typename T>
    T* GetState(void* memory) const
    {
        ASSERT((int32)sizeof(T) <= GetStateSize());
        return reinterpret_cast<T*>((byte*)memory + _memoryOffset);
    }

protected:
    virtual void InvokeReleaseState(const BehaviorUpdateContext& context);
};
};
