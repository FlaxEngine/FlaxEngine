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

public:
    static bool Iterate(const StringView& searchText, int32& index);
    static String GetCommandName(int32 index);
};
