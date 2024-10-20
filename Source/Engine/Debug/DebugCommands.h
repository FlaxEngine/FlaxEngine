// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Debug commands and console variables system.
/// </summary>
API_CLASS(static) class FLAXENGINE_API DebugCommands
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(DebugCommands);

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

public:
    static bool Iterate(const StringView& searchText, int32& index);
    static StringView GetCommandName(int32 index);
};
