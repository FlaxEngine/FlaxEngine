#include "../Button.h"
#include "Engine/Render2D/Render2D.h"

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"

UIButton::UIButton(const SpawnParams& params) : UIComponent(params)
{
    ButtonState = None;
    BrushHover = nullptr;
    BrushPressed = nullptr;
    BrushNormal = nullptr;
}
void UIButton::OnDraw()
{
    switch (ButtonState)
    {
    case UIButton::State::Hover:
        if (BrushHover)
            BrushHover->Draw(GetRect());
        break;
    case UIButton::State::Press:
    case UIButton::State::Pressing:
        if (BrushPressed)
            BrushPressed->Draw(GetRect());
        break;
    default:
        if (BrushNormal)
            BrushNormal->Draw(GetRect());
        break;
    }
}

UIEventResponse UIButton::OnPointerInput(const UIPointerEvent& InEvent) 
{
    bool GotAnnyLocationInside = false;
    for (auto location : InEvent.Locations)
    {
        if (Contains(location))
        {
            GotAnnyLocationInside = true;
            break;
        }
    }
    if(GotAnnyLocationInside)
    {
        //UI button has maping 1 to 1 with InputActionState
        SetState((UIButton::State)InEvent.State);
        return Response;
    }
    else
    {
        SetState(None);
    }

    return UIEventResponse::None;
}

void UIButton::SetState(State InNewState)
{
    if (InNewState != ButtonState)
    {
        ButtonState = InNewState;
        if (StateChanged.IsBinded())
        {
            StateChanged(this,ButtonState);
        }
    }
}

void UIButton::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIComponent::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UIButton);
    if (Response != UIEventResponse::Focus) 
    {
        SERIALIZE(Response);
    }
    SERIALIZE(BrushNormal);
    SERIALIZE(BrushHover);
    SERIALIZE(BrushPressed);
}

void UIButton::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIComponent::Deserialize(stream, modifier);
    DESERIALIZE(Response);
    DESERIALIZE(BrushNormal);
    DESERIALIZE(BrushHover);
    DESERIALIZE(BrushPressed);
}

void UIButton::OnDeleteObject()
{
    UIComponent::OnDeleteObject();
}
