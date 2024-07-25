// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "KeyboardKeys.h"
#include "Enums.h"
#include "VirtualInput.h"

class Mouse;
class Keyboard;
class Gamepad;
class InputDevice;

/// <summary>
/// The user input handling service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Input
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Input);

    /// <summary>
    /// Gets the mouse (null if platform does not support mouse or it is not connected).
    /// </summary>
    API_FIELD(ReadOnly) static Mouse* Mouse;

    /// <summary>
    /// Gets the keyboard (null if platform does not support keyboard or it is not connected).
    /// </summary>
    API_FIELD(ReadOnly) static Keyboard* Keyboard;

    /// <summary>
    /// Gets the gamepads.
    /// </summary>
    API_FIELD(ReadOnly) static Array<Gamepad*, FixedAllocation<MAX_GAMEPADS>> Gamepads;

    /// <summary>
    /// Gets the gamepads count.
    /// </summary>
    /// <returns>The amount of active gamepads devices.</returns>
    API_PROPERTY() static int32 GetGamepadsCount();

    /// <summary>
    /// Gets the gamepads count.
    /// </summary>
    /// <param name="index">The gamepad index.</param>
    /// <returns>The gamepad device or null if index is invalid.</returns>
    API_FUNCTION() static Gamepad* GetGamepad(int32 index);

    /// <summary>
    /// Action called when gamepads collection gets changed (during input update).
    /// </summary>
    API_EVENT() static Action GamepadsChanged;

    /// <summary>
    /// Called when gamepads collection gets changed.
    /// </summary>
    static void OnGamepadsChanged();

    /// <summary>
    /// Gets or sets the custom input devices.
    /// </summary>
    static Array<InputDevice*, InlinedAllocation<16>> CustomDevices;

public:

    /// <summary>
    /// Event fired on character input.
    /// </summary>
    API_EVENT() static Delegate<Char> CharInput;

    /// <summary>
    /// Event fired on key pressed.
    /// </summary>
    API_EVENT() static Delegate<KeyboardKeys> KeyDown;

    /// <summary>
    /// Event fired on key released.
    /// </summary>
    API_EVENT() static Delegate<KeyboardKeys> KeyUp;

    /// <summary>
    /// Event fired when mouse button goes down.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, MouseButton> MouseDown;

    /// <summary>
    /// Event fired when mouse button goes up.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, MouseButton> MouseUp;

    /// <summary>
    /// Event fired when mouse button double clicks.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, MouseButton> MouseDoubleClick;

    /// <summary>
    /// Event fired when mouse wheel is scrolling (wheel delta is normalized).
    /// </summary>
    API_EVENT() static Delegate<const Float2&, float> MouseWheel;

    /// <summary>
    /// Event fired when mouse moves.
    /// </summary>
    API_EVENT() static Delegate<const Float2&> MouseMove;

    /// <summary>
    /// Event fired when mouse moves while in relative mode.
    /// </summary>
    API_EVENT() static Delegate<const Float2&> MouseMoveRelative;

    /// <summary>
    /// Event fired when mouse leaves window.
    /// </summary>
    API_EVENT() static Action MouseLeave;

    /// <summary>
    /// Event fired when touch action begins.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, int32> TouchDown;

    /// <summary>
    /// Event fired when touch action moves.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, int32> TouchMove;

    /// <summary>
    /// Event fired when touch action ends.
    /// </summary>
    API_EVENT() static Delegate<const Float2&, int32> TouchUp;

public:
    /// <summary>
    /// Gets the text entered during the current frame (Unicode).
    /// </summary>
    /// <returns>The input text (Unicode).</returns>
    API_PROPERTY() static StringView GetInputText();

    /// <summary>
    /// Gets the key state (true if key is being pressed during this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True while the user holds down the key identified by id</returns>
    API_FUNCTION() static bool GetKey(KeyboardKeys key);

    /// <summary>
    /// Gets the key 'down' state (true if key was pressed in this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True during the frame the user starts pressing down the key</returns>
    API_FUNCTION() static bool GetKeyDown(KeyboardKeys key);

    /// <summary>
    /// Gets the key 'up' state (true if key was released in this frame).
    /// </summary>
    /// <param name="key">Key ID to check</param>
    /// <returns>True during the frame the user releases the key</returns>
    API_FUNCTION() static bool GetKeyUp(KeyboardKeys key);

public:
    /// <summary>
    /// Gets the mouse position in game window coordinates.
    /// </summary>
    /// <returns>Mouse cursor coordinates</returns>
    API_PROPERTY() static Float2 GetMousePosition();

    /// <summary>
    /// Sets the mouse position in game window coordinates.
    /// </summary>
    /// <param name="position">Mouse position to set on</param>
    API_PROPERTY() static void SetMousePosition(const Float2& position);

    /// <summary>
    /// Gets the mouse position in screen-space coordinates.
    /// </summary>
    /// <returns>Mouse cursor coordinates</returns>
    API_PROPERTY() static Float2 GetMouseScreenPosition();

    /// <summary>
    /// Sets the mouse position in screen-space coordinates.
    /// </summary>
    /// <param name="position">Mouse position to set on</param>
    API_PROPERTY() static void SetMouseScreenPosition(const Float2& position);

    /// <summary>
    /// Gets the mouse position change during the last frame.
    /// </summary>
    /// <returns>Mouse cursor position delta</returns>
    API_PROPERTY() static Float2 GetMousePositionDelta();

    /// <summary>
    /// Gets the mouse wheel change during the last frame.
    /// </summary>
    /// <returns>Mouse wheel value delta</returns>
    API_PROPERTY() static float GetMouseScrollDelta();

    /// <summary>
    /// Gets the mouse button state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True while the user holds down the button</returns>
    API_FUNCTION() static bool GetMouseButton(MouseButton button);

    /// <summary>
    /// Gets the mouse button down state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True during the frame the user starts pressing down the button</returns>
    API_FUNCTION() static bool GetMouseButtonDown(MouseButton button);

    /// <summary>
    /// Gets the mouse button up state.
    /// </summary>
    /// <param name="button">Mouse button to check</param>
    /// <returns>True during the frame the user releases the button</returns>
    API_FUNCTION() static bool GetMouseButtonUp(MouseButton button);

public:
    /// <summary>
    /// Gets the gamepad axis value.
    /// </summary>
    /// <param name="gamepadIndex">The gamepad index</param>
    /// <param name="axis">Gamepad axis to check</param>
    /// <returns>Axis value.</returns>
    API_FUNCTION() static float GetGamepadAxis(int32 gamepadIndex, GamepadAxis axis);

    /// <summary>
    /// Gets the gamepad button state (true if being pressed during the current frame).
    /// </summary>
    /// <param name="gamepadIndex">The gamepad index</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user holds down the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButton(int32 gamepadIndex, GamepadButton button);

    /// <summary>
    /// Gets the gamepad button down state (true if was pressed during the current frame).
    /// </summary>
    /// <param name="gamepadIndex">The gamepad index</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user starts pressing down the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButtonDown(int32 gamepadIndex, GamepadButton button);

    /// <summary>
    /// Gets the gamepad button up state (true if was released during the current frame).
    /// </summary>
    /// <param name="gamepadIndex">The gamepad index</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user releases the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButtonUp(int32 gamepadIndex, GamepadButton button);

    /// <summary>
    /// Gets the gamepad axis value.
    /// </summary>
    /// <param name="gamepad">The gamepad</param>
    /// <param name="axis">Gamepad axis to check</param>
    /// <returns>Axis value.</returns>
    API_FUNCTION() static float GetGamepadAxis(InputGamepadIndex gamepad, GamepadAxis axis);

    /// <summary>
    /// Gets the gamepad button state (true if being pressed during the current frame).
    /// </summary>
    /// <param name="gamepad">The gamepad</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user holds down the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButton(InputGamepadIndex gamepad, GamepadButton button);

    /// <summary>
    /// Gets the gamepad button down state (true if was pressed during the current frame).
    /// </summary>
    /// <param name="gamepad">The gamepad</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user starts pressing down the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButtonDown(InputGamepadIndex gamepad, GamepadButton button);

    /// <summary>
    /// Gets the gamepad button up state (true if was released during the current frame).
    /// </summary>
    /// <param name="gamepad">The gamepad</param>
    /// <param name="button">Gamepad button to check</param>
    /// <returns>True if user releases the button, otherwise false.</returns>
    API_FUNCTION() static bool GetGamepadButtonUp(InputGamepadIndex gamepad, GamepadButton button);

public:
    /// <summary>
    /// Maps a discrete button or key press events to a "friendly name" that will later be bound to event-driven behavior. The end effect is that pressing (and/or releasing) a key, mouse button, or keypad button.
    /// </summary>
    API_FIELD() static Array<ActionConfig> ActionMappings;

    /// <summary>
    /// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
    /// </summary>
    API_FIELD() static Array<AxisConfig> AxisMappings;

    /// <summary>
    /// Event fired when virtual input action is triggered. Called before scripts update. See <see cref="ActionMappings"/> to edit configuration.
    /// </summary>
    /// <seealso cref="InputEvent"/>
    API_EVENT() static Delegate<StringView, InputActionState> ActionTriggered;

    /// <summary>
    /// Event fired when virtual input axis is changed. Called before scripts update. See <see cref="AxisMappings"/> to edit configuration.
    /// </summary>
    /// <seealso cref="InputAxis"/>
    API_EVENT() static Delegate<StringView> AxisValueChanged;

    /// <summary>
    /// Gets the value of the virtual action identified by name. Use <see cref="ActionMappings"/> to get the current config.
    /// </summary>
    /// <param name="name">The action name.</param>
    /// <returns>True if action has been triggered in the current frame (e.g. button pressed), otherwise false.</returns>
    /// <seealso cref="ActionMappings"/>
    API_FUNCTION() static bool GetAction(const StringView& name);

    /// <summary>
    /// Gets the value of the virtual action identified by name. Use <see cref="ActionMappings"/> to get the current config.
    /// </summary>
    /// <param name="name">The action name.</param>
    /// <returns>A InputActionPhase determining the current phase of the Action (e.g If it was just pressed, is being held or just released).</returns>
    /// <seealso cref="ActionMappings"/>
    API_FUNCTION() static InputActionState GetActionState(const StringView& name);

    /// <summary>
    /// Gets the value of the virtual axis identified by name. Use <see cref="AxisMappings"/> to get the current config.
    /// </summary>
    /// <param name="name">The action name.</param>
    /// <returns>The current axis value (e.g for gamepads it's in the range -1..1). Value is smoothed to reduce artifacts.</returns>
    /// <seealso cref="AxisMappings"/>
    API_FUNCTION() static float GetAxis(const StringView& name);

    /// <summary>
    /// Gets the raw value of the virtual axis identified by name with no smoothing filtering applied. Use <see cref="AxisMappings"/> to get the current config.
    /// </summary>
    /// <param name="name">The action name.</param>
    /// <returns>The current axis value (e.g for gamepads it's in the range -1..1). No smoothing applied.</returns>
    /// <seealso cref="AxisMappings"/>
    API_FUNCTION() static float GetAxisRaw(const StringView& name);
};
