// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/Core/Common.h"
#include "Engine/UI/Experimental/Types/ISlot.h"
#include "Engine/UI/Experimental/Types/UIElement.h"

inline API_FUNCTION() bool ISlotMinimal::RemoveChild(UIElement* Element) { return false; }

inline API_FUNCTION() bool ISlotMinimal::AddChild(UIElement* Element) { return false; }

inline API_FUNCTION()Array<UIElement*> ISlotMinimal::GetChildren() { return Array<UIElement*>(); }

inline API_FUNCTION() int ISlotMinimal::GetCountOfFreeSlots() { return 0; }

inline API_FUNCTION() void ISlotMinimal::Layout()
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

inline API_FUNCTION()Float2 ISlotMinimal::GetDesiredSize() { return Float2::One; }

inline API_FUNCTION()Float2 ISlotMinimal::GetDesiredLocation() { return Float2::One; }
