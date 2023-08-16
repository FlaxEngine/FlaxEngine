// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "BehaviorTreeNode.h"
#include "Engine/Core/Collections/Array.h"

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
};

/// <summary>
/// Sequence node updates all its children as long as they return success. If any child fails, the sequence is failed.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorTreeSequenceNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeSequenceNode, BehaviorTreeCompoundNode);
};

/// <summary>
/// Root node of the behavior tree. Contains logic properties and definitions for the runtime.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API BehaviorTreeRootNode : public BehaviorTreeCompoundNode
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeRootNode, BehaviorTreeCompoundNode);
    API_AUTO_SERIALIZATION();

    // Full typename of the blackboard data type (structure or class). Spawned for each instance of the behavior.
    API_FIELD(Attributes="EditorOrder(0), TypeReference(\"\", \"IsValidBlackboardType\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.TypeNameEditor\")")
    StringAnsi BlackboardType;
};
