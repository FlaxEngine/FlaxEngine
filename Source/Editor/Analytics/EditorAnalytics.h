// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Editor user analytics reporting and telemetry service.
/// </summary>
class EditorAnalytics
{
public:
    /// <summary>
    /// Determines whether analytics session is active.
    /// </summary>
    static bool IsSessionActive();

    /// <summary>
    /// Starts the session.
    /// </summary>
    static void StartSession();

    /// <summary>
    /// Ends the session.
    /// </summary>
    static void EndSession();

    /// <summary>
    /// Sends the custom event.
    /// </summary>
    /// <param name="category">The event category name.</param>
    /// <param name="name">The event name.</param>
    /// <param name="label">The event label.</param>
    static void SendEvent(const char* category, const char* name, const char* label = nullptr);

    /// <summary>
    /// Sends the custom event.
    /// </summary>
    /// <param name="category">The event category name.</param>
    /// <param name="name">The event name.</param>
    /// <param name="label">The event label.</param>
    static void SendEvent(const char* category, const char* name, const StringView& label);

    /// <summary>
    /// Sends the custom event.
    /// </summary>
    /// <param name="category">The event category name.</param>
    /// <param name="name">The event name.</param>
    /// <param name="label">The event label.</param>
    static void SendEvent(const char* category, const char* name, const Char* label);
};
