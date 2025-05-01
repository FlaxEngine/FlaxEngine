// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Settings for new process.
/// </summary>
API_STRUCT(NoDefault) struct CreateProcessSettings
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(CreateProcessSettings);

    /// <summary>
    /// The path to the executable file.
    /// </summary>
    API_FIELD() String FileName;

    /// <summary>
    /// The custom arguments for command line.
    /// </summary>
    API_FIELD() String Arguments;

    /// <summary>
    /// The custom folder path where start process. Empty if unused.
    /// </summary>
    API_FIELD() String WorkingDirectory;

    /// <summary>
    /// True if capture process output and print to the log.
    /// </summary>
    API_FIELD() bool LogOutput = true;

    /// <summary>
    /// True if capture process output and store it as Output text array.
    /// </summary>
    API_FIELD() bool SaveOutput = false;

    /// <summary>
    /// True if wait for the process execution end.
    /// </summary>
    API_FIELD() bool WaitForEnd = true;

    /// <summary>
    /// True if hint process to hide window. Supported only on Windows platform.
    /// </summary>
    API_FIELD() bool HiddenWindow = true;

    /// <summary>
    /// True if use operating system shell to start the process. Supported only on Windows platform.
    /// </summary>
    API_FIELD() bool ShellExecute = false;

    /// <summary>
    /// Custom environment variables to set for the process. Empty if unused. Additionally newly spawned process inherits this process vars which can be overriden here.
    /// </summary>
    API_FIELD() Dictionary<String, String> Environment;

    /// <summary>
    /// Output process contents.
    /// </summary>
    API_FIELD() Array<Char> Output;
};
