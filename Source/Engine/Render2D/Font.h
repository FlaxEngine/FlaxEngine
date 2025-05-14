// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "TextLayoutOptions.h"

class FontAsset;
struct FontTextureAtlasSlot;

// The default DPI that engine is using
#define DefaultDPI 96

/// <summary>
/// The text range.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API TextRange
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(TextRange);

    /// <summary>
    /// The start index (inclusive).
    /// </summary>
    API_FIELD() int32 StartIndex;

    /// <summary>
    /// The end index (exclusive).
    /// </summary>
    API_FIELD() int32 EndIndex;

    /// <summary>
    /// Gets the range length.
    /// </summary>
    FORCE_INLINE int32 Length() const
    {
        return EndIndex - StartIndex;
    }

    /// <summary>
    /// Gets a value indicating whether range is empty.
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return (EndIndex - StartIndex) <= 0;
    }

    /// <summary>
    /// Determines whether this range contains the character index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns><c>true</c> if range contains the specified character index; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool Contains(int32 index) const
    {
        return index >= StartIndex && index < EndIndex;
    }

    /// <summary>
    /// Determines whether this range intersects with the other range.
    /// </summary>
    /// <param name="other">The other text range.</param>
    /// <returns><c>true</c> if range intersects with the specified range index;, <c>false</c>.</returns>
    bool Intersect(const TextRange& other) const
    {
        return Math::Min(EndIndex, other.EndIndex) > Math::Max(StartIndex, other.StartIndex);
    }

    /// <summary>
    /// Gets the substring from the source text.
    /// </summary>
    /// <param name="text">The text.</param>
    /// <returns>The substring of the original text of the defined range.</returns>
    StringView Substring(const StringView& text) const
    {
        return StringView(text.Get() + StartIndex, EndIndex - StartIndex);
    }
};

template<>
struct TIsPODType<TextRange>
{
    enum { Value = true };
};

/// <summary>
/// The font line info generated during text processing.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API FontLineCache
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(FontLineCache);

    /// <summary>
    /// The root position of the line (upper left corner).
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// The line bounds (width and height).
    /// </summary>
    API_FIELD() Float2 Size;

    /// <summary>
    /// The first character index (from the input text).
    /// </summary>
    API_FIELD() int32 FirstCharIndex;

    /// <summary>
    /// The last character index (from the input text), inclusive.
    /// </summary>
    API_FIELD() int32 LastCharIndex;
};

template<>
struct TIsPODType<FontLineCache>
{
    enum { Value = true };
};

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
    /// The owner font.
    /// </summary>
    API_FIELD() const class Font* Font;
};

template<>
struct TIsPODType<FontCharacterEntry>
{
    enum { Value = true };
};

/// <summary>
/// Represents font object that can be using during text rendering (it uses Font Asset but with pre-cached data for chosen font properties).
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API Font : public ManagedScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Font);
    friend FontAsset;

private:
    FontAsset* _asset;
    float _size;
    int32 _height;
    int32 _ascender;
    int32 _descender;
    int32 _lineGap;
    bool _hasKerning;
    Dictionary<Char, FontCharacterEntry> _characters;
    mutable Dictionary<uint32, int32> _kerningTable;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Font"/> class.
    /// </summary>
    /// <param name="parentAsset">The parent asset.</param>
    /// <param name="size">The size.</param>
    Font(FontAsset* parentAsset, float size);

    /// <summary>
    /// Finalizes an instance of the <see cref="Font"/> class.
    /// </summary>
    ~Font();

public:
    /// <summary>
    /// The active fallback fonts.
    /// </summary>
    API_FIELD() static Array<AssetReference<FontAsset>, HeapAllocation> FallbackFonts;

    /// <summary>
    /// Gets parent font asset that contains font family used by this font.
    /// </summary>
    API_PROPERTY() FORCE_INLINE FontAsset* GetAsset() const
    {
        return _asset;
    }

    /// <summary>
    /// Gets font size.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetSize() const
    {
        return _size;
    }

    /// <summary>
    /// Gets characters height.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Gets the largest vertical distance above the baseline for any character in the font.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetAscender() const
    {
        return _ascender;
    }

    /// <summary>
    /// Gets the largest vertical distance below the baseline for any character in the font.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetDescender() const
    {
        return _descender;
    }

    /// <summary>
    /// Gets the line gap property.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLineGap() const
    {
        return _lineGap;
    }

public:
    /// <summary>
    /// Gets character entry.
    /// </summary>
    /// <param name="c">The character.</param>
    /// <param name="result">The output character entry.</param>
    /// <param name="enableFallback">True if fallback to secondary font when the primary font doesn't contains this character.</param>
    void GetCharacter(Char c, FontCharacterEntry& result, bool enableFallback = true);

    /// <summary>
    /// Gets the kerning amount for a pair of characters.
    /// </summary>
    /// <param name="first">The first character in the pair.</param>
    /// <param name="second">The second character in the pair.</param>
    /// <returns>The kerning amount or 0 if no kerning.</returns>
    API_FUNCTION() int32 GetKerning(Char first, Char second) const;

    /// <summary>
    /// Caches the given text to prepared for the rendering.
    /// </summary>
    /// <param name="text">The text witch characters to cache.</param>
    API_FUNCTION() void CacheText(const StringView& text);

    /// <summary>
    /// Invalidates all cached dynamic font atlases using this font. Can be used to reload font characters after changing font asset options.
    /// </summary>
    API_FUNCTION() void Invalidate();

public:
    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="layout">The layout properties.</param>
    /// <param name="outputLines">The output lines list.</param>
    void ProcessText(const StringView& text, Array<FontLineCache, InlinedAllocation<8>>& outputLines, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() Array<FontLineCache, InlinedAllocation<8>> ProcessText(const StringView& text, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        Array<FontLineCache, InlinedAllocation<8>> lines;
        ProcessText(text, lines, layout);
        return lines;
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() Array<FontLineCache, InlinedAllocation<8>> ProcessText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        Array<FontLineCache, InlinedAllocation<8>> lines;
        ProcessText(textRange.Substring(text), lines, layout);
        return lines;
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() FORCE_INLINE Array<FontLineCache, InlinedAllocation<8>> ProcessText(const StringView& text)
    {
        return ProcessText(text, TextLayoutOptions());
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() FORCE_INLINE Array<FontLineCache, InlinedAllocation<8>> ProcessText(const StringView& text, API_PARAM(Ref) const TextRange& textRange)
    {
        return ProcessText(textRange.Substring(text), TextLayoutOptions());
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The minimum size for that text and font to render properly.</returns>
    API_FUNCTION() Float2 MeasureText(const StringView& text, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The minimum size for that text and font to render properly.</returns>
    API_FUNCTION() Float2 MeasureText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        return MeasureText(textRange.Substring(text), layout);
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text
    /// </summary>.
    /// <param name="text">The input text to test.</param>
    /// <returns>The minimum size for that text and font to render properly.</returns>
    API_FUNCTION() FORCE_INLINE Float2 MeasureText(const StringView& text)
    {
        return MeasureText(text, TextLayoutOptions());
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text
    /// </summary>.
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <returns>The minimum size for that text and font to render properly.</returns>
    API_FUNCTION() FORCE_INLINE Float2 MeasureText(const StringView& text, API_PARAM(Ref) const TextRange& textRange)
    {
        return MeasureText(textRange.Substring(text), TextLayoutOptions());
    }

    /// <summary>
    /// Calculates hit character index at given location.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="location">The input location to test.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
    API_FUNCTION() int32 HitTestText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, const Float2& location, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        return HitTestText(textRange.Substring(text), location, layout);
    }

    /// <summary>
    /// Calculates hit character index at given location.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="location">The input location to test.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
    API_FUNCTION() int32 HitTestText(const StringView& text, const Float2& location, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Calculates hit character index at given location.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="location">The input location to test.</param>
    /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
    API_FUNCTION() FORCE_INLINE int32 HitTestText(const StringView& text, const Float2& location)
    {
        return HitTestText(text, location, TextLayoutOptions());
    }

    /// <summary>
    /// Calculates hit character index at given location.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="location">The input location to test.</param>
    /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
    API_FUNCTION() FORCE_INLINE int32 HitTestText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, const Float2& location)
    {
        return HitTestText(textRange.Substring(text), location, TextLayoutOptions());
    }

    /// <summary>
    /// Calculates character position for given text and character index.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="index">The text position to get coordinates of.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
    API_FUNCTION() Float2 GetCharPosition(const StringView& text, int32 index, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Calculates character position for given text and character index.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="index">The text position to get coordinates of.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
    API_FUNCTION() Float2 GetCharPosition(const StringView& text, API_PARAM(Ref) const TextRange& textRange, int32 index, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        return GetCharPosition(textRange.Substring(text), index, layout);
    }

    /// <summary>
    /// Calculates character position for given text and character index
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="index">The text position to get coordinates of.</param>
    /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
    API_FUNCTION() FORCE_INLINE Float2 GetCharPosition(const StringView& text, int32 index)
    {
        return GetCharPosition(text, index, TextLayoutOptions());
    }

    /// <summary>
    /// Calculates character position for given text and character index
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="index">The text position to get coordinates of.</param>
    /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
    API_FUNCTION() FORCE_INLINE Float2 GetCharPosition(const StringView& text, API_PARAM(Ref) const TextRange& textRange, int32 index)
    {
        return GetCharPosition(textRange.Substring(text), index, TextLayoutOptions());
    }

    /// <summary>
    /// Flushes the size of the face with the Free Type library backend.
    /// </summary>
    void FlushFaceSize() const;

public:
    // [Object]
    String ToString() const override;
};
