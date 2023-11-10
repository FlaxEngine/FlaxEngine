#pragma once
#include "Engine/Debug/DebugLog.h"
#include "Engine/UI/Experimental/Anchor.h"
#include "Engine/UI/Experimental/Types/VisabilityFlags.h"
#include "Engine/UI/Experimental/Types/ClippingFlags.h"
#include "Engine/UI/Experimental/UIRenderTransform.h"
#include "ISlot.h"

/// <summary>
/// Base class for any UI element
/// </summary>
API_CLASS()
class FLAXENGINE_API UIElement : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIElement);
public:

    UIElement(const SpawnParams& params, bool isInDesigner);

    /// <summary>
    /// The parent
    /// </summary>
    ISlot* Parent;

    /// <summary>
    /// The transform
    /// </summary>
    API_FIELD(Attributes = "EditorDisplay(\"Render Transform\"), Tooltip(\"The Render Transform.\")")
    UIRenderTransform* RenderTransform;
    
    /// <summary>
    /// The render transform pivot controls the location about witch transforms are applied.
    /// <br>This value is normalized coordinate about which things like location will occur</br>
    /// </summary>
    API_FIELD(Attributes = "EditorDisplay(\"Povit\"), Tooltip(\"The Render Transform.\")")
    Float2 Povit;

    /// <summary>
    /// The Clipping
    /// </summary>
    API_FIELD(Attributes = "EditorDisplay(\"Clipping\"), Tooltip(\"The clipping flags.\")")
        ClippingFlags Clipping = ClippingFlags::ClipToBounds;

    /// <summary>
    /// The Visability Flags
    /// </summary>
    API_FIELD(Attributes = "EditorDisplay(\"Visability\"), Tooltip(\"The visability flags.\")")
        VisabilityFlags Visability = (VisabilityFlags)(VisabilityFlags::Visable | VisabilityFlags::HitSelf | VisabilityFlags::HitChildren);

    /// <summary>
    /// Caled when UIElement is Cunstructed
    /// Called in editor and game
    /// Warning 
    /// The PreCunstruct can run before any game/editor related states are ready
    /// use only for crating UI elements
    /// </summary>
    /// <param name="isInDesigner">true if is used By Editor was successful otherwise false</param>
    API_FUNCTION() virtual void OnPreCunstruct(bool isInDesigner);

    /// <summary>
    /// Caled when UIElement is created
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnCunstruct();

    /// <summary>
    /// Called when UIElement is destroyed
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnDestruct();

    /// <summary>
    /// Draws current element
    /// Don't Call Draw on children !!
    /// in this function
    /// </summary>
    API_FUNCTION() virtual void OnDraw();

    /// <summary>
    /// removes this from <see cref="ISlot"/> Parent if exists
    /// </summary>
    API_FUNCTION() void Detach();

    /// <summary>
    /// Attach this to <see cref="ISlot"/>
    /// </summary>
    API_FUNCTION() void Attach(ISlot* To);

public:
    // [Object]
    void OnScriptingDispose() override;
    void OnDeleteObject() override;
};
