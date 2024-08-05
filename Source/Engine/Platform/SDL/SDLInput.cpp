// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_SDL

#include "SDLInput.h"
#include "SDLWindow.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Log.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Gamepad.h"

#include <SDL3/SDL_events.h>

class SDLMouse;
class SDLKeyboard;
class SDLGamepad;

// TODO: Turn these into customizable values
#define TRIGGER_THRESHOLD 30
#define LEFT_STICK_THRESHOLD 7849
#define RIGHT_STICK_THRESHOLD 8689

namespace SDLInputImpl
{
    SDLMouse* Mouse = nullptr;
    SDLKeyboard* Keyboard = nullptr;
    Dictionary<SDL_JoystickID, SDLGamepad*> Gamepads;
}

static const KeyboardKeys SDL_TO_FLAX_KEYS_MAP[] =
{
    KeyboardKeys::None, // SDL_SCANCODE_UNKNOWN
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::A,
    KeyboardKeys::B,
    KeyboardKeys::C,
    KeyboardKeys::D,
    KeyboardKeys::E,
    KeyboardKeys::F,
    KeyboardKeys::G,
    KeyboardKeys::H,
    KeyboardKeys::I,
    KeyboardKeys::J,
    KeyboardKeys::K,
    KeyboardKeys::L,
    KeyboardKeys::M,
    KeyboardKeys::N,
    KeyboardKeys::O,
    KeyboardKeys::P,
    KeyboardKeys::Q,
    KeyboardKeys::R,
    KeyboardKeys::S,
    KeyboardKeys::T,
    KeyboardKeys::U,
    KeyboardKeys::V,
    KeyboardKeys::W,
    KeyboardKeys::X,
    KeyboardKeys::Y,
    KeyboardKeys::Z, // 29
    KeyboardKeys::Alpha1,
    KeyboardKeys::Alpha2,
    KeyboardKeys::Alpha3,
    KeyboardKeys::Alpha4,
    KeyboardKeys::Alpha5,
    KeyboardKeys::Alpha6,
    KeyboardKeys::Alpha7,
    KeyboardKeys::Alpha8,
    KeyboardKeys::Alpha9,
    KeyboardKeys::Alpha0, // 39
    KeyboardKeys::Return,
    KeyboardKeys::Escape,
    KeyboardKeys::Backspace,
    KeyboardKeys::Tab,
    KeyboardKeys::Spacebar,
    KeyboardKeys::Minus,
    KeyboardKeys::None, //KeyboardKeys::Equals, // ?
    KeyboardKeys::LeftBracket,
    KeyboardKeys::RightBracket,
    KeyboardKeys::Backslash, // SDL_SCANCODE_BACKSLASH ?
    KeyboardKeys::Oem102, // SDL_SCANCODE_NONUSHASH ?
    KeyboardKeys::Colon, // SDL_SCANCODE_SEMICOLON ?
    KeyboardKeys::Quote, // SDL_SCANCODE_APOSTROPHE
    KeyboardKeys::BackQuote, // SDL_SCANCODE_GRAVE
    KeyboardKeys::Comma,
    KeyboardKeys::Period,
    KeyboardKeys::Slash,
    KeyboardKeys::Capital,
    KeyboardKeys::F1,
    KeyboardKeys::F2,
    KeyboardKeys::F3,
    KeyboardKeys::F4,
    KeyboardKeys::F5,
    KeyboardKeys::F6,
    KeyboardKeys::F7,
    KeyboardKeys::F8,
    KeyboardKeys::F9,
    KeyboardKeys::F10,
    KeyboardKeys::F11,
    KeyboardKeys::F12,
    KeyboardKeys::PrintScreen,
    KeyboardKeys::Scroll,
    KeyboardKeys::Pause,
    KeyboardKeys::Insert,
    KeyboardKeys::Home,
    KeyboardKeys::PageUp,
    KeyboardKeys::Delete,
    KeyboardKeys::End,
    KeyboardKeys::PageDown,
    KeyboardKeys::ArrowRight,
    KeyboardKeys::ArrowLeft,
    KeyboardKeys::ArrowDown,
    KeyboardKeys::ArrowUp,
    KeyboardKeys::Numlock,
    KeyboardKeys::NumpadDivide,
    KeyboardKeys::NumpadMultiply,
    KeyboardKeys::NumpadSubtract,
    KeyboardKeys::NumpadAdd,
    KeyboardKeys::Return, // SDL_SCANCODE_KP_ENTER ?
    KeyboardKeys::Numpad1,
    KeyboardKeys::Numpad2,
    KeyboardKeys::Numpad3,
    KeyboardKeys::Numpad4,
    KeyboardKeys::Numpad5,
    KeyboardKeys::Numpad6,
    KeyboardKeys::Numpad7,
    KeyboardKeys::Numpad8,
    KeyboardKeys::Numpad9,
    KeyboardKeys::Numpad0, //98
    KeyboardKeys::NumpadDecimal, // SDL_SCANCODE_KP_PERIOD
    KeyboardKeys::Backslash, // SDL_SCANCODE_NONUSBACKSLASH ?
    KeyboardKeys::Applications,
    KeyboardKeys::Sleep, // SDL_SCANCODE_POWER ?
    KeyboardKeys::None, // SDL_SCANCODE_KP_EQUALS ?
    KeyboardKeys::F13,
    KeyboardKeys::F14,
    KeyboardKeys::F15,
    KeyboardKeys::F16,
    KeyboardKeys::F17,
    KeyboardKeys::F18,
    KeyboardKeys::F19,
    KeyboardKeys::F20,
    KeyboardKeys::F21,
    KeyboardKeys::F22,
    KeyboardKeys::F23,
    KeyboardKeys::F24,
    KeyboardKeys::Execute,
    KeyboardKeys::Help,
    KeyboardKeys::LeftMenu, // SDL_SCANCODE_MENU ?
    KeyboardKeys::Select,
    KeyboardKeys::None, // SDL_SCANCODE_STOP
    KeyboardKeys::None, // SDL_SCANCODE_AGAIN
    KeyboardKeys::None, // SDL_SCANCODE_UNDO
    KeyboardKeys::None, // SDL_SCANCODE_CUT
    KeyboardKeys::None, // SDL_SCANCODE_COPY
    KeyboardKeys::None, // SDL_SCANCODE_PASTE
    KeyboardKeys::None, // SDL_SCANCODE_FIND
    KeyboardKeys::None, // SDL_SCANCODE_MUTE
    KeyboardKeys::None, // SDL_SCANCODE_VOLUMEUP
    KeyboardKeys::None, // SDL_SCANCODE_VOLUMEDOWN
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::NumpadSeparator, // SDL_SCANCODE_KP_COMMA ?
    KeyboardKeys::None, // SDL_SCANCODE_KP_EQUALSAS400
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL1
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL2
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL3
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL4
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL5
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL6
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL7
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL8
    KeyboardKeys::None, // SDL_SCANCODE_INTERNATIONAL9
    KeyboardKeys::None, // SDL_SCANCODE_LANG1
    KeyboardKeys::None, // SDL_SCANCODE_LANG2
    KeyboardKeys::None, // SDL_SCANCODE_LANG3
    KeyboardKeys::None, // SDL_SCANCODE_LANG4
    KeyboardKeys::None, // SDL_SCANCODE_LANG5
    KeyboardKeys::None, // SDL_SCANCODE_LANG6
    KeyboardKeys::None, // SDL_SCANCODE_LANG7
    KeyboardKeys::None, // SDL_SCANCODE_LANG8
    KeyboardKeys::None, // SDL_SCANCODE_LANG9
    KeyboardKeys::None, // SDL_SCANCODE_ALTERASE
    KeyboardKeys::None, // SDL_SCANCODE_SYSREQ
    KeyboardKeys::None, // SDL_SCANCODE_CANCEL
    KeyboardKeys::Clear, // SDL_SCANCODE_CLEAR
    KeyboardKeys::None, // SDL_SCANCODE_PRIOR
    KeyboardKeys::None, // SDL_SCANCODE_RETURN2
    KeyboardKeys::None, // SDL_SCANCODE_SEPARATOR
    KeyboardKeys::None, // SDL_SCANCODE_OUT
    KeyboardKeys::None, // SDL_SCANCODE_OPER
    KeyboardKeys::None, // SDL_SCANCODE_CLEARAGAIN
    KeyboardKeys::None, // SDL_SCANCODE_CRSEL
    KeyboardKeys::None, // SDL_SCANCODE_EXSEL
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None, // SDL_SCANCODE_KP_00
    KeyboardKeys::None, // SDL_SCANCODE_KP_000
    KeyboardKeys::None, // SDL_SCANCODE_THOUSANDSSEPARATOR
    KeyboardKeys::None, // SDL_SCANCODE_DECIMALSEPARATOR
    KeyboardKeys::None, // SDL_SCANCODE_CURRENCYUNIT
    KeyboardKeys::None, // SDL_SCANCODE_CURRENCYSUBUNIT
    KeyboardKeys::None, // SDL_SCANCODE_KP_LEFTPAREN = 182,
    KeyboardKeys::None, // SDL_SCANCODE_KP_RIGHTPAREN = 183,
    KeyboardKeys::None, // SDL_SCANCODE_KP_LEFTBRACE = 184,
    KeyboardKeys::None, // SDL_SCANCODE_KP_RIGHTBRACE = 185,
    KeyboardKeys::None, // SDL_SCANCODE_KP_TAB = 186,
    KeyboardKeys::None, // SDL_SCANCODE_KP_BACKSPACE = 187,
    KeyboardKeys::None, // SDL_SCANCODE_KP_A = 188,
    KeyboardKeys::None, // SDL_SCANCODE_KP_B = 189,
    KeyboardKeys::None, // SDL_SCANCODE_KP_C = 190,
    KeyboardKeys::None, // SDL_SCANCODE_KP_D = 191,
    KeyboardKeys::None, // SDL_SCANCODE_KP_E = 192,
    KeyboardKeys::None, // SDL_SCANCODE_KP_F = 193,
    KeyboardKeys::None, // SDL_SCANCODE_KP_XOR = 194,
    KeyboardKeys::None, // SDL_SCANCODE_KP_POWER = 195,
    KeyboardKeys::None, // SDL_SCANCODE_KP_PERCENT = 196,
    KeyboardKeys::None, // SDL_SCANCODE_KP_LESS = 197,
    KeyboardKeys::None, // SDL_SCANCODE_KP_GREATER = 198,
    KeyboardKeys::None, // SDL_SCANCODE_KP_AMPERSAND = 199,
    KeyboardKeys::None, // SDL_SCANCODE_KP_DBLAMPERSAND = 200,
    KeyboardKeys::None, // SDL_SCANCODE_KP_VERTICALBAR = 201,
    KeyboardKeys::None, // SDL_SCANCODE_KP_DBLVERTICALBAR = 202,
    KeyboardKeys::None, // SDL_SCANCODE_KP_COLON = 203,
    KeyboardKeys::None, // SDL_SCANCODE_KP_HASH = 204,
    KeyboardKeys::None, // SDL_SCANCODE_KP_SPACE = 205,
    KeyboardKeys::None, // SDL_SCANCODE_KP_AT = 206,
    KeyboardKeys::None, // SDL_SCANCODE_KP_EXCLAM = 207,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMSTORE = 208,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMRECALL = 209,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMCLEAR = 210,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMADD = 211,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMSUBTRACT = 212,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMMULTIPLY = 213,
    KeyboardKeys::None, // SDL_SCANCODE_KP_MEMDIVIDE = 214,
    KeyboardKeys::None, // SDL_SCANCODE_KP_PLUSMINUS = 215,
    KeyboardKeys::None, // SDL_SCANCODE_KP_CLEAR = 216,
    KeyboardKeys::None, // SDL_SCANCODE_KP_CLEARENTRY = 217,
    KeyboardKeys::None, // SDL_SCANCODE_KP_BINARY = 218,
    KeyboardKeys::None, // SDL_SCANCODE_KP_OCTAL = 219,
    KeyboardKeys::None, // SDL_SCANCODE_KP_DECIMAL = 220,
    KeyboardKeys::None, // SDL_SCANCODE_KP_HEXADECIMAL = 221,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::Control, // SDL_SCANCODE_LCTRL = 224,
    KeyboardKeys::Shift, // SDL_SCANCODE_LSHIFT = 225,
    KeyboardKeys::Alt, // SDL_SCANCODE_LALT = 226,
    KeyboardKeys::LeftMenu, // SDL_SCANCODE_LGUI = 227,
    KeyboardKeys::Control, // SDL_SCANCODE_RCTRL = 228,
    KeyboardKeys::Shift, // SDL_SCANCODE_RSHIFT = 229,
    KeyboardKeys::Alt, // SDL_SCANCODE_RALT = 230,
    KeyboardKeys::RightMenu, // SDL_SCANCODE_RGUI = 231,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::None,
    KeyboardKeys::Modechange, // SDL_SCANCODE_MODE
    KeyboardKeys::Sleep, // SDL_SCANCODE_SLEEP
    KeyboardKeys::None, // SDL_SCANCODE_WAKE
    KeyboardKeys::None, // SDL_SCANCODE_CHANNEL_INCREMENT = 260,
    KeyboardKeys::None, // SDL_SCANCODE_CHANNEL_DECREMENT = 261,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_PLAY = 262,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_PAUSE = 263,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_RECORD = 264,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_FAST_FORWARD = 265,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_REWIND = 266,
    KeyboardKeys::MediaNextTrack, // SDL_SCANCODE_MEDIA_NEXT_TRACK = 267,
    KeyboardKeys::MediaPrevTrack, // SDL_SCANCODE_MEDIA_PREVIOUS_TRACK = 268,
    KeyboardKeys::MediaStop, // SDL_SCANCODE_MEDIA_STOP = 269,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_EJECT = 270,
    KeyboardKeys::MediaPlayPause, // SDL_SCANCODE_MEDIA_PLAY_PAUSE = 271,
    KeyboardKeys::None, // SDL_SCANCODE_MEDIA_SELECT = 272,
    KeyboardKeys::None, // SDL_SCANCODE_AC_NEW = 273,
    KeyboardKeys::None, // SDL_SCANCODE_AC_OPEN = 274,
    KeyboardKeys::None, // SDL_SCANCODE_AC_CLOSE = 275,
    KeyboardKeys::None, // SDL_SCANCODE_AC_EXIT = 276,
    KeyboardKeys::None, // SDL_SCANCODE_AC_SAVE = 277,
    KeyboardKeys::None, // SDL_SCANCODE_AC_PRINT = 278,
    KeyboardKeys::None, // SDL_SCANCODE_AC_PROPERTIES = 279,
    KeyboardKeys::None, // SDL_SCANCODE_AC_SEARCH = 280,
    KeyboardKeys::None, // SDL_SCANCODE_AC_HOME = 281,
    KeyboardKeys::None, // SDL_SCANCODE_AC_BACK = 282,
    KeyboardKeys::None, // SDL_SCANCODE_AC_FORWARD = 283,
    KeyboardKeys::None, // SDL_SCANCODE_AC_STOP = 284,
    KeyboardKeys::None, // SDL_SCANCODE_AC_REFRESH = 285,
    KeyboardKeys::None, // SDL_SCANCODE_AC_BOOKMARKS = 286,
    KeyboardKeys::None, // SDL_SCANCODE_SOFTLEFT = 287,
    KeyboardKeys::None, // SDL_SCANCODE_SOFTRIGHT = 288,
    KeyboardKeys::None, // SDL_SCANCODE_CALL = 289,
    KeyboardKeys::None, // SDL_SCANCODE_ENDCALL = 290
};

/// <summary>
/// Implementation of the keyboard device for Windows platform.
/// </summary>
/// <seealso cref="Keyboard" />
class SDLKeyboard : public Keyboard
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SDLKeyboard"/> class.
    /// </summary>
    explicit SDLKeyboard()
        : Keyboard()
    {
    }

public:

};

/// <summary>
/// Implementation of the mouse device for SDL platform.
/// </summary>
/// <seealso cref="Mouse" />
class SDLMouse : public Mouse
{
private:
    Float2 oldPosition;
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SDLMouse"/> class.
    /// </summary>
    explicit SDLMouse()
        : Mouse()
    {
    }

public:

    Float2 GetMousePosition() const
    {
        return oldPosition;
    }

    // [Mouse]
    void SetMousePosition(const Float2& newPosition) final override
    {
        SDL_WarpMouseGlobal(newPosition.X, newPosition.Y);
        
        OnMouseMoved(newPosition);
    }

    void SetRelativeMode(bool relativeMode) final override
    {
        if (relativeMode == _relativeMode)
            return;

        if (relativeMode)
            SDL_GetGlobalMouseState(&oldPosition.X, &oldPosition.Y);

        Mouse::SetRelativeMode(relativeMode);
        if (SDL_SetRelativeMouseMode(relativeMode ? SDL_TRUE : SDL_FALSE) != 0)
            LOG(Error, "Failed to set mouse relative mode: {0}", String(SDL_GetError()));

        if (!relativeMode)
        {
            SDL_WarpMouseGlobal(oldPosition.X, oldPosition.Y);
            OnMouseMoved(oldPosition);
        }
    }
};

/// <summary>
/// Implementation of the gamepad device for SDL platform.
/// </summary>
/// <seealso cref="Gamepad" />
class SDLGamepad : public Gamepad
{
private:

    SDL_Gamepad* _gamepad;
    SDL_JoystickID _instanceId;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SDLGamepad"/> class.
    /// </summary>
    /// <param name="userIndex">The joystick.</param>
    explicit SDLGamepad(SDL_JoystickID instanceId);

    /// <summary>
    /// Finalizes an instance of the <see cref="SDLGamepad"/> class.
    /// </summary>
    ~SDLGamepad();

private:

    SDLGamepad(SDL_Gamepad* gamepad, SDL_JoystickID instanceId);

public:

    static SDLGamepad* GetGamepadById(SDL_JoystickID id)
    {
        SDLGamepad* gamepad = nullptr;
        SDLInputImpl::Gamepads.TryGet(id, gamepad);
        return gamepad;
    }

    SDL_JoystickID GetJoystickInstanceId() const
    {
        return _instanceId;
    }

    void OnAxisMotion(SDL_GamepadAxis axis, int16 value);

    void OnButtonState(SDL_GamepadButton axis, uint8 state);

    // [Gamepad]
    void SetVibration(const GamepadVibrationState& state) override;

protected:

    // [Gamepad]
    bool UpdateState() override;
};

void SDLInput::Init()
{
    Input::Mouse = SDLInputImpl::Mouse = New<SDLMouse>();
    Input::Keyboard = SDLInputImpl::Keyboard = New<SDLKeyboard>();
}

void SDLInput::Update()
{
}

float NormalizeAxisValue(const int16 axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    const float norm = axisVal <= 0 ? 32768.0f : 32767.0f;
    return float(axisVal) / norm;
}

bool SDLInput::HandleEvent(SDLWindow* window, SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION:
    {
        if (Input::Mouse->IsRelative())
        {
            const Float2 mouseDelta(event.motion.xrel, event.motion.yrel);
            Input::Mouse->OnMouseMoveRelative(mouseDelta, window);
        }
        else
        {
            const Float2 mousePos = window->ClientToScreen({ event.motion.x, event.motion.y });
            Input::Mouse->OnMouseMove(mousePos, window);
        }
        return true;
    }
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    {
        Input::Mouse->OnMouseLeave(window);
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        Float2 mousePos = window->ClientToScreen({ event.button.x, event.button.y });
        MouseButton button = MouseButton::None;
        if (event.button.button == SDL_BUTTON_LEFT)
            button = MouseButton::Left;
        else if (event.button.button == SDL_BUTTON_RIGHT)
            button = MouseButton::Right;
        else if (event.button.button == SDL_BUTTON_MIDDLE)
            button = MouseButton::Middle;
        else if (event.button.button == SDL_BUTTON_X1)
            button = MouseButton::Extended1;
        else if (event.button.button == SDL_BUTTON_X2)
            button = MouseButton::Extended2;

        if (Input::Mouse->IsRelative())
        {
            // Use the previous visible mouse position here, the event or global
            // mouse position would cause input to trigger in other editor windows.
            mousePos = SDLInputImpl::Mouse->GetMousePosition();
        }

        if (event.button.state == SDL_RELEASED)
            Input::Mouse->OnMouseUp(mousePos, button, window);
        // Prevent sending multiple mouse down event when double-clicking UI elements
        else if (event.button.clicks % 2 == 1)
            Input::Mouse->OnMouseDown(mousePos, button, window);
        else
            Input::Mouse->OnMouseDoubleClick(mousePos, button, window);

        return true;
    }
    case SDL_EVENT_MOUSE_WHEEL:
    {
        Float2 mousePos = window->ClientToScreen({ event.wheel.mouse_x, event.wheel.mouse_y });
        const float delta = event.wheel.y;

        if (Input::Mouse->IsRelative())
        {
            // Use the previous visible mouse position here, the event or global
            // mouse position would cause input to trigger in other editor windows.
            mousePos = SDLInputImpl::Mouse->GetMousePosition();
        }

        Input::Mouse->OnMouseWheel(mousePos, delta, window);
        return true;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        // TODO: scancode support
        KeyboardKeys key = SDL_TO_FLAX_KEYS_MAP[event.key.scancode];
        //event.key.mod
        if (event.key.state == SDL_RELEASED)
            Input::Keyboard->OnKeyUp(key, window);
        else
            Input::Keyboard->OnKeyDown(key, window);
        return true;
    }
    case SDL_EVENT_TEXT_EDITING:
    {
        auto edit = event.edit;
        return true;
    }
    case SDL_EVENT_TEXT_INPUT:
    {
        String text(event.text.text);
        for (int i = 0; i < text.Length(); i++)
        {
            Input::Keyboard->OnCharInput(text[i], window);
        }
        return true;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    {
        SDLGamepad* gamepad = SDLGamepad::GetGamepadById(event.gaxis.which);
        SDL_GamepadAxis axis = (SDL_GamepadAxis)event.gaxis.axis;
        gamepad->OnAxisMotion(axis, event.gaxis.value);

        LOG(Info, "SDL_EVENT_GAMEPAD_AXIS_MOTION");
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    {
        SDLGamepad* gamepad = SDLGamepad::GetGamepadById(event.gbutton.which);
        SDL_GamepadButton button = (SDL_GamepadButton)event.gbutton.button;
        gamepad->OnButtonState(button, event.gbutton.state);

        LOG(Info, "SDL_EVENT_GAMEPAD_BUTTON_");
        break;
    }
    case SDL_EVENT_GAMEPAD_ADDED:
    {
        Input::Gamepads.Add(New<SDLGamepad>(event.gdevice.which));
        Input::OnGamepadsChanged();

        LOG(Info, "SDL_EVENT_GAMEPAD_ADDED");
        break;
    }
    case SDL_EVENT_GAMEPAD_REMOVED:
    {
        for (int i = 0; i < Input::Gamepads.Count(); i++)
        {
            SDLGamepad* gamepad = static_cast<SDLGamepad*>(Input::Gamepads[i]);
            if (gamepad->GetJoystickInstanceId() == event.gdevice.which)
            {
                Input::Gamepads[i]->DeleteObject();
                Input::Gamepads.RemoveAtKeepOrder(i);
                Input::OnGamepadsChanged();
                break;
            }
        }
        
        LOG(Info, "SDL_EVENT_GAMEPAD_REMOVED");
        break;
    }
    case SDL_EVENT_GAMEPAD_REMAPPED:
    {
        auto ev = event.gdevice;
        LOG(Info, "SDL_EVENT_GAMEPAD_REMAPPED");
        break;
    }
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    {
        LOG(Info, "SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN");
        break;
    }
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
    {
        LOG(Info, "SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION");
        break;
    }
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    {
        LOG(Info, "SDL_EVENT_GAMEPAD_TOUCHPAD_UP");
        break;
    }
    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
    {
        LOG(Info, "SDL_EVENT_GAMEPAD_SENSOR_UPDATE");
        break;
    }
    case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
    {
        auto ev = event.gdevice;
        LOG(Info, "SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED");
        break;
    }
    }
    return false;
}

Guid GetGamepadGuid(SDL_JoystickID instanceId)
{
    SDL_GUID joystickGuid = SDL_GetGamepadGUIDForID(instanceId);
    Guid guid;
    Platform::MemoryCopy(&guid.Raw, joystickGuid.data, sizeof(uint8) * 16);
    return guid;
}

SDLGamepad::SDLGamepad(SDL_JoystickID instanceId)
    : SDLGamepad(SDL_OpenGamepad(instanceId), instanceId)
{
}

SDLGamepad::SDLGamepad(SDL_Gamepad* gamepad, SDL_JoystickID instanceId)
    : Gamepad(GetGamepadGuid(instanceId), String(SDL_GetGamepadName(gamepad)))
    , _gamepad(gamepad)
    , _instanceId(instanceId)
{
    SDLInputImpl::Gamepads.Add(_instanceId, this);
}

SDLGamepad::~SDLGamepad()
{
    SDL_CloseGamepad(_gamepad);
    SDLInputImpl::Gamepads.Remove(_instanceId);
}

void SDLGamepad::SetVibration(const GamepadVibrationState& state)
{
    Gamepad::SetVibration(state);
}

bool SDLGamepad::UpdateState()
{
    return false;
}

void SDLGamepad::OnAxisMotion(SDL_GamepadAxis sdlAxis, int16 value)
{
    GamepadAxis axis;
    int16 deadzone = 1; // SDL reports -1 for centered axis?
    float valueNormalized = NormalizeAxisValue(value);
    switch (sdlAxis)
    {
    case SDL_GAMEPAD_AXIS_LEFTX:
        axis = GamepadAxis::LeftStickX;
        deadzone = LEFT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::LeftStickLeft] = value > LEFT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::LeftStickRight] = value < -LEFT_STICK_THRESHOLD;
        break;
    case SDL_GAMEPAD_AXIS_LEFTY:
        axis = GamepadAxis::LeftStickY;
        deadzone = LEFT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::LeftStickUp] = value < -LEFT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::LeftStickDown] = value > LEFT_STICK_THRESHOLD;
        valueNormalized = -valueNormalized;
        break;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        deadzone = RIGHT_STICK_THRESHOLD;
        axis = GamepadAxis::RightStickX;
        _state.Buttons[(int32)GamepadButton::RightStickLeft] = value > RIGHT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::RightStickRight] = value < -RIGHT_STICK_THRESHOLD;
        break;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        deadzone = RIGHT_STICK_THRESHOLD;
        axis = GamepadAxis::RightStickY;
        _state.Buttons[(int32)GamepadButton::RightStickUp] = value < -RIGHT_STICK_THRESHOLD;
        _state.Buttons[(int32)GamepadButton::RightStickDown] = value > RIGHT_STICK_THRESHOLD;
        valueNormalized = -valueNormalized;
        break;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        deadzone = TRIGGER_THRESHOLD;
        axis = GamepadAxis::LeftTrigger;
        _state.Buttons[(int32)GamepadButton::LeftTrigger] = value > TRIGGER_THRESHOLD;
        break;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        deadzone = TRIGGER_THRESHOLD;
        axis = GamepadAxis::RightTrigger;
        _state.Buttons[(int32)GamepadButton::RightTrigger] = value > TRIGGER_THRESHOLD;
        break;
    default:
        return;
    }
    if (value <= deadzone && value >= -deadzone)
        valueNormalized = 0.0f;
    _state.Axis[(int32)axis] = valueNormalized;
}

void SDLGamepad::OnButtonState(SDL_GamepadButton sdlButton, uint8 state)
{
    switch (sdlButton)
    {
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH:
        _state.Buttons[(int32)GamepadButton::A] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_EAST:
        _state.Buttons[(int32)GamepadButton::B] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_WEST:
        _state.Buttons[(int32)GamepadButton::X] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_NORTH:
        _state.Buttons[(int32)GamepadButton::Y] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        _state.Buttons[(int32)GamepadButton::LeftShoulder] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        _state.Buttons[(int32)GamepadButton::RightShoulder] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_BACK:
        _state.Buttons[(int32)GamepadButton::Back] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_START:
        _state.Buttons[(int32)GamepadButton::Start] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_STICK:
        _state.Buttons[(int32)GamepadButton::LeftThumb] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        _state.Buttons[(int32)GamepadButton::RightThumb] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_UP:
        _state.Buttons[(int32)GamepadButton::DPadUp] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        _state.Buttons[(int32)GamepadButton::DPadDown] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        _state.Buttons[(int32)GamepadButton::DPadLeft] = state == SDL_PRESSED;
        break;
    case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        _state.Buttons[(int32)GamepadButton::DPadRight] = state == SDL_PRESSED;
        break;
    }
}

#endif
