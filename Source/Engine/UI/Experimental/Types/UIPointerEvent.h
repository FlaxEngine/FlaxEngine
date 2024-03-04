//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Input/Enums.h"
#include "Engine/Input/KeyboardKeys.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// UI pointer event's: mouse, touch, stylus, gamepad emulated mouse, etc.
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API UIPointerEvent
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(UIPointerEvent)
public:

    /// <summary>
    /// The locations it can be 1 or more if using user is on mobile phone
    /// </summary>
    API_FIELD() Array<Float2> Locations;

    /// <summary>
    /// The imput state
    /// </summary>
    API_FIELD() InputActionState State;

    /// <summary>
    /// The mouse button state
    /// </summary>
    API_FIELD() MouseButton MouseButton;

    /// <summary>
    /// The gamepad axis state
    /// </summary>
    API_FIELD() GamepadAxis GamepadAxis;

    /// <summary>
    /// The is touch
    /// </summary>
    API_FIELD() bool IsTouch;

    //templates
    FORCE_INLINE bool IsPressd() { return State == InputActionState::Press; }
    FORCE_INLINE bool IsPressing() { return State == InputActionState::Pressing; }
    FORCE_INLINE bool IsReleased() { return State == InputActionState::Release; }
};
