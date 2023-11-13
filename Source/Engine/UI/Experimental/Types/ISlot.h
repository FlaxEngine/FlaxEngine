// Writen by Nori_SC for https://flaxengine.com all copyright transferred to Wojciech Figat
// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once
#include "Anchor.h"

class UIElement;

API_INTERFACE(Namespace = "FlaxEngine.Experimental.UI") 
class FLAXENGINE_API ISlotMinimal
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ISlotMinimal);

    /// <summary>
    /// Removes the specyfic child
    /// </summary>
    /// <returns>true if Remove Child was successful otherwise false</returns>
    API_FUNCTION() virtual bool RemoveChild(class UIElement* Element);

    /// <summary>
    /// Adds the specyfic child
    /// </summary>
    /// <returns>true if add child was successful otherwise false</returns>
    API_FUNCTION() virtual bool AddChild(UIElement* Element);

    /// <summary>
    /// gets children of ISlot
    /// </summary>
    /// <returns>A array of slots</returns>
    API_FUNCTION() virtual Array<UIElement*> GetChildren();

    /// <summary>
    /// counts number of free slots
    /// </summary>
    /// <returns>Free slot count</returns>
    API_FUNCTION() virtual int GetCountOfFreeSlots();

    /// <summary>
    /// Calculates Layout for this element
    /// </summary>
    API_FUNCTION() virtual void Layout();
    /// <summary>
    /// Gets desired size for this element
    /// </summary>
    API_FUNCTION() virtual Float2 GetDesiredSize();

    API_FUNCTION() virtual Float2 GetDesiredLocation();
};
