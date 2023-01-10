// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Delegate.h"

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

#if USE_EDITOR
    // Custom event that is called every time the Anim Graph signal flows over the graph (including the data connections). Can be used to read and visualize the animation blending logic. Args are: anim graph asset, animated object, node id, box id
    API_EVENT() static Delegate<Asset*, ScriptingObject*, uint32, uint32> DebugFlow;
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
