// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Math/Vector2.h"

struct FontTextureAtlasSlot;
class Font;

// Font glyph metrics:
//
//                       xmin                     xmax
//                        |                         |
//                        |<-------- width -------->|
//                        |                         |
//              |         +-------------------------+----------------- ymax
//              |         |    ggggggggg   ggggg    |     ^        ^
//              |         |   g:::::::::ggg::::g    |     |        |
//              |         |  g:::::::::::::::::g    |     |        |
//              |         | g::::::ggggg::::::gg    |     |        |
//              |         | g:::::g     g:::::g     |     |        |
//    offsetX  -|-------->| g:::::g     g:::::g     |  offsetY     |
//              |         | g:::::g     g:::::g     |     |        |
//              |         | g::::::g    g:::::g     |     |        |
//              |         | g:::::::ggggg:::::g     |     |        |
//              |         |  g::::::::::::::::g     |     |      height
//              |         |   gg::::::::::::::g     |     |        |
//  baseline ---*---------|---- gggggggg::::::g-----*--------      |
//            / |         |             g:::::g     |              |
//     origin   |         | gggggg      g:::::g     |              |
//              |         | g:::::gg   gg:::::g     |              |
//              |         |  g::::::ggg:::::::g     |              |
//              |         |   gg:::::::::::::g      |              |
//              |         |     ggg::::::ggg        |              |
//              |         |         gggggg          |              v
//              |         +-------------------------+----------------- ymin
//              |                                   |
//              |------------- advanceX ----------->|

/// <summary>
/// The cached font character entry (read for rendering and further processing).
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API FontCharacterEntry
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(FontCharacterEntry);

    /// <summary>
    /// The character represented by this entry.
    /// </summary>
    API_FIELD() Char Character;

    /// <summary>
    /// True if entry is valid, otherwise false.
    /// </summary>
    API_FIELD() bool IsValid = false;

    /// <summary>
    /// The index to a specific texture in the font cache.
    /// </summary>
    API_FIELD() byte TextureIndex;

    /// <summary>
    /// The left bearing expressed in integer pixels.
    /// </summary>
    API_FIELD() int16 OffsetX;

    /// <summary>
    /// The top bearing expressed in integer pixels.
    /// </summary>
    API_FIELD() int16 OffsetY;

    /// <summary>
    /// The amount to advance in X before drawing the next character in a string.
    /// </summary>
    API_FIELD() int16 AdvanceX;

    /// <summary>
    /// The distance from baseline to glyph top most point.
    /// </summary>
    API_FIELD() int16 BearingY;

    /// <summary>
    /// The height in pixels of the glyph.
    /// </summary>
    API_FIELD() int16 Height;

    /// <summary>
    /// The start location of the character in the texture (in texture coordinates space).
    /// </summary>
    API_FIELD() Float2 UV;

    /// <summary>
    /// The size the character in the texture (in texture coordinates space).
    /// </summary>
    API_FIELD() Float2 UVSize;

    /// <summary>
    /// The slot in texture atlas, containing the pixel data of the glyph.
    /// </summary>
    API_FIELD() const FontTextureAtlasSlot* Slot;

    /// <summary>
    /// The font containing metrics.
    /// </summary>
    API_FIELD() const class Font* Font;
};

template<>
struct TIsPODType<FontCharacterEntry>
{
    enum { Value = true };
};
