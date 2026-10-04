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
/// The joint of the tracked hand. The joint forward direction points along the bone towards the fingertip and the up direction points out of the back of the hand.
/// </summary>
API_ENUM() enum class XRHandJoint
{
    /// <summary>
    /// The center of the palm.
    /// </summary>
    Palm = 0,

    /// <summary>
    /// The wrist.
    /// </summary>
    Wrist = 1,

    /// <summary>
    /// The thumb metacarpal joint (base of the thumb).
    /// </summary>
    ThumbMetacarpal = 2,

    /// <summary>
    /// The thumb proximal joint.
    /// </summary>
    ThumbProximal = 3,

    /// <summary>
    /// The thumb distal joint.
    /// </summary>
    ThumbDistal = 4,

    /// <summary>
    /// The thumb tip.
    /// </summary>
    ThumbTip = 5,

    /// <summary>
    /// The index finger metacarpal joint.
    /// </summary>
    IndexMetacarpal = 6,

    /// <summary>
    /// The index finger proximal joint (knuckle).
    /// </summary>
    IndexProximal = 7,

    /// <summary>
    /// The index finger intermediate joint.
    /// </summary>
    IndexIntermediate = 8,

    /// <summary>
    /// The index finger distal joint.
    /// </summary>
    IndexDistal = 9,

    /// <summary>
    /// The index finger tip.
    /// </summary>
    IndexTip = 10,

    /// <summary>
    /// The middle finger metacarpal joint.
    /// </summary>
    MiddleMetacarpal = 11,

    /// <summary>
    /// The middle finger proximal joint (knuckle).
    /// </summary>
    MiddleProximal = 12,

    /// <summary>
    /// The middle finger intermediate joint.
    /// </summary>
    MiddleIntermediate = 13,

    /// <summary>
    /// The middle finger distal joint.
    /// </summary>
    MiddleDistal = 14,

    /// <summary>
    /// The middle finger tip.
    /// </summary>
    MiddleTip = 15,

    /// <summary>
    /// The ring finger metacarpal joint.
    /// </summary>
    RingMetacarpal = 16,

    /// <summary>
    /// The ring finger proximal joint (knuckle).
    /// </summary>
    RingProximal = 17,

    /// <summary>
    /// The ring finger intermediate joint.
    /// </summary>
    RingIntermediate = 18,

    /// <summary>
    /// The ring finger distal joint.
    /// </summary>
    RingDistal = 19,

    /// <summary>
    /// The ring finger tip.
    /// </summary>
    RingTip = 20,

    /// <summary>
    /// The little finger metacarpal joint.
    /// </summary>
    LittleMetacarpal = 21,

    /// <summary>
    /// The little finger proximal joint (knuckle).
    /// </summary>
    LittleProximal = 22,

    /// <summary>
    /// The little finger intermediate joint.
    /// </summary>
    LittleIntermediate = 23,

    /// <summary>
    /// The little finger distal joint.
    /// </summary>
    LittleDistal = 24,

    /// <summary>
    /// The little finger tip.
    /// </summary>
    LittleTip = 25,

    /// <summary>
    /// The count of the hand joints.
    /// </summary>
    API_ENUM(Attributes="HideInEditor")
    MAX
};

/// <summary>
/// The range of motion of the tracked hand joints when the hand holds a controller.
/// </summary>
API_ENUM() enum class XRHandMotionRange
{
    /// <summary>
    /// The joints move as if the hand was not holding the controller, so the fingers can close into a fist that goes through the controller model.
    /// </summary>
    Unobstructed = 0,

    /// <summary>
    /// The joints wrap around the held controller, so the fingers don't go through the controller model.
    /// </summary>
    ConformingToController = 1,
};

/// <summary>
/// The source of the tracked hand joints data.
/// </summary>
API_ENUM() enum class XRHandTrackingSource
{
    /// <summary>
    /// The hand is not tracked or the XR runtime doesn't report the source.
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// The hand is tracked directly (eg. by the headset cameras) without holding a controller.
    /// </summary>
    Unobstructed = 1,

    /// <summary>
    /// The hand pose is estimated from the held controller (eg. from the capacitive sensors and the buttons).
    /// </summary>
    Controller = 2,
};

/// <summary>
/// The XR service used for virtual reality head-mounted displays.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API XR
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(XR);

public:
    /// <summary>
    /// Gets a value indicating whether XR is used by the game (enabled in the XR settings or with the -xr command line option). The XR runtime might still be unavailable.
    /// </summary>
    /// <returns>True if XR is enabled, otherwise false.</returns>
    API_PROPERTY() static bool IsEnabled();

    /// <summary>
    /// Gets a value indicating whether connecting to the XR runtime is in progress (see Reconnect).
    /// </summary>
    /// <returns>True if connecting to the XR runtime, otherwise false.</returns>
    API_PROPERTY() static bool IsConnecting();

    /// <summary>
    /// Gets a value indicating whether the XR runtime is connected. The head-mounted display might still be not found (see IsAvailable).
    /// </summary>
    /// <returns>True if the XR runtime is connected, otherwise false.</returns>
    API_PROPERTY() static bool IsConnected();

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
    /// Connects to the XR runtime again if it's not available (eg. the runtime was not running when the engine started). Connecting runs in the background and can start the runtime, which takes a few seconds (see IsConnecting). Does nothing if XR is disabled, or the runtime is already connected or connecting.
    /// </summary>
    API_FUNCTION() static void Reconnect();

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
    /// Gets a value indicating whether the XR runtime and the head-mounted display support tracking the hand joints. Requires the runtime support for XR_EXT_hand_tracking and the Hand Tracking option in the XR settings. Valid only if XR is active.
    /// </summary>
    /// <returns>True if hand tracking is supported, otherwise false.</returns>
    API_PROPERTY() static bool IsHandTrackingSupported();

    /// <summary>
    /// Gets the range of motion of the tracked hand joints when the hand holds a controller. Requires the runtime support for XR_EXT_hand_joints_motion_range.
    /// </summary>
    /// <returns>The motion range.</returns>
    API_PROPERTY() static XRHandMotionRange GetHandMotionRange();

    /// <summary>
    /// Sets the range of motion of the tracked hand joints when the hand holds a controller. Requires the runtime support for XR_EXT_hand_joints_motion_range.
    /// </summary>
    /// <param name="value">The motion range.</param>
    API_PROPERTY() static void SetHandMotionRange(XRHandMotionRange value);

    /// <summary>
    /// Gets the pose of the tracked hand joint.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <param name="joint">The hand joint.</param>
    /// <param name="pose">The result pose (relative to the tracking space origin). The forward direction points along the bone towards the fingertip and the up direction points out of the back of the hand.</param>
    /// <param name="radius">The result radius of the joint (distance from the joint center to the skin surface, in units).</param>
    /// <returns>True if the joint is tracked, otherwise false.</returns>
    API_FUNCTION() static bool GetHandJointPose(XRHand hand, XRHandJoint joint, API_PARAM(Out) Transform& pose, API_PARAM(Out) float& radius);

    /// <summary>
    /// Gets the source of the tracked hand joints data (eg. the headset cameras or the held controller). Requires the runtime support for XR_EXT_hand_tracking_data_source.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <returns>The hand tracking source, or Unknown if the hand is not tracked.</returns>
    API_FUNCTION() static XRHandTrackingSource GetHandTrackingSource(XRHand hand);

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
