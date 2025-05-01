// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Enums.h"
#include "KeyboardKeys.h"

/// <summary>
/// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
/// </summary>
API_STRUCT() struct ActionConfig
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ActionConfig);

    /// <summary>
    /// The action "friendly name" used to access it from code.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    String Name;

    /// <summary>
    /// The trigger mode. Allows to specify when input event should be fired.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(5)")
    InputActionMode Mode;

    /// <summary>
    /// The keyboard key to map for this action. Use <see cref="KeyboardKeys.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    KeyboardKeys Key;

    /// <summary>
    /// The mouse button to map for this action. Use <see cref="MouseButton.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    MouseButton MouseButton;

    /// <summary>
    /// The gamepad button to map for this action. Use <see cref="GamepadButton.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    GamepadButton GamepadButton;

    /// <summary>
    /// Which gamepad should be used.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40)")
    InputGamepadIndex Gamepad;
};

/// <summary>
/// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
/// </summary>
API_STRUCT() struct AxisConfig
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AxisConfig);

    /// <summary>
    /// The axis "friendly name" used to access it from code.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    String Name;

    /// <summary>
    /// The axis type (mouse, gamepad, etc.).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    InputAxisType Axis;

    /// <summary>
    /// Which gamepad should be used.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    InputGamepadIndex Gamepad;

    /// <summary>
    /// The button to be pressed for movement in positive direction. Use <see cref="KeyboardKeys.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    KeyboardKeys PositiveButton;

    /// <summary>
    /// The button to be pressed for movement in negative direction. Use <see cref="KeyboardKeys.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40)")
    KeyboardKeys NegativeButton;

    /// <summary>
    /// The button to be pressed for movement in positive direction. Use <see cref="GamepadButton.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(45)")
    GamepadButton GamepadPositiveButton;

    /// <summary>
    /// The button to be pressed for movement in negative direction. Use <see cref="GamepadButton.None"/> to ignore it.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(46)")
    GamepadButton GamepadNegativeButton;

    /// <summary>
    /// Any positive or negative values that are less than this number will register as zero. Useful for gamepads to specify the deadzone.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50)")
    float DeadZone;

    /// <summary>
    /// For keyboard input, a larger value will result in faster response time (in units/s). A lower value will be more smooth. For Mouse delta the value will scale the actual mouse delta.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60)")
    float Sensitivity;

    /// <summary>
    /// For keyboard input describes how fast will the input recenter. Speed (in units/s) that output value will rest to neutral value if not when device at rest.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(70)")
    float Gravity;

    /// <summary>
    /// Additional scale parameter applied to the axis value. Allows to invert it or modify the range.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(80)")
    float Scale;

    /// <summary>
    /// If enabled, the axis value will be immediately reset to zero after it receives opposite inputs. For keyboard input only.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(90)")
    bool Snap;
};
