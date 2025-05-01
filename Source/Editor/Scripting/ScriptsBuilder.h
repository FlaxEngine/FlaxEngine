// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Game scrips building service. Compiles user C# scripts into binary assemblies. Exposes many events used to track scripts compilation and reloading.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class ScriptsBuilder
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ScriptsBuilder);
public:

    typedef Delegate<const String&, const String&, int32> CompileMsgDelegate;

public:

    /// <summary>
    /// Action called on compilation end, bool param is true if success, otherwise false
    /// </summary>
    static Delegate<bool> OnCompilationEnd;

    /// <summary>
    /// Action called when compilation success
    /// </summary>
    static Action OnCompilationSuccess;

    /// <summary>
    /// Action called when compilation fails
    /// </summary>
    static Action OnCompilationFailed;

public:

    /// <summary>
    /// Gets amount of source code compile actions since Editor startup.
    /// </summary>
    API_PROPERTY() static int32 GetCompilationsCount();

    /// <summary>
    /// Gets the full path to the Flax.Build app.
    /// </summary>
    API_PROPERTY() static String GetBuildToolPath();

    /// <summary>
    /// Checks if last scripting building failed due to errors.
    /// </summary>
    /// <returns>True if last compilation result was a fail, otherwise false.</returns>
    API_PROPERTY() static bool LastCompilationFailed();

    /// <summary>
    /// Returns true if source code has been edited.
    /// </summary>
    /// <returns>True if source code is dirty, otherwise false.</returns>
    API_PROPERTY() static bool IsSourceDirty();

    /// <summary>
    /// Returns true if source code workspace has been edited (source file moved or deleted).
    /// </summary>
    /// <returns>True if source code workspace is dirty, otherwise false.</returns>
    API_PROPERTY() static bool IsSourceWorkspaceDirty();

    /// <summary>
    /// Returns true if source code has been edited and is dirty for given amount of time
    /// </summary>
    /// <param name="timeout">Time to use for checking.</param>
    /// <returns>True if source code is dirty, otherwise false.</returns>
    API_FUNCTION() static bool IsSourceDirtyFor(const TimeSpan& timeout);

    /// <summary>
    /// Returns true if scripts are being now compiled/reloaded.
    /// </summary>
    /// <returns>True if scripts are being compiled/reloaded now.</returns>
    API_PROPERTY() static bool IsCompiling();

    /// <summary>
    /// Returns true if source code has been compiled and assemblies are ready to load.
    /// </summary>
    /// <returns>True if assemblies are ready to load.</returns>
    API_PROPERTY() static bool IsReady();

    /// <summary>
    /// Indicates that scripting directory has been modified so scripts need to be rebuild.
    /// </summary>
    API_FUNCTION() static void MarkWorkspaceDirty();

    /// <summary>
    /// Checks if need to compile source code. If so calls compilation.
    /// </summary>
    API_FUNCTION() static void CheckForCompile();

    /// <summary>
    /// Requests project source code compilation.
    /// </summary>
    API_FUNCTION() static void Compile();

    /// <summary>
    /// Invokes the Flax.Build tool in the current project workspace and waits for the process end (blocking). Prints the build tool output to the log. Can be invoked from any thread.
    /// </summary>
    /// <param name="args">The Flax.Build tool invocation arguments.</param>
    /// <param name="workingDir">The custom working directory. Use empty or null to execute build tool in the project folder.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool RunBuildTool(const StringView& args, const StringView& workingDir = StringView::Empty);

    /// <summary>
    /// Generates the project files.
    /// </summary>
    /// <param name="customArgs">The additional Flax.Build tool invocation arguments.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool GenerateProject(const StringView& customArgs = StringView::Empty);

    /// <summary>
    /// Tries to find a script type with the given name.
    /// </summary>
    /// <param name="scriptName">The script full name.</param>
    /// <returns>Found script type or null if missing or invalid name.</returns>
    API_FUNCTION() static MClass* FindScript(const StringView& scriptName);

    // Gets the list of existing in-build code editors.
    API_FUNCTION() static void GetExistingEditors(int32* result, int32 count);

    // Gets the options for the game scripts to use for the Editor.
    API_FUNCTION() static void GetBinariesConfiguration(API_PARAM(Out) StringView& target, API_PARAM(Out) StringView& platform, API_PARAM(Out) StringView& architecture, API_PARAM(Out) StringView& configuration);
    static void GetBinariesConfiguration(const Char*& target, const Char*& platform, const Char*& architecture, const Char*& configuration);

public:

    /// <summary>
    /// Filters the project namespace text value to be valid (remove whitespace characters and other invalid ones).
    /// </summary>
    /// <param name="value">The in/out value.</param>
    static void FilterNamespaceText(String& value);
};
