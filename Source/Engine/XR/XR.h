// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Scripting/ScriptingType.h"

class Gamepad;

/// <summary>
/// The hand of the XR controller.
/// </summary>
API_ENUM() enum class XRHand
{
    /// <summary>
    /// The left hand.
    /// </summary>
    Left = 0,

    /// <summary>
    /// The right hand.
    /// </summary>
    Right = 1,
};

/// <summary>
/// The pose type of the XR controller.
/// </summary>
API_ENUM() enum class XRPoseType
{
    /// <summary>
    /// The pose of the hand holding the controller. Use it to render the controller model or to hold objects.
    /// </summary>
    Grip = 0,

    /// <summary>
    /// The pose of the pointing ray from the controller (forward direction). Use it to point at objects and UI.
    /// </summary>
    Aim = 1,
};

/// <summary>
/// The XR service used for virtual reality head-mounted displays.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API XR
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(XR);

public:
    /// <summary>
    /// Gets a value indicating whether an XR runtime is running and a head-mounted display is connected.
    /// </summary>
    /// <returns>True if XR is available, otherwise false.</returns>
    API_PROPERTY() static bool IsAvailable();

    /// <summary>
    /// Gets a value indicating whether an XR session is running and frames are being submitted to the head-mounted display.
    /// </summary>
    /// <returns>True if XR is active, otherwise false.</returns>
    API_PROPERTY() static bool IsActive();

    /// <summary>
    /// Gets a value indicating whether the XR session can be started to render to the head-mounted display. Enabled by default in the game. In the editor it's enabled only in play mode (if Play in VR is checked).
    /// </summary>
    /// <returns>True if the XR session can be started, otherwise false.</returns>
    API_PROPERTY() static bool GetSessionEnabled();

    /// <summary>
    /// Sets a value indicating whether the XR session can be started to render to the head-mounted display. Disabling it ends the running session but the XR runtime stays connected, so the session can be started again later.
    /// </summary>
    /// <param name="value">True if the XR session can be started, otherwise false.</param>
    API_PROPERTY() static void SetSessionEnabled(bool value);

    /// <summary>
    /// Gets the name of the XR runtime (eg. SteamVR/OpenXR). Empty if no runtime was found.
    /// </summary>
    /// <returns>The runtime name.</returns>
    API_PROPERTY() static StringView GetRuntimeName();

    /// <summary>
    /// Gets the version of the XR runtime (eg. 2.7.4). Empty if no runtime was found.
    /// </summary>
    /// <returns>The runtime version.</returns>
    API_PROPERTY() static StringView GetRuntimeVersion();

    /// <summary>
    /// Gets the name of the head-mounted display system reported by the XR runtime. Empty if no head-mounted display was found.
    /// </summary>
    /// <returns>The system name.</returns>
    API_PROPERTY() static StringView GetSystemName();

    /// <summary>
    /// Gets the refresh rate (in Hz) of the head-mounted display. Zero if XR is not active.
    /// </summary>
    /// <returns>The display refresh rate.</returns>
    API_PROPERTY() static float GetDisplayRefreshRate();

    /// <summary>
    /// Gets the world transformation of the tracking space origin (the floor center of the play area). The head and controller poses are relative to it. The main camera is used as the tracking space origin.
    /// </summary>
    /// <returns>The tracking space origin transformation (in world-space).</returns>
    API_PROPERTY() static Transform GetTrackingOrigin();

    /// <summary>
    /// Gets the pose of the head-mounted display.
    /// </summary>
    /// <param name="pose">The result pose (relative to the tracking space origin).</param>
    /// <returns>True if the pose is tracked, otherwise false.</returns>
    API_FUNCTION() static bool GetHeadPose(API_PARAM(Out) Transform& pose);

    /// <summary>
    /// Gets the pose of the controller.
    /// </summary>
    /// <param name="hand">The hand of the controller.</param>
    /// <param name="type">The pose type.</param>
    /// <param name="pose">The result pose (relative to the tracking space origin).</param>
    /// <returns>True if the pose is tracked, otherwise false.</returns>
    API_FUNCTION() static bool GetHandPose(XRHand hand, XRPoseType type, API_PARAM(Out) Transform& pose);

    /// <summary>
    /// Vibrates the controller.
    /// </summary>
    /// <param name="hand">The hand of the controller.</param>
    /// <param name="amplitude">The vibration amplitude (normalized). Use 0 to stop the vibration.</param>
    /// <param name="duration">The vibration duration (in seconds). Use 0 for the shortest vibration supported by the controller.</param>
    /// <param name="frequency">The vibration frequency (in Hz). Use 0 for the default frequency of the controller.</param>
    API_FUNCTION() static void Vibrate(XRHand hand, float amplitude, float duration, float frequency = 0.0f);

    /// <summary>
    /// Gets the gamepad that exposes the buttons and axes of the controllers. The thumbsticks and triggers are mapped to the sticks and triggers of the gamepad, the grips and bumpers to the shoulders, the A/B/X/Y buttons and the directional pad to the same gamepad buttons, and the menu and view buttons to Start and Back. It allows to use the Input Settings actions and axes with the controllers.
    /// </summary>
    /// <returns>The gamepad, or null if XR is not active.</returns>
    API_PROPERTY() static Gamepad* GetGamepad();
};
