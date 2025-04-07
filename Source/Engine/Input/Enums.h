// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

// The maximum amount of supported gamepads connected at once.
#define MAX_GAMEPADS 8

/// <summary>
/// Hardware mouse cursor behavior.
/// </summary>
API_ENUM() enum class CursorLockMode
{
    /// <summary>
    /// The default mode.
    /// </summary>
    None = 0,

    /// <summary>
    /// Cursor position is locked to the center of the game window.
    /// </summary>
    Locked = 1,

    /// <summary>
    /// Cursor position is confined to the bounds of the game window.
    /// </summary>
    Clipped = 2,
};

/// <summary>
/// Mouse buttons types.
/// </summary>
API_ENUM() enum class MouseButton
{
    /// <summary>
    /// No button.
    /// </summary>
    None = 0,

    /// <summary>
    /// Left button.
    /// </summary>
    Left = 1,

    /// <summary>
    /// Middle button.
    /// </summary>
    Middle = 2,

    /// <summary>
    /// Right button.
    /// </summary>
    Right = 3,

    /// <summary>
    /// Extended button 1 (or XButton1).
    /// </summary>
    Extended1 = 4,

    /// <summary>
    /// Extended button 2 (or XButton2).
    /// </summary>
    Extended2 = 5,

    MAX
};

/// <summary>
/// Axis for gamepad.
/// </summary>
API_ENUM() enum class GamepadAxis
{
    /// <summary>
    /// No axis.
    /// </summary>
    None = 0,

    /// <summary>
    /// The X-Axis of the left thumb stick.
    /// </summary>
    LeftStickX = 1,

    /// <summary>
    /// The Y-Axis of the left thumb stick.
    /// </summary>
    LeftStickY = 2,

    /// <summary>
    /// The X-Axis of the right thumb stick.
    /// </summary>
    RightStickX = 3,

    /// <summary>
    /// The Y-Axis of the right thumb stick.
    /// </summary>
    RightStickY = 4,

    /// <summary>
    /// The left trigger.
    /// </summary>
    LeftTrigger = 5,

    /// <summary>
    /// The right trigger.
    /// </summary>
    RightTrigger = 6,

    MAX
};

/// <summary>
/// Buttons for gamepad.
/// </summary>
API_ENUM() enum class GamepadButton
{
    /// <summary>
    /// No buttons.	
    /// </summary>
    None = 0,

    /// <summary>
    /// PadUp button (DPad / Directional Pad).
    /// </summary>
    DPadUp = 1,

    /// <summary>
    /// PadDown button (DPad / Directional Pad).
    /// </summary>
    DPadDown = 2,

    /// <summary>
    /// PadLeft button (DPad / Directional Pad).
    /// </summary>	
    DPadLeft = 3,

    /// <summary>
    /// PadRight button (DPad / Directional Pad).
    /// </summary>	
    DPadRight = 4,

    /// <summary>
    /// Start button.
    /// </summary>
    Start = 5,

    /// <summary>
    /// Back button.
    /// </summary>
    Back = 6,

    /// <summary>
    /// Left thumbstick button.
    /// </summary>
    LeftThumb = 7,

    /// <summary>
    /// Right thumbstick button.
    /// </summary>
    RightThumb = 8,

    /// <summary>
    /// Left shoulder button.
    /// </summary>
    LeftShoulder = 9,

    /// <summary>
    /// Right shoulder button.
    /// </summary>
    RightShoulder = 10,

    /// <summary>
    /// Left trigger button.
    /// </summary>
    LeftTrigger = 11,

    /// <summary>
    /// Right trigger button.
    /// </summary>
    RightTrigger = 12,

    /// <summary>
    /// A (face button down).
    /// </summary>
    A = 13,

    /// <summary>
    /// B (face button right).
    /// </summary>
    B = 14,

    /// <summary>
    /// X (face button left).
    /// </summary>
    X = 15,

    /// <summary>
    /// Y (face button up).
    /// </summary>
    Y = 16,

    /// <summary>
    /// The left stick up.
    /// </summary>
    LeftStickUp = 17,

    /// <summary>
    /// The left stick down.
    /// </summary>
    LeftStickDown = 18,

    /// <summary>
    /// The left stick left.
    /// </summary>
    LeftStickLeft = 19,

    /// <summary>
    /// The left stick right.
    /// </summary>
    LeftStickRight = 20,

    /// <summary>
    /// The right stick up.
    /// </summary>
    RightStickUp = 21,

    /// <summary>
    /// The right stick down.
    /// </summary>
    RightStickDown = 22,

    /// <summary>
    /// The right stick left.
    /// </summary>
    RightStickLeft = 23,

    /// <summary>
    /// The right stick right.
    /// </summary>
    RightStickRight = 24,

    MAX
};

/// <summary>
/// The input action event trigger modes.
/// </summary>
API_ENUM() enum class InputActionMode
{
    /// <summary>
    /// User is pressing the key/button.
    /// </summary>
    Pressing = 0,

    /// <summary>
    /// User pressed the key/button (but wasn't pressing it in the previous frame).
    /// </summary>
    Press = 1,

    /// <summary>
    /// User released the key/button (was pressing it in the previous frame).
    /// </summary>
    Release = 2,
};

/// <summary>
/// The input action event phases.
/// </summary>
API_ENUM() enum class InputActionState
{
    /// <summary>
    /// The key/button is not assigned.
    /// </summary>
    None = 0,

    /// <summary>
    /// The key/button is waiting for input.
    /// </summary>
    Waiting = 1,

    /// <summary>
    /// User is pressing the key/button.
    /// </summary>
    Pressing = 2,

    /// <summary>
    /// User pressed the key/button (but wasn't pressing it in the previous frame).
    /// </summary>
    Press = 3,

    /// <summary>
    /// User released the key/button (was pressing it in the previous frame).
    /// </summary>
    Release = 4,
};

/// <summary>
/// The input gamepad index.
/// </summary>
API_ENUM() enum class InputGamepadIndex
{
    /// <summary>
    /// All detected gamepads.
    /// </summary>
    All = -1,

    /// <summary>
    /// The gamepad no. 0.
    /// </summary>
    Gamepad0 = 0,

    /// <summary>
    /// The gamepad no. 1.
    /// </summary>
    Gamepad1 = 1,

    /// <summary>
    /// The gamepad no. 2.
    /// </summary>
    Gamepad2 = 2,

    /// <summary>
    /// The gamepad no. 3.
    /// </summary>
    Gamepad3 = 3,

    /// <summary>
    /// The gamepad no. 4.
    /// </summary>
    Gamepad4 = 4,

    /// <summary>
    /// The gamepad no. 5.
    /// </summary>
    Gamepad5 = 5,
};

/// <summary>
/// The input axes types.
/// </summary>
API_ENUM() enum class InputAxisType
{
    /// <summary>
    /// The mouse X-Axis (mouse delta position scaled by the sensitivity).
    /// </summary>
    MouseX = 0,

    /// <summary>
    /// The mouse Y-Axis (mouse delta position scaled by the sensitivity).
    /// </summary>
    MouseY = 1,

    /// <summary>
    /// The mouse wheel (mouse wheel delta scaled by the sensitivity).
    /// </summary>
    MouseWheel = 2,

    /// <summary>
    /// The gamepad X-Axis of the left thumb stick.
    /// </summary>
    GamepadLeftStickX = 3,

    /// <summary>
    /// The gamepad Y-Axis of the left thumb stick.
    /// </summary>
    GamepadLeftStickY = 4,

    /// <summary>
    /// The gamepad X-Axis of the right thumb stick.
    /// </summary>
    GamepadRightStickX = 5,

    /// <summary>
    /// The gamepad Y-Axis of the right thumb stick.
    /// </summary>
    GamepadRightStickY = 6,

    /// <summary>
    /// The gamepad left trigger.
    /// </summary>
    GamepadLeftTrigger = 7,

    /// <summary>
    /// The gamepad right trigger.
    /// </summary>
    GamepadRightTrigger = 8,

    /// <summary>
    /// The keyboard only mode. For key inputs.
    /// </summary>
    KeyboardOnly = 9,

    /// <summary>
    /// Pad X axis - left/right (DPad / Directional Pad).
    /// </summary>
    GamepadDPadX = 10,

    /// <summary>
    /// Pad Y axis - up/down (DPad / Directional Pad).
    /// </summary>
    GamepadDPadY = 11,
};
