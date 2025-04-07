// Copyright (c) Wojciech Figat. All rights reserved.

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
    /// <param name="name">The event name.</param>
    /// <param name="parameters">The event parameters (key and value pairs).</param>
    static void SendEvent(const char* name, Span<Pair<const char*, const char*>> parameters);
};
