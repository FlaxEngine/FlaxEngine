#include "Engine/Core/Common.h"
#include "Engine/UI/Experimental/Primitive/ISlot.h"

Float2 ISlot::GetSize()
{
    return Size;
}

void ISlot::SetSize(Float2 newSize)
{
    Size = newSize;
}

Anchor* ISlot::GetAnchor()
{
    return &Anchors;
}
void ISlot::SetAnchor(Anchor& anchor)
{
    Anchors = anchor;
}
bool ISlot::GetSizeToContent()
{
    return SizeToContent;
}

int ISlot::GetCountOfFreeSlots()
{
    auto gc = GetChildren();
    int out = -1;
    if (gc.HasItems())
    {
        for (auto i = 0; i < gc.Count(); i++)
        {
            if (gc[i] == nullptr)
            {
                out++;
            }
        }
    }
    return out;
}

Float2 ISlot::GetLocation()
{
    return Location;
}
void ISlot::SetLocation(Float2 newLocation)
{
    Location = newLocation;
}

void ISlot::SetSizeToContent(bool value)
{
    SizeToContent = value;
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
    if (Anchors.Min == Float2(0, 0) && Anchors.Max == Float2(0, 0)) { return Anchor::Presets::TopLeft; }
    if (Anchors.Min == Float2(0.5f, 0) && Anchors.Max == Float2(0.5f, 0)) { return Anchor::Presets::TopCenter; }
    if (Anchors.Min == Float2(1, 0) && Anchors.Max == Float2(1, 0)) { return Anchor::Presets::TopRight; }
    if (Anchors.Min == Float2(0, 0.5f) && Anchors.Max == Float2(0, 0.5f)) { return Anchor::Presets::MiddleLeft; }
    if (Anchors.Min == Float2(0.5f, 0.5f) && Anchors.Max == Float2(0.5f, 0.5f)) { return Anchor::Presets::MiddleCenter; }
    if (Anchors.Min == Float2(1, 0.5f) && Anchors.Max == Float2(1, 0.5f)) { return Anchor::Presets::MiddleRight; }
    if (Anchors.Min == Float2(0, 1) && Anchors.Max == Float2(0, 1)) { return Anchor::Presets::BottomLeft; }
    if (Anchors.Min == Float2(0.5f, 1) && Anchors.Max == Float2(0.5f, 1)) { return Anchor::Presets::BottomCenter; }
    if (Anchors.Min == Float2(1, 1) && Anchors.Max == Float2(1, 1)) { return Anchor::Presets::BottomRight; }
    if (Anchors.Min == Float2(0, 0) && Anchors.Max == Float2(1, 0)) { return Anchor::Presets::HorizontalStretchTop; }
    if (Anchors.Min == Float2(0, 0.5f) && Anchors.Max == Float2(1, 0.5f)) { return Anchor::Presets::HorizontalStretchMiddle; }
    if (Anchors.Min == Float2(0, 1) && Anchors.Max == Float2(1, 1)) { return Anchor::Presets::HorizontalStretchBottom; }
    if (Anchors.Min == Float2(0, 0) && Anchors.Max == Float2(0, 1)) { return Anchor::Presets::VerticalStretchLeft; }
    if (Anchors.Min == Float2(0.5f, 0) && Anchors.Max == Float2(0.5f, 1)) { return Anchor::Presets::VerticalStretchCenter; }
    if (Anchors.Min == Float2(1, 0) && Anchors.Max == Float2(1, 1)) { return Anchor::Presets::VerticalStretchRight; }
    if (Anchors.Min == Float2(0, 0) && Anchors.Max == Float2(1, 1)) { return Anchor::Presets::StretchAll; }

    return Anchor::Presets::Custom;
}
