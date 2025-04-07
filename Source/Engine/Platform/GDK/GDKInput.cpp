// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_XBOX_ONE || PLATFORM_XBOX_SCARLETT

#include "GDKInput.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include <GameInput.h>

static_assert(sizeof(Guid) <= sizeof(APP_LOCAL_DEVICE_ID), "Invalid Game Input deviceId size.");

String ToString(GameInputString const* name)
{
    return name && name->data ? String(name->data) : String::Empty;
}

/// <summary>
/// Implementation of the gamepad device for GDK platform.
/// </summary>
/// <seealso cref="Gamepad" />
class GDKGamepad : public Gamepad
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GDKGamepad"/> class.
    /// </summary>
    /// <param name="device">The input device.</param>
    explicit GDKGamepad(IGameInputDevice* device)
        : Gamepad(*(Guid*)&device->GetDeviceInfo()->deviceId, ::ToString(device->GetDeviceInfo()->displayName))
        , Device(device)
    {
        Device->AddRef();
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GDKGamepad"/> class.
    /// </summary>
    ~GDKGamepad()
    {
        Device->Release();
    }

    /// <summary>
    /// The device.
    /// </summary>
    IGameInputDevice* Device;

public:

    // [Gamepad]
    void SetVibration(const GamepadVibrationState& state) override;

protected:

    // [Gamepad]
    bool UpdateState() override;
};

namespace GDKInputImpl
{
    IGameInput* GameInput = nullptr;
    IGameInputReading* PrevReading = nullptr;
}

using namespace GDKInputImpl;

bool IsSameDevice(const APP_LOCAL_DEVICE_ID& first, const APP_LOCAL_DEVICE_ID& second)
{
    return memcmp(&first, &second, APP_LOCAL_DEVICE_ID_SIZE) == 0;
}

bool IsSameDevice(IGameInputDevice* first, IGameInputDevice* second)
{
    return IsSameDevice(first->GetDeviceInfo()->deviceId, second->GetDeviceInfo()->deviceId);
}

void TryAddGamepad(IGameInputDevice* device)
{
    for (auto& gamepad : Input::Gamepads)
    {
        if (IsSameDevice(((GDKGamepad*)gamepad)->Device, device))
            return;
    }

    Input::Gamepads.Add(New<GDKGamepad>(device));
    Input::OnGamepadsChanged();
}

void CALLBACK OnDeviceEnumerated(
    _In_ GameInputCallbackToken callbackToken,
    _In_ void* context,
    _In_ IGameInputDevice* device,
    _In_ uint64_t timestamp,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus previousStatus)
{
    TryAddGamepad(device);
}

void GDKInput::Init()
{
    GameInputCreate(&GameInput);

    // Find connected devices
    GameInputCallbackToken token;
    if (SUCCEEDED(GameInput->RegisterDeviceCallback(
        nullptr,
        GameInputKindGamepad,
        GameInputDeviceAnyStatus,
        GameInputBlockingEnumeration,
        nullptr,
        OnDeviceEnumerated,
        &token)))
    {
        GameInput->UnregisterCallback(token, 5000);
    }
}

void GDKInput::Exit()
{
    if (PrevReading)
    {
        PrevReading->Release();
        PrevReading = nullptr;
    }
    if (GameInput)
    {
        GameInput->Release();
        GameInput = nullptr;
    }
}

void GDKInput::Update()
{
    while (true)
    {
        // Read input
        IGameInputReading* reading = nullptr;
        if (!PrevReading)
        {
            if (FAILED(GDKInputImpl::GameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
            {
                break;
            }
            PrevReading = reading;
        }
        else
        {
            const HRESULT hr = GameInput->GetNextReading(PrevReading, GameInputKindGamepad, nullptr, &reading);
            if (SUCCEEDED(hr))
            {
                PrevReading->Release();
                PrevReading = reading;
            }
            else if (hr != GAMEINPUT_E_READING_NOT_FOUND)
            {
                PrevReading->Release();
                PrevReading = nullptr;
                break;
            }
        }
        if (!reading)
            break;

        // Check if new device was connected
        IGameInputDevice* device;
        reading->GetDevice(&device);
        TryAddGamepad(device);
        device->Release();
    }
}

void GDKGamepad::SetVibration(const GamepadVibrationState& state)
{
    GameInputRumbleParams vibration;
    vibration.lowFrequency = state.LeftSmall;
    vibration.highFrequency = state.RightSmall;
    vibration.leftTrigger = state.LeftLarge;
    vibration.rightTrigger = state.RightLarge;
    Device->SetRumbleState(&vibration);
}

bool GDKGamepad::UpdateState()
{
    // Gather device state
    IGameInputReading* reading = nullptr;
    if (FAILED(GDKInputImpl::GameInput->GetCurrentReading(GameInputKindGamepad,Device, &reading)))
    {
        return true;
    }
    GameInputGamepadState state;
    if (!reading->GetGamepadState(&state))
    {
        return true;
    }
    const float deadZone = 0.2f;

    // Process buttons state
    _state.Buttons[(int32)GamepadButton::A] = state.buttons & GameInputGamepadA;
    _state.Buttons[(int32)GamepadButton::B] = state.buttons & GameInputGamepadB;
    _state.Buttons[(int32)GamepadButton::X] = state.buttons & GameInputGamepadX;
    _state.Buttons[(int32)GamepadButton::Y] = state.buttons & GameInputGamepadY;
    _state.Buttons[(int32)GamepadButton::LeftShoulder] = state.buttons & GameInputGamepadLeftShoulder;
    _state.Buttons[(int32)GamepadButton::RightShoulder] = state.buttons & GameInputGamepadRightShoulder;
    _state.Buttons[(int32)GamepadButton::Back] = state.buttons & GameInputGamepadMenu;
    _state.Buttons[(int32)GamepadButton::Start] = state.buttons & GameInputGamepadView;
    _state.Buttons[(int32)GamepadButton::LeftThumb] = state.buttons & GameInputGamepadLeftThumbstick;
    _state.Buttons[(int32)GamepadButton::RightThumb] = state.buttons & GameInputGamepadRightThumbstick;
    _state.Buttons[(int32)GamepadButton::LeftTrigger] = state.leftTrigger > deadZone;
    _state.Buttons[(int32)GamepadButton::RightTrigger] = state.rightTrigger > deadZone;
    _state.Buttons[(int32)GamepadButton::DPadUp] = state.buttons & GameInputGamepadDPadUp;
    _state.Buttons[(int32)GamepadButton::DPadDown] = state.buttons & GameInputGamepadDPadDown;
    _state.Buttons[(int32)GamepadButton::DPadLeft] = state.buttons & GameInputGamepadDPadLeft;
    _state.Buttons[(int32)GamepadButton::DPadRight] = state.buttons & GameInputGamepadDPadRight;
    _state.Buttons[(int32)GamepadButton::LeftStickUp] = state.leftThumbstickY > deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickDown] = state.leftThumbstickY < -deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickLeft] = state.leftThumbstickX < -deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickRight] = state.leftThumbstickX > deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickUp] = state.rightThumbstickY > deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickDown] = state.rightThumbstickY < -deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickLeft] = state.rightThumbstickX < -deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickRight] = state.rightThumbstickX > deadZone;

    // Process axes state
    _state.Axis[(int32)GamepadAxis::LeftStickX] = state.leftThumbstickX;
    _state.Axis[(int32)GamepadAxis::LeftStickY] = state.leftThumbstickY;
    _state.Axis[(int32)GamepadAxis::RightStickX] = state.rightThumbstickX;
    _state.Axis[(int32)GamepadAxis::RightStickY] = state.rightThumbstickY;
    _state.Axis[(int32)GamepadAxis::LeftTrigger] = state.leftTrigger;
    _state.Axis[(int32)GamepadAxis::RightTrigger] = state.rightTrigger;

    reading->Release();

    return false;
}

#elif PLATFORM_GDK

void GDKInput::Init()
{
}

void GDKInput::Exit()
{
}

void GDKInput::Update()
{    
}

#endif
