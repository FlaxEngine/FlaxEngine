// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Scripting/ScriptingType.h"

class TaskGraph;
class JsonAsset;

/// <summary>
/// The main engine class.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Engine
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Engine);
public:

    /// <summary>
    /// The engine start time (local time).
    /// </summary>
    API_FIELD(ReadOnly) static DateTime StartupTime;

    /// <summary>
    /// True if app has focus (one of the windows is being focused).
    /// </summary>
    API_FIELD(ReadOnly) static bool HasFocus;

    /// <summary>
    /// Gets the current update counter since the start of the game.
    /// </summary>
    API_FIELD(ReadOnly) static uint64 UpdateCount;

    /// <summary>
    /// Gets the current frame (drawing) count since the start of the game.
    /// </summary>
    API_FIELD(ReadOnly) static uint64 FrameCount;

public:

    /// <summary>
    /// Event called on engine fixed update.
    /// </summary>
    static Action FixedUpdate;

    /// <summary>
    /// Event called on engine update.
    /// </summary>
    static Action Update;

    /// <summary>
    /// Task graph for engine update.
    /// </summary>
    API_FIELD(ReadOnly) static TaskGraph* UpdateGraph;

    /// <summary>
    /// Event called after engine update.
    /// </summary>
    static Action LateUpdate;

    /// <summary>
    /// Event called after engine update.
    /// </summary>
    static Action LateFixedUpdate;

    /// <summary>
    /// Event called during frame rendering and can be used to invoke custom rendering with GPUDevice.
    /// </summary>
    static Action Draw;

    /// <summary>
    /// Event called during game loop when application gets paused (engine tick will be postponed until unpause). Used on platforms that support only one app on screen.
    /// </summary>
    static Action Pause;

    /// <summary>
    /// Event called during game loop when application gets unpaused (engine tick will continue). Used on platforms that support only one app on screen.
    /// </summary>
    static Action Unpause;

public:

    /// <summary>
    /// The main engine function (must be called from platform specific entry point).
    /// </summary>
    /// <param name="cmdLine">The input application command line arguments.</param>
    /// <returns>The application exit code.</returns>
    static int32 Main(const Char* cmdLine);

    /// <summary>
    /// Exits the engine.
    /// </summary>
    /// <param name="exitCode">The exit code.</param>
    static void Exit(int32 exitCode = -1);

    /// <summary>
    /// Requests normal engine exit.
    /// </summary>
    /// <param name="exitCode">The exit code.</param>
    API_FUNCTION() static void RequestExit(int32 exitCode = 0);

public:

    /// <summary>
    /// Fixed update callback used by the physics simulation (fixed stepping).
    /// </summary>
    static void OnFixedUpdate();

    /// <summary>
    /// Updates game and all engine services.
    /// </summary>
    static void OnUpdate();

    /// <summary>
    /// Late update callback.
    /// </summary>
    static void OnLateUpdate();

    /// <summary>
    /// Late fixed update callback.
    /// </summary>
    static void OnLateFixedUpdate();

    /// <summary>
    /// Draw callback.
    /// </summary>
    static void OnDraw();

    /// <summary>
    /// Called when engine exists. Disposes engine services and shuts down the engine.
    /// </summary>
    static void OnExit();

public:

    // Returns true if engine is running without main window (aka headless mode).
    API_PROPERTY() static bool IsHeadless();

    // True if Engine is ready to work (init and not disposing)
    static bool IsReady();

    static bool ShouldExit();

    /// <summary>
    /// Returns true if the game is running in the Flax Editor; false if run from any deployment target. Use this property to perform Editor-related actions.
    /// </summary>
    API_PROPERTY() static bool IsEditor();

    /// <summary>
    /// Gets the amount of frames rendered during last second known as Frames Per Second. User scripts updates or fixed updates for physics may run at a different frequency than scene rendering. Use this property to get an accurate amount of frames rendered during the last second.
    /// </summary>
    API_PROPERTY() static int32 GetFramesPerSecond();

    /// <summary>
    /// Gets the application command line arguments.
    /// </summary>
    API_PROPERTY() static const String& GetCommandLine();

    /// <summary>
    /// Gets the custom game settings asset referenced by the given key.
    /// </summary>
    /// <param name="key">The settings key.</param>
    /// <returns>The returned asset. Returns null if key is invalid, cannot load asset or data is missing.</returns>
    API_FUNCTION() static JsonAsset* GetCustomSettings(const StringView& key);

    // The main window handle

    static Window* MainWindow;

    /// <summary>
    /// Brings focused to the game viewport (game can receive input).
    /// </summary>
    API_FUNCTION() static void FocusGameViewport();

    /// <summary>
    /// Checks whenever the game viewport is focused by the user (eg. can receive input).
    /// </summary>
    API_PROPERTY() static bool HasGameViewportFocus();

private:

    static void OnPause();
    static void OnUnpause();
};
