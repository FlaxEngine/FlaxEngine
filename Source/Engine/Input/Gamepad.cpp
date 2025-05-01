// Copyright (c) Wojciech Figat. All rights reserved.

#include "Gamepad.h"

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
