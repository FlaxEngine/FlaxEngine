// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Gamepad.h"
#include "../Win32/IncludeWindowsHeaders.h"

/// <summary>
/// Implementation of the window class for Universal Windows Platform (UWP)
/// </summary>
class FLAXENGINE_API UWPWindow : public WindowBase
{
    friend UWPPlatform;

public:

    class UWPMouse : public Mouse
    {
    public:

        int IsLeftButtonPressed : 1;
        int IsMiddleButtonPressed : 1;
        int IsRightButtonPressed : 1;
        int IsXButton1Pressed : 1;
        int IsXButton2Pressed : 1;
        Float2 MousePosition;

    public:

        explicit UWPMouse()
            : Mouse()
        {
            IsLeftButtonPressed = 0;
            IsMiddleButtonPressed = 0;
            IsRightButtonPressed = 0;
            IsXButton1Pressed = 0;
            IsXButton2Pressed = 0;
            MousePosition = Float2::Zero;
        }

    public:

        void onMouseMoved(float x, float y);
        void onPointerPressed(UWPWindowImpl::PointerData* pointer);
        void onPointerMoved(UWPWindowImpl::PointerData* pointer);
        void onPointerWheelChanged(UWPWindowImpl::PointerData* pointer);
        void onPointerReleased(UWPWindowImpl::PointerData* pointer);
        void onPointerExited(UWPWindowImpl::PointerData* pointer);

    public:

        // [Mouse]
        void SetMousePosition(const Float2& newPosition) override;
    };

    class UWPKeyboard : public Keyboard
    {
    public:

        explicit UWPKeyboard()
            : Keyboard()
        {
        }

    public:

        void onCharacterReceived(int key);
        void onKeyDown(int key);
        void onKeyUp(int key);
    };

    class UWPGamepad : public Gamepad
    {
    public:

        explicit UWPGamepad(UWPWindow* win, const int32 index)
            : Gamepad(Guid(index, 0, 0, 11), TEXT("Gamepad"))
        {
            Index = index;
            Window = win;
        }

        int32 Index;
        UWPWindow* Window;

    public:

        // [Gamepad]
        void SetVibration(const GamepadVibrationState& state) override;
        bool UpdateState() override;
    };

private:

    UWPWindowImpl* _impl;

    Float2 _logicalSize;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="UWPWindow" /> class.
    /// </summary>
    /// <param name="settings">The initial settings.</param>
    /// <param name="impl">The implementation.</param>
    UWPWindow(const CreateWindowSettings& settings, UWPWindowImpl* impl);

    /// <summary>
    /// Finalizes an instance of the <see cref="UWPWindow"/> class.
    /// </summary>
    ~UWPWindow();

public:

    /// <summary>
    /// Gets the UWP window implementation.
    /// </summary>
    /// <returns>The window.</returns>
    FORCE_INLINE UWPWindowImpl* GetImpl() const
    {
        return _impl;
    }

public:

    void onSizeChanged(float width, float height);
    void onVisibilityChanged(bool visible);
    void onDpiChanged(float dpi);
    void onClosed();
    void onFocusChanged(bool focused);
    void onKeyDown(int key);
    void onKeyUp(int key);
    void onCharacterReceived(int key);
    void onMouseMoved(float x, float y);
    void onPointerPressed(UWPWindowImpl::PointerData* pointer);
    void onPointerMoved(UWPWindowImpl::PointerData* pointer);
    void onPointerWheelChanged(UWPWindowImpl::PointerData* pointer);
    void onPointerReleased(UWPWindowImpl::PointerData* pointer);
    void onPointerExited(UWPWindowImpl::PointerData* pointer);

private:

    void OnSizeChange();

public:

    // [WindowBase]
    void* GetNativePtr() const override
    {
        return _impl->GetHandle();
    }
    void Show() override;
    void Hide() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    bool IsClosed() const override;
    void BringToFront(bool force = false) override;
    void SetClientBounds(const Rectangle& clientArea) override;
    void SetPosition(const Float2& position) override;
    void SetClientPosition(const Float2& position) override;
    Float2 GetPosition() const override;
    Float2 GetSize() const override;
    Float2 GetClientSize() const override;
    Float2 ScreenToClient(const Float2& screenPos) const override;
    Float2 ClientToScreen(const Float2& clientPos) const override;
    void FlashWindow() override
    {
    }
    float GetOpacity() const override
    {
        return 1.0f;
    }
    void SetOpacity(float opacity) override
    {
    }
    void Focus() override;
    void SetTitle(const StringView& title) override;
    DragDropEffect DoDragDrop(const StringView& data) override;
    void StartTrackingMouse(bool useMouseScreenOffset) override;
    void EndTrackingMouse() override;
    void SetCursor(CursorType type) override;
};

#endif
