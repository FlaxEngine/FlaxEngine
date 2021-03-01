// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Engine/Platform/Window.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Input/Input.h"
#include "Engine/Platform/IGuiData.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

// Helper macros for calling C# events
#define BEGIN_INVOKE_EVENT(name, paramsCount) \
	auto managedInstance = GetManagedInstance(); \
    if (managedInstance) \
	{ \
	    static MMethod* _method_##name = nullptr; \
	    if (_method_##name == nullptr) \
	    { \
		    _method_##name = GetClass()->GetMethod("Internal_" #name, paramsCount); \
		    if (_method_##name == nullptr) \
		    { \
			    LOG(Fatal, "Missing Window method " #name); \
		    } \
	    }
#define END_INVOKE_EVENT(name) \
        if (exception) \
	    { \
		    MException ex(exception); \
		    ex.Log(LogType::Error, TEXT("Window." #name)); \
	    } \
	}
#define INVOKE_EVENT(name, paramsCount, param0, param1, param2) BEGIN_INVOKE_EVENT(name, paramsCount) \
	    void* params[3]; \
	    params[0] = param0; \
	    params[1] = param1; \
	    params[2] = param2; \
	    MonoObject* exception = nullptr; \
	    _method_##name->Invoke(managedInstance, params, &exception); \
	    END_INVOKE_EVENT(name)
#define INVOKE_EVENT_PARAMS_3(name, param0, param1, param2) INVOKE_EVENT(name, 3, param0, param1, param2)
#define INVOKE_EVENT_PARAMS_2(name, param0, param1) INVOKE_EVENT(name, 2, param0, param1, nullptr)
#define INVOKE_EVENT_PARAMS_1(name, param0) INVOKE_EVENT(name, 1, param0, nullptr, nullptr)
#define INVOKE_EVENT_PARAMS_0(name) INVOKE_EVENT(name, 0, nullptr, nullptr, nullptr)
#define INVOKE_DRAG_EVENT(name) \
	if (result != DragDropEffect::None) return; \
    BEGIN_INVOKE_EVENT(name, 3) \
	void* params[3]; \
	params[0] = (void*)&mousePosition; \
	Array<String> outputData; \
	bool isText; \
	if(data->GetType() == IGuiData::Type::Text)\
	{ \
		isText = true; outputData.Add(data->GetAsText()); \
	} \
	else \
	{ \
		isText = false; data->GetAsFiles(&outputData); \
	} \
	params[1] = (void*)&isText; \
	MonoArray* outputDataMono = mono_array_new(mono_domain_get(), mono_get_string_class(), outputData.Count()); \
	for (int32 i = 0; i < outputData.Count(); i++) \
		*(MonoString**)mono_array_addr_with_size(outputDataMono, sizeof(MonoString*), i) = MUtils::ToString(outputData[i]); \
	params[2] = outputDataMono; \
	MonoObject* exception = nullptr; \
	auto resultObj = _method_##name->Invoke(GetManagedInstance(), params, &exception); \
    if (resultObj) \
	    result = (DragDropEffect)MUtils::Unbox<int32>(resultObj); \
	END_INVOKE_EVENT(name)

WindowBase::WindowBase(const CreateWindowSettings& settings)
    : PersistentScriptingObject(SpawnParams(Guid::New(), TypeInitializer))
    , _visible(false)
    , _minimized(false)
    , _maximized(false)
    , _isClosing(false)
    , _focused(false)
    , _swapChain(nullptr)
    , _settings(settings)
    , _title(settings.Title)
    , _cursor(CursorType::Default)
    , _trackingMouseOffset(Vector2::Zero)
    , _isUsingMouseOffset(false)
    , _isTrackingMouse(false)
    , RenderTask(nullptr)
{
    _showAfterFirstPaint = settings.ShowAfterFirstPaint;
    _clientSize = Vector2(settings.Size.X, settings.Size.Y);

    // Update window location based on start location
    if (settings.StartPosition == WindowStartPosition::CenterParent
        || settings.StartPosition == WindowStartPosition::CenterScreen)
    {
        Rectangle parentBounds = Rectangle(Vector2::Zero, Platform::GetDesktopSize());
        if (settings.Parent != nullptr && settings.StartPosition == WindowStartPosition::CenterParent)
            parentBounds = settings.Parent->GetClientBounds();

        // Move to the center of target bounds area
        // A little hack but platform implementation constructor will place a window
        ((CreateWindowSettings&)settings).Position = parentBounds.Location + (parentBounds.Size - settings.Size) * 0.5f;
    }

    WindowsManager::Register((Window*)this);
}

WindowBase::~WindowBase()
{
    ASSERT(!RenderTask);
    ASSERT(!_swapChain);
}

bool WindowBase::IsMain() const
{
    const auto window = Engine::MainWindow;
    return window == this || window == nullptr;
}

bool WindowBase::IsFullscreen() const
{
    return _swapChain ? _swapChain->IsFullscreen() : false;
}

void WindowBase::SetIsFullscreen(bool isFullscreen)
{
    LOG(Info, "Changing window fullscreen mode to {0}", isFullscreen);

    if (_swapChain)
    {
        _swapChain->SetFullscreen(isFullscreen);
    }
}

bool WindowBase::IsVisible() const
{
    return _visible;
}

void WindowBase::SetIsVisible(bool isVisible)
{
    // Check if state will change
    if (IsVisible() != isVisible)
    {
        // Check if show or hide a window
        if (isVisible)
            Show();
        else
            Hide();
    }
}

String WindowBase::ToString() const
{
    return GetTitle();
}

void WindowBase::OnDeleteObject()
{
#if !USE_EDITOR

    // Unlink main task (if was used)
    if (RenderTask && RenderTask == MainRenderTask::Instance)
    {
        MainRenderTask::Instance->SwapChain = nullptr;
        RenderTask = nullptr;
    }

#endif

    // Release resources
    SAFE_DELETE(RenderTask);
    SAFE_DELETE(_swapChain);

    // Base
    PersistentScriptingObject::OnDeleteObject();
}

bool WindowBase::GetRenderingEnabled() const
{
    return RenderTask && RenderTask->Enabled;
}

void WindowBase::SetRenderingEnabled(bool value)
{
    if (RenderTask)
        RenderTask->Enabled = value;
}

void WindowBase::OnCharInput(Char c)
{
    PROFILE_CPU_NAMED("GUI.OnCharInput");
    CharInput(c);
    INVOKE_EVENT_PARAMS_1(OnCharInput, &c);
}

void WindowBase::OnKeyDown(KeyboardKeys key)
{
    PROFILE_CPU_NAMED("GUI.OnKeyDown");
    KeyDown(key);
    INVOKE_EVENT_PARAMS_1(OnKeyDown, &key);
}

void WindowBase::OnKeyUp(KeyboardKeys key)
{
    PROFILE_CPU_NAMED("GUI.OnKeyUp");
    KeyUp(key);
    INVOKE_EVENT_PARAMS_1(OnKeyUp, &key);
}

void WindowBase::OnMouseDown(const Vector2& mousePosition, MouseButton button)
{
    PROFILE_CPU_NAMED("GUI.OnMouseDown");
    MouseDown(mousePosition, button);
    INVOKE_EVENT_PARAMS_2(OnMouseDown, (void*)&mousePosition, &button);
}

void WindowBase::OnMouseUp(const Vector2& mousePosition, MouseButton button)
{
    PROFILE_CPU_NAMED("GUI.OnMouseUp");
    MouseUp(mousePosition, button);
    INVOKE_EVENT_PARAMS_2(OnMouseUp, (void*)&mousePosition, &button);
}

void WindowBase::OnMouseDoubleClick(const Vector2& mousePosition, MouseButton button)
{
    PROFILE_CPU_NAMED("GUI.OnMouseDoubleClick");
    MouseDoubleClick(mousePosition, button);
    INVOKE_EVENT_PARAMS_2(OnMouseDoubleClick, (void*)&mousePosition, &button);
}

void WindowBase::OnMouseWheel(const Vector2& mousePosition, float delta)
{
    PROFILE_CPU_NAMED("GUI.OnMouseWheel");
    MouseWheel(mousePosition, delta);
    INVOKE_EVENT_PARAMS_2(OnMouseWheel, (void*)&mousePosition, &delta);
}

void WindowBase::OnMouseMove(const Vector2& mousePosition)
{
    PROFILE_CPU_NAMED("GUI.OnMouseMove");
    MouseMove(mousePosition);
    INVOKE_EVENT_PARAMS_1(OnMouseMove, (void*)&mousePosition);
}

void WindowBase::OnMouseLeave()
{
    PROFILE_CPU_NAMED("GUI.OnMouseLeave");
    MouseLeave();
    INVOKE_EVENT_PARAMS_0(OnMouseLeave);
}

void WindowBase::OnTouchDown(const Vector2& pointerPosition, int32 pointerId)
{
    PROFILE_CPU_NAMED("GUI.OnTouchDown");
    TouchDown(pointerPosition, pointerId);
    INVOKE_EVENT_PARAMS_2(OnTouchDown, (void*)&pointerPosition, &pointerId);
}

void WindowBase::OnTouchMove(const Vector2& pointerPosition, int32 pointerId)
{
    PROFILE_CPU_NAMED("GUI.OnTouchMove");
    TouchMove(pointerPosition, pointerId);
    INVOKE_EVENT_PARAMS_2(OnTouchMove, (void*)&pointerPosition, &pointerId);
}

void WindowBase::OnTouchUp(const Vector2& pointerPosition, int32 pointerId)
{
    PROFILE_CPU_NAMED("GUI.OnTouchUp");
    TouchUp(pointerPosition, pointerId);
    INVOKE_EVENT_PARAMS_2(OnTouchUp, (void*)&pointerPosition, &pointerId);
}

void WindowBase::OnDragEnter(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result)
{
    DragEnter(data, mousePosition, result);
    INVOKE_DRAG_EVENT(OnDragEnter);
}

void WindowBase::OnDragOver(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result)
{
    DragOver(data, mousePosition, result);
    INVOKE_DRAG_EVENT(OnDragOver);
}

void WindowBase::OnDragDrop(IGuiData* data, const Vector2& mousePosition, DragDropEffect& result)
{
    DragDrop(data, mousePosition, result);
    INVOKE_DRAG_EVENT(OnDragDrop);
}

void WindowBase::OnDragLeave()
{
    INVOKE_EVENT_PARAMS_0(OnDragLeave);
}

void WindowBase::OnHitTest(const Vector2& mousePosition, WindowHitCodes& result, bool& handled)
{
    HitTest(mousePosition, result, handled);
    if (handled)
        return;
    INVOKE_EVENT_PARAMS_3(OnHitTest, (void*)&mousePosition, &result, &handled);
}

void WindowBase::OnLeftButtonHit(WindowHitCodes hit, bool& result)
{
    LeftButtonHit(hit, result);
    if (result)
        return;
    INVOKE_EVENT_PARAMS_2(OnLeftButtonHit, &hit, &result);
}

void WindowBase::OnClosing(ClosingReason reason, bool& cancel)
{
    Closing(reason, cancel);
    INVOKE_EVENT_PARAMS_2(OnClosing, &reason, &cancel);
}

StringView WindowBase::GetInputText() const
{
    return _settings.AllowInput && _focused ? Input::GetInputText() : StringView::Empty;
}

bool WindowBase::GetKey(KeyboardKeys key) const
{
    return _settings.AllowInput && _focused ? Input::GetKey(key) : false;
}

bool WindowBase::GetKeyDown(KeyboardKeys key) const
{
    return _settings.AllowInput && _focused ? Input::GetKeyDown(key) : false;
}

bool WindowBase::GetKeyUp(KeyboardKeys key) const
{
    return _settings.AllowInput && _focused ? Input::GetKeyUp(key) : false;
}

Vector2 WindowBase::GetMousePosition() const
{
    return _settings.AllowInput && _focused ? ScreenToClient(Input::GetMouseScreenPosition()) : Vector2::Minimum;
}

void WindowBase::SetMousePosition(const Vector2& position) const
{
    if (_settings.AllowInput && _focused)
    {
        Input::SetMouseScreenPosition(ClientToScreen(position));
    }
}

Vector2 WindowBase::GetMousePositionDelta() const
{
    return _settings.AllowInput && _focused ? Input::GetMousePositionDelta() : Vector2::Zero;
}

float WindowBase::GetMouseScrollDelta() const
{
    return _settings.AllowInput && _focused ? Input::GetMouseScrollDelta() : 0.0f;
}

bool WindowBase::GetMouseButton(MouseButton button) const
{
    return _settings.AllowInput && _focused ? Input::GetMouseButton(button) : false;
}

bool WindowBase::GetMouseButtonDown(MouseButton button) const
{
    return _settings.AllowInput && _focused ? Input::GetMouseButtonDown(button) : false;
}

bool WindowBase::GetMouseButtonUp(MouseButton button) const
{
    return _settings.AllowInput && _focused ? Input::GetMouseButtonUp(button) : false;
}

void WindowBase::OnShow()
{
    PROFILE_CPU_NAMED("GUI.OnShow");
    INVOKE_EVENT_PARAMS_0(OnShow);
    Shown();
}

void WindowBase::OnResize(int32 width, int32 height)
{
    PROFILE_CPU_NAMED("GUI.OnResize");
    if (_swapChain)
        _swapChain->Resize(width, height);
    if (RenderTask)
        RenderTask->Resize(width, height);
    INVOKE_EVENT_PARAMS_2(OnResize, &width, &height);
}

void WindowBase::OnClosed()
{
    // We have to call this from close event to finish WindowBase destroy process (you cannot use WindowBase after Close)
    ASSERT(_isClosing);

    // Dispose swap chain (it will wait for GPU work to be done)
    if (_swapChain)
        _swapChain->ReleaseGPU();

    // Send event
    Closed();
    INVOKE_EVENT_PARAMS_0(OnClosed);

    // Unregister
    WindowsManager::Unregister(static_cast<Window*>(this));

    // Disable rendering
    if (RenderTask)
        RenderTask->Enabled = false;

    // Delete object
    DeleteObject(1);
}

void WindowBase::OnGotFocus()
{
    if (_focused)
        return;
    _focused = true;

    GotFocus();
    INVOKE_EVENT_PARAMS_0(OnGotFocus);
}

void WindowBase::OnLostFocus()
{
    if (!_focused)
        return;
    _focused = false;

    LostFocus();
    INVOKE_EVENT_PARAMS_0(OnLostFocus);
}

void WindowBase::OnUpdate(float dt)
{
    PROFILE_CPU_NAMED("GUI.OnUpdate");
    Update(dt);
    INVOKE_EVENT_PARAMS_1(OnUpdate, &dt);
}

void WindowBase::OnDraw()
{
    PROFILE_CPU_NAMED("GUI.OnDraw");
    INVOKE_EVENT_PARAMS_0(OnDraw);
    Draw();
}

bool WindowBase::InitSwapChain()
{
    // Setup swapchain
    if (_swapChain == nullptr)
    {
        _swapChain = GPUDevice::Instance->CreateSwapChain((Window*)this);
        if (!_swapChain)
            return true;
    }
    if (_swapChain->Resize(static_cast<int32>(_clientSize.X), static_cast<int32>(_clientSize.Y)))
        return true;
    if (_settings.Fullscreen)
        _swapChain->SetFullscreen(true);

    // Setup render task
    if (RenderTask == nullptr)
    {
#if !USE_EDITOR
        if (IsMain())
        {
            // Override main task output (render directly to the window backbuffer)
            ASSERT(MainRenderTask::Instance);
            ASSERT(MainRenderTask::Instance->SwapChain == nullptr);
            RenderTask = MainRenderTask::Instance;
            MainRenderTask::Instance->Deleted.Bind<WindowBase, &WindowBase::OnMainRenderTaskDelete>(this);
        }
        else
#endif
        {
            RenderTask = New<::RenderTask>();
        }
        RenderTask->SwapChain = _swapChain;
        RenderTask->Enabled = false;
        RenderTask->Order = 100;
    }

    return false;
}

void WindowBase::Show()
{
    const auto clientSize = GetClientSize();
    const auto width = static_cast<int32>(clientSize.X);
    const auto height = static_cast<int32>(clientSize.Y);
    _visible = true;

    // Ensure to have backbuffer and swapchain ready
    if (InitSwapChain())
    {
        Platform::Fatal(TEXT("Cannot init rendering output for a window."));
    }

    if (RenderTask)
    {
        // Resize render task to fit WindowBase client size
        RenderTask->Resize(width, height);

        // Enable rendering
        RenderTask->Enabled = true;
    }

    // Call GUI event
    OnResize(width, height);
    OnShow();
}

void WindowBase::Hide()
{
    _visible = false;
    _showAfterFirstPaint = _settings.ShowAfterFirstPaint;
    Hidden();
}

void WindowBase::Close(ClosingReason reason)
{
    // Prevent from calling close during or after close action
    if (_isClosing)
        return;
    _isClosing = true;

    // Check if can close window
    bool cancel = false;
    OnClosing(reason, cancel);
    if (cancel && reason != ClosingReason::EngineExit) // Disallow to skip closing on Engine Exit (danger situation)
    {
        // Back
        _isClosing = false;
        return;
    }

    // Close
    EndTrackingMouse();
    Hide();
    OnClosed();
}

bool WindowBase::IsForegroundWindow() const
{
    return _focused;
}
