//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../InputEventHook.h"
#include "../../../../Debug/DebugLog.h"
#include <Engine/Scripting/Internal/InternalCalls.h>
InputEventHook::InputEventHook(const SpawnParams& params) : UIComponent(params){}

UIEventResponse InputEventHook::OnPointerInput(const UIPointerEvent& InEvent)
{
    DebugLog::Log(String("HI from OnPointerInput"));
    if (PointerInput.IsBinded())
    {
        PointerInput(InEvent);
    }
    return UIEventResponse::None;
}

UIEventResponse InputEventHook::OnActionInput(const UIActionEvent& InEvent)
{
    if (ActionInput.IsBinded())
    {
        ActionInput(InEvent);
    }
    return UIEventResponse::None;
}
