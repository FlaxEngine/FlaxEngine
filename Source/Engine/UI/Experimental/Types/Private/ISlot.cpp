// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/Core/Common.h"
#include "Engine/UI/Experimental/Types/ISlot.h"
#include "Engine/UI/Experimental/Types/UIElement.h"

bool ISlotMinimal::RemoveChild(UIElement* Element) { return false; }

bool ISlotMinimal::AddChild(UIElement* Element) { return false; }

Array<UIElement*> ISlotMinimal::GetChildren() { return Array<UIElement*>(); }

int ISlotMinimal::GetCountOfFreeSlots() 
{
    return 0;
}

void ISlotMinimal::Layout()
{
    if (GetCountOfFreeSlots())
    {
        auto children = GetChildren();
        for (auto i = 0; i < children.Count(); i++)
        {
            // Update cached transformation matrix
            children[i]->RenderTransform->UpdateTransformCache(GetDesiredLocation(), GetDesiredSize(), Float2::One * 0.5f);
        }
    }
}
Float2 ISlotMinimal::GetDesiredSize() { return Float2::One; }

Float2 ISlotMinimal::GetDesiredLocation() { return Float2::One; }
