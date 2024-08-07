// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Input/Enums.h"

/// <summary>
/// Global engine variables container.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Globals
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Globals);

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

    // Project content directory path
    API_FIELD(ReadOnly) static String ProjectContentFolder;

#if USE_MONO
    // Mono library folder path
    API_FIELD(ReadOnly) static String MonoPath;
#endif

    // State

    // True if fatal error occurred (engine is exiting)
    static bool FatalErrorOccurred;

    // True if engine needs to be closed
    static bool IsRequestingExit;

    /// <summary>
    /// True if engine needs to be closed
    /// </summary>
    API_PROPERTY() FORCE_INLINE static bool GetIsRequestingExit() { return IsRequestingExit; }

    /// <summary>
    /// True if fatal error occurred (engine is exiting)
    /// </summary>
    API_PROPERTY() FORCE_INLINE static bool GetFatalErrorOccurred() { return FatalErrorOccurred; }

    // Exit code
    static int32 ExitCode;

    // Threading

    // Main Engine thread id
    API_FIELD(ReadOnly) static uint64 MainThreadID;

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
