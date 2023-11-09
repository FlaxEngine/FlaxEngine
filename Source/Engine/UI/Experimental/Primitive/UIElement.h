#pragma once
#include "../UITransform.h"
#include <Engine/UI/Experimental/Anchor.h>

/// <summary>
    /// Visability flags
    /// </summary>
enum VisabilityFlags
    {
        /// <summary>
        /// Visable on the screen
        /// </summary>
        Visable = 1,
        /// <summary>
        /// Hiden on the screen
        /// </summary>
        Hiden = 2,
        /// <summary>
        /// if Events will fire on this 
        /// </summary>
        HitSelf = 4,
        /// <summary>
        /// if Events will fire on children
        /// </summary>
        HitChildren = 8,
        /// <summary>
        /// not visible not hittable and takes up no space in the UI
        /// </summary>
        Colapsed = 16
    };
/// <summary>
/// Visability flags
/// </summary>
enum ClippingFlags
{
    /// <summary>
    /// Keeps the parent flags
    /// </summary>
    Inherit,
    /// <summary>
    /// Ignore clipping
    /// </summary>
    DontClip,
    /// <summary>
    /// Clip children to this element to the to size
    /// </summary>
    ClipToBounds,
};

/// <summary>
/// base class for any UI element
/// </summary>
class UIElement
{
public:
    /// <summary>
    /// The Visability Flags
    /// </summary>
    API_FIELD()//[ExpandGroups, EditorDisplay("Visability"), Tooltip("The Visability Flags.")]
    VisabilityFlags Visability = (VisabilityFlags)(VisabilityFlags::Visable | VisabilityFlags::HitSelf | VisabilityFlags::HitChildren);

    /// <summary>
    /// The Transform
    /// </summary>
    //[EditorDisplay("Transform")]
    API_FIELD()
    UITransform Transform;

    /// <summary>
    /// The Clipping
    /// </summary>
    //[ExpandGroups, EditorDisplay("Visability"), Tooltip("The Visability Flags.")]
    API_FIELD()
    ClippingFlags Clipping;

    /// <summary>
    /// Draws curent element
    /// </summary>
    API_FUNCTION()
    virtual void  Draw() {};

    /// <summary>
    /// sets the preset for anchor
    /// </summary>
    API_FUNCTION()
        Anchor GetAnchorPreset(Anchor::Presets presets)
    {
        Anchor A = Anchor();
        switch (presets)
        {
            case Anchor::Presets::TopLeft:                  A.Min = Float2(0, 0);         A.Max = Float2(0, 0); break;
            case Anchor::Presets::TopCenter:                A.Min = Float2(0.5f, 0);      A.Max = Float2(0.5f, 0); break;
            case Anchor::Presets::TopRight:                 A.Min = Float2(1, 0);         A.Max = Float2(1, 0); break;
            case Anchor::Presets::MiddleLeft:               A.Min = Float2(0, 0.5f);      A.Max = Float2(0, 0.5f); break;
            case Anchor::Presets::MiddleCenter:             A.Min = Float2(0.5f, 0.5f);   A.Max = Float2(0.5f, 0.5f); break;
            case Anchor::Presets::MiddleRight:              A.Min = Float2(1, 0.5f);      A.Max = Float2(1, 0.5f); break;
            case Anchor::Presets::BottomLeft:               A.Min = Float2(0, 1);         A.Max = Float2(0, 1); break;
            case Anchor::Presets::BottomCenter:             A.Min = Float2(0.5f, 1);      A.Max = Float2(0.5f, 1); break;
            case Anchor::Presets::BottomRight:              A.Min = Float2(1, 1);         A.Max = Float2(1, 1); break;
            case Anchor::Presets::HorizontalStretchTop:     A.Min = Float2(0, 0);         A.Max = Float2(1, 0); break;
            case Anchor::Presets::HorizontalStretchMiddle:  A.Min = Float2(0, 0.5f);      A.Max = Float2(1, 0.5f); break;
            case Anchor::Presets::HorizontalStretchBottom:  A.Min = Float2(0, 1);         A.Max = Float2(1, 1); break;
            case Anchor::Presets::VerticalStretchLeft:      A.Min = Float2(0, 0);         A.Max = Float2(0, 1); break;
            case Anchor::Presets::VerticalStretchCenter:    A.Min = Float2(0.5f, 0);      A.Max = Float2(0.5f, 1); break;
            case Anchor::Presets::VerticalStretchRight:     A.Min = Float2(1, 0);         A.Max = Float2(1, 1); break;
            case Anchor::Presets::StretchAll:               A.Min = Float2(0, 0);         A.Max = Float2(1, 1); break;
        }
        return A;
    }
};
