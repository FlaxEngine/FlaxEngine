// Copyright (c) Wojciech Figat. All rights reserved.

#include "Gamepad.h"

namespace
{
    GamepadAxis GetButtonAxis(GamepadButton button, bool& positive)
    {
        positive = true;
        switch (button)
        {
        case GamepadButton::LeftTrigger:
            return GamepadAxis::LeftTrigger;
        case GamepadButton::RightTrigger:
            return GamepadAxis::RightTrigger;
        case GamepadButton::LeftStickUp:
            return GamepadAxis::LeftStickY;
        case GamepadButton::LeftStickDown:
            positive = false;
            return GamepadAxis::LeftStickY;
        case GamepadButton::LeftStickLeft:
            positive = false;
            return GamepadAxis::LeftStickX;
        case GamepadButton::LeftStickRight:
            return GamepadAxis::LeftStickX;
        case GamepadButton::RightStickUp:
            return GamepadAxis::RightStickY;
        case GamepadButton::RightStickDown:
            positive = false;
            return GamepadAxis::RightStickY;
        case GamepadButton::RightStickLeft:
            positive = false;
            return GamepadAxis::RightStickX;
        case GamepadButton::RightStickRight:
            return GamepadAxis::RightStickX;
        default:
            return GamepadAxis::None;
        }
    }

    bool GetButtonState(const Gamepad::State& state, GamepadButton button, float deadZone)
    {
        if (deadZone > 0.01f)
        {
            switch (button)
            {
            case GamepadButton::LeftTrigger:
            case GamepadButton::RightTrigger:
            case GamepadButton::LeftStickUp:
            case GamepadButton::LeftStickDown:
            case GamepadButton::LeftStickLeft:
            case GamepadButton::LeftStickRight:
            case GamepadButton::RightStickUp:
            case GamepadButton::RightStickDown:
            case GamepadButton::RightStickLeft:
            case GamepadButton::RightStickRight:
            {
                bool positive;
                float axis = state.Axis[(int32)GetButtonAxis(button, positive)];
                return positive ? axis >= deadZone : axis <= -deadZone;
            }
            default:
                break;
            }
        }
        return state.Buttons[(int32)button];
    }
}

void GamepadLayout::Init()
{
    for (int32 i = 0; i < (int32)GamepadButton::MAX; i++)
        Buttons[i] = (GamepadButton)i;
    for (int32 i = 0; i < (int32)GamepadAxis::MAX; i++)
        Axis[i] = (GamepadAxis)i;
    for (int32 i = 0; i < (int32)GamepadAxis::MAX; i++)
        AxisMap[i] = Float2::UnitX;
}

Gamepad::Gamepad(const Guid& productId, const String& name)
    : InputDevice(SpawnParams(Guid::New(), TypeInitializer), name)
    , _productId(productId)
{
    _state.Clear();
    _mappedState.Clear();
    _mappedPrevState.Clear();
    Layout.Init();
}

void Gamepad::ResetState()
{
    InputDevice::ResetState();

    _state.Clear();
    _mappedState.Clear();
    _mappedPrevState.Clear();
}

bool Gamepad::GetButton(GamepadButton button, float deadZone) const
{
    return GetButtonState(_mappedState, button, deadZone);
}

bool Gamepad::GetButtonDown(GamepadButton button, float deadZone) const
{
    return GetButtonState(_mappedState, button, deadZone) && !GetButtonState(_mappedPrevState, button, deadZone);
}

bool Gamepad::GetButtonUp(GamepadButton button, float deadZone) const
{
    return !GetButtonState(_mappedState, button, deadZone) && GetButtonState(_mappedPrevState, button, deadZone);
}

bool Gamepad::IsAnyButtonDown() const
{
    // TODO: optimize with SIMD
    bool result = false;
    for (auto e : _state.Buttons)
        result |= e;
    return result;
}

bool Gamepad::Update(EventQueue& queue)
{
    // Copy state
    Platform::MemoryCopy(&_mappedPrevState, &_mappedState, sizeof(State));
    _mappedState.Clear();

    // Gather current state
    if (UpdateState())
        return true;

    // Map state
    for (int32 i = 0; i < (int32)GamepadButton::MAX; i++)
    {
        auto e = Layout.Buttons[i];
        _mappedState.Buttons[static_cast<int32>(e)] = _state.Buttons[i];
    }
    for (int32 i = 0; i < (int32)GamepadAxis::MAX; i++)
    {
        auto e = Layout.Axis[i];
        float value = _state.Axis[i];
        value = Layout.AxisMap[i].X * value + Layout.AxisMap[i].Y;
        _mappedState.Axis[static_cast<int32>(e)] = value;
    }

    return false;
}
