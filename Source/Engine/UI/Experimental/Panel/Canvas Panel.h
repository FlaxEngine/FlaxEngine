// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once
#include "Engine/UI/Experimental/Common.h"

API_INTERFACE(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API ICanvasSlot : public ISlotMinimal
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ICanvasSlot);

    /// <summary>
    /// Location of ISlot
    /// </summary>
    API_FIELD() Float2 Location = Float2::Zero;

    /// <summary>
    /// Size of ISlot
    /// </summary>
    API_FIELD() Float2 Size;

    /// <summary>
    /// The anchor
    /// </summary>
    API_FIELD() Anchor Anchors;

    /// <summary>
    /// Resizes the element to fit the Content if flag is true
    /// </summary>
    API_FIELD() bool SizeToContent;

    /*virtual int GetCountOfFreeSlots() override
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
    }*/

};
