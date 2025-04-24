#pragma once
#include "Input.h"

#include "Engine/Core/ISerializable.h"

API_CLASS() class FLAXENGINE_API InputAxis : public ScriptingObject, public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(InputAxis);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="InputEvent"/> class.
    /// </summary>
    /// <param name="name">The action name.</param>
    InputAxis(String name);

    ~InputAxis();

    /// <summary>
    /// The name of the axis to use. See <see cref="Input.AxisMappings"/>.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// Gets the current axis value.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetValue() const { return Input::GetAxis(Name); }

    /// <summary>
    /// Gets the current axis raw value.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetValueRaw() const { return Input::GetAxisRaw(Name); }

    /// <summary>
    /// Occurs when axis is changed. Called before scripts update.
    /// </summary>
    API_EVENT() Action ValueChanged;

    /// <summary>
    /// Dispose of this object.
    /// </summary>
    API_FUNCTION() void Dispose();

private:
    void Handler(StringView name);
};
