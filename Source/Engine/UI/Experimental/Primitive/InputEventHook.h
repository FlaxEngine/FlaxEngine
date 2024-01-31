//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Core/Math/Vector2.h"
#include "../Types/UIComponent.h"
#include "../Common/Button.h"


/// <summary>
/// this UI Component allows to hook in to UI input events
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"Input Event Hook\",CategoryName=\"Primitive\",EditorComponent=false,HiddenInDesigner=false)")
class InputEventHook : public UIComponent
{
    DECLARE_SCRIPTING_TYPE(InputEventHook);
public:
    /// <summary>
    /// Called when input has event's: mouse, touch, stylus, gamepad emulated mouse, etc. and value has changed
    /// </summary>
    API_FIELD() Function<UIEventResponse(const UIPointerEvent&)> PointerInput;
    /// <summary>
    /// Called when input has event's: keyboard, gamepay buttons, etc. and value has changed
    /// Note: keyboard can have action keys where value is from 0 to 1
    /// </summary>
    API_FIELD() Function<UIEventResponse(const UIActionEvent&)> ActionInput;

private:
    virtual UIEventResponse OnPointerInput(const UIPointerEvent& InEvent) override;
    virtual UIEventResponse OnActionInput(const UIActionEvent& InEvent) override;
};
