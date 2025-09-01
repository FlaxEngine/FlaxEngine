#include "InputEvent.h"

InputEvent::InputEvent(const SpawnParams& params)
    : ScriptingObject(params)
{
    Input::ActionTriggered.Bind<InputEvent, &InputEvent::Handler>(this);
}

InputEvent::InputEvent(String name)
    : ScriptingObject(SpawnParams(Guid::New(), GetTypeHandle()))
{
    Input::ActionTriggered.Bind<InputEvent, &InputEvent::Handler>(this);
    Name = name;
}

InputEvent::~InputEvent()
{
    Input::ActionTriggered.Unbind<InputEvent, &InputEvent::Handler>(this);
    Pressed.UnbindAll();
    Pressing.UnbindAll();
    Released.UnbindAll();
}

void InputEvent::Dispose()
{
    Input::ActionTriggered.Unbind<InputEvent, &InputEvent::Handler>(this);
    Pressed.UnbindAll();
    Pressing.UnbindAll();
    Released.UnbindAll();
}

void InputEvent::Handler(StringView name, InputActionState state)
{
    if (name.Compare(StringView(Name), StringSearchCase::IgnoreCase) != 0)
        return;
    switch (state)
    {
    case InputActionState::None: break;
    case InputActionState::Waiting: break;
    case InputActionState::Pressing:
        Pressing();
        break;
    case InputActionState::Press:
        Pressed();
        break;
    case InputActionState::Release:
        Released();
        break;
    default: break;
    }
}
