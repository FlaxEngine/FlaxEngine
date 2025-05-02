// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Types.h"

/// <summary>
/// The main application class.
/// </summary>
class ApplicationBase
{
public:

    /// <summary>
    /// Loads the application info.
    /// </summary>
    /// <returns>The error code or 0 if succeed.</returns>
    static int32 LoadProduct() = delete;

    /// <summary>
    /// Creates the main window of the application.
    /// </summary>
    /// <returns>The main window (null if failed).</returns>
    static Window* CreateMainWindow() = delete;

    /// <summary>
    /// Initializes the application. Called after initialization of all engine services.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Init() = delete;

    /// <summary>
    /// Called just before main engine loop start after full engine initialization.
    /// </summary>
    static void BeforeRun() = delete;

    /// <summary>
    /// Called just before engine shutdown.
    /// </summary>
    static void BeforeExit() = delete;
};
