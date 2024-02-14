//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../Background Blur.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Serialization/Serialization.h"
#include "../../System/UISerializationConfig.h"

UIBackgroundBlurSlot::UIBackgroundBlurSlot(const SpawnParams& params) : UIPanelSlot(params)
{
    VerticalAlignment = UIVerticalAlignment::Fill;
    HorizontalAlignment = UIHorizontalAlignment::Fill;
    Padding = UIMargin();
}

const UIMargin& UIBackgroundBlurSlot::GetPadding() { return Padding; }
UIHorizontalAlignment UIBackgroundBlurSlot::GetHorizontalAlignment() { return HorizontalAlignment; }
UIVerticalAlignment UIBackgroundBlurSlot::GetVerticalAlignment() { return VerticalAlignment; }

void UIBackgroundBlurSlot::Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt, const Rectangle& InNewParentBounds)
{
    Rectangle bounds = InNewBounds;
    auto contentDesiredSize = Content->ComputeDesiredSize();
    switch (VerticalAlignment)
    {
    case UIVerticalAlignment::Fill:
        bounds.SetTop(InNewParentBounds.GetTop());
        bounds.SetBottom(InNewParentBounds.GetBottom());
        break;
    case UIVerticalAlignment::Top:
        bounds.SetTop(InNewParentBounds.GetTop());
        bounds.Size.Y = bounds.Size.Y;
        break;
    case UIVerticalAlignment::Center:
        bounds.Size.Y = bounds.Size.Y;
        bounds.SetCenterY(InNewParentBounds.GetCenter());
        break;
    case UIVerticalAlignment::Bottom:
        bounds.SetBottom(InNewParentBounds.GetBottom());
        bounds.SetTop(InNewParentBounds.GetBottom() - bounds.Size.Y);
        break;
    }
    switch (HorizontalAlignment)
    {
    case UIHorizontalAlignment::Fill:
        bounds.SetLeft(InNewParentBounds.GetLeft());
        bounds.SetRight(InNewParentBounds.GetRight());
        break;
    case UIHorizontalAlignment::Left:
        bounds.SetLeft(InNewParentBounds.GetLeft());
        bounds.Size.X = bounds.Size.X;
        break;
    case UIHorizontalAlignment::Center:
        bounds.Size.X = bounds.Size.X;
        bounds.SetCenterX(InNewParentBounds.GetCenter());
        break;
    case UIHorizontalAlignment::Right:
        bounds.SetLeft(InNewParentBounds.GetLeft());
        bounds.SetRight(InNewParentBounds.GetRight() - bounds.Size.Y);
        break;
    }

    bounds.SetBottom(bounds.GetBottom() - Padding.Bottom);
    bounds.SetTop(bounds.GetTop() + Padding.Top);
    bounds.SetRight(bounds.GetRight() - Padding.Right);
    bounds.SetLeft(bounds.GetLeft() + Padding.Left);

    UIPanelSlot::Applay(bounds, InNewPoivt);
}
void UIBackgroundBlurSlot::SetPadding(const UIMargin& InPadding)
{
    if (Padding != InPadding)
        Content->InvalidateLayout();
    Padding = InPadding;
}
void UIBackgroundBlurSlot::SetHorizontalAlignment(UIHorizontalAlignment InHorizontalAlignment)
{
    if (HorizontalAlignment != InHorizontalAlignment)
        Content->InvalidateLayout();
    HorizontalAlignment = InHorizontalAlignment;
}
void UIBackgroundBlurSlot::SetVerticalAlignment(UIVerticalAlignment InVerticalAlignment)
{
    if (VerticalAlignment != InVerticalAlignment)
        Content->InvalidateLayout();
    VerticalAlignment = InVerticalAlignment;
}

void UIBackgroundBlurSlot::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(UIBackgroundBlurSlot);
    //pack Margin to array
    float floats[]
    {
        Padding.Left,
        Padding.Right,
        Padding.Top,
        Padding.Bottom
    };
    stream.JKEY("Margin");
    stream.StartArray();
    for (auto i = 0; i < 4; i++)
    {
        stream.Float(floats[i]);
    }
    stream.EndArray();
#if UI_USE_COMPACT_SERIALIZATION
    stream.Blob(&floats[0], sizeof(floats));
#else
    stream.JKEY("Margin");
    stream.StartArray();
    for (auto i = 0; i < 4; i++)
    {
        stream.Float(floats[i]);
    }
    stream.EndArray();
#endif

    SERIALIZE(VerticalAlignment);
    SERIALIZE(HorizontalAlignment);
}
void UIBackgroundBlurSlot::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    const auto transform = stream.FindMember(rapidjson_flax::Value("Margin", (sizeof("Margin") / sizeof("Margin"[0])) - 1));
    if (transform != stream.MemberEnd())
    {
        //custom unpacking for transform
        float floats[4] = { 0,0,0,0 };
        if (transform->value.IsString())
        {
            Encryption::Base64Decode(transform->value.GetString(), sizeof(floats), (byte*)&floats[0]);
        }
        else
        {
            auto ar = transform->value.GetArray();
            for (uint32 i = 0; i < ar.Size(); i++)
            {
                floats[i] = ar[i].GetFloat();
            }
        }
        Padding.Left = (floats[0]);
        Padding.Right = (floats[1]);
        Padding.Top = (floats[2]);
        Padding.Bottom = (floats[3]);
    }
}


UIBackgroundBlur::UIBackgroundBlur(const SpawnParams& params) : UIPanelComponent(params)
{
    BlurStrength = 0;
    CanHaveMultipleChildren = false;
}

void UIBackgroundBlur::OnDraw()
{
    Render2D::DrawBlur(GetRect(), BlurStrength);
}

ScriptingTypeInitializer& UIBackgroundBlur::GetSlotClass() const
{
    return UIBackgroundBlurSlot::TypeInitializer;
}

void UIBackgroundBlur::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIPanelComponent::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UIBackgroundBlur);
    if (BlurStrength != 0)
        SERIALIZE(BlurStrength);
}

void UIBackgroundBlur::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIPanelComponent::Deserialize(stream, modifier);
    DESERIALIZE(BlurStrength)
}
