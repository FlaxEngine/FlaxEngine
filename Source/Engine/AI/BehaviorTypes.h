// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

class Behavior;
class BehaviorTree;
class BehaviorTreeNode;
class BehaviorKnowledge;

/// <summary>
/// Behavior update context state.
/// </summary>
API_STRUCT() struct FLAXENGINE_API BehaviorUpdateContext
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
    /// Simulation time delta (in seconds) since the last update.
    /// </summary>
    API_FIELD() float DeltaTime;
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
