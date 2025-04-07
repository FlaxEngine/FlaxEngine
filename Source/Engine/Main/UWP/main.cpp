// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UWP

// @formatter:off

#include "main.h"
#include <sal.h>

// Use better GPU
extern "C" {
_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Devices::Input;
using namespace Windows::Gaming::Input;
using namespace Windows::System;
using namespace Platform;
using namespace FlaxEngine;

Game::Game()
{
    PlatformImpl::Init();
}

void Game::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Game::OnActivated);
    CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &Game::OnSuspending);
    CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &Game::OnResuming);
}

void Game::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
    PlatformImpl::InitWindow(window);

    PointerVisualizationSettings^ visualizationSettings = PointerVisualizationSettings::GetForCurrentView();
    visualizationSettings->IsContactFeedbackEnabled = false;
    visualizationSettings->IsBarrelButtonFeedbackEnabled = false;
}

void Game::Run()
{
    RunUWP();
}

void Game::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    window->Activate();
}

void Game::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
}

void Game::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // Restore any data or state that was unloaded on suspend. By default, data
    // and state are persisted when resuming from suspend. Note that this event
    // does not occur if the app was previously terminated.
    // TODO: m_main->Resume();
}

PlatformImpl MainPlatform;
WindowImpl MainWindow;

void WindowEvents::Register(WindowImpl* win)
{
    _win = win;

    auto window = win->Window;

    window->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &WindowEvents::OnActivationChanged);
    window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &WindowEvents::OnSizeChanged);
    window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &WindowEvents::OnClosed);
    window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &WindowEvents::OnVisibilityChanged);
    window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WindowEvents::OnPointerPressed);
    window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WindowEvents::OnPointerMoved);
    window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WindowEvents::OnPointerWheelChanged);
    window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WindowEvents::OnPointerReleased);
    window->PointerExited += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WindowEvents::OnPointerExited);
    window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WindowEvents::OnKeyDown);
    window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WindowEvents::OnKeyUp);
    window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &WindowEvents::OnCharacterReceived);

    MouseDevice::GetForCurrentView()->MouseMoved += ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &WindowEvents::OnMouseMoved);

    DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
    currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &WindowEvents::OnDisplayDpiChanged);
    currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &WindowEvents::OnOrientationChanged);
    currentDisplayInformation->StereoEnabledChanged += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &WindowEvents::OnStereoEnabledChanged);
    DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &WindowEvents::OnDisplayContentsInvalidated);

    // Detect gamepad connection and disconnection events
    Gamepad::GamepadAdded += ref new EventHandler<Gamepad^>(this, &WindowEvents::OnGamepadAdded);
    Gamepad::GamepadRemoved += ref new EventHandler<Gamepad^>(this, &WindowEvents::OnGamepadRemoved);
}

void WindowEvents::OnActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args)
{
    bool hasFocus;
    if (args->WindowActivationState == Windows::UI::Core::CoreWindowActivationState::Deactivated)
        hasFocus = false;
    else if (args->WindowActivationState == Windows::UI::Core::CoreWindowActivationState::CodeActivated
        || args->WindowActivationState == Windows::UI::Core::CoreWindowActivationState::PointerActivated)
        hasFocus = true;

    if (_win->FocusChanged)
        _win->FocusChanged(hasFocus, _win->UserData);
}

void WindowEvents::OnSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args)
{
    if (_win->SizeChanged)
        _win->SizeChanged(args->Size.Width, args->Size.Height, _win->UserData);
}

void WindowEvents::OnClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args)
{
    if (_win->Closed)
        _win->Closed(_win->UserData);
}

void WindowEvents::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
    if (_win->VisibilityChanged)
        _win->VisibilityChanged(args->Visible, _win->UserData);
}

void WindowEvents::OnStereoEnabledChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
{
    //m_deviceResources->UpdateStereoState();
    //m_main->CreateWindowSizeDependentResources();
}

void WindowEvents::OnDisplayDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
{
    if (_win->DpiChanged)
        _win->DpiChanged(sender->LogicalDpi, _win->UserData);
}

void WindowEvents::OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
{
    //m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
    //m_main->CreateWindowSizeDependentResources();
}

void WindowEvents::OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args)
{
    //m_deviceResources->ValidateDevice();
}

void WindowEvents::OnGamepadAdded(Object^ sender, Gamepad^ gamepad)
{
    //_win->GamepadsChanged = true;
}

void WindowEvents::OnGamepadRemoved(Object^ sender, Gamepad^ gamepad)
{
    //_win->GamepadsChanged = true;
}

void GetPointerData(UWPWindowImpl::PointerData & data, PointerEventArgs^ args)
{
    PointerPoint^ point = args->CurrentPoint;
    Point pointerPosition = point->Position;
    PointerPointProperties^ pointProperties = point->Properties;
    auto pointerDevice = point->PointerDevice;
    auto pointerDeviceType = pointerDevice->PointerDeviceType;

    data.PointerId = point->PointerId;
    data.PositionX = pointerPosition.X;
    data.PositionY = pointerPosition.Y;
    data.MouseWheelDelta = pointProperties->MouseWheelDelta;
    data.IsLeftButtonPressed = pointProperties->IsLeftButtonPressed;
    data.IsMiddleButtonPressed = pointProperties->IsMiddleButtonPressed;
    data.IsRightButtonPressed = pointProperties->IsRightButtonPressed;
    data.IsXButton1Pressed = pointProperties->IsXButton1Pressed;
    data.IsXButton2Pressed = pointProperties->IsXButton2Pressed;

    data.IsMouse = pointerDeviceType == PointerDeviceType::Mouse;
    data.IsPen = pointerDeviceType == PointerDeviceType::Pen;
    data.IsTouch = pointerDeviceType == PointerDeviceType::Touch;

    args->Handled = true;
}

void WindowEvents::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (_win->PointerPressed)
    {
        UWPWindowImpl::PointerData data;
        GetPointerData(data, args);
        _win->PointerPressed(&data, _win->UserData);
    }
}

void WindowEvents::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (_win->PointerMoved)
    {
        UWPWindowImpl::PointerData data;
        GetPointerData(data, args);
        _win->PointerMoved(&data, _win->UserData);
    }
}

void WindowEvents::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (_win->PointerWheelChanged)
    {
        UWPWindowImpl::PointerData data;
        GetPointerData(data, args);
        _win->PointerWheelChanged(&data, _win->UserData);
    }
}

void WindowEvents::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (_win->PointerReleased)
    {
        UWPWindowImpl::PointerData data;
        GetPointerData(data, args);
        _win->PointerReleased(&data, _win->UserData);
    }
}

void WindowEvents::OnPointerExited(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (_win->PointerExited)
    {
        UWPWindowImpl::PointerData data;
        GetPointerData(data, args);
        _win->PointerExited(&data, _win->UserData);
    }
}

void WindowEvents::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
    if (_win->KeyDown)
    {
        _win->KeyDown((int)args->VirtualKey, _win->UserData);
    }
}

void WindowEvents::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
    if (_win->KeyUp)
    {
        _win->KeyUp((int)args->VirtualKey, _win->UserData);
        args->Handled = true;
    }
}

void WindowEvents::OnCharacterReceived(CoreWindow^ sender, CharacterReceivedEventArgs^ args)
{
    if (_win->CharacterReceived)
    {
        _win->CharacterReceived(args->KeyCode, _win->UserData);
        args->Handled = true;
    }
}

void WindowEvents::OnMouseMoved(MouseDevice^ mouseDevice, MouseEventArgs^ args)
{
    if (_win->MouseMoved)
    {
        Point point = _win->Window->PointerPosition;
        _win->MouseMoved(point.X, point.Y, _win->UserData);
    }
}

void WindowImpl::Init(CoreWindow^ window)
{
    Window = window;
    Events = ref new WindowEvents();

    window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);

    PointerVisualizationSettings^ visualizationSettings = PointerVisualizationSettings::GetForCurrentView();
    visualizationSettings->IsContactFeedbackEnabled = false;
    visualizationSettings->IsBarrelButtonFeedbackEnabled = false;

    Events->Register(this);
}

void WindowImpl::SetGamepadVibration(const int index, UWPGamepadStateVibration& vibration)
{
    auto gamepads = Gamepad::Gamepads;
    if (index >= (int)gamepads->Size)
        return;
    auto gamepad = gamepads->GetAt(index);

    GamepadVibration gamepadVibration;
    gamepadVibration.LeftMotor = vibration.LeftLarge;
    gamepadVibration.RightMotor = vibration.RightLarge;
    gamepadVibration.LeftTrigger = vibration.LeftSmall;
    gamepadVibration.RightTrigger = vibration.RightSmall;
    gamepad->Vibration = gamepadVibration;
}

void WindowImpl::GetGamepadState(const int index, UWPGamepadState& state)
{
    auto gamepads = Gamepad::Gamepads;
    if (index >= (int)gamepads->Size)
        return;
    auto gamepad = gamepads->GetAt(index);

    GamepadReading reading = gamepad->GetCurrentReading();

    state.Buttons = (int)reading.Buttons;
    state.LeftThumbstickX = (float)reading.LeftThumbstickX;
    state.LeftThumbstickY = (float)reading.LeftThumbstickY;
    state.RightThumbstickX = (float)reading.RightThumbstickX;
    state.RightThumbstickY = (float)reading.RightThumbstickY;
    state.LeftTrigger = (float)reading.LeftTrigger;
    state.RightTrigger = (float)reading.LeftTrigger;
}

void PlatformImpl::Init()
{
    // Register UWP platform implementation for the FlaxEngine API
    CUWPPlatform = &MainPlatform;
}

void PlatformImpl::InitWindow(Windows::UI::Core::CoreWindow^ window)
{
    MainWindow.Init(window);
}

UWPWindowImpl* PlatformImpl::GetMainWindowImpl()
{
    return &MainWindow;
}

void PlatformImpl::Tick()
{
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
}

int PlatformImpl::GetDpi()
{
    auto currentDisplayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    return (int)currentDisplayInformation->LogicalDpi;
}

int PlatformImpl::GetSpecialFolderPath(const SpecialFolder type, wchar_t* buffer, int bufferLength)
{
    String^ path = nullptr;
    switch (type)
    {
    case SpecialFolder::Desktop:
        path = Windows::Storage::KnownFolders::DocumentsLibrary->Path + "/../Desktop";
        break;
    case SpecialFolder::Documents:
        path = Windows::Storage::KnownFolders::DocumentsLibrary->Path;
        break;
    case SpecialFolder::Pictures:
        path = Windows::Storage::KnownFolders::PicturesLibrary->Path;
        break;
    case SpecialFolder::AppData:
        path = Windows::Storage::ApplicationData::Current->RoamingFolder->Path;
        break;
    case SpecialFolder::LocalAppData:
        path = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        break;
    case SpecialFolder::ProgramData:
        path = Windows::Storage::ApplicationData::Current->RoamingFolder->Path;
        break;
    case SpecialFolder::Temporary:
        //path = Windows::Storage::ApplicationData::Current->TemporaryFolder->Path;
        path = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        break;
    }

    if (path != nullptr)
    {
        int length = path->Length();
        if (length >= bufferLength)
            length = bufferLength - 1;
        const wchar_t* data = path->Data();
        for (int i = 0; i < length; i++)
            buffer[i] = data[i];
        buffer[length] = 0;
        return length;
    }

    return 0;
}

void PlatformImpl::GetDisplaySize(float* width, float* height)
{
    auto bounds = ApplicationView::GetForCurrentView()->VisibleBounds;
    auto scaleFactor = DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
    *width = (float)(bounds.Width * scaleFactor);
    *height = (float)(bounds.Height * scaleFactor);
}

UWPPlatformImpl::DialogResult PlatformImpl::ShowMessageDialog(UWPWindowImpl* window, const wchar_t* text, const wchar_t* caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    Platform::String^ content = ref new Platform::String(text);
    Platform::String^ title = ref new Platform::String(caption);
    Windows::UI::Popups::MessageDialog^ msg = ref new Windows::UI::Popups::MessageDialog(content, title);

    // TODO: implement buttons
    // TODO: gather result

    msg->ShowAsync();

    return DialogResult::OK;
}

#endif
