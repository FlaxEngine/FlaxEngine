// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Threading/ConcurrentSystemLocker.h"

class TaskGraphSystem;
class AnimatedModel;
class Asset;

/// <summary>
/// The animations playback service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Animations
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Animations);

    /// <summary>
    /// The system for Animations update.
    /// </summary>
    API_FIELD(ReadOnly) static TaskGraphSystem* System;

    // Data access locker for animations data.
    static ConcurrentSystemLocker SystemLocker;

#if USE_EDITOR
    // Data wrapper for the debug flow information.
    API_STRUCT(NoDefault) struct DebugFlowInfo
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(DebugFlowInfo);

        // Anim Graph asset
        API_FIELD() Asset* Asset = nullptr;
        // Animated actor
        API_FIELD() ScriptingObject* Instance = nullptr;
        // Graph node id.
        API_FIELD() uint32 NodeId = 0;
        // Graph box id.
        API_FIELD() uint32 BoxId = 0;
        // Ids of graph nodes (call of hierarchy).
        API_FIELD(Internal, NoArray) uint32 NodePath[8] = {};
    };

    // Custom event that is called every time the Anim Graph signal flows over the graph (including the data connections). Can be used to read and visualize the animation blending logic.
    API_EVENT() static Delegate<DebugFlowInfo> DebugFlow;
#endif

    /// <summary>
    /// Adds an animated model to update.
    /// </summary>
    /// <param name="obj">The object.</param>
    static void AddToUpdate(AnimatedModel* obj);

    /// <summary>
    /// Removes the animated model from update.
    /// </summary>
    /// <param name="obj">The object.</param>
    static void RemoveFromUpdate(AnimatedModel* obj);
};
