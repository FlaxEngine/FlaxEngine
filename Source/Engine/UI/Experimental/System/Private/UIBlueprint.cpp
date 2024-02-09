//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../UIBlueprint.h"
#include "Engine/Scripting/Scripting.h"

void SendEventRecursive(UIComponent* InFromUIComponent, const UIPointerEvent& InEvent, const UIComponent*& OutHit, UIEventResponse& OutEventResponse)
{
    if (InFromUIComponent->GetVisibility() == UIComponentVisibility::Collapsed)
    {
        OutHit = nullptr;
        OutEventResponse = UIEventResponse::None;
        return;
    }
    if (!EnumHasAnyFlags(InFromUIComponent->GetVisibility(), UIComponentVisibility::IgnoreRaycastSelf))
    {
        auto eventResponse = InFromUIComponent->OnPointerInput(InEvent);
        if (eventResponse != UIEventResponse::None)
        {
            OutHit = InFromUIComponent;
            OutEventResponse = eventResponse;
            return;
        }
    }
    if (!EnumHasAnyFlags(InFromUIComponent->GetVisibility(), UIComponentVisibility::IgnoreRaycastChildren))
    {
        if (auto uipc = InFromUIComponent->Cast<UIPanelComponent>())
        {
            for (auto slot : uipc->GetSlots())
            {
                SendEventRecursive(slot->Content, InEvent, OutHit, OutEventResponse);
                if (OutEventResponse != UIEventResponse::None)
                {
                    return;
                }
            }
        }
    }
    OutHit = nullptr;
    OutEventResponse = UIEventResponse::None;
    return;
}
UIEventResponse UIBlueprint::SendEvent(const UIPointerEvent& InEvent, const UIComponent*& OutHit)
{
    UIEventResponse OutEventResponse = UIEventResponse::None;
    SendEventRecursive(Component, InEvent, OutHit, OutEventResponse);
    return OutEventResponse;
}

UIBlueprint::UIBlueprint(const SpawnParams& params) : UIComponent(params)
{
    IsReady = false;
    Variables = Array<Variable>();
}

void UIBlueprint::OnInitialized()
{
}
void UIBlueprint::PreConstruct(bool IsDesignTime) {}
void UIBlueprint::Construct() {}
void UIBlueprint::Tick(float DeltaTime) {}
void UIBlueprint::Destruct() {}
