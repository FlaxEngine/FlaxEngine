// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "BehaviorKnowledge.h"
#include "BehaviorTreeNode.h"
#include "BehaviorKnowledgeSelector.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Base class for compound Behavior Tree nodes that composite child nodes.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeCompoundNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeCompoundNode, BehaviorTreeNode);

    /// <summary>
    /// List with all child nodes.
    /// </summary>
    API_FIELD(Readonly) Array<BehaviorTreeNode*, InlinedAllocation<8>> Children;

public:
    // [BehaviorTreeNode]
    void Init(BehaviorTree* tree) override;
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;
};

/// <summary>
/// Sequence node updates all its children (from left to right) as long as they return success. If any child fails, the sequence is failed.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeSequenceNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSequenceNode, BehaviorTreeCompoundNode);

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(Behavior* behavior, void* memory) override;
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;

private:
    struct State
    {
        int32 CurrentChildIndex = 0;
    };
};

/// <summary>
/// Selector node updates all its children (from left to right) until one of them succeeds. If all children fail, the selector fails.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeSelectorNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSelectorNode, BehaviorTreeCompoundNode);

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(Behavior* behavior, void* memory) override;
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;

private:
    struct State
    {
        int32 CurrentChildIndex = 0;
    };
};

/// <summary>
/// Root node of the behavior tree. Contains logic properties and definitions for the runtime.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeRootNode : public BehaviorTreeSequenceNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeRootNode, BehaviorTreeSequenceNode);
    API_AUTO_SERIALIZATION();

    // Full typename of the blackboard data type (structure or class). Spawned for each instance of the behavior.
    API_FIELD(Attributes="EditorOrder(0), TypeReference(\"\", \"IsValidBlackboardType\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.TypeNameEditor\")")
    StringAnsi BlackboardType;

    // The target amount of the behavior logic updates per second.
    API_FIELD(Attributes="EditorOrder(10)")
    float UpdateFPS = 10.0f;
};

/// <summary>
/// Delay node that waits a specific amount of time while executed.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeDelayNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeDelayNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Time in seconds to wait when node gets activated. Unused if WaitTimeSelector is used.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    float WaitTime = 3.0f;

    // Wait time randomization range to deviate original value.
    API_FIELD(Attributes="EditorOrder(20), Limit(0)")
    float RandomDeviation = 0.0f;

    // Wait time from behavior's knowledge (blackboard, goal or sensor). If set, overrides WaitTime but still uses RandomDeviation.
    API_FIELD(Attributes="EditorOrder(30)")
    BehaviorKnowledgeSelector<float> WaitTimeSelector;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(Behavior* behavior, void* memory) override;
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;

private:
    struct State
    {
        float TimeLeft;
    };
};

/// <summary>
/// Sub-tree node runs a nested Behavior Tree within this tree.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeSubTreeNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSubTreeNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Nested behavior tree to execute within this node.
    API_FIELD(Attributes="EditorOrder(10), Limit(0)")
    AssetReference<BehaviorTree> Tree;

public:
    // [BehaviorTreeNode]
    int32 GetStateSize() const override;
    void InitState(Behavior* behavior, void* memory) override;
    void ReleaseState(Behavior* behavior, void* memory) override;
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;

    struct State
    {
        Array<byte> Memory;
        BitArray<> RelevantNodes;
    };
};

/// <summary>
/// Forces behavior execution end with a specific result (eg. force fail).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeForceFinishNode : public BehaviorTreeNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeForceFinishNode, BehaviorTreeNode);
    API_AUTO_SERIALIZATION();

    // Execution result.
    API_FIELD() BehaviorUpdateResult Result = BehaviorUpdateResult::Success;

public:
    // [BehaviorTreeNode]
    BehaviorUpdateResult Update(BehaviorUpdateContext context) override;
};
