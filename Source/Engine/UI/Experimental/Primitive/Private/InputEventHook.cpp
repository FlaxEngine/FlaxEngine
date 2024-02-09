//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../InputEventHook.h"
#include "../../../../Debug/DebugLog.h"
#include <Engine/Scripting/Internal/InternalCalls.h>
UIInputEventHook::UIInputEventHook(const SpawnParams& params) : UIComponent(params){}

UIEventResponse UIInputEventHook::OnPointerInput(const UIPointerEvent& InEvent)
{
    if (PointerInput.IsBinded())
    {
        PointerInput(InEvent);
    }
    
    return UIEventResponse::None;
}

UIEventResponse UIInputEventHook::OnActionInput(const UIActionEvent& InEvent)
{
    if (ActionInput.IsBinded())
    {
        ActionInput(InEvent);
    }
    return UIEventResponse::None;
}

 void UIInputEventHook::CSHACK_SetActionInputUIEventResponse(const UIEventResponse& InEventResponse)
{
    CSHACK_ActionInputUIEventResponse = InEventResponse;
}

 void UIInputEventHook::CSHACK_SetPointerInputUIEventResponse(const UIEventResponse& InEventResponse)
{
    CSHACK_PointerInputUIEventResponse = InEventResponse;
}

void UIInputEventHook::CSHACK_BindActionInput()
{
    if (ActionInput.IsBinded())
        ActionInput.Unbind();
    ActionInput.Bind([this](const UIActionEvent& InEvent) {return CSHACK_InvokeActionInput(InEvent); });
}

void UIInputEventHook::CSHACK_BindPointerInput()
{
    if (PointerInput.IsBinded())
        PointerInput.Unbind();
    PointerInput.Bind([this](const UIPointerEvent& InEvent) {return CSHACK_InvokePointerInput(InEvent); });
}

void UIInputEventHook::CSHACK_UnBindActionInput()
{
    if (ActionInput.IsBinded())
        ActionInput.Unbind();
}

void UIInputEventHook::CSHACK_UnBindPointerInput()
{
    if (PointerInput.IsBinded())
        PointerInput.Unbind();
}

bool UIInputEventHook::CSHACK_IsBindedActionInput()
{
    return ActionInput.IsBinded();
}

bool UIInputEventHook::CSHACK_IsBindedPointerInput()
{
    return PointerInput.IsBinded();
}

UIEventResponse UIInputEventHook::CSHACK_InvokePointerInput(const UIPointerEvent& InEvent)
{
    CSHACK_PointerInput(InEvent);
    return CSHACK_PointerInputUIEventResponse;
}

UIEventResponse UIInputEventHook::CSHACK_InvokeActionInput(const UIActionEvent& InEvent)
{
    CSHACK_ActionInput(InEvent);
    return CSHACK_ActionInputUIEventResponse;
}
