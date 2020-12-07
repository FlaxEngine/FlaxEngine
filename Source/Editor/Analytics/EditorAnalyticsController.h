// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

/// <summary>
/// The controller object for the tracking events for the editor analytics.
/// </summary>
class EditorAnalyticsController
{
public:

    /// <summary>
    /// Starts the service (registers to event handlers).
    /// </summary>
    void Init();

    /// <summary>
    /// Ends the service (unregisters to event handlers).
    /// </summary>
    void Cleanup();
};
