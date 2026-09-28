// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

/// <summary>
/// The OpenXR session with the frame loop and the lifecycle state machine.
/// </summary>
class OpenXRSession
{
public:
    /// <summary>
    /// Processes the runtime events and creates the session once the head-mounted display is available.
    /// </summary>
    static void Update();

    /// <summary>
    /// Begins the frame and renders the swapchain images.
    /// </summary>
    static void Draw();

    /// <summary>
    /// Submits the rendered swapchain images and ends the frame.
    /// </summary>
    static void PostDraw();

    /// <summary>
    /// Waits for the next frame to render (used as the engine frame pacer while the session is running).
    /// </summary>
    static void WaitFrame();

    /// <summary>
    /// Destroys the session.
    /// </summary>
    static void Dispose();

    /// <summary>
    /// Checks if the session is running and frames are being submitted to the runtime.
    /// </summary>
    /// <returns>True if running, otherwise false.</returns>
    static bool IsRunning();
};

#endif
