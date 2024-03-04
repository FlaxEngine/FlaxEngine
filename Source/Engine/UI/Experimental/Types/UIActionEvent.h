//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Input/Enums.h"
#include "Engine/Input/KeyboardKeys.h"

/// <summary>
/// UI action event's: keyboard, gamepay buttons, etc.
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI") 
struct FLAXENGINE_API UIActionEvent
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(UIActionEvent)
public:
    /// <summary>
    /// The value
    /// <para> Note:</para>
    /// This is for action keys but action keys are not suported at this time
    /// </summary>
    API_FIELD() float Value = 0;

    /// <summary>
    /// curent key
    /// </summary>
    API_FIELD() KeyboardKeys Key;
    /// <summary>
    /// curent gamepad button
    /// </summary>
    API_FIELD() GamepadButton Button;

    /// <summary>
    /// The input action state
    /// </summary>
    API_FIELD() InputActionState State;

    /// <summary>
    /// The character input
    /// <para> Note:</para>
    /// Sometimes can contain more than 1 character. 
    /// </summary>
    API_FIELD() StringView Chars;

    //Helpers funcions
    template<typename T>
    FORCE_INLINE bool IsKeyDown(T value) { return (value == Key && State == InputActionState::Press); }
    template<typename T>
    FORCE_INLINE bool IsKey(T value) { return (value == Key && State == InputActionState::Pressing); }
    template<typename T>
    FORCE_INLINE bool IsKeyUp(T value) { return (value == Key && State == InputActionState::Release); }
    template<typename T>
    FORCE_INLINE bool IsButtonDown(T value) { return (value == Button && State == InputActionState::Press); }
    template<typename T>
    FORCE_INLINE bool IsButton(T value) { return (value == Button && State == InputActionState::Pressing); }
    template<typename T>
    FORCE_INLINE bool IsButtonUp(T value) { return (value == Button && State == InputActionState::Release); }
};
