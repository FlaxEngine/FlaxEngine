#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"

/// <summary>
/// the Anchor
/// </summary>
API_STRUCT() struct FLAXENGINE_API Anchor
{
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
        Float2 Min;

    /// <summary>
    /// max value
    /// </summary>
    API_FIELD()
        Float2 Max;
};
