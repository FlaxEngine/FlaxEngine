#include "InputAxis.h"

InputAxis::InputAxis(const SpawnParams& params)
    : ScriptingObject(params)
{
    Input::AxisValueChanged.Bind<InputAxis, &InputAxis::Handler>(this);
}

InputAxis::InputAxis(String name)
    : ScriptingObject(SpawnParams(Guid::New(), GetTypeHandle()))
{
    Input::AxisValueChanged.Bind<InputAxis, &InputAxis::Handler>(this);
    Name = name;
}

InputAxis::~InputAxis()
{
    Input::AxisValueChanged.Unbind<InputAxis, &InputAxis::Handler>(this);
    ValueChanged.UnbindAll();
}

void InputAxis::Dispose()
{
    Input::AxisValueChanged.Unbind<InputAxis, &InputAxis::Handler>(this);
    ValueChanged.UnbindAll();
}

void InputAxis::Handler(StringView name)
{
    if (name.Compare(StringView(Name), StringSearchCase::IgnoreCase) == 0)
        ValueChanged();
}
