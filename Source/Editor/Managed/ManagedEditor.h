// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Platform/Window.h"
#include "Engine/ShadowsOfMordor/Types.h"

namespace CSG
{
    class Brush;
}

/// <summary>
/// Managed Editor root object
/// </summary>
class ManagedEditor : public ScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ManagedEditor);

public:

    static Guid ObjectID;

    struct InternalOptions
    {
        byte AutoReloadScriptsOnMainWindowFocus = 1;
        byte ForceScriptCompilationOnStartup = 1;
        byte UseAssetImportPathRelative = 1;
        byte AutoRebuildCSG = 1;
        float AutoRebuildCSGTimeoutMs = 50;
        byte AutoRebuildNavMesh = 1;
        float AutoRebuildNavMeshTimeoutMs = 100;
    };

    static InternalOptions ManagedEditorOptions;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ManagedEditor"/> class.
    /// </summary>
    ManagedEditor();

    /// <summary>
    /// Finalizes an instance of the <see cref="ManagedEditor"/> class.
    /// </summary>
    ~ManagedEditor();

public:

    /// <summary>
    /// Initializes managed editor.
    /// </summary>
    void Init();

    /// <summary>
    /// Updates managed editor.
    /// </summary>
    void Update();

    /// <summary>
    /// Exits managed editor.
    /// </summary>
    void Exit();

    /// <summary>
    /// Gets the main window created by the c# editor.
    /// </summary>
    /// <returns>The main window</returns>
    Window* GetMainWindow();

    /// <summary>
    /// Determines whether this managed editor allows reload scripts (based on editor state).
    /// </summary>
    bool CanReloadScripts();

    /// <summary>
    /// Determines whether this managed editor allows to reload scripts by auto (based on editor options).
    /// </summary>
    bool CanAutoReloadScripts() const
    {
        return ManagedEditorOptions.AutoReloadScriptsOnMainWindowFocus != 0;
    }

    /// <summary>
    /// Determines whether this managed editor allows auto build CSG mesh on brush modification (based on editor state and settings).
    /// </summary>
    bool CanAutoBuildCSG();

    /// <summary>
    /// Determines whether this managed editor allows auto build navigation mesh on scene modification (based on editor state and settings).
    /// </summary>
    bool CanAutoBuildNavMesh();

public:

    /// <summary>
    /// Checks whenever the game viewport is focused by the user (eg. can receive input).
    /// </summary>
    /// <returns>True if game viewport is focused, otherwise false.</returns>
    bool HasGameViewportFocus() const;

    /// <summary>
    /// Gives focus to the game viewport (game can receive input).
    /// </summary>
    void FocusGameViewport() const;

    /// <summary>
    /// Converts the screen-space position to the game viewport position.
    /// </summary>
    /// <param name="screenPos">The screen-space position.</param>
    /// <returns>The game viewport position.</returns>
    Float2 ScreenToGameViewport(const Float2& screenPos) const;

    /// <summary>
    /// Converts the game viewport position to the screen-space position.
    /// </summary>
    /// <param name="viewportPos">The game viewport position.</param>
    /// <returns>The screen-space position.</returns>
    Float2 GameViewportToScreen(const Float2& viewportPos) const;

    /// <summary>
    /// Gets the game window used to simulate game in editor. Can be used to capture input for the game scripts.
    /// </summary>
    /// <param name="forceGet">True if always get the window, otherwise only if focused.</param>
    /// <returns>The window or null if hidden.</returns>
    Window* GetGameWindow(bool forceGet = false);

    /// <summary>
    /// Gets the size of the game window output.
    /// </summary>
    /// <returns>The size.</returns>
    Float2 GetGameWindowSize();

    /// <summary>
    /// Called when application code calls exit. Editor may end play mode or exit normally.
    /// </summary>
    /// <returns>True if exit engine, otherwise false.</returns>
    bool OnAppExit();

    /// <summary>
    /// Requests play mode when the editor is in edit mode ( once ).
    /// </summary>
    void RequestStartPlayOnEditMode();

private:

    void OnEditorAssemblyLoaded(MAssembly* assembly);

public:

    // [ScriptingObject]
    void DestroyManaged() override;
};
