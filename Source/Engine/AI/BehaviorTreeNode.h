// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/SerializableScriptingObject.h"
#include "BehaviorTypes.h"

/// <summary>
/// Base class for Behavior Tree nodes.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeNode : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeNode, SerializableScriptingObject);
    friend class Behavior;
    friend class BehaviorTreeGraph;
    friend class BehaviorKnowledge;
    friend class BehaviorTreeSubTreeNode;
    friend class BehaviorTreeCompoundNode;

protected:
    // Raw memory byte offset from the start of the behavior memory block.
    API_FIELD(ReadOnly) int32 _memoryOffset = 0;
    // Execution index of the node within tree.
    API_FIELD(ReadOnly) int32 _executionIndex = -1;
    // Parent node that owns this node (parent composite or decorator attachment node).
    API_FIELD(ReadOnly) BehaviorTreeNode* _parent = nullptr;

private:
    Array<class BehaviorTreeDecorator*, InlinedAllocation<8>> _decorators;

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
    /// Initializes node instance state. Called when starting logic simulation for a given behavior. Call constructor of the state container.
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    API_FUNCTION() virtual void InitState(const BehaviorUpdateContext& context)
    {
    }

    /// <summary>
    /// Cleanups node instance state. Called when stopping logic simulation for a given behavior. Call destructor of the state container.
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    API_FUNCTION() virtual void ReleaseState(const BehaviorUpdateContext& context)
    {
    }

    /// <summary>
    /// Updates node logic.
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    /// <returns>Operation result enum.</returns>
    API_FUNCTION() virtual BehaviorUpdateResult Update(const BehaviorUpdateContext& context)
    {
        return BehaviorUpdateResult::Success;
    }

#if USE_EDITOR
    /// <summary>
    /// Gets the node debug state text (multiline). Used in Editor-only to display nodes state. Can be called without valid Behavior/Knowledge/Memory to display default debug info (eg. node properties).
    /// </summary>
    /// <param name="context">Behavior context data.</param>
    /// <returns>Debug info text (multiline).</returns>
    API_FUNCTION() virtual String GetDebugInfo(const BehaviorUpdateContext& context) const
    {
        return String::Empty;
    }
#endif

    // Helper utility to update node with state creation/cleanup depending on node relevancy.
    BehaviorUpdateResult InvokeUpdate(const BehaviorUpdateContext& context);
    // Helper utility to make node relevant and init it state.
    virtual void BecomeRelevant(const BehaviorUpdateContext& context);
    // Helper utility to make node irrelevant and release its state (including any nested nodes).
    virtual void BecomeIrrelevant(const BehaviorUpdateContext& context);

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
};

/// <summary>
/// Base class for Behavior Tree node decorators. Decorators can implement conditional filtering or override node logic and execution flow.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeDecorator : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeDecorator, BehaviorTreeNode);

    /// <summary>
    /// Checks if the node can be updated (eg. decorator can block it depending on the gameplay conditions or its state).
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    /// <returns>True if can update, otherwise false to block it.</returns>
    API_FUNCTION() virtual bool CanUpdate(const BehaviorUpdateContext& context)
    {
        return true;
    }

    /// <summary>
    /// Called after node update to post-process result or perform additional action.
    /// </summary>
    /// <param name="context">Behavior update context data.</param>
    /// <param name="result">The node update result. Can be modified by the decorator (eg. to force success).</param>
    API_FUNCTION() virtual void PostUpdate(const BehaviorUpdateContext& context, API_PARAM(ref) BehaviorUpdateResult& result)
    {
    }
};
