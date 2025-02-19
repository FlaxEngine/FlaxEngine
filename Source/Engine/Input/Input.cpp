// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Input.h"
#include "InputSettings.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Gamepad.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Platform/Window.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Screen.h"
#include "Engine/Engine/Time.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/JsonTools.h"

struct AxisEvaluation
{
    float RawValue;
    float Value;
    float PrevKeyValue;
    bool Used;
};

struct ActionData
{
    bool Active = false;
    uint64 FrameIndex = 0;
    InputActionState State = InputActionState::Waiting;
};

struct AxisData
{
    float Value = 0.0f;
    float ValueRaw = 0.0f;
    float PrevValue = 0.0f;
    float PrevKeyValue = 0.0f;
    uint64 FrameIndex = 0;
};

namespace InputImpl
{
    Dictionary<String, ActionData> Actions;
    Dictionary<String, AxisData> Axes;
    bool GamepadsChanged = true;
    Array<AxisEvaluation> AxesValues;
    InputDevice::EventQueue InputEvents;
}

using namespace InputImpl;

class InputService : public EngineService
{
public:
    InputService()
        : EngineService(TEXT("Input"), -60)
    {
    }

    void Update() override;
    void Dispose() override;
};

InputService InputServiceInstance;

Mouse* Input::Mouse = nullptr;
Keyboard* Input::Keyboard = nullptr;
Array<Gamepad*, FixedAllocation<MAX_GAMEPADS>> Input::Gamepads;
Action Input::GamepadsChanged;
Array<InputDevice*, InlinedAllocation<16>> Input::CustomDevices;
Delegate<Char> Input::CharInput;
Delegate<KeyboardKeys> Input::KeyDown;
Delegate<KeyboardKeys> Input::KeyUp;
Delegate<const Float2&, MouseButton> Input::MouseDown;
Delegate<const Float2&, MouseButton> Input::MouseUp;
Delegate<const Float2&, MouseButton> Input::MouseDoubleClick;
Delegate<const Float2&, float> Input::MouseWheel;
Delegate<const Float2&> Input::MouseMove;
Action Input::MouseLeave;
Delegate<const Float2&, int32> Input::TouchDown;
Delegate<const Float2&, int32> Input::TouchMove;
Delegate<const Float2&, int32> Input::TouchUp;
Delegate<StringView, InputActionState> Input::ActionTriggered;
Delegate<StringView> Input::AxisValueChanged;
Array<ActionConfig> Input::ActionMappings;
Array<AxisConfig> Input::AxisMappings;

void InputSettings::Apply()
{
    Input::ActionMappings = ActionMappings;
    Input::AxisMappings = AxisMappings;
}

void InputSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    const auto actionMappings = stream.FindMember("ActionMappings");
    if (actionMappings != stream.MemberEnd())
    {
        auto& actionMappingsArray = actionMappings->value;
        if (actionMappingsArray.IsArray())
        {
            ActionMappings.Resize(actionMappingsArray.Size(), false);
            for (uint32 i = 0; i < actionMappingsArray.Size(); i++)
            {
                auto& v = actionMappingsArray[i];
                if (!v.IsObject())
                    continue;

                ActionConfig& config = ActionMappings[i];
                config.Name = JsonTools::GetString(v, "Name");
                config.Mode = JsonTools::GetEnum(v, "Mode", InputActionMode::Pressing);
                config.Key = JsonTools::GetEnum(v, "Key", KeyboardKeys::None);
                config.MouseButton = JsonTools::GetEnum(v, "MouseButton", MouseButton::None);
                config.GamepadButton = JsonTools::GetEnum(v, "GamepadButton", GamepadButton::None);
                config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
            }
        }
        else
        {
            ActionMappings.Resize(0, false);
        }
    }

    const auto axisMappings = stream.FindMember("AxisMappings");
    if (axisMappings != stream.MemberEnd())
    {
        auto& axisMappingsArray = axisMappings->value;
        if (axisMappingsArray.IsArray())
        {
            AxisMappings.Resize(axisMappingsArray.Size(), false);
            for (uint32 i = 0; i < axisMappingsArray.Size(); i++)
            {
                auto& v = axisMappingsArray[i];
                if (!v.IsObject())
                    continue;

                AxisConfig& config = AxisMappings[i];
                config.Name = JsonTools::GetString(v, "Name");
                config.Axis = JsonTools::GetEnum(v, "Axis", InputAxisType::MouseX);
                config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
                config.PositiveButton = JsonTools::GetEnum(v, "PositiveButton", KeyboardKeys::None);
                config.NegativeButton = JsonTools::GetEnum(v, "NegativeButton", KeyboardKeys::None);
                config.GamepadPositiveButton = JsonTools::GetEnum(v, "GamepadPositiveButton", GamepadButton::None);
                config.GamepadNegativeButton = JsonTools::GetEnum(v, "GamepadNegativeButton", GamepadButton::None);
                config.DeadZone = JsonTools::GetFloat(v, "DeadZone", 0.1f);
                config.Sensitivity = JsonTools::GetFloat(v, "Sensitivity", 0.4f);
                config.Gravity = JsonTools::GetFloat(v, "Gravity", 1.0f);
                config.Scale = JsonTools::GetFloat(v, "Scale", 1.0f);
                config.Snap = JsonTools::GetBool(v, "Snap", false);
            }
        }
        else
        {
            AxisMappings.Resize(0, false);
        }
    }
}

void Mouse::OnMouseMoved(const Float2& newPosition)
{
    _prevState.MousePosition = newPosition;
    _state.MousePosition = newPosition;
}

void Mouse::OnMouseDown(const Float2& position, const MouseButton button, Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseDown;
    e.Target = target;
    e.MouseData.Button = button;
    e.MouseData.Position = position;
}

bool Mouse::IsAnyButtonDown() const
{
    // TODO: optimize with SIMD
    bool result = false;
    for (auto e : Mouse::_state.MouseButtons)
        result |= e;
    return result;
}

void Mouse::OnMouseUp(const Float2& position, const MouseButton button, Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseUp;
    e.Target = target;
    e.MouseData.Button = button;
    e.MouseData.Position = position;
}

void Mouse::OnMouseDoubleClick(const Float2& position, const MouseButton button, Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseDoubleClick;
    e.Target = target;
    e.MouseData.Button = button;
    e.MouseData.Position = position;
}

void Mouse::OnMouseMove(const Float2& position, Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseMove;
    e.Target = target;
    e.MouseData.Position = position;
}

void Mouse::OnMouseLeave(Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseLeave;
    e.Target = target;
}

void Mouse::OnMouseWheel(const Float2& position, float delta, Window* target)
{
    Event& e = _queue.AddOne();
    e.Type = EventType::MouseWheel;
    e.Target = target;
    e.MouseWheelData.WheelDelta = delta;
    e.MouseWheelData.Position = position;
}

void Mouse::ResetState()
{
    InputDevice::ResetState();

    _prevState.Clear();
    _state.Clear();
}

bool Mouse::Update(EventQueue& queue)
{
    // Move the current state to the previous
    Platform::MemoryCopy(&_prevState, &_state, sizeof(State));

    // Gather new events
    if (UpdateState())
        return true;

    // Handle events
    _state.MouseWheelDelta = 0;
    for (int32 i = 0; i < _queue.Count(); i++)
    {
        const Event& e = _queue[i];
        switch (e.Type)
        {
        case EventType::MouseDown:
        {
            _state.MouseButtons[static_cast<int32>(e.MouseData.Button)] = true;
            break;
        }
        case EventType::MouseUp:
        {
            _state.MouseButtons[static_cast<int32>(e.MouseData.Button)] = false;
            break;
        }
        case EventType::MouseDoubleClick:
        {
            _state.MouseButtons[static_cast<int32>(e.MouseData.Button)] = true;
            break;
        }
        case EventType::MouseWheel:
        {
            _state.MouseWheelDelta += e.MouseWheelData.WheelDelta;
            break;
        }
        case EventType::MouseMove:
        {
            _state.MousePosition = e.MouseData.Position;
            break;
        }
        case EventType::MouseLeave:
        {
            break;
        }
        }
    }

    // Send events further
    queue.Add(_queue);
    _queue.Clear();
    return false;
}

void Keyboard::State::Clear()
{
    Platform::MemoryClear(this, sizeof(State));
}

Keyboard::Keyboard()
    : InputDevice(SpawnParams(Guid::New(), TypeInitializer), TEXT("Keyboard"))
{
    _state.Clear();
    _prevState.Clear();
}

bool Keyboard::IsAnyKeyDown() const
{
    // TODO: optimize with SIMD
    bool result = false;
    for (auto e : _state.Keys)
        result |= e;
    return result;
}

void Keyboard::OnCharInput(Char c, Window* target)
{
    // Skip control characters
    if (c < 32)
        return;

    Event& e = _queue.AddOne();
    e.Type = EventType::Char;
    e.Target = target;
    e.CharData.Char = c;
}

void Keyboard::OnKeyUp(KeyboardKeys key, Window* target)
{
    if (key >= KeyboardKeys::MAX)
        return;
    Event& e = _queue.AddOne();
    e.Type = EventType::KeyUp;
    e.Target = target;
    e.KeyData.Key = key;
}

void Keyboard::OnKeyDown(KeyboardKeys key, Window* target)
{
    if (key >= KeyboardKeys::MAX)
        return;
    Event& e = _queue.AddOne();
    e.Type = EventType::KeyDown;
    e.Target = target;
    e.KeyData.Key = key;
}

void Keyboard::ResetState()
{
    InputDevice::ResetState();

    _prevState.Clear();
    _state.Clear();
}

bool Keyboard::Update(EventQueue& queue)
{
    // Move the current state to the previous
    Platform::MemoryCopy(&_prevState, &_state, sizeof(State));

    // Gather new events
    if (UpdateState())
        return true;

    // Handle events
    _state.InputTextLength = 0;
    for (int32 i = 0; i < _queue.Count(); i++)
    {
        const Event& e = _queue[i];
        switch (e.Type)
        {
        case EventType::Char:
        {
            if (_state.InputTextLength < ARRAY_COUNT(_state.InputText) - 1)
                _state.InputText[_state.InputTextLength++] = e.CharData.Char;
            break;
        }
        case EventType::KeyDown:
        {
            _state.Keys[static_cast<int32>(e.KeyData.Key)] = true;
            break;
        }
        case EventType::KeyUp:
        {
            _state.Keys[static_cast<int32>(e.KeyData.Key)] = false;
            break;
        }
        }
    }

    // Send events further
    queue.Add(_queue);
    _queue.Clear();
    return false;
}

int32 Input::GetGamepadsCount()
{
    return Gamepads.Count();
}

Gamepad* Input::GetGamepad(int32 index)
{
    if (index >= 0 && index < Gamepads.Count())
        return Gamepads[index];
    return nullptr;
}

void Input::OnGamepadsChanged()
{
    ::GamepadsChanged = true;
}

StringView Input::GetInputText()
{
    return Keyboard ? Keyboard->GetInputText() : StringView::Empty;
}

bool Input::GetKey(const KeyboardKeys key)
{
    return Keyboard ? Keyboard->GetKey(key) : false;
}

bool Input::GetKeyDown(const KeyboardKeys key)
{
    return Keyboard ? Keyboard->GetKeyDown(key) : false;
}

bool Input::GetKeyUp(const KeyboardKeys key)
{
    return Keyboard ? Keyboard->GetKeyUp(key) : false;
}

Float2 Input::GetMousePosition()
{
    return Mouse ? Screen::ScreenToGameViewport(Mouse->GetPosition()) : Float2::Minimum;
}

void Input::SetMousePosition(const Float2& position)
{
    if (Mouse && Engine::HasGameViewportFocus())
    {
        const auto pos = Screen::GameViewportToScreen(position);
        if (pos > Float2::Minimum)
            Mouse->SetMousePosition(pos);
    }
}

Float2 Input::GetMouseScreenPosition()
{
    return Mouse ? Mouse->GetPosition() : Float2::Minimum;
}

void Input::SetMouseScreenPosition(const Float2& position)
{
    if (Mouse && Engine::HasFocus)
    {
        Mouse->SetMousePosition(position);
    }
}

Float2 Input::GetMousePositionDelta()
{
    return Mouse ? Mouse->GetPositionDelta() : Float2::Zero;
}

float Input::GetMouseScrollDelta()
{
    return Mouse ? Mouse->GetScrollDelta() : 0.0f;
}

bool Input::GetMouseButton(const MouseButton button)
{
    return Mouse ? Mouse->GetButton(button) : false;
}

bool Input::GetMouseButtonDown(const MouseButton button)
{
    return Mouse ? Mouse->GetButtonDown(button) : false;
}

bool Input::GetMouseButtonUp(const MouseButton button)
{
    return Mouse ? Mouse->GetButtonUp(button) : false;
}

float Input::GetGamepadAxis(int32 gamepadIndex, GamepadAxis axis)
{
    if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
        return Gamepads[gamepadIndex]->GetAxis(axis);
    return 0.0f;
}

bool Input::GetGamepadButton(int32 gamepadIndex, GamepadButton button)
{
    if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
        return Gamepads[gamepadIndex]->GetButton(button);
    return false;
}

bool Input::GetGamepadButtonDown(int32 gamepadIndex, GamepadButton button)
{
    if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
        return Gamepads[gamepadIndex]->GetButtonDown(button);
    return false;
}

bool Input::GetGamepadButtonUp(int32 gamepadIndex, GamepadButton button)
{
    if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
        return Gamepads[gamepadIndex]->GetButtonUp(button);
    return false;
}

float Input::GetGamepadAxis(InputGamepadIndex gamepad, GamepadAxis axis)
{
    if (gamepad == InputGamepadIndex::All)
    {
        float result = 0.0f;
        for (auto g : Gamepads)
        {
            float v = g->GetAxis(axis);
            if (Math::Abs(v) > Math::Abs(result))
                result = v;
        }
        return result;
    }
    else
    {
        const auto index = static_cast<int32>(gamepad);
        if (index < Gamepads.Count())
            return Gamepads[index]->GetAxis(axis);
    }
    return false;
}

bool Input::GetGamepadButton(InputGamepadIndex gamepad, GamepadButton button)
{
    if (gamepad == InputGamepadIndex::All)
    {
        for (auto g : Gamepads)
        {
            if (g->GetButton(button))
                return true;
        }
    }
    else
    {
        const auto index = static_cast<int32>(gamepad);
        if (index < Gamepads.Count())
            return Gamepads[index]->GetButton(button);
    }
    return false;
}

bool Input::GetGamepadButtonDown(InputGamepadIndex gamepad, GamepadButton button)
{
    if (gamepad == InputGamepadIndex::All)
    {
        for (auto g : Gamepads)
        {
            if (g->GetButtonDown(button))
                return true;
        }
    }
    else
    {
        const auto index = static_cast<int32>(gamepad);
        if (index < Gamepads.Count())
            return Gamepads[index]->GetButtonDown(button);
    }
    return false;
}

bool Input::GetGamepadButtonUp(InputGamepadIndex gamepad, GamepadButton button)
{
    if (gamepad == InputGamepadIndex::All)
    {
        for (auto g : Gamepads)
        {
            if (g->GetButtonUp(button))
                return true;
        }
    }
    else
    {
        const auto index = static_cast<int32>(gamepad);
        if (index < Gamepads.Count())
            return Gamepads[index]->GetButtonUp(button);
    }
    return false;
}

bool Input::GetAction(const StringView& name)
{
    const auto e = Actions.TryGet(name);
    return e ? e->Active : false;
}

InputActionState Input::GetActionState(const StringView& name)
{
    const auto e = Actions.TryGet(name);
    if (e != nullptr)
    {
        return e->State;
    }
    return InputActionState::None;
}

float Input::GetAxis(const StringView& name)
{
    const auto e = Axes.TryGet(name);
    return e ? e->Value : false;
}

float Input::GetAxisRaw(const StringView& name)
{
    const auto e = Axes.TryGet(name);
    return e ? e->ValueRaw : false;
}

void Input::SetInputMappingFromSettings(const JsonAssetReference<InputSettings>& settings)
{
    auto actionMappings = settings.GetInstance()->ActionMappings;
    ActionMappings.Resize(actionMappings.Count(), false);
    for (int i = 0; i < actionMappings.Count(); i++)
    {
        ActionMappings[i] = actionMappings.At(i);
    }

    auto axisMappings = settings.GetInstance()->AxisMappings;
    AxisMappings.Resize(axisMappings.Count(), false);
    for (int i = 0; i < axisMappings.Count(); i++)
    {
        AxisMappings[i] = axisMappings.At(i);
    }
    Axes.Clear();
    Actions.Clear();
}

void Input::SetInputMappingToDefaultSettings()
{
    InputSettings* settings = InputSettings::Get();
    if (settings)
    {
        ActionMappings = settings->ActionMappings;
        AxisMappings = settings->AxisMappings;
        Axes.Clear();
        Actions.Clear();
    }
}

ActionConfig Input::GetActionConfigByName(const StringView& name)
{
    ActionConfig config = {};
    for (const auto& a : ActionMappings)
    {
        if (a.Name == name)
        {
            config = a;
            return config;
        }
    }
    return config;
}

Array<ActionConfig> Input::GetAllActionConfigsByName(const StringView& name)
{
    Array<ActionConfig> actionConfigs;
    for (const auto& a : ActionMappings)
    {
        if (a.Name == name)
            actionConfigs.Add(a);
    }
    return actionConfigs;
}

AxisConfig Input::GetAxisConfigByName(const StringView& name)
{
    AxisConfig config = {};
    for (const auto& a : AxisMappings)
    {
        if (a.Name == name)
        {
            config = a;
            return config;
        }
    }
    return config;
}

Array<AxisConfig> Input::GetAllAxisConfigsByName(const StringView& name)
{
    Array<AxisConfig> actionConfigs;
    for (const auto& a : AxisMappings)
    {
        if (a.Name == name)
            actionConfigs.Add(a);
    }
    return actionConfigs;
}

void Input::SetAxisConfigByName(const StringView& name, AxisConfig& config, bool all)
{
    for (int i = 0; i < AxisMappings.Count(); ++i)
    {
        auto& mapping = AxisMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0)
        {
            if (config.Name.IsEmpty())
                config.Name = name;
            mapping = config;
            if (!all)
                break;
        }
    }
}

void Input::SetAxisConfigByName(const StringView& name, InputAxisType inputType, const KeyboardKeys positiveButton, const KeyboardKeys negativeButton, bool all)
{
    for (int i = 0; i < AxisMappings.Count(); ++i)
    {
        auto& mapping = AxisMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0 && mapping.Axis == inputType)
        {
            mapping.PositiveButton = positiveButton;
            mapping.NegativeButton = negativeButton;
            if (!all)
                break;
        }
    }
}

void Input::SetAxisConfigByName(const StringView& name, InputAxisType inputType, const GamepadButton positiveButton, const GamepadButton negativeButton, InputGamepadIndex gamepadIndex, bool all)
{
    for (int i = 0; i < AxisMappings.Count(); ++i)
    {
        auto& mapping = AxisMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0 && mapping.Gamepad == gamepadIndex && mapping.Axis == inputType)
        {
            mapping.GamepadPositiveButton = positiveButton;
            mapping.GamepadNegativeButton = negativeButton;
            if (!all)
                break;
        }
    }
}

void Input::SetAxisConfigByName(const StringView& name, InputAxisType inputType, const float gravity, const float deadZone, const float sensitivity, const float scale, const bool snap, bool all)
{
    for (int i = 0; i < AxisMappings.Count(); ++i)
    {
        auto& mapping = AxisMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0 && mapping.Axis == inputType)
        {
            mapping.Gravity = gravity;
            mapping.DeadZone = deadZone;
            mapping.Sensitivity = sensitivity;
            mapping.Scale = scale;
            mapping.Snap = snap;
            if (!all)
                break;
        }
    }
}

void Input::SetActionConfigByName(const StringView& name, const KeyboardKeys key, bool all)
{
    for (int i = 0; i < ActionMappings.Count(); ++i)
    {
        auto& mapping = ActionMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0)
        {
            mapping.Key = key;
            if (!all)
                break;
        }
    }
}

void Input::SetActionConfigByName(const StringView& name, const MouseButton mouseButton, bool all)
{
    for (int i = 0; i < ActionMappings.Count(); ++i)
    {
        auto& mapping = ActionMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0)
        {
            mapping.MouseButton = mouseButton;
            if (!all)
                break;
        }
    }
}

void Input::SetActionConfigByName(const StringView& name, const GamepadButton gamepadButton, InputGamepadIndex gamepadIndex, bool all)
{
    for (int i = 0; i < ActionMappings.Count(); ++i)
    {
        auto& mapping = ActionMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0 && mapping.Gamepad == gamepadIndex)
        {
            mapping.GamepadButton = gamepadButton;
            if (!all)
                break;
        }
    }
}

void Input::SetActionConfigByName(const StringView& name, ActionConfig& config, bool all)
{
    for (int i = 0; i < ActionMappings.Count(); ++i)
    {
        auto& mapping = ActionMappings.At(i);
        if (mapping.Name.Compare(name.ToString()) == 0)
        {
            if (config.Name.IsEmpty())
                config.Name = name;
            mapping = config;
            if (!all)
                break;
        }
    }
}

void InputService::Update()
{
    PROFILE_CPU();
    const auto frame = Time::Update.TicksCount;
    const auto dt = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
    InputEvents.Clear();

    // If application has no user focus then simply clear the state
    if (!Engine::HasFocus)
    {
        if (Input::Mouse)
            Input::Mouse->ResetState();
        if (Input::Keyboard)
            Input::Keyboard->ResetState();
        for (int32 i = 0; i < Input::Gamepads.Count(); i++)
            Input::Gamepads[i]->ResetState();
        Axes.Clear();
        Actions.Clear();
        return;
    }

    // Update input devices state
    if (Input::Mouse)
    {
        if (Input::Mouse->Update(InputEvents))
        {
            Input::Mouse->DeleteObject();
            Input::Mouse = nullptr;
        }
    }
    if (Input::Keyboard)
    {
        if (Input::Keyboard->Update(InputEvents))
        {
            Input::Keyboard->DeleteObject();
            Input::Keyboard = nullptr;
        }
    }
    for (int32 i = 0; i < Input::Gamepads.Count(); i++)
    {
        if (Input::Gamepads[i]->Update(InputEvents))
        {
            Input::Gamepads[i]->DeleteObject();
            Input::Gamepads.RemoveAtKeepOrder(i);
            Input::OnGamepadsChanged();
            i--;
            if (Input::Gamepads.IsEmpty())
                break;
        }
    }
    for (int32 i = 0; i < Input::CustomDevices.Count(); i++)
    {
        if (Input::CustomDevices[i]->Update(InputEvents))
        {
            Input::CustomDevices[i]->DeleteObject();
            Input::CustomDevices.RemoveAtKeepOrder(i);
            i--;
            if (Input::CustomDevices.IsEmpty())
                break;
        }
    }

    // Send gamepads change events
    if (GamepadsChanged)
    {
        GamepadsChanged = false;
        Input::GamepadsChanged();
    }

    // Pick the first focused window for input events
    WindowsManager::WindowsLocker.Lock();
    Window* defaultWindow = nullptr;
    for (auto window : WindowsManager::Windows)
    {
        if (window->IsFocused() && window->GetSettings().AllowInput)
        {
            defaultWindow = window;
            break;
        }
    }
    WindowsManager::WindowsLocker.Unlock();

    // Send input events for the focused window
    WindowsManager::WindowsLocker.Lock();
    for (const auto& e : InputEvents)
    {
        auto window = e.Target ? e.Target : defaultWindow;
        if (!window || !WindowsManager::Windows.Contains(window))
            continue;
        switch (e.Type)
        {
        // Keyboard events
        case InputDevice::EventType::Char:
            window->OnCharInput(e.CharData.Char);
            break;
        case InputDevice::EventType::KeyDown:
            window->OnKeyDown(e.KeyData.Key);
            break;
        case InputDevice::EventType::KeyUp:
            window->OnKeyUp(e.KeyData.Key);
            break;
        // Mouse events
        case InputDevice::EventType::MouseDown:
            window->OnMouseDown(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseUp:
            window->OnMouseUp(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseDoubleClick:
            window->OnMouseDoubleClick(window->ScreenToClient(e.MouseData.Position), e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseWheel:
            window->OnMouseWheel(window->ScreenToClient(e.MouseWheelData.Position), e.MouseWheelData.WheelDelta);
            break;
        case InputDevice::EventType::MouseMove:
            window->OnMouseMove(window->ScreenToClient(e.MouseData.Position));
            break;
        case InputDevice::EventType::MouseLeave:
            window->OnMouseLeave();
            break;
        // Touch events
        case InputDevice::EventType::TouchDown:
            window->OnTouchDown(window->ScreenToClient(e.TouchData.Position), e.TouchData.PointerId);
            break;
        case InputDevice::EventType::TouchMove:
            window->OnTouchMove(window->ScreenToClient(e.TouchData.Position), e.TouchData.PointerId);
            break;
        case InputDevice::EventType::TouchUp:
            window->OnTouchUp(window->ScreenToClient(e.TouchData.Position), e.TouchData.PointerId);
            break;
        }
    }
    WindowsManager::WindowsLocker.Unlock();

    // Skip if game has no focus to handle the input
    if (!Engine::HasGameViewportFocus())
    {
        Axes.Clear();
        Actions.Clear();
        return;
    }

    // Send input events
    for (const auto& e : InputEvents)
    {
        switch (e.Type)
        {
        // Keyboard events
        case InputDevice::EventType::Char:
            Input::CharInput(e.CharData.Char);
            break;
        case InputDevice::EventType::KeyDown:
            Input::KeyDown(e.KeyData.Key);
            break;
        case InputDevice::EventType::KeyUp:
            Input::KeyUp(e.KeyData.Key);
            break;
        // Mouse events
        case InputDevice::EventType::MouseDown:
            Input::MouseDown(e.MouseData.Position, e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseUp:
            Input::MouseUp(e.MouseData.Position, e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseDoubleClick:
            Input::MouseDoubleClick(e.MouseData.Position, e.MouseData.Button);
            break;
        case InputDevice::EventType::MouseWheel:
            Input::MouseWheel(e.MouseWheelData.Position, e.MouseWheelData.WheelDelta);
            break;
        case InputDevice::EventType::MouseMove:
            Input::MouseMove(e.MouseData.Position);
            break;
        case InputDevice::EventType::MouseLeave:
            Input::MouseLeave();
            break;
        // Touch events
        case InputDevice::EventType::TouchDown:
            Input::TouchDown(e.TouchData.Position, e.TouchData.PointerId);
            break;
        case InputDevice::EventType::TouchMove:
            Input::TouchMove(e.TouchData.Position, e.TouchData.PointerId);
            break;
        case InputDevice::EventType::TouchUp:
            Input::TouchUp(e.TouchData.Position, e.TouchData.PointerId);
            break;
        }
    }

    // Update all actions
    for (int32 i = 0; i < Input::ActionMappings.Count(); i++)
    {
        const auto& config = Input::ActionMappings[i];
        const StringView name = config.Name;
        ActionData& data = Actions[name];

        data.Active = false;
        data.State = InputActionState::Waiting;

        // Mark as updated in this frame
        data.FrameIndex = frame;
    }
    for (int32 i = 0; i < Input::ActionMappings.Count(); i++)
    {
        const auto& config = Input::ActionMappings[i];
        const StringView name = config.Name;
        ActionData& data = Actions[name];

        bool isActive;
        if (config.Mode == InputActionMode::Pressing)
        {
            isActive = Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadButton);
        }
        else if (config.Mode == InputActionMode::Press)
        {
            isActive = Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) || Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton);
        }
        else
        {
            isActive = Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) || Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton);
        }

        if (Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) || Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton))
        {
            data.State = InputActionState::Press;
        }
        else if (Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadButton))
        {
            data.State = InputActionState::Pressing;
        }
        else if (Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) || Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton))
        {
            data.State = InputActionState::Release;
        }

        data.Active |= isActive;
    }

    // Update all axes
    AxesValues.Resize(Input::AxisMappings.Count(), false);
    for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
    {
        const auto& config = Input::AxisMappings[i];
        const StringView name = config.Name;
        const AxisData& data = Axes[name];

        // Get key raw value
        const bool isPositiveKey = Input::GetKey(config.PositiveButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadPositiveButton);
        const bool isNegativeKey = Input::GetKey(config.NegativeButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadNegativeButton);
        float keyRawValue = 0;
        if (isPositiveKey && !isNegativeKey)
        {
            keyRawValue = 1;
        }
        else if (!isPositiveKey && isNegativeKey)
        {
            keyRawValue = -1;
        }

        // Apply keyboard curve smoothing and snapping
        float prevKeyValue = data.PrevKeyValue;
        if (config.Snap && Math::NotSameSign(data.PrevKeyValue, keyRawValue))
        {
            prevKeyValue = 0;
        }
        float keyValue;
        if (Math::Abs(prevKeyValue) <= Math::Abs(keyRawValue))
        {
            keyValue = Math::LerpStable(prevKeyValue, keyRawValue, Math::Saturate(dt * config.Sensitivity));
        }
        else
        {
            keyValue = Math::LerpStable(prevKeyValue, keyRawValue, Math::Saturate(dt * config.Gravity));
        }

        // Get axis raw value
        float axisRawValue = 0.0f;
        switch (config.Axis)
        {
        case InputAxisType::MouseX:
            axisRawValue = Input::GetMousePositionDelta().X * config.Sensitivity;
            break;
        case InputAxisType::MouseY:
            axisRawValue = Input::GetMousePositionDelta().Y * config.Sensitivity;
            break;
        case InputAxisType::MouseWheel:
            axisRawValue = Input::GetMouseScrollDelta() * config.Sensitivity;
            break;
        case InputAxisType::GamepadLeftStickX:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftStickX);
            break;
        case InputAxisType::GamepadLeftStickY:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftStickY);
            break;
        case InputAxisType::GamepadRightStickX:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightStickX);
            break;
        case InputAxisType::GamepadRightStickY:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightStickY);
            break;
        case InputAxisType::GamepadLeftTrigger:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftTrigger);
            break;
        case InputAxisType::GamepadRightTrigger:
            axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightTrigger);
            break;
        case InputAxisType::GamepadDPadX:
            if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadRight))
                axisRawValue = 1;
            else if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadLeft))
                axisRawValue = -1;
            break;
        case InputAxisType::GamepadDPadY:
            if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadUp))
                axisRawValue = 1;
            else if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadDown))
                axisRawValue = -1;
            break;
        }

        // Apply dead zone
        const float deadZone = config.DeadZone;
        float axisValue = axisRawValue >= deadZone || axisRawValue <= -deadZone ? axisRawValue : 0.0f;
        keyValue = keyValue >= deadZone || keyValue <= -deadZone ? keyValue : 0.0f;

        auto& e = AxesValues[i];
        e.Used = false;
        e.PrevKeyValue = keyRawValue;

        // Select keyboard input or axis input (choose the higher absolute values)
        e.Value = Math::Abs(keyValue) > Math::Abs(axisValue) ? keyValue : axisValue;
        e.RawValue = Math::Abs(keyRawValue) > Math::Abs(axisRawValue) ? keyRawValue : axisRawValue;

        // Scale
        e.Value *= config.Scale;
    }
    for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
    {
        auto& e = AxesValues[i];
        if (e.Used)
            continue;
        const auto& config = Input::AxisMappings[i];
        const StringView name = config.Name;
        AxisData& data = Axes[name];

        // Blend final axis raw value between all entries
        // Virtual axis with the same name may be used more than once, select the highest absolute value
        for (int32 j = i + 1; j < Input::AxisMappings.Count(); j++)
        {
            auto& other = AxesValues[j];
            if (!other.Used && Input::AxisMappings[j].Name == config.Name)
            {
                if (Math::Abs(other.Value) > Math::Abs(e.Value))
                {
                    e = other;
                }
                other.Used = true;
            }
        }

        // Setup axis data
        data.PrevKeyValue = e.PrevKeyValue;
        data.PrevValue = data.Value;
        data.ValueRaw = e.RawValue;
        data.Value = e.Value;

        // Mark as updated in this frame
        data.FrameIndex = frame;
    }

    // Remove not used entries
    for (auto i = Actions.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value.FrameIndex != frame)
        {
            Actions.Remove(i);
        }
    }
    for (auto i = Axes.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value.FrameIndex != frame)
        {
            Axes.Remove(i);
        }
    }

    // Lock mouse if need to
    const auto lockMode = Screen::GetCursorLock();
    if (lockMode == CursorLockMode::Locked)
    {
        Input::SetMousePosition(Screen::GetSize() * 0.5f);
    }

    // Send events for the active actions and axes (send events only in play mode)
    if (!Time::GetGamePaused())
    {
        for (auto i = Axes.Begin(); i.IsNotEnd(); ++i)
        {
            if (Math::NotNearEqual(i->Value.Value, i->Value.PrevValue))
            {
                Input::AxisValueChanged(i->Key);
            }
        }
        
        for (auto i = Actions.Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value.State != InputActionState::Waiting)
            {
                Input::ActionTriggered(i->Key, i->Value.State);
            }
        }
    }
}

void InputService::Dispose()
{
    // Dispose input devices
    if (Input::Mouse)
    {
        Input::Mouse->DeleteObject();
        Input::Mouse = nullptr;
    }
    if (Input::Keyboard)
    {
        Input::Keyboard->DeleteObject();
        Input::Keyboard = nullptr;
    }
    for (int32 i = 0; i < Input::Gamepads.Count(); i++)
        Input::Gamepads[i]->DeleteObject();
    Input::Gamepads.Clear();
    for (int32 i = 0; i < Input::CustomDevices.Count(); i++)
        Input::CustomDevices[i]->DeleteObject();
    Input::CustomDevices.Clear();
}
