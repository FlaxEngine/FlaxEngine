// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if XR_API_OPENXR

#include "Engine/Core/Math/Transform.h"
#include "Engine/XR/XR.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

class Gamepad;

/// <summary>
/// The OpenXR input with the controller actions, poses and haptics. The controller buttons and axes are exposed as a gamepad.
/// </summary>
class OpenXRInput
{
public:
    /// <summary>
    /// Creates the action set and suggests the bindings for the supported controllers. Called once for the OpenXR instance.
    /// </summary>
    /// <param name="instance">The OpenXR instance.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Init(XrInstance instance);

    /// <summary>
    /// Checks if the action set has been created for the OpenXR instance.
    /// </summary>
    /// <returns>True if initialized, otherwise false.</returns>
    static bool IsInitialized();

    /// <summary>
    /// Attaches the action set to the session and creates the pose spaces.
    /// </summary>
    /// <param name="session">The OpenXR session.</param>
    /// <param name="space">The reference space used to locate the poses.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool InitSession(XrSession session, XrSpace space);

    /// <summary>
    /// Destroys the pose spaces of the session.
    /// </summary>
    static void DisposeSession();

    /// <summary>
    /// Destroys the action set.
    /// </summary>
    static void Dispose();

    /// <summary>
    /// Synchronizes the actions state and locates the poses at the predicted display time.
    /// </summary>
    /// <param name="time">The predicted display time of the frame.</param>
    static void Update(XrTime time);

    /// <summary>
    /// Locates the poses at the predicted display time (used to update the poses right before rendering).
    /// </summary>
    /// <param name="time">The predicted display time of the frame.</param>
    static void LocatePoses(XrTime time);

    /// <summary>
    /// Logs the interaction profile used by the runtime for each hand.
    /// </summary>
    static void LogInteractionProfiles();

    /// <summary>
    /// Gets the pose of the head-mounted display.
    /// </summary>
    /// <param name="pose">The result pose (relative to the tracking space).</param>
    /// <returns>True if the pose is valid, otherwise false.</returns>
    static bool GetHeadPose(Transform& pose);

    /// <summary>
    /// Gets the pose of the controller.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <param name="type">The pose type.</param>
    /// <param name="pose">The result pose (relative to the tracking space).</param>
    /// <returns>True if the pose is valid, otherwise false.</returns>
    static bool GetHandPose(XRHand hand, XRPoseType type, Transform& pose);

    /// <summary>
    /// Checks if the hand trackers have been created for the session.
    /// </summary>
    /// <returns>True if hand tracking is supported, otherwise false.</returns>
    static bool IsHandTrackingSupported();

    /// <summary>
    /// Gets the pose of the tracked hand joint.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <param name="joint">The hand joint.</param>
    /// <param name="pose">The result pose (relative to the tracking space).</param>
    /// <param name="radius">The result radius of the joint (in units).</param>
    /// <returns>True if the joint is tracked, otherwise false.</returns>
    static bool GetHandJointPose(XRHand hand, XRHandJoint joint, Transform& pose, float& radius);

    /// <summary>
    /// Gets the source of the tracked hand joints data.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <returns>The hand tracking source.</returns>
    static XRHandTrackingSource GetHandTrackingSource(XRHand hand);

    /// <summary>
    /// Starts the controller vibration.
    /// </summary>
    /// <param name="hand">The hand.</param>
    /// <param name="amplitude">The vibration amplitude (normalized). Zero stops the vibration.</param>
    /// <param name="duration">The vibration duration (in seconds).</param>
    /// <param name="frequency">The vibration frequency (in Hz). Zero uses the default frequency of the controller.</param>
    static void Vibrate(XRHand hand, float amplitude, float duration, float frequency);

    /// <summary>
    /// Gets the gamepad that exposes the controller buttons and axes.
    /// </summary>
    /// <returns>The gamepad, or null if the session is not running.</returns>
    static Gamepad* GetGamepad();
};

#endif
