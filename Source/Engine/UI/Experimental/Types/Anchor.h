//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

/// <summary>
/// the Anchor
/// </summary>
API_STRUCT(Namespace = "FlaxEngine.Experimental.UI")
struct FLAXENGINE_API Anchor : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(Anchor);
public:
    /// <summary>
    /// UI control anchors presets.
    /// </summary>
    API_ENUM()
        enum FLAXENGINE_API Presets
    {
        /// <summary>
        /// The empty preset.
        /// </summary>
        Custom,

        /// <summary>
        /// The top left corner of the parent control.
        /// </summary>
        TopLeft,

        /// <summary>
        /// The center of the top edge of the parent control.
        /// </summary>
        TopCenter,

        /// <summary>
        /// The top right corner of the parent control.
        /// </summary>
        TopRight,

        /// <summary>
        /// The middle of the left edge of the parent control.
        /// </summary>
        MiddleLeft,

        /// <summary>
        /// The middle center! Right in the middle of the parent control.
        /// </summary>
        MiddleCenter,

        /// <summary>
        /// The middle of the right edge of the parent control.
        /// </summary>
        MiddleRight,

        /// <summary>
        /// The bottom left corner of the parent control.
        /// </summary>
        BottomLeft,

        /// <summary>
        /// The center of the bottom edge of the parent control.
        /// </summary>
        BottomCenter,

        /// <summary>
        /// The bottom right corner of the parent control.
        /// </summary>
        BottomRight,

        /// <summary>
        /// The vertical stretch on the left of the parent control.
        /// </summary>
        VerticalStretchLeft,

        /// <summary>
        /// The vertical stretch on the center of the parent control.
        /// </summary>
        VerticalStretchCenter,

        /// <summary>
        /// The vertical stretch on the right of the parent control.
        /// </summary>
        VerticalStretchRight,

        /// <summary>
        /// The horizontal stretch on the top of the parent control.
        /// </summary>
        HorizontalStretchTop,

        /// <summary>
        /// The horizontal stretch in the middle of the parent control.
        /// </summary>
        HorizontalStretchMiddle,

        /// <summary>
        /// The horizontal stretch on the bottom of the parent control.
        /// </summary>
        HorizontalStretchBottom,

        /// <summary>
        /// All parent control edges.
        /// </summary>
        StretchAll,
    };

    /// <summary>
    /// min value
    /// </summary>
    API_FIELD()
        Float2 Min = Float2(0.5f, 0.5f);

    /// <summary>
    /// max value
    /// </summary>
    API_FIELD()
        Float2 Max = Float2(0.5f, 0.5f);

    /// <summary>
    /// sets the preset for anchor
    /// </summary>
    void SetAnchorPreset(Anchor::Presets presets)
    {
        switch (presets)
        {
        case Anchor::Presets::TopLeft:                  Min = Float2(0, 0);         Max = Float2(0, 0);         break;
        case Anchor::Presets::TopCenter:                Min = Float2(0.5f, 0);      Max = Float2(0.5f, 0);      break;
        case Anchor::Presets::TopRight:                 Min = Float2(1, 0);         Max = Float2(1, 0);         break;
        case Anchor::Presets::MiddleLeft:               Min = Float2(0, 0.5f);      Max = Float2(0, 0.5f);      break;
        case Anchor::Presets::MiddleCenter:             Min = Float2(0.5f, 0.5f);   Max = Float2(0.5f, 0.5f);   break;
        case Anchor::Presets::MiddleRight:              Min = Float2(1, 0.5f);      Max = Float2(1, 0.5f);      break;
        case Anchor::Presets::BottomLeft:               Min = Float2(0, 1);         Max = Float2(0, 1);         break;
        case Anchor::Presets::BottomCenter:             Min = Float2(0.5f, 1);      Max = Float2(0.5f, 1);      break;
        case Anchor::Presets::BottomRight:              Min = Float2(1, 1);         Max = Float2(1, 1);         break;
        case Anchor::Presets::HorizontalStretchTop:     Min = Float2(0, 0);         Max = Float2(1, 0);         break;
        case Anchor::Presets::HorizontalStretchMiddle:  Min = Float2(0, 0.5f);      Max = Float2(1, 0.5f);      break;
        case Anchor::Presets::HorizontalStretchBottom:  Min = Float2(0, 1);         Max = Float2(1, 1);         break;
        case Anchor::Presets::VerticalStretchLeft:      Min = Float2(0, 0);         Max = Float2(0, 1);         break;
        case Anchor::Presets::VerticalStretchCenter:    Min = Float2(0.5f, 0);      Max = Float2(0.5f, 1);      break;
        case Anchor::Presets::VerticalStretchRight:     Min = Float2(1, 0);         Max = Float2(1, 1);         break;
        case Anchor::Presets::StretchAll:               Min = Float2(0, 0);         Max = Float2(1, 1);         break;
        }
    }

    /// <summary>
    /// Gets the preset from anchor
    /// </summary>
    Anchor::Presets GetAnchorPreset()
    {
        if (Min == Float2(0, 0)         && Max == Float2(0, 0)) { return Anchor::Presets::TopLeft; }
        if (Min == Float2(0.5f, 0)      && Max == Float2(0.5f, 0)) { return Anchor::Presets::TopCenter; }
        if (Min == Float2(1, 0)         && Max == Float2(1, 0)) { return Anchor::Presets::TopRight; }
        if (Min == Float2(0, 0.5f)      && Max == Float2(0, 0.5f)) { return Anchor::Presets::MiddleLeft; }
        if (Min == Float2(0.5f, 0.5f)   && Max == Float2(0.5f, 0.5f)) { return Anchor::Presets::MiddleCenter; }
        if (Min == Float2(1, 0.5f)      && Max == Float2(1, 0.5f)) { return Anchor::Presets::MiddleRight; }
        if (Min == Float2(0, 1)         && Max == Float2(0, 1)) { return Anchor::Presets::BottomLeft; }
        if (Min == Float2(0.5f, 1)      && Max == Float2(0.5f, 1)) { return Anchor::Presets::BottomCenter; }
        if (Min == Float2(1, 1)         && Max == Float2(1, 1)) { return Anchor::Presets::BottomRight; }
        if (Min == Float2(0, 0)         && Max == Float2(1, 0)) { return Anchor::Presets::HorizontalStretchTop; }
        if (Min == Float2(0, 0.5f)      && Max == Float2(1, 0.5f)) { return Anchor::Presets::HorizontalStretchMiddle; }
        if (Min == Float2(0, 1)         && Max == Float2(1, 1)) { return Anchor::Presets::HorizontalStretchBottom; }
        if (Min == Float2(0, 0)         && Max == Float2(0, 1)) { return Anchor::Presets::VerticalStretchLeft; }
        if (Min == Float2(0.5f, 0)      && Max == Float2(0.5f, 1)) { return Anchor::Presets::VerticalStretchCenter; }
        if (Min == Float2(1, 0)         && Max == Float2(1, 1)) { return Anchor::Presets::VerticalStretchRight; }
        if (Min == Float2(0, 0)         && Max == Float2(1, 1)) { return Anchor::Presets::StretchAll; }

        return Anchor::Presets::Custom;
    };
};
