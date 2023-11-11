#pragma once
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"

API_INTERFACE(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API IBrush
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(IBrush);
protected:
    class UIElement* Owner;
public:
    /// <summary>
    /// Caled when IBrush is Cunstructed
    /// Called in editor and game
    /// Warning 
    /// The PreCunstruct can run before any game/editor related states are ready
    /// use only for IBrush
    /// </summary>
    API_FUNCTION() virtual void OnPreCunstruct(bool isInDesigner);

    /// <summary>
    /// Caled when IBrush is created
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnCunstruct();

    /// <summary>
    /// Draws the IBrush
    /// </summary>
    API_FUNCTION() virtual void OnDraw(const Float2& At);

    /// <summary>
    /// Called when IBrush is destroyed
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnDestruct();;

    /// <summary>
    /// Gets desired size for this element
    /// </summary>
    API_FUNCTION() virtual Float2 GetDesiredSize();;
};
