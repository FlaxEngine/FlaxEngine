// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Debug commands and console variables system.
/// </summary>
API_CLASS(static) class FLAXENGINE_API DebugCommands
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(DebugCommands);

    // Types of debug command flags.
    API_ENUM(Attributes="Flags") enum class CommandFlags
    {
        // Incorrect or missing command.
        None = 0,
        // Executable method.
        Exec = 1,
        // Can get value.
        Read = 2,
        // Can set value.
        Write = 4,
        // Can get and set value.
        ReadWrite = Read | Write,
    };

public:
    /// <summary>
    /// Executes the command.
    /// </summary>
    /// <param name="command">The command line (optionally with arguments).</param>
    API_FUNCTION() static void Execute(StringView command);

    /// <summary>
    /// Searches the list of commands to return candidates that match the given query text.
    /// </summary>
    /// <param name="searchText">The query text.</param>
    /// <param name="matches">The output list of commands that match a given query (unsorted).</param>
    /// <param name="startsWith">True if filter commands that start with a specific search text, otherwise will return commands that contain a specific query.</param>
    API_FUNCTION() static void Search(StringView searchText, API_PARAM(Out) Array<StringView, HeapAllocation>& matches, bool startsWith = false);

    /// <summary>
    /// Starts asynchronous debug commands caching. Cna be used to minimize time-to-interactive when using console interface or when using scripted actions.
    /// </summary>
    API_FUNCTION() static void InitAsync();

    /// <summary>
    /// Returns flags of the command.
    /// </summary>
    /// <param name="command">The full name of the command.</param>
    API_FUNCTION() static CommandFlags GetCommandFlags(StringView command);

public:
    static bool Iterate(const StringView& searchText, int32& index);
    static StringView GetCommandName(int32 index);
};

DECLARE_ENUM_OPERATORS(DebugCommands::CommandFlags);
