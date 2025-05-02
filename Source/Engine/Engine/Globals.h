// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Input/Enums.h"

/// <summary>
/// Global engine variables container.
/// </summary>
API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API Globals
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Globals);

public:
    // Paths

    // Main engine directory path.
    API_FIELD(ReadOnly) static String StartupFolder;

    // Temporary folder path.
    API_FIELD(ReadOnly) static String TemporaryFolder;

    // Directory that contains project
    API_FIELD(ReadOnly) static String ProjectFolder;

    /// <summary>
    /// The product local data directory.
    /// </summary>
    API_FIELD(ReadOnly) static String ProductLocalFolder;

    /// <summary>
    /// The game executable files location.
    /// </summary>
    API_FIELD(ReadOnly) static String BinariesFolder;

#if USE_EDITOR
    // Project specific cache folder path (editor-only).
    API_FIELD(ReadOnly) static String ProjectCacheFolder;

    // Engine content directory path (editor-only).
    API_FIELD(ReadOnly) static String EngineContentFolder;

    // Game source code directory path (editor-only).
    API_FIELD(ReadOnly) static String ProjectSourceFolder;
#endif

    // Project content directory path.
    API_FIELD(ReadOnly) static String ProjectContentFolder;

#if USE_MONO
    // Mono library folder path.
    API_FIELD(ReadOnly) static String MonoPath;
#endif

public:
    // State

    // True if fatal error occurred (engine is exiting).
    // [Deprecated in v1.10]
    DEPRECATED("Use Engine::FatalError instead.") static bool FatalErrorOccurred;

    // True if engine needs to be closed.
    // [Deprecated in v1.10]
    DEPRECATED("Use Engine::IsRequestingExit instead.") static bool IsRequestingExit;

    /// <summary>
    /// Flags set to true if engine needs to be closed (exit is pending).
    /// [Deprecated in v1.10]
    /// </summary>
    API_PROPERTY() DEPRECATED("Use Engine::IsRequestingExit instead.") static bool GetIsRequestingExit()
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS;
        return IsRequestingExit;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS;
    }

    /// <summary>
    /// Flags set to true if fatal error occurred (engine is exiting).
    /// [Deprecated in v1.10]
    /// </summary>
    API_PROPERTY() DEPRECATED("Use Engine::FatalError instead.") static bool GetFatalErrorOccurred()
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS;
        return FatalErrorOccurred;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS;
    }

    // Process exit code (pending to return).
    // [Deprecated in v1.10]
    DEPRECATED("Use Engine::ExitCode instead.") static int32 ExitCode;

public:
    // Threading

    // Main Engine thread id.
    API_FIELD(ReadOnly) static uint64 MainThreadID;

public:
    // Config

    /// <summary>
    /// The full engine version.
    /// </summary>
    API_FIELD(ReadOnly) static String EngineVersion;

    /// <summary>
    /// The engine build version.
    /// </summary>
    API_FIELD(ReadOnly) static int32 EngineBuildNumber;

    /// <summary>
    /// The short name of the product (can be `Flax Editor` or name of the game e.g. `My Space Shooter`).
    /// </summary>
    API_FIELD(ReadOnly) static String ProductName;

    /// <summary>
    /// The company name (short name used for app data directory).
    /// </summary>
    API_FIELD(ReadOnly) static String CompanyName;

    /// <summary>
    /// The content data keycode.
    /// </summary>
    static int32 ContentKey;
};
