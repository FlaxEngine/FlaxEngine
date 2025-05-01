// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UWP

#include "UWPWindow.h"
#include "UWPPlatform.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Core/Log.h"
#include "Engine/GraphicsDevice/DirectX/DX11/GPUSwapChainDX11.h"
#include "Engine/Graphics/RenderTask.h"

namespace Impl
{
    UWPWindow::UWPKeyboard* Keyboard = nullptr;
    UWPWindow::UWPMouse* Mouse = nullptr;
    UWPWindow* Window = nullptr;
}

void OnSizeChanged(float width, float height, void* userData)
{
    ((UWPWindow*)userData)->onSizeChanged(width, height);
}

void OnVisibilityChanged(bool visible, void* userData)
{
    ((UWPWindow*)userData)->onVisibilityChanged(visible);
}

void OnDpiChanged(float dpi, void* userData)
{
    ((UWPWindow*)userData)->onDpiChanged(dpi);
}

void OnClosed(void* userData)
{
    ((UWPWindow*)userData)->onClosed();
}

void OnFocusChanged(bool focused, void* userData)
{
    ((UWPWindow*)userData)->onFocusChanged(focused);
}

void OnKeyDown(int key, void* userData)
{
    ((UWPWindow*)userData)->onKeyDown(key);
}

void OnKeyUp(int key, void* userData)
{
    ((UWPWindow*)userData)->onKeyUp(key);
}

void OnCharacterReceived(int key, void* userData)
{
    ((UWPWindow*)userData)->onCharacterReceived(key);
}

void OnMouseMoved(float x, float y, void* userData)
{
    ((UWPWindow*)userData)->onMouseMoved(x, y);
}

void OnPointerPressed(UWPWindowImpl::PointerData* pointer, void* userData)
{
    ((UWPWindow*)userData)->onPointerPressed(pointer);
}

void OnPointerMoved(UWPWindowImpl::PointerData* pointer, void* userData)
{
    ((UWPWindow*)userData)->onPointerMoved(pointer);
}

void OnPointerWheelChanged(UWPWindowImpl::PointerData* pointer, void* userData)
{
    ((UWPWindow*)userData)->onPointerWheelChanged(pointer);
}

void OnPointerReleased(UWPWindowImpl::PointerData* pointer, void* userData)
{
    ((UWPWindow*)userData)->onPointerReleased(pointer);
}

void OnPointerExited(UWPWindowImpl::PointerData* pointer, void* userData)
{
    ((UWPWindow*)userData)->onPointerExited(pointer);
}

// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
inline float ConvertDipsToPixels(float dips, float dpi)
{
    static const float dipsPerInch = 96.0f;
    return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

// Converts a length in physical pixels to a length in device-independent pixels (DIPs) using the provided DPI. This removes the need
// to call this function from a thread associated with a CoreWindow, which is required by DisplayInformation::GetForCurrentView().
inline float ConvertPixelsToDips(float pixels, float dpi)
{
    static const float dipsPerInch = 96.0f;
    return pixels * dipsPerInch / dpi; // Do not round.
}

void UWPWindow::UWPMouse::onMouseMoved(float x, float y)
{
    const Float2 mousePos(x, y);
    if (!Float2::NearEqual(mousePos, MousePosition))
    {
        MousePosition = mousePos;
        OnMouseMove(mousePos);
    }
}

void UWPWindow::UWPMouse::onPointerPressed(UWPWindowImpl::PointerData* pointer)
{
    const Float2 mousePos(pointer->PositionX, pointer->PositionY);

#define CHECK_MOUSE_BUTTON(button, val1) \
		if (!val1 && pointer->val1) \
		{ \
			val1 = pointer->val1; \
			OnMouseDown(mousePos, MouseButton::button); \
		}

    CHECK_MOUSE_BUTTON(Left, IsLeftButtonPressed);
    CHECK_MOUSE_BUTTON(Right, IsRightButtonPressed);
    CHECK_MOUSE_BUTTON(Middle, IsMiddleButtonPressed);
    CHECK_MOUSE_BUTTON(Extended1, IsXButton1Pressed);
    CHECK_MOUSE_BUTTON(Extended2, IsXButton2Pressed);

#undef CHECK_MOUSE_BUTTON
}

void UWPWindow::UWPMouse::onPointerMoved(UWPWindowImpl::PointerData* pointer)
{
    // Note: UWP reports pointer pressed event for the first mouse button clicked and pointer released event for the last button released but other buttons are reported using pointer moved event.
    // Reference: https://docs.microsoft.com/en-us/windows/uwp/design/input/handle-pointer-input

    const Float2 mousePos(pointer->PositionX, pointer->PositionY);

#define CHECK_MOUSE_BUTTON(button, val1) \
		if (val1 != pointer->val1) \
		{ \
			val1 = pointer->val1; \
			if (val1) \
				OnMouseDown(mousePos, MouseButton::button); \
			else \
				OnMouseUp(mousePos, MouseButton::button); \
		}

    CHECK_MOUSE_BUTTON(Left, IsLeftButtonPressed);
    CHECK_MOUSE_BUTTON(Right, IsRightButtonPressed);
    CHECK_MOUSE_BUTTON(Middle, IsMiddleButtonPressed);
    CHECK_MOUSE_BUTTON(Extended1, IsXButton1Pressed);
    CHECK_MOUSE_BUTTON(Extended2, IsXButton2Pressed);

#undef CHECK_MOUSE_BUTTON

    if (!Float2::NearEqual(mousePos, MousePosition))
    {
        MousePosition = mousePos;
        OnMouseMove(mousePos);
    }
}

void UWPWindow::UWPMouse::onPointerWheelChanged(UWPWindowImpl::PointerData* pointer)
{
    int32 delta = pointer->MouseWheelDelta;
    if (delta != 0)
    {
        const float deltaNormalized = static_cast<float>(delta) / WHEEL_DELTA;
        OnMouseWheel(MousePosition, deltaNormalized);
    }
}

void UWPWindow::UWPMouse::onPointerReleased(UWPWindowImpl::PointerData* pointer)
{
    Float2 mousePos = Float2(pointer->PositionX, pointer->PositionY);

#define CHECK_MOUSE_BUTTON(button, val1) \
		if (!val1 && pointer->val1) \
		{ \
			val1 = pointer->val1; \
			OnMouseUp(mousePos, MouseButton::button); \
		}

    CHECK_MOUSE_BUTTON(Left, IsLeftButtonPressed);
    CHECK_MOUSE_BUTTON(Right, IsRightButtonPressed);
    CHECK_MOUSE_BUTTON(Middle, IsMiddleButtonPressed);
    CHECK_MOUSE_BUTTON(Extended1, IsXButton1Pressed);
    CHECK_MOUSE_BUTTON(Extended2, IsXButton2Pressed);

#undef CHECK_MOUSE_BUTTON
}

void UWPWindow::UWPMouse::onPointerExited(UWPWindowImpl::PointerData* pointer)
{
    MousePosition = Float2(pointer->PositionX, pointer->PositionY);
    OnMouseLeave();
}

void UWPWindow::UWPMouse::SetMousePosition(const Float2& newPosition)
{
    const float dpiScale = Impl::Window->_dpiScale;
    Impl::Window->GetImpl()->SetMousePosition(newPosition.X / dpiScale, newPosition.Y / dpiScale);

    OnMouseMoved(newPosition);
}

void UWPWindow::UWPKeyboard::onCharacterReceived(int key)
{
    OnCharInput((Char)key);
}

void UWPWindow::UWPKeyboard::onKeyDown(int key)
{
    OnKeyDown(static_cast<KeyboardKeys>(key));
}

void UWPWindow::UWPKeyboard::onKeyUp(int key)
{
    OnKeyUp(static_cast<KeyboardKeys>(key));
}

// See GamepadButtons enum
#define UWP_GAMEPAD_Menu 1
#define UWP_GAMEPAD_View 2
#define UWP_GAMEPAD_A 4
#define UWP_GAMEPAD_B 8
#define UWP_GAMEPAD_X 16
#define UWP_GAMEPAD_Y 32
#define UWP_GAMEPAD_DPadUp 64
#define UWP_GAMEPAD_DPadDown 128
#define UWP_GAMEPAD_DPadLeft 256
#define UWP_GAMEPAD_DPadRight 512
#define UWP_GAMEPAD_LeftShoulder 1024
#define UWP_GAMEPAD_RightShoulder 2048
#define UWP_GAMEPAD_LeftThumbstick 4096
#define UWP_GAMEPAD_RightThumbstick 8192
#define UWP_GAMEPAD_Paddle1 16384
#define UWP_GAMEPAD_Paddle2 32768
#define UWP_GAMEPAD_Paddle3 65536
#define UWP_GAMEPAD_Paddle4 131072

void UWPWindow::UWPGamepad::SetVibration(const GamepadVibrationState& state)
{
    Gamepad::SetVibration(state);

    Window->GetImpl()->SetGamepadVibration(Index, *(UWPGamepadStateVibration*)&state);
}

bool UWPWindow::UWPGamepad::UpdateState()
{
    UWPGamepadState state;
    Window->GetImpl()->GetGamepadState(Index, state);

    // Gather buttons state
    const float deadZone = 0.01f;
    _state.Buttons[(int32)GamepadButton::A] = !!(state.Buttons & UWP_GAMEPAD_A);
    _state.Buttons[(int32)GamepadButton::B] = !!(state.Buttons & UWP_GAMEPAD_B);
    _state.Buttons[(int32)GamepadButton::X] = !!(state.Buttons & UWP_GAMEPAD_X);
    _state.Buttons[(int32)GamepadButton::Y] = !!(state.Buttons & UWP_GAMEPAD_Y);
    _state.Buttons[(int32)GamepadButton::LeftShoulder] = !!(state.Buttons & UWP_GAMEPAD_LeftShoulder);
    _state.Buttons[(int32)GamepadButton::RightShoulder] = !!(state.Buttons & UWP_GAMEPAD_RightShoulder);
    _state.Buttons[(int32)GamepadButton::Back] = !!(state.Buttons & UWP_GAMEPAD_View);
    _state.Buttons[(int32)GamepadButton::Start] = !!(state.Buttons & UWP_GAMEPAD_Menu);
    _state.Buttons[(int32)GamepadButton::LeftThumb] = !!(state.Buttons & UWP_GAMEPAD_LeftThumbstick);
    _state.Buttons[(int32)GamepadButton::RightThumb] = !!(state.Buttons & UWP_GAMEPAD_RightThumbstick);
    _state.Buttons[(int32)GamepadButton::LeftTrigger] = state.LeftTrigger > deadZone;
    _state.Buttons[(int32)GamepadButton::RightTrigger] = state.RightTrigger > deadZone;
    _state.Buttons[(int32)GamepadButton::DPadUp] = !!(state.Buttons & UWP_GAMEPAD_DPadUp);
    _state.Buttons[(int32)GamepadButton::DPadDown] = !!(state.Buttons & UWP_GAMEPAD_DPadDown);
    _state.Buttons[(int32)GamepadButton::DPadLeft] = !!(state.Buttons & UWP_GAMEPAD_DPadLeft);
    _state.Buttons[(int32)GamepadButton::DPadRight] = !!(state.Buttons & UWP_GAMEPAD_DPadRight);
    _state.Buttons[(int32)GamepadButton::LeftStickUp] = state.LeftThumbstickY > deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickDown] = state.LeftThumbstickY < -deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickLeft] = state.LeftThumbstickX < -deadZone;
    _state.Buttons[(int32)GamepadButton::LeftStickRight] = state.LeftThumbstickX > deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickUp] = state.RightThumbstickY > deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickDown] = state.RightThumbstickY < -deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickLeft] = state.RightThumbstickX < -deadZone;
    _state.Buttons[(int32)GamepadButton::RightStickRight] = state.RightThumbstickX > deadZone;

    // Gather axis state
    _state.Axis[(int32)GamepadAxis::LeftStickX] = state.LeftThumbstickX;
    _state.Axis[(int32)GamepadAxis::LeftStickY] = state.LeftThumbstickY;
    _state.Axis[(int32)GamepadAxis::RightStickX] = state.RightThumbstickX;
    _state.Axis[(int32)GamepadAxis::RightStickY] = state.RightThumbstickY;
    _state.Axis[(int32)GamepadAxis::LeftTrigger] = state.LeftTrigger;
    _state.Axis[(int32)GamepadAxis::RightTrigger] = state.RightTrigger;

    return false;
}

UWPWindow::UWPWindow(const CreateWindowSettings& settings, UWPWindowImpl* impl)
    : WindowBase(settings)
    , _impl(impl)
    , _logicalSize(Float2::Zero)
{
    ASSERT(Impl::Window == nullptr);
    Impl::Window = this;

    // Link
    _impl->UserData = this;
    _impl->SizeChanged = &::OnSizeChanged;
    _impl->VisibilityChanged = &::OnVisibilityChanged;
    _impl->DpiChanged = &::OnDpiChanged;
    _impl->Closed = &::OnClosed;
    _impl->FocusChanged = &::OnFocusChanged;
    _impl->KeyDown = &::OnKeyDown;
    _impl->KeyUp = &::OnKeyUp;
    _impl->CharacterReceived = &::OnCharacterReceived;
    _impl->MouseMoved = &::OnMouseMoved;
    _impl->PointerPressed = &::OnPointerPressed;
    _impl->PointerMoved = &::OnPointerMoved;
    _impl->PointerWheelChanged = &::OnPointerWheelChanged;
    _impl->PointerReleased = &::OnPointerReleased;
    _impl->PointerExited = &::OnPointerExited;

    // Peek properties
    Char buffer[200];
    _impl->GetTitle(buffer, 200);
    _title = buffer;
    _impl->GetDpi(&_dpi);
    _dpiScale = _dpi / 96.0f;
    float x, y;
    _impl->GetBounds(&x, &y, &_logicalSize.X, &_logicalSize.Y);
    _impl->GetMousePosition(&Impl::Mouse->MousePosition.X, &Impl::Mouse->MousePosition.Y);
    OnSizeChange();
}

UWPWindow::~UWPWindow()
{
}

void UWPWindow::Show()
{
    if (!_visible)
    {
        // TODO: show

        // Base
        WindowBase::Show();
    }
}

void UWPWindow::Hide()
{
    // Not supported
}

void UWPWindow::Minimize()
{
    // Not supported
}

void UWPWindow::Maximize()
{
    // Not supported
}

void UWPWindow::Restore()
{
    // Not supported
}

bool UWPWindow::IsClosed() const
{
    return _isClosing;
}

void UWPWindow::BringToFront(bool force)
{
    Focus();
}

void UWPWindow::SetClientBounds(const Rectangle& clientArea)
{
    // Not supported
}

void UWPWindow::SetPosition(const Float2& position)
{
    // Not supported
}

void UWPWindow::SetClientPosition(const Float2& position)
{
    SetClientPosition(position);
}

Float2 UWPWindow::GetPosition() const
{
    float x, y, width, height;
    _impl->GetBounds(&x, &y, &width, &height);
    return Float2(x, y);
}

Float2 UWPWindow::GetSize() const
{
    return _clientSize;
}

Float2 UWPWindow::GetClientSize() const
{
    return GetSize();
}

Float2 UWPWindow::ScreenToClient(const Float2& screenPos) const
{
    return screenPos - GetPosition();
}

Float2 UWPWindow::ClientToScreen(const Float2& clientPos) const
{
    return clientPos + GetPosition();
}

void UWPWindow::Focus()
{
    _impl->Activate();
}

void UWPWindow::SetTitle(const StringView& title)
{
    _impl->SetTitle(title.Get());
}

DragDropEffect UWPWindow::DoDragDrop(const StringView& data)
{
    // Not supported
    return DragDropEffect::None;
}

void UWPWindow::StartTrackingMouse(bool useMouseScreenOffset)
{
    if (!_isTrackingMouse)
    {
        _isTrackingMouse = true;
        _trackingMouseOffset = Float2::Zero;
        _isUsingMouseOffset = useMouseScreenOffset;
    }
}

void UWPWindow::EndTrackingMouse()
{
    if (_isTrackingMouse)
    {
        _isTrackingMouse = false;
    }
}

void UWPWindow::SetCursor(CursorType type)
{
    _impl->SetCursor((UWPWindowImpl::CursorType)type);

    // Base
    WindowBase::SetCursor(type);
}

void UWPWindow::onSizeChanged(float width, float height)
{
    _logicalSize.X = width;
    _logicalSize.Y = height;

    OnSizeChange();
}

void UWPWindow::onVisibilityChanged(bool visible)
{
}

void UWPWindow::onDpiChanged(float dpi)
{
    if (_dpi != dpi)
    {
        _dpi = (int)dpi;
        _dpiScale = _dpi / 96.0f;

        // When the display DPI changes, the logical size of the window (measured in Dips) also changes and needs to be updated
        float x, y;
        _impl->GetBounds(&x, &y, &_logicalSize.X, &_logicalSize.Y);

        OnSizeChange();
    }
}

void UWPWindow::onClosed()
{
}

void UWPWindow::onFocusChanged(bool focused)
{
    if (focused)
        OnGotFocus();
    else
        OnLostFocus();
}

void UWPWindow::onKeyDown(int key)
{
    if (key < 7)
        return;

    Impl::Keyboard->onKeyDown(key);
}

void UWPWindow::onKeyUp(int key)
{
    if (key < 7)
        return;

    Impl::Keyboard->onKeyUp(key);
}

void UWPWindow::onCharacterReceived(int key)
{
    Impl::Keyboard->onCharacterReceived(key);
}

void UWPWindow::onMouseMoved(float x, float y)
{
    Impl::Mouse->onMouseMoved(x * _dpiScale, y * _dpiScale);
}

void UWPWindow::onPointerPressed(UWPWindowImpl::PointerData* pointer)
{
    if (pointer->IsMouse)
    {
        pointer->PositionX *= _dpiScale;
        pointer->PositionY *= _dpiScale;
        Impl::Mouse->onPointerPressed(pointer);
    }

    // TODO: impl mobile touch support
}

void UWPWindow::onPointerMoved(UWPWindowImpl::PointerData* pointer)
{
    if (pointer->IsMouse)
    {
        pointer->PositionX *= _dpiScale;
        pointer->PositionY *= _dpiScale;
        Impl::Mouse->onPointerMoved(pointer);
    }

    // TODO: impl mobile touch support
}

void UWPWindow::onPointerWheelChanged(UWPWindowImpl::PointerData* pointer)
{
    if (pointer->IsMouse)
    {
        pointer->PositionX *= _dpiScale;
        pointer->PositionY *= _dpiScale;
        Impl::Mouse->onPointerWheelChanged(pointer);
    }
}

void UWPWindow::onPointerReleased(UWPWindowImpl::PointerData* pointer)
{
    if (pointer->IsMouse)
    {
        pointer->PositionX *= _dpiScale;
        pointer->PositionY *= _dpiScale;
        Impl::Mouse->onPointerReleased(pointer);
    }

    // TODO: impl mobile touch support
}

void UWPWindow::onPointerExited(UWPWindowImpl::PointerData* pointer)
{
    if (pointer->IsMouse)
    {
        pointer->PositionX *= _dpiScale;
        pointer->PositionY *= _dpiScale;
        Impl::Mouse->onPointerExited(pointer);
    }

    // TODO: impl mobile touch support
}

void UWPWindow::OnSizeChange()
{
    // Update the actual rendering output resolution
    _clientSize.X = ConvertDipsToPixels(_logicalSize.X, (float)_dpi);
    _clientSize.Y = ConvertDipsToPixels(_logicalSize.Y, (float)_dpi);

    // Check if output has been created
    if (_swapChain != nullptr)
    {
        const int32 width = Math::Max((int32)_clientSize.X, 0);
        const int32 height = Math::Max((int32)_clientSize.Y, 0);
        if (width > 0 && height > 0 && (width != _swapChain->GetWidth() || height != _swapChain->GetHeight()))
        {
            OnResize(width, height);
        }
    }
}

#endif
