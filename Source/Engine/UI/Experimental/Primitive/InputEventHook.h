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
class UIInputEventHook : public UIComponent
{
    DECLARE_SCRIPTING_TYPE(UIInputEventHook);
public:

    /// <summary>
    /// Called when input has event's: mouse, touch, stylus, gamepad emulated mouse, etc. and value has changed
    /// </summary>
    Function<UIEventResponse(const UIPointerEvent&)> PointerInput;

    /// <summary>
    /// Called when input has event's: keyboard, gamepay buttons, etc. and value has changed
    /// Note: keyboard can have action keys where value is from 0 to 1
    /// </summary>
    Function<UIEventResponse(const UIActionEvent&)> ActionInput;
private:

    virtual UIEventResponse OnPointerInput(const UIPointerEvent& InEvent) override;
    virtual UIEventResponse OnActionInput(const UIActionEvent& InEvent) override;

    // Todo : The Function cant generete proper bindings This is a HACK to get it working on c# side remove stuff from CSHACK regions when Function<> can be directy Accesed by c# side
    // and add to PointerInput,ActionInput API_FIELD() tag.
    
#pragma region CSHACK
    UIEventResponse CSHACK_ActionInputUIEventResponse;
    UIEventResponse CSHACK_PointerInputUIEventResponse;
    API_EVENT(internal) Delegate<const UIActionEvent&> CSHACK_ActionInput;
    API_EVENT(internal) Delegate<const UIPointerEvent&> CSHACK_PointerInput;
    API_FUNCTION(internal) void CSHACK_SetActionInputUIEventResponse(const UIEventResponse& InEventResponse);
    API_FUNCTION(internal) void CSHACK_SetPointerInputUIEventResponse(const UIEventResponse& InEventResponse);
    API_FUNCTION(internal) void CSHACK_BindActionInput();
    API_FUNCTION(internal) void CSHACK_BindPointerInput();
    API_FUNCTION(internal) void CSHACK_UnBindActionInput();
    API_FUNCTION(internal) void CSHACK_UnBindPointerInput();
    API_FUNCTION(internal) bool CSHACK_IsBindedActionInput();
    API_FUNCTION(internal) bool CSHACK_IsBindedPointerInput();
    UIEventResponse CSHACK_InvokePointerInput(const UIPointerEvent& InEvent);
    UIEventResponse CSHACK_InvokeActionInput(const UIActionEvent& InEvent);
#pragma endregion
};
