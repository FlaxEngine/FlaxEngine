#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Anchor.h"

class UIElement;

API_INTERFACE(Namespace = "FlaxEngine.Experimental.UI") 
class FLAXENGINE_API ISlot
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ISlot);
private:
    /// <summary>
    /// Location of ISlot
    /// </summary>
    Float2 Location;

    /// <summary>
    /// Size of ISlot
    /// </summary>
    Float2 Size;

    /// <summary>
    /// The anchor
    /// </summary>
    Anchor Anchors;

    /// <summary>
    /// Resizes the element to fit the Content if flag is true
    /// </summary>
    bool SizeToContent;
public:
    /// <summary>
    /// Removes the specyfic child
    /// </summary>
    /// <returns>true if Remove Child was successful otherwise false</returns>
    API_FUNCTION() virtual bool RemoveChild(UIElement* Element) = 0;

    /// <summary>
    /// Adds the specyfic child
    /// </summary>
    /// <returns>true if add child was successful otherwise false</returns>
    API_FUNCTION() virtual bool AddChild(UIElement* Element) = 0;

    /// <summary>
    /// gets children of ISlot
    /// </summary>
    /// <returns>A array of slots</returns>
    API_FUNCTION() virtual Array<class UIElement*> GetChildren() = 0;

    /// <summary>
    /// counts number of free slots
    /// </summary>
    /// <returns>Free slot count</returns>
    API_FUNCTION() virtual int GetCountOfFreeSlots() = 0;
    /// <summary>
    /// Gets a Location
    /// </summary>
    /// <returns>Anchor</returns>
    API_FUNCTION() Float2 GetLocation();

    /// <summary>
    /// sets the Location
    /// </summary>
    /// <param name="anchor">new anchor</param>
    API_FUNCTION() void SetLocation(Float2 newLocation);

    /// <summary>
    /// Gets a size
    /// </summary>
    /// <returns>Anchor</returns>
    API_FUNCTION() Float2 GetSize();

    /// <summary>
    /// sets the size
    /// </summary>
    /// <param name="anchor">new anchor</param>
    API_FUNCTION() void SetSize(Float2 newSize);

    /// <summary>
    /// Gets a anchor
    /// </summary>
    /// <returns>Anchor</returns>
    API_FUNCTION() Anchor* GetAnchor();

    /// <summary>
    /// sets the anchor
    /// </summary>
    /// <param name="anchor">new anchor</param>
    API_FUNCTION() void SetAnchor(Anchor& anchor);

    /// <summary>
    /// Gets a size to content
    /// </summary>
    /// <returns>Anchor</returns>
    API_FUNCTION() bool GetSizeToContent();

    /// <summary>
    /// sets the size to content
    /// </summary>
    /// <param name="anchor">New size to content value</param>
    API_FUNCTION() void SetSizeToContent(bool value);

    /// <summary>
    /// sets the preset for anchor
    /// </summary>
    API_FUNCTION() void SetAnchorPreset(Anchor::Presets presets);
    /// <summary>
    /// Gets the preset from anchor
    /// </summary>
    API_FUNCTION() Anchor::Presets GetAnchorPreset();
};
