// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "Engine/Platform/UWP/UWPPlatformImpl.h"

// @formatter:off

class WindowImpl;

namespace FlaxEngine
{
    /// <summary>
    /// The helper bridge interface to initialize and run a game.
    /// </summary>
    public ref class Game sealed
    {
    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="Game"/> class.
        /// </summary>
        Game();

    public:

        /// <summary>
        /// A method for app view initialization, which is called when an app object is launched.
        /// </summary>
        /// <param name="applicationView">The default view provided by the app object. You can use this instance in your implementation to obtain the CoreWindow created by the app object, and register callbacks for the Activated event.</param>
        void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);

        /// <summary>
        /// A method that sets the current window for the game rendering output.
        /// </summary>
        /// <param name="window">The current window for the app object.</param>
        void SetWindow(Windows::UI::Core::CoreWindow^ window);

        /// <summary>
        /// A method that starts the game.
        /// </summary>
        void Run();

    private:

        // Application lifecycle event handlers
        void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
        void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
        void OnResuming(Platform::Object^ sender, Platform::Object^ args);
    };
}

namespace FlaxEngine
{
    private ref class WindowEvents sealed
    {
        friend WindowImpl;

    private:

        WindowImpl* _win;

    private:

        void Register(WindowImpl* win);

    private:

        void OnActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
        void OnSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
        void OnClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
        void OnStereoEnabledChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnDisplayDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerExited(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);
        void OnMouseMoved(Windows::Devices::Input::MouseDevice^ mouseDevice, Windows::Devices::Input::MouseEventArgs^ args);
        void OnGamepadAdded(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);
        void OnGamepadRemoved(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);
    };
}

class WindowImpl : public UWPWindowImpl
{
public:

    //Platform::Agile<Windows::UI::Core::CoreWindow> Window
    Windows::UI::Core::CoreWindow^ Window;
    FlaxEngine::WindowEvents^ Events;

public:

    void Init(Windows::UI::Core::CoreWindow^ window);

public:

    // [UWPWindowImpl]
    void* GetHandle() const override
    {
        //return (void*)reinterpret_cast<IUnknown*>(Window.Get());
        //return (void*)reinterpret_cast<IUnknown*>(Window);
        return reinterpret_cast<void*>(Window);
    }

    void SetMousePosition(float x, float y) override
    {
        Window->PointerPosition = Windows::Foundation::Point(x, y);
    }

    void GetMousePosition(float* x, float* y) override
    {
        Windows::Foundation::Point point = Window->PointerPosition;
        *x = point.X;
        *y = point.Y;
    }

    void SetCursor(CursorType type) override
    {
        auto window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
        if (window == nullptr)
            return;
        if (type == CursorType::Hidden)
        {
            // TODO: use custom cursor with empty image, when PointerCursor is null mouse events are not passed to the window events
            window->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
        }
        else
        {
            Windows::UI::Core::CoreCursorType uwpType;
            switch (type)
            {
            case CursorType::Cross:
                uwpType = Windows::UI::Core::CoreCursorType::Cross;
                break;
            case CursorType::Hand:
                uwpType = Windows::UI::Core::CoreCursorType::Hand;
                break;
            case CursorType::Help:
                uwpType = Windows::UI::Core::CoreCursorType::Help;
                break;
            case CursorType::IBeam:
                uwpType = Windows::UI::Core::CoreCursorType::IBeam;
                break;
            case CursorType::No:
                uwpType = Windows::UI::Core::CoreCursorType::UniversalNo;
                break;
            case CursorType::Wait:
                uwpType = Windows::UI::Core::CoreCursorType::Wait;
                break;
            case CursorType::SizeAll:
                uwpType = Windows::UI::Core::CoreCursorType::SizeAll;
                break;
            case CursorType::SizeNESW:
                uwpType = Windows::UI::Core::CoreCursorType::SizeNortheastSouthwest;
                break;
            case CursorType::SizeNS:
                uwpType = Windows::UI::Core::CoreCursorType::SizeNorthSouth;
                break;
            case CursorType::SizeNWSE:
                uwpType = Windows::UI::Core::CoreCursorType::SizeNorthwestSoutheast;
                break;
            case CursorType::SizeWE:
                uwpType = Windows::UI::Core::CoreCursorType::SizeWestEast;
                break;
            default:
                uwpType = Windows::UI::Core::CoreCursorType::Arrow;
                break;
            }
            window->PointerCursor = ref new Windows::UI::Core::CoreCursor(uwpType, 0);
        }
    }

    void GetBounds(float* x, float* y, float* width, float* height) override
    {
        auto bounds = Window->Bounds;
        *x = bounds.X;
        *y = bounds.Y;
        *width = bounds.Width;
        *height = bounds.Height;
    }

    void GetDpi(int* dpi) override
    {
        auto currentDisplayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
        *dpi = (int)currentDisplayInformation->LogicalDpi;
    }

    void GetTitle(wchar_t* buffer, int bufferLength) override
    {
        auto appView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
        auto title = appView->Title;

        if (title)
        {
            int length = title->Length();
            if (length >= bufferLength)
                length = bufferLength - 1;
            const wchar_t* data = title->Data();
            for (int i = 0; i < length; i++)
                buffer[i] = data[i];
            buffer[length] = 0;
        }
        else
        {
            buffer[0] = 0;
        }
    }

    void SetTitle(const wchar_t* title) override
    {
        auto appView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
        appView->Title = ref new Platform::String(title);
    }

    int GetGamepadsCount()
    {
        return Windows::Gaming::Input::Gamepad::Gamepads->Size;
    }

    void SetGamepadVibration(const int index, UWPGamepadStateVibration& vibration) override;
    void GetGamepadState(const int index, UWPGamepadState& state) override;

    void Activate() override
    {
        Window->Activate();
    }

    void Close() override
    {
        Window->Close();
    }
};

class PlatformImpl : public UWPPlatformImpl
{
public:

    /// <summary>
    /// Registers platform.
    /// </summary>
    static void Init();

    static void InitWindow(Windows::UI::Core::CoreWindow^ window);

public:

    // [UWPPlatformImpl]
    UWPWindowImpl* GetMainWindowImpl() override;
    void Tick() override;
    int GetDpi() override;
    int GetSpecialFolderPath(const SpecialFolder type, wchar_t* buffer, int bufferLength) override;
    void GetDisplaySize(float* width, float* height) override;
    DialogResult ShowMessageDialog(UWPWindowImpl* window, const wchar_t* text, const wchar_t* caption, MessageBoxButtons buttons, MessageBoxIcon icon) override;
};

#endif
