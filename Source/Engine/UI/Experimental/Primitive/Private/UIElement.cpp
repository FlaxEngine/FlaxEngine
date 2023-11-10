#include "Engine/Core/Memory/Memory.h"
#include "Engine/UI/Experimental/Primitive/UIElement.h"

void UIElement::Detach()
{
    if (Parent) 
    {
        Parent->RemoveChild(this);
    }
    else
    {
        DebugLog::LogWarning(const StringView(TEXT("Failed to Detach from ISlot, the UIElement do not have parent")));
    }
}
void UIElement::Attach(ISlot* To)
{
    if (To)
    {
        To->AddChild(this);
    }
    else
    {
        DebugLog::LogWarning(const StringView(TEXT("Failed to Attach ISlot, ISlot* To == nullptr")));
    }
}
Array<UIElement*> ISlot::GetChildren()
{
    return Array<UIElement*>();
}

Anchor* ISlot::GetAnchor()
{
    return &Anchors;
}
void ISlot::SetAnchor(Anchor& anchor)
{
    Anchors = anchor;
}
void ISlot::Layout() {}


/// <summary>
/// Gets desired size for this element
/// </summary>

Float2 ISlot::GetDesiredSize()
{
    return Transform->Size;
}

void ISlot::SetAnchorPreset(Anchor::Presets presets)
{
    switch (presets)
    {
    case Anchor::Presets::TopLeft:                  Anchors.Min = Float2(0, 0);         Anchors.Max = Float2(0, 0);         break;
    case Anchor::Presets::TopCenter:                Anchors.Min = Float2(0.5f, 0);      Anchors.Max = Float2(0.5f, 0);      break;
    case Anchor::Presets::TopRight:                 Anchors.Min = Float2(1, 0);         Anchors.Max = Float2(1, 0);         break;
    case Anchor::Presets::MiddleLeft:               Anchors.Min = Float2(0, 0.5f);      Anchors.Max = Float2(0, 0.5f);      break;
    case Anchor::Presets::MiddleCenter:             Anchors.Min = Float2(0.5f, 0.5f);   Anchors.Max = Float2(0.5f, 0.5f);   break;
    case Anchor::Presets::MiddleRight:              Anchors.Min = Float2(1, 0.5f);      Anchors.Max = Float2(1, 0.5f);      break;
    case Anchor::Presets::BottomLeft:               Anchors.Min = Float2(0, 1);         Anchors.Max = Float2(0, 1);         break;
    case Anchor::Presets::BottomCenter:             Anchors.Min = Float2(0.5f, 1);      Anchors.Max = Float2(0.5f, 1);      break;
    case Anchor::Presets::BottomRight:              Anchors.Min = Float2(1, 1);         Anchors.Max = Float2(1, 1);         break;
    case Anchor::Presets::HorizontalStretchTop:     Anchors.Min = Float2(0, 0);         Anchors.Max = Float2(1, 0);         break;
    case Anchor::Presets::HorizontalStretchMiddle:  Anchors.Min = Float2(0, 0.5f);      Anchors.Max = Float2(1, 0.5f);      break;
    case Anchor::Presets::HorizontalStretchBottom:  Anchors.Min = Float2(0, 1);         Anchors.Max = Float2(1, 1);         break;
    case Anchor::Presets::VerticalStretchLeft:      Anchors.Min = Float2(0, 0);         Anchors.Max = Float2(0, 1);         break;
    case Anchor::Presets::VerticalStretchCenter:    Anchors.Min = Float2(0.5f, 0);      Anchors.Max = Float2(0.5f, 1);      break;
    case Anchor::Presets::VerticalStretchRight:     Anchors.Min = Float2(1, 0);         Anchors.Max = Float2(1, 1);         break;
    case Anchor::Presets::StretchAll:               Anchors.Min = Float2(0, 0);         Anchors.Max = Float2(1, 1);         break;
    }
}
Anchor::Presets ISlot::GetAnchorPreset()
{
    if(Anchors.Min == Float2(0, 0)        && Anchors.Max == Float2(0, 0)      ){return Anchor::Presets::TopLeft                ;}
    if(Anchors.Min == Float2(0.5f, 0)     && Anchors.Max == Float2(0.5f, 0)   ){return Anchor::Presets::TopCenter              ;}
    if(Anchors.Min == Float2(1, 0)        && Anchors.Max == Float2(1, 0)      ){return Anchor::Presets::TopRight               ;}
    if(Anchors.Min == Float2(0, 0.5f)     && Anchors.Max == Float2(0, 0.5f)   ){return Anchor::Presets::MiddleLeft             ;}
    if(Anchors.Min == Float2(0.5f, 0.5f)  && Anchors.Max == Float2(0.5f, 0.5f)){return Anchor::Presets::MiddleCenter           ;}
    if(Anchors.Min == Float2(1, 0.5f)     && Anchors.Max == Float2(1, 0.5f)   ){return Anchor::Presets::MiddleRight            ;}
    if(Anchors.Min == Float2(0, 1)        && Anchors.Max == Float2(0, 1)      ){return Anchor::Presets::BottomLeft             ;}
    if(Anchors.Min == Float2(0.5f, 1)     && Anchors.Max == Float2(0.5f, 1)   ){return Anchor::Presets::BottomCenter           ;}
    if(Anchors.Min == Float2(1, 1)        && Anchors.Max == Float2(1, 1)      ){return Anchor::Presets::BottomRight            ;}
    if(Anchors.Min == Float2(0, 0)        && Anchors.Max == Float2(1, 0)      ){return Anchor::Presets::HorizontalStretchTop   ;}
    if(Anchors.Min == Float2(0, 0.5f)     && Anchors.Max == Float2(1, 0.5f)   ){return Anchor::Presets::HorizontalStretchMiddle;}
    if(Anchors.Min == Float2(0, 1)        && Anchors.Max == Float2(1, 1)      ){return Anchor::Presets::HorizontalStretchBottom;}
    if(Anchors.Min == Float2(0, 0)        && Anchors.Max == Float2(0, 1)      ){return Anchor::Presets::VerticalStretchLeft    ;}
    if(Anchors.Min == Float2(0.5f, 0)     && Anchors.Max == Float2(0.5f, 1)   ){return Anchor::Presets::VerticalStretchCenter  ;}
    if(Anchors.Min == Float2(1, 0)        && Anchors.Max == Float2(1, 1)      ){return Anchor::Presets::VerticalStretchRight   ;}
    if(Anchors.Min == Float2(0, 0)        && Anchors.Max == Float2(1, 1)      ){return Anchor::Presets::StretchAll             ;}

    return Anchor::Presets::Custom;
}

UIElement::UIElement(const SpawnParams& params) : ScriptingObject(params)
{
    RenderTransform = New<UIRenderTransform>();
    OnPreCunstruct(false);
}

UIElement::UIElement(const SpawnParams& params, bool isInDesigner) : ScriptingObject(params)
{
    RenderTransform = New<UIRenderTransform>();
    OnPreCunstruct(true);
}

void UIElement::OnPreCunstruct(bool isInDesigner){}
void UIElement::OnCunstruct(){}
void UIElement::OnDestruct(){}
void UIElement::OnDraw(){}
void UIElement::OnScriptingDispose()
{
    RemoveFromParent();
    OnDestruct();
}
void UIElement::OnDeleteObject()
{
    SAFE_DELETE(RenderTransform);
}
