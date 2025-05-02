// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BehaviorTree.h"
#include "BehaviorKnowledge.h"
#include "BehaviorTypes.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Behavior instance script that runs Behavior Tree execution.
/// </summary>
API_CLASS(Attributes="Category(\"Flax Engine\")") class FLAXENGINE_API Behavior : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(Behavior);
    friend class BehaviorSystem;

    /// <summary>
    /// The system for behaviors update.
    /// </summary>
    API_FIELD(ReadOnly) static class TaskGraphSystem* System;

private:
    BehaviorKnowledge _knowledge;
    float _accumulatedTime = 0.0f;
    float _totalTime = 0.0f;
    BehaviorUpdateResult _result = BehaviorUpdateResult::Success;

    void UpdateAsync();

public:
    /// <summary>
    /// Behavior Tree asset to use for logic execution.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    AssetReference<BehaviorTree> Tree;

    /// <summary>
    /// If checked, auto starts the logic on begin play.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    bool AutoStart = true;

    /// <summary>
    /// The behavior logic update rate scale (multiplies the UpdateFPS defined in Behavior Tree root node). Can be used to improve performance via LOD to reduce updates frequency (eg. by 0.5) for behaviors far from player.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), Limit(0, 10, 0.01f)")
    float UpdateRateScale = 1.0f;

public:
    /// <summary>
    /// Gets the current behavior knowledge instance. Empty if not started.
    /// </summary>
    API_PROPERTY() BehaviorKnowledge* GetKnowledge()
    {
        return &_knowledge;
    }

    /// <summary>
    /// Gets the last behavior tree execution result.
    /// </summary>
    API_PROPERTY() BehaviorUpdateResult GetResult() const
    {
        return _result;
    }

    /// <summary>
    /// Event called when behavior tree execution ends with a result.
    /// </summary>
    API_EVENT() Action Finished;

    /// <summary>
    /// Starts the logic.
    /// </summary>
    API_FUNCTION() void StartLogic();

    /// <summary>
    /// Stops the logic.
    /// </summary>
    /// <param name="result">The logic result.</param>
    API_FUNCTION() void StopLogic(BehaviorUpdateResult result = BehaviorUpdateResult::Success);

    /// <summary>
    /// Resets the behavior logic by clearing knowledge (clears blackboard and removes goals) and resetting execution state (goes back to root).
    /// </summary>
    API_FUNCTION() void ResetLogic();

    // [Script]
    void OnEnable() override;
    void OnDisable() override;

private:
#if USE_EDITOR
    // Editor-only utilities to debug nodes state.
    API_FUNCTION(Internal) static bool GetNodeDebugRelevancy(const BehaviorTreeNode* node, const Behavior* behavior);
    API_FUNCTION(Internal) static String GetNodeDebugInfo(const BehaviorTreeNode* node, Behavior* behavior);
#endif
};
