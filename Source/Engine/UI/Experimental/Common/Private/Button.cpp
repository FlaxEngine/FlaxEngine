#include "../Button.h"
#include "Engine/Render2D/Render2D.h"

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"

UIButton::UIButton(const SpawnParams& params) : UIComponent(params)
{
    ButtonState = None;
}
void UIButton::OnDraw()
{
    switch (ButtonState)
    {
    case UIButton::Hover:
        Render2D::FillRectangle(GetRect(), Color::Gray);
        break;
    case UIButton::Press:
    case UIButton::Pressing:
        Render2D::FillRectangle(GetRect(), Color::DarkGray);
        break;
    default:
        Render2D::FillRectangle(GetRect(), Color::White);
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
    if (Response != UIEventResponse::Focus) 
    {
        SERIALIZE_GET_OTHER_OBJ(UIButton);
        SERIALIZE(Response);
    }
}

void UIButton::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIComponent::Deserialize(stream, modifier);
    DESERIALIZE(Response);
}
