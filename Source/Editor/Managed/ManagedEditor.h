// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Platform/Window.h"
#include "Engine/ShadowsOfMordor/Types.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Tools/ModelTool/ModelTool.h"
#include "Engine/Tools/AudioTool/AudioTool.h"

namespace CSG
{
    class Brush;
}

/// <summary>
/// The main managed editor class. Editor root object.
/// </summary>
API_CLASS(Namespace="FlaxEditor", Name="Editor", NoSpawn, NoConstructor) class ManagedEditor : private ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ManagedEditor);
    static Guid ObjectID;

    API_ENUM(Attributes="Flags", Internal) enum class StartupFlags
    {
        None = 0,
        Headless = 1,
        SkipCompile = 2,
        NewProject = 4,
        Exit = 8,
    };

    struct InternalOptions
    {
        byte AutoReloadScriptsOnMainWindowFocus = 1;
        byte ForceScriptCompilationOnStartup = 1;
        byte UseAssetImportPathRelative = 1;
        byte EnableParticlesPreview = 1;
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

    void BeforeRun();

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

public:
    /// <summary>
    /// Imports the asset file to the target location.
    /// </summary>
    /// <param name="inputPath">The source file path.</param>
    /// <param name="outputPath">The result asset file path.</param>
    /// <param name="arg">The custom argument 9native data).</param>
    /// <returns>True if importing failed, otherwise false.</returns>
    API_FUNCTION() static bool Import(String inputPath, String outputPath, void* arg = nullptr);

#if COMPILE_WITH_TEXTURE_TOOL
    /// <summary>
    /// Imports the texture asset file to the target location.
    /// </summary>
    /// <param name="inputPath">The source file path.</param>
    /// <param name="outputPath">The result asset file path.</param>
    /// <param name="options">The import settings.</param>
    /// <returns>True if importing failed, otherwise false.</returns>
    API_FUNCTION() static bool Import(const String& inputPath, const String& outputPath, const TextureTool::Options& options);

    /// <summary>
    /// Tries the restore the asset import options from the target resource file.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <param name="assetPath">The asset path.</param>
    /// <returns>True settings has been restored, otherwise false.</returns>
    API_FUNCTION() static bool TryRestoreImportOptions(API_PARAM(Ref) TextureTool::Options& options, String assetPath);
#endif

#if COMPILE_WITH_MODEL_TOOL
    /// <summary>
    /// Imports the model asset file to the target location.
    /// </summary>
    /// <param name="inputPath">The source file path.</param>
    /// <param name="outputPath">The result asset file path.</param>
    /// <param name="options">The import settings.</param>
    /// <returns>True if importing failed, otherwise false.</returns>
    API_FUNCTION() static bool Import(const String& inputPath, const String& outputPath, const ModelTool::Options& options);

    /// <summary>
    /// Tries the restore the asset import options from the target resource file.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <param name="assetPath">The asset path.</param>
    /// <returns>True settings has been restored, otherwise false.</returns>
    API_FUNCTION() static bool TryRestoreImportOptions(API_PARAM(Ref) ModelTool::Options& options, String assetPath);
#endif

#if COMPILE_WITH_AUDIO_TOOL
    /// <summary>
    /// Imports the audio asset file to the target location.
    /// </summary>
    /// <param name="inputPath">The source file path.</param>
    /// <param name="outputPath">The result asset file path.</param>
    /// <param name="options">The import settings.</param>
    /// <returns>True if importing failed, otherwise false.</returns>
    API_FUNCTION() static bool Import(const String& inputPath, const String& outputPath, const AudioTool::Options& options);

    /// <summary>
    /// Tries the restore the asset import options from the target resource file.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <param name="assetPath">The asset path.</param>
    /// <returns>True settings has been restored, otherwise false.</returns>
    API_FUNCTION() static bool TryRestoreImportOptions(API_PARAM(Ref) AudioTool::Options& options, String assetPath);
#endif

    /// <summary>
    /// Creates a new asset at the target location.
    /// </summary>
    /// <param name="tag">New asset type.</param>
    /// <param name="outputPath">Output asset path.</param>
    API_FUNCTION() static bool CreateAsset(const String& tag, String outputPath);

    /// <summary>
    /// Gets a list of asset references of a given asset.
    /// </summary>
    /// <param name="assetId">The asset ID.</param>
    /// <returns>List of referenced assets.</returns>
    API_FUNCTION() static Array<Guid> GetAssetReferences(const Guid& assetId);

public:
    API_STRUCT(Internal, NoDefault) struct VisualScriptStackFrame
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(VisualScriptStackFrame);

        API_FIELD() class VisualScript* Script;
        API_FIELD() uint32 NodeId;
        API_FIELD() int32 BoxId;
    };

    API_STRUCT(Internal, NoDefault) struct VisualScriptLocal
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(VisualScriptLocal);

        API_FIELD() String Value;
        API_FIELD() String ValueTypeName;
        API_FIELD() uint32 NodeId;
        API_FIELD() int32 BoxId;
    };

    API_FUNCTION(Internal) static Array<VisualScriptStackFrame> GetVisualScriptStackFrames();
    API_FUNCTION(Internal) static VisualScriptStackFrame GetVisualScriptPreviousScopeFrame();
    API_FUNCTION(Internal) static Array<VisualScriptLocal> GetVisualScriptLocals();
    API_FUNCTION(Internal) static bool EvaluateVisualScriptLocal(VisualScript* script, API_PARAM(Ref) VisualScriptLocal& local);
    API_FUNCTION(Internal) static void WipeOutLeftoverSceneObjects();

private:
    void OnEditorAssemblyLoaded(MAssembly* assembly);

public:
    // [ScriptingObject]
    void DestroyManaged() override;
};

DECLARE_ENUM_OPERATORS(ManagedEditor::StartupFlags);
