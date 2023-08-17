// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

class Behavior;
class BehaviorTree;

/// <summary>
/// Behavior logic component knowledge data container. Contains blackboard values, sensors data and goals storage for Behavior Tree execution.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorKnowledge : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorKnowledge, ScriptingObject);
    ~BehaviorKnowledge();

    /// <summary>
    /// Owning Behavior instance (constant).
    /// </summary>
    API_FIELD(ReadOnly) Behavior* Behavior = nullptr;

    /// <summary>
    /// Used Behavior Tree asset (defines blackboard and memory constraints).
    /// </summary>
    API_FIELD(ReadOnly) BehaviorTree* Tree = nullptr;

    /// <summary>
    /// Raw memory chunk with all Behavior Tree nodes state.
    /// </summary>
    API_FIELD(ReadOnly) void* Memory = nullptr;

    /// <summary>
    /// Instance of the behaviour blackboard (structure or class).
    /// </summary>
    API_FIELD() Variant Blackboard;

    // TODO: sensors data
    // TODO: goals
    // TODO: GetGoal/HasGoal

    /// <summary>
    /// Initializes the knowledge for a certain tree.
    /// </summary>
    void InitMemory(BehaviorTree* tree);

    /// <summary>
    /// Releases the memory of the knowledge.
    /// </summary>
    void FreeMemory();
};
