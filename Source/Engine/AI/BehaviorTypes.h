// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

class Behavior;
class BehaviorTree;
class BehaviorTreeNode;
class BehaviorKnowledge;

/// <summary>
/// Behavior update context state.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API BehaviorUpdateContext
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(BehaviorUpdateContext);

    /// <summary>
    /// Behavior to simulate.
    /// </summary>
    API_FIELD() Behavior* Behavior;

    /// <summary>
    /// Behavior's logic knowledge container (data, goals and sensors).
    /// </summary>
    API_FIELD() BehaviorKnowledge* Knowledge;

    /// <summary>
    /// Current instance memory buffer location (updated while moving down the tree).
    /// </summary>
    API_FIELD() void* Memory;

    /// <summary>
    /// Pointer to array with per-node bit indicating whether node is relevant (active in graph with state created).
    /// </summary>
    API_FIELD() void* RelevantNodes;

    /// <summary>
    /// Simulation time delta (in seconds) since the last update.
    /// </summary>
    API_FIELD() float DeltaTime;

    /// <summary>
    /// Simulation time (in seconds) since the first update of the Behavior (sum of all deltas since the start).
    /// </summary>
    API_FIELD() float Time;
};

/// <summary>
/// Behavior update result.
/// </summary>
API_ENUM() enum class BehaviorUpdateResult
{
    // Action completed successfully.
    Success,
    // Action is still running and active.
    Running,
    // Action failed.
    Failed,
};

/// <summary>
/// Comparison function modes for behavior knowledge values.
/// </summary>
API_ENUM() enum class BehaviorValueComparison
{
    // If A is equal to B, the comparison passes.
    Equal,
    // If A is not equal to B, the comparison passes.
    NotEqual,
    // If A is less than the B, the comparison passes.
    Less,
    // If A is less than or equal to the B, the comparison passes.
    LessEqual,
    // If A is greater than the B, the comparison passes.
    Greater,
    // If A is greater than or equal to the B, the comparison passes.
    GreaterEqual,
};
