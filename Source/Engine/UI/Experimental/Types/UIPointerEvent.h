// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Input/Enums.h"
#include "Engine/Input/KeyboardKeys.h"

/// <summary>
/// UI pointer event's: mouse, touch, stylus, gamepad emulated mouse, etc.
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API UIPointerEvent
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(UIPointerEvent)
public:

    /// <summary>
    /// The value
    /// </summary>
    API_FIELD() Float2 Value;

    /// <summary>
    /// The state
    /// </summary>
    API_FIELD() InputActionState State;

    /// <summary>
    /// The mouse button state
    /// </summary>
    API_FIELD() MouseButton MouseButtonState;

    /// <summary>
    /// The gamepad axis state
    /// </summary>
    API_FIELD() GamepadAxis GamepadAxisState;

    /// <summary>
    /// The is touch
    /// </summary>
    API_FIELD() bool IsTouch;

    //templates
    FORCE_INLINE bool IsPressd() { return State == InputActionState::Press; }
    FORCE_INLINE bool IsPressing() { return State == InputActionState::Pressing; }
    FORCE_INLINE bool IsReleased() { return State == InputActionState::Release; }
};
