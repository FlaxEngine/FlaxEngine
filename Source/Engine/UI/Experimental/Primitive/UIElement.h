#pragma once
#include "../UITransform.h"

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
    UITransform Transform = UITransform(Rectangle(0, 0, 100, 100), Float2::One, Float2::One * 0.5f, 0);

    /// <summary>
    /// The Clipping
    /// </summary>
    //[ExpandGroups, EditorDisplay("Visability"), Tooltip("The Visability Flags.")]
    ClippingFlags Clipping;

    /// <summary>
    /// Draws curent element
    /// </summary>
    virtual void  Draw() {};
};
