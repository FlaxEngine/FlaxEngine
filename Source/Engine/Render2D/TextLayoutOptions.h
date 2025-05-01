// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Rectangle.h"

/// <summary>
/// Specifies the alignment of the text along horizontal or vertical direction in the layout box.
/// </summary>
API_ENUM() enum class TextAlignment
{
    /// <summary>
    /// Align text near the edge.
    /// </summary>
    Near = 0,

    /// <summary>
    /// Align text to the center.
    /// </summary>
    Center,

    /// <summary>
    /// Align text to the far edge.
    /// </summary>
    Far,
};

/// <summary>
/// Specifies text wrapping to be used in a particular multiline paragraph.
/// </summary>
API_ENUM() enum class TextWrapping
{
    /// <summary>
    /// No text wrapping.
    /// </summary>
    NoWrap = 0,

    /// <summary>
    /// Wrap only whole words that overflow.
    /// </summary>
    WrapWords,

    /// <summary>
    /// Wrap single characters that overflow.
    /// </summary>
    WrapChars,
};

/// <summary>
/// Structure which describes text layout properties.
/// </summary>
API_STRUCT(NoDefault) struct TextLayoutOptions
{
DECLARE_SCRIPTING_TYPE_MINIMAL(TextLayoutOptions);

    /// <summary>
    /// The layout rectangle (text bounds).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0)")
    Rectangle Bounds;

    /// <summary>
    /// The horizontal alignment mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10)")
    TextAlignment HorizontalAlignment;

    /// <summary>
    /// The vertical alignment mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20)")
    TextAlignment VerticalAlignment;

    /// <summary>
    /// The text wrapping mode.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(TextWrapping.NoWrap)")
    TextWrapping TextWrapping;

    /// <summary>
    /// The text scale factor. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(1.0f), Limit(-1000, 1000, 0.01f)")
    float Scale;

    /// <summary>
    /// Base line gap scale. Default is 1.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(1.0f), Limit(-1000, 1000, 0.01f)")
    float BaseLinesGapScale;

    /// <summary>
    /// Initializes a new instance of the <see cref="TextLayoutOptions"/> struct.
    /// </summary>
    TextLayoutOptions()
    {
        Bounds = Rectangle(0, 0, MAX_float, MAX_float);
        HorizontalAlignment = TextAlignment::Near;
        VerticalAlignment = TextAlignment::Near;
        TextWrapping = TextWrapping::NoWrap;
        Scale = 1.0f;
        BaseLinesGapScale = 1.0f;
    }

    FORCE_INLINE bool operator==(const TextLayoutOptions& other) const
    {
        return Bounds == other.Bounds
                && HorizontalAlignment == other.HorizontalAlignment
                && VerticalAlignment == other.VerticalAlignment
                && TextWrapping == other.TextWrapping
                && Math::NearEqual(Scale, other.Scale)
                && Math::NearEqual(BaseLinesGapScale, other.BaseLinesGapScale);
    }

    FORCE_INLINE bool operator!=(const TextLayoutOptions& other) const
    {
        return !operator==(other);
    }
};
