#pragma once
#include "Input.h"

#include "Engine/Core/ISerializable.h"

API_CLASS(Attributes="CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.InputEventEditor\")") class FLAXENGINE_API InputEvent : public ScriptingObject, public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(InputEvent);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="InputEvent"/> class.
    /// </summary>
    /// <param name="name">The action name.</param>
    InputEvent(String name);

    ~InputEvent();

    /// <summary>
    /// The name of the action to use. See <see cref="Input.ActionMappings"/>.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// Returns true if the event has been triggered during the current frame (e.g. user pressed a key). Use <see cref="Pressed"/> to catch events without active waiting.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool GetActive() const { return Input::GetAction(Name); }

    /// <summary>
    /// Returns the event state. Use <see cref="Pressed"/>, <see cref="Pressing"/>, <see cref="Released"/> to catch events without active waiting.
    /// </summary>
    API_PROPERTY() FORCE_INLINE InputActionState GetState() const { return Input::GetActionState(Name); }

    /// <summary>
    /// Occurs when event is pressed (e.g. user pressed a key). Called before scripts update.
    /// </summary>
    API_EVENT() Action Pressed;

    /// <summary>
    /// Occurs when event is being pressing (e.g. user pressing a key). Called before scripts update.
    /// </summary>
    API_EVENT() Action Pressing;

    /// <summary>
    /// Occurs when event is released (e.g. user releases a key). Called before scripts update.
    /// </summary>
    API_EVENT() Action Released;

    /// <summary>
    /// Dispose of this object.
    /// </summary>
    API_FUNCTION() void Dispose();
    
private:
    void Handler(StringView name, InputActionState state);
};
