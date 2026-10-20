// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

struct Guid;

/// <summary>
/// The result of a scene action run.
/// </summary>
enum class SceneResult
{
    Success,
    Failed,
    Wait,
};

/// <summary>
/// An action on the scenes (eg. load, unload or save a scene) that Level queues and runs in order on the main thread.
/// </summary>
class FLAXENGINE_API SceneAction
{
public:
    virtual ~SceneAction()
    {
    }

    struct Context
    {
        // Amount of seconds that action can take to run within a budget.
        float TimeBudget = MAX_float;
    };

    virtual SceneResult Do(Context& context)
    {
        return SceneResult::Failed;
    }

    virtual bool IsPreloading(const Guid* sceneId = nullptr)
    {
        return false;
    }

public:
    /// <summary>
    /// Runs the queued actions in order, within the time budget of the context. The first action runs until it is done: while it waits, the actions after it wait too. The done actions are removed from the queue and deleted.
    /// </summary>
    /// <param name="actions">The queue of actions.</param>
    /// <param name="context">The context with the time budget.</param>
    static void RunActions(Array<SceneAction*>& actions, Context& context);

    /// <summary>
    /// Removes the queued actions that preload a scene from the queue, and deletes them.
    /// </summary>
    /// <param name="actions">The queue of actions.</param>
    static void RemovePreloadingActions(Array<SceneAction*>& actions);
};
