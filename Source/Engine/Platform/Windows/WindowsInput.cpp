// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsInput.h"
#include "WindowsWindow.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Gamepad.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Math/Int2.h"
#include "../Win32/IncludeWindowsHeaders.h"

#define DIRECTINPUT_VERSION 0x0800
#include <Xinput.h>
#include <hidusage.h>

/// <summary>
/// Implementation of the keyboard device for Windows platform.
/// </summary>
/// <seealso cref="Keyboard" />
class WindowsKeyboard : public Keyboard
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowsKeyboard"/> class.
    /// </summary>
    explicit WindowsKeyboard()
        : Keyboard()
    {
    }

public:

    bool WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
    bool WndProcRawInput(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
};

/// <summary>
/// Implementation of the mouse device for Windows platform.
/// </summary>
/// <seealso cref="Mouse" />
class WindowsMouse : public Mouse
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowsMouse"/> class.
    /// </summary>
    explicit WindowsMouse()
        : Mouse()
    {
    }

public:

    bool WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);
    bool WndProcRawInput(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam);

public:

    // [Mouse]
    void SetMousePosition(const Float2& newPosition) final override
    {
        ::SetCursorPos(static_cast<int32>(newPosition.X), static_cast<int32>(newPosition.Y));

        OnMouseMoved(newPosition);
    }
};

/// <summary>
/// Implementation of the gamepad device for Windows platform.
/// </summary>
/// <seealso cref="Gamepad" />
class WindowsGamepad : public Gamepad
{
private:

    Windows::DWORD _userIndex;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowsGamepad"/> class.
    /// </summary>
    /// <param name="userIndex">The user index.</param>
    explicit WindowsGamepad(Windows::DWORD userIndex);

    /// <summary>
    /// Finalizes an instance of the <see cref="WindowsGamepad"/> class.
    /// </summary>
    ~WindowsGamepad();

public:

    // [Gamepad]
    void SetVibration(const GamepadVibrationState& state) override;

protected:

    // [Gamepad]
    bool UpdateState() override;
};

namespace WindowsInputImpl
{
    float XInputLastUpdateTime = 0;
    bool XInputGamepads[XUSER_MAX_COUNT] = { false };
    WindowsMouse* Mouse = nullptr;
    WindowsKeyboard* Keyboard = nullptr;
}

namespace WindowsRawInputImpl
{
    bool UseRawInput = true;
    Window* LastLeaveWindow = nullptr;
    Window* CurrentInputWindow = nullptr;
}

void WindowsInput::Init()
{
    Input::Mouse = WindowsInputImpl::Mouse = New<WindowsMouse>();
    Input::Keyboard = WindowsInputImpl::Keyboard = New<WindowsKeyboard>();

    if (WindowsRawInputImpl::UseRawInput)
    {
        RAWINPUTDEVICE rid[2] = {};

        // Register generic mouse
        rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        rid[0].hwndTarget = nullptr;
        //rid[0].dwFlags = RIDEV_NOLEGACY; // TODO: Enable when double-click and other legacy events are handled in raw input

        // Register generic keyboard
        rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
        rid[1].hwndTarget = nullptr;

        if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
            LOG(Error, "Failed to register RawInput devices. Error: {0}", GetLastError());
    }
}

void WindowsInput::Update()
{
    // Update every second
    const float time = (float)Platform::GetTimeSeconds();
    if (time - WindowsInputImpl::XInputLastUpdateTime < 1.0f)
        return;
    WindowsInputImpl::XInputLastUpdateTime = time;

    PROFILE_CPU_NAMED("Input.ScanGamepads");

    XINPUT_STATE state;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
    {
        if (!WindowsInputImpl::XInputGamepads[i])
        {
            const DWORD result = XInputGetState(i, &state);
            if (result == ERROR_SUCCESS)
            {
                Input::Gamepads.Add(New<WindowsGamepad>(i));
                Input::OnGamepadsChanged();
            }
        }
    }
}

bool WindowsInput::WndProc(Window* window, Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam)
{
    if (WindowsInputImpl::Mouse->WndProc(window, msg, wParam, lParam))
        return true;
    if (WindowsInputImpl::Keyboard->WndProc(window, msg, wParam, lParam))
        return true;
    return false;
}

bool OnRawInput(Float2 mousePosition, Window* window, HRAWINPUT input)
{
    // TODO: use GetRawInputBuffer to avoid filling the message queue with high polling rate mice

    static BYTE* dataBuffer = nullptr;
    static uint32 dataBufferSize = 0;

    // Query the size of the buffer first, and allocate enough for the buffer
    uint32 dataSize;
    GetRawInputData(input, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
    if (dataSize > dataBufferSize)
    {
        delete[] dataBuffer;
        dataBuffer = new BYTE[dataSize];
        dataBufferSize = dataSize;
    }

    GetRawInputData(input, RID_INPUT, dataBuffer, &dataSize, sizeof(RAWINPUTHEADER));

    // Workaround to send the input to the topmost window after focusing the window from other applications.
    if (WindowsRawInputImpl::LastLeaveWindow == window || WindowsRawInputImpl::LastLeaveWindow == nullptr)
    {
        if (WindowsRawInputImpl::CurrentInputWindow != nullptr)
            window = WindowsRawInputImpl::CurrentInputWindow;
    }

    const RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(dataBuffer);
    if (rawInput->header.dwType == RIM_TYPEKEYBOARD)
    {
        const RAWKEYBOARD rawKeyboard = rawInput->data.keyboard;
        if ((rawKeyboard.Flags & RI_KEY_BREAK) != 0)
            Input::Keyboard->OnKeyUp(static_cast<KeyboardKeys>(rawKeyboard.VKey), window);
        else
            Input::Keyboard->OnKeyDown(static_cast<KeyboardKeys>(rawKeyboard.VKey), window);

        return true;
    }
    if (rawInput->header.dwType == RIM_TYPEMOUSE)
    {
        const RAWMOUSE rawMouse = rawInput->data.mouse;
        Float2 mousePos;
        if ((rawMouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
            const bool virtualDesktop = (rawMouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            mousePos = Float2(Int2(
                static_cast<int>((rawMouse.lLastX / 65535.0f) * width),
                static_cast<int>((rawMouse.lLastY / 65535.0f) * height)));

            Input::Mouse->OnMouseMove(mousePos, window);
        }
        else
        {
            // FIXME: This is wrong. The current mouse position does not include
            // delta movement accumulated during this frame.
            mousePos = mousePosition;

            const Float2 mouseDelta(Int2(rawMouse.lLastX, rawMouse.lLastY));
            mousePos += mouseDelta;

            if (!mouseDelta.IsZero())
                Input::Mouse->OnMouseMoveDelta(mouseDelta, window);
        }

        if ((rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) != 0)
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Left, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) != 0)
            Input::Mouse->OnMouseUp(mousePos, MouseButton::Left, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0)
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Right, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) != 0)
            Input::Mouse->OnMouseUp(mousePos, MouseButton::Right, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0)
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Middle, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) != 0)
            Input::Mouse->OnMouseUp(mousePos, MouseButton::Middle, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) != 0)
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Extended1, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) != 0)
            Input::Mouse->OnMouseUp(mousePos, MouseButton::Extended1, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) != 0)
            Input::Mouse->OnMouseDown(mousePos, MouseButton::Extended2, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) != 0)
            Input::Mouse->OnMouseUp(mousePos, MouseButton::Extended2, window);
        if ((rawMouse.usButtonFlags & RI_MOUSE_WHEEL) != 0)
        {
            const int16 delta = static_cast<int16>(rawMouse.usButtonData);
            const float deltaNormalized = static_cast<float>(delta) / WHEEL_DELTA;
            Input::Mouse->OnMouseWheel(mousePos, deltaNormalized, window);
        }

        return true;
    }

    return false;
}

bool WindowsKeyboard::WndProc(Window* window, const Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam)
{
    if (WindowsRawInputImpl::UseRawInput && WndProcRawInput(window, msg, wParam, lParam))
        return true;

    bool result = false;

    // Handle keyboard related messages
    switch (msg)
    {
    case WM_CHAR:
    {
        OnCharInput(static_cast<Char>(wParam), window);
        result = true;
        break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        OnKeyDown(static_cast<KeyboardKeys>(wParam), window);
        result = true;
        break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        OnKeyUp(static_cast<KeyboardKeys>(wParam), window);
        result = true;
        break;
    }
    }

    return result;
}

bool WindowsKeyboard::WndProcRawInput(Window* window, const Windows::UINT msg, Windows::WPARAM wParam, Windows::LPARAM lParam)
{
    bool result = false;

    // Handle keyboard related messages
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        // Ignored with raw input
        result = true;
        break;
    }
    }

    return result;
}

bool WindowsMouse::WndProc(Window* window, const UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (WindowsRawInputImpl::UseRawInput && WndProcRawInput(window, msg, wParam, lParam))
        return true;

    bool result = false;

    // Get mouse position in screen coordinates
    POINT p;
    p.x = static_cast<LONG>(WINDOWS_GET_X_LPARAM(lParam));
    p.y = static_cast<LONG>(WINDOWS_GET_Y_LPARAM(lParam));
    ::ClientToScreen(window->GetHWND(), &p);
    const Float2 mousePos(static_cast<float>(p.x), static_cast<float>(p.y));

    // Handle mouse related messages
    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        if (!WindowsRawInputImpl::UseRawInput)
        {
            // Calculate mouse deltas since last message
            static Float2 lastPos = mousePos;
            if (_state.MouseWasReset)
            {
                lastPos = _state.MousePosition;
                _state.MouseWasReset = false;
            }

            Float2 deltaPos = mousePos - lastPos;
            if (!deltaPos.IsZero())
                OnMouseMoveDelta(deltaPos, window);
            lastPos = mousePos;
        }

        OnMouseMove(mousePos, window);
        result = true;
        break;
    }    
    case WM_NCMOUSEMOVE:
    {
        OnMouseMove(mousePos, window);
        result = true;
        break;
    }
    case WM_MOUSELEAVE:
    {
        OnMouseLeave(window);
        result = true;
        break;
    }
    case WM_LBUTTONDOWN:
    {
        OnMouseDown(mousePos, MouseButton::Left, window);
        result = true;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        OnMouseDown(mousePos, MouseButton::Right, window);
        result = true;
        break;
    }
    case WM_MBUTTONDOWN:
    {
        OnMouseDown(mousePos, MouseButton::Middle, window);
        result = true;
        break;
    }
    case WM_XBUTTONDOWN:
    {
        const auto button = (HIWORD(wParam) & XBUTTON1) ? MouseButton::Extended1 : MouseButton::Extended2;
        OnMouseDown(mousePos, button, window);

        result = true;
        break;
    }
    case WM_LBUTTONUP:
    {
        OnMouseUp(mousePos, MouseButton::Left, window);
        result = true;
        break;
    }
    case WM_RBUTTONUP:
    {
        OnMouseUp(mousePos, MouseButton::Right, window);
        result = true;
        break;
    }
    case WM_MBUTTONUP:
    {
        OnMouseUp(mousePos, MouseButton::Middle, window);
        result = true;
        break;
    }
    case WM_XBUTTONUP:
    {
        const auto button = (HIWORD(wParam) & XBUTTON1) ? MouseButton::Extended1 : MouseButton::Extended2;
        OnMouseUp(mousePos, button, window);

        result = true;
        break;
    }
    case WM_LBUTTONDBLCLK:
    {
        OnMouseDoubleClick(mousePos, MouseButton::Left, window);
        result = true;
        break;
    }
    case WM_RBUTTONDBLCLK:
    {
        OnMouseDoubleClick(mousePos, MouseButton::Right, window);
        result = true;
        break;
    }
    case WM_MBUTTONDBLCLK:
    {
        OnMouseDoubleClick(mousePos, MouseButton::Middle, window);
        result = true;
        break;
    }
    case WM_MOUSEWHEEL:
    {
        const int16 delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta != 0)
        {
            const float deltaNormalized = static_cast<float>(delta) / WHEEL_DELTA;
            // Use cached mouse position, the input pos is sometimes wrong in Win32
            OnMouseWheel(_state.MousePosition, deltaNormalized, window);
        }

        result = true;
        break;
    }
    }

    return result;
}

bool WindowsMouse::WndProcRawInput(Window* window, const UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool result = false;

    // Handle mouse related messages
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    case WM_MOUSEWHEEL:
    {
        // Ignored with raw input
        WindowsRawInputImpl::CurrentInputWindow = window;
        result = true;
        break;
    }
    case WM_MOUSEMOVE:
    {
        WindowsRawInputImpl::CurrentInputWindow = window;
        break;
    }
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    {
        // Might need to be handled here
        break;
    }
    case WM_MOUSELEAVE:
    {
        WindowsRawInputImpl::LastLeaveWindow = window;
        break;
    }
    case WM_ACTIVATE:
    {
        break;
    }
    case WM_ACTIVATEAPP:
    {
        // Reset when switching between apps
        WindowsRawInputImpl::LastLeaveWindow = nullptr;
        break;
    }
    case WM_INPUT:
    {
        result = OnRawInput(_state.MousePosition, window, reinterpret_cast<HRAWINPUT>(lParam));
        break;
    }
    }

    return result;
}

float NormalizeXInputAxis(const int16 axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    const float norm = axisVal <= 0 ? 32768.0f : 32767.0f;
    return float(axisVal) / norm;
}

WindowsGamepad::WindowsGamepad(Windows::DWORD userIndex)
    : Gamepad(Guid(0, 0, 0, userIndex), TEXT("XInput Gamepad"))
    , _userIndex(userIndex)
{
    WindowsInputImpl::XInputGamepads[_userIndex] = true;
}

WindowsGamepad::~WindowsGamepad()
{
    WindowsInputImpl::XInputGamepads[_userIndex] = false;
}

void WindowsGamepad::SetVibration(const GamepadVibrationState& state)
{
    Gamepad::SetVibration(state);

    XINPUT_VIBRATION xInputVibration;
    const float leftMotor = Math::Saturate((state.LeftLarge + state.LeftSmall) * 0.5f);
    const float rightMotor = Math::Saturate((state.RightLarge + state.RightSmall) * 0.5f);
    xInputVibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    xInputVibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    XInputSetState(_userIndex, &xInputVibration);
}

bool WindowsGamepad::UpdateState()
{
    // Gather XInput device state
    XINPUT_STATE inputState;
    if (XInputGetState(_userIndex, &inputState) != ERROR_SUCCESS)
    {
        return true;
    }

    // Process buttons state
    _state.Buttons[(int32)GamepadButton::A] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_A);
    _state.Buttons[(int32)GamepadButton::B] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_B);
    _state.Buttons[(int32)GamepadButton::X] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_X);
    _state.Buttons[(int32)GamepadButton::Y] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
    _state.Buttons[(int32)GamepadButton::LeftShoulder] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
    _state.Buttons[(int32)GamepadButton::RightShoulder] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
    _state.Buttons[(int32)GamepadButton::Back] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
    _state.Buttons[(int32)GamepadButton::Start] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_START);
    _state.Buttons[(int32)GamepadButton::LeftThumb] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
    _state.Buttons[(int32)GamepadButton::RightThumb] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
    _state.Buttons[(int32)GamepadButton::LeftTrigger] = inputState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    _state.Buttons[(int32)GamepadButton::RightTrigger] = inputState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    _state.Buttons[(int32)GamepadButton::DPadUp] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
    _state.Buttons[(int32)GamepadButton::DPadDown] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
    _state.Buttons[(int32)GamepadButton::DPadLeft] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
    _state.Buttons[(int32)GamepadButton::DPadRight] = !!(inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
    _state.Buttons[(int32)GamepadButton::LeftStickUp] = inputState.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::LeftStickDown] = inputState.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::LeftStickLeft] = inputState.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::LeftStickRight] = inputState.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::RightStickUp] = inputState.Gamepad.sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::RightStickDown] = inputState.Gamepad.sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::RightStickLeft] = inputState.Gamepad.sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    _state.Buttons[(int32)GamepadButton::RightStickRight] = inputState.Gamepad.sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

    // Process axes state
    _state.Axis[(int32)GamepadAxis::LeftStickX] = NormalizeXInputAxis(inputState.Gamepad.sThumbLX);
    _state.Axis[(int32)GamepadAxis::LeftStickY] = NormalizeXInputAxis(inputState.Gamepad.sThumbLY);
    _state.Axis[(int32)GamepadAxis::RightStickX] = NormalizeXInputAxis(inputState.Gamepad.sThumbRX);
    _state.Axis[(int32)GamepadAxis::RightStickY] = NormalizeXInputAxis(inputState.Gamepad.sThumbRY);
    _state.Axis[(int32)GamepadAxis::LeftTrigger] = inputState.Gamepad.bLeftTrigger / 255.0f;
    _state.Axis[(int32)GamepadAxis::RightTrigger] = inputState.Gamepad.bRightTrigger / 255.0f;

    return false;
}

#endif
