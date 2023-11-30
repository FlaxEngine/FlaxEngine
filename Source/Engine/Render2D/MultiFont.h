#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Font.h"
#include "FontAsset.h"

struct TextRange;
class Font;
class FontAsset;

/// <summary>
/// The font block info generated during text processing.
/// </summary>
API_STRUCT(NoDefault) struct MultiFontBlockCache
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MultiFontBlockCache);

    /// <summary>
    /// The root position of the block (upper left corner), relative to line.
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// The height of the current block
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

    /// <summary>
    /// The index of the font to render with
    /// </summary>
    API_FIELD() int32 FontIndex;
};

template<>
struct TIsPODType<MultiFontBlockCache>
{
    enum { Value = true };
};

/// <summary>
/// Line of font blocks info generated during text processing.
/// </summary>
API_STRUCT(NoDefault) struct MultiFontLineCache
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MultiFontLineCache);

    /// <summary>
    /// The root position of the line (upper left corner).
    /// </summary>
    API_FIELD() Float2 Location;

    /// <summary>
    /// The line bounds (width and height).
    /// </summary>
    API_FIELD() Float2 Size;

    /// <summary>
    /// The maximum ascender of the line. 
    /// </summary>
    API_FIELD() float MaxAscender;

    /// <summary>
    /// The index of the font to render with
    /// </summary>
    API_FIELD() Array<MultiFontBlockCache> Blocks;
};

API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API MultiFont : public ManagedScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(MultiFont);
private:
    Array<Font*> _fonts;

public:
    MultiFont(const Array<Font*>& fonts);

    API_FUNCTION() FORCE_INLINE static MultiFont* Create(const Array<Font*>& fonts) {
        return New<MultiFont>(fonts);
    }

    API_FUNCTION() FORCE_INLINE static MultiFont* Create(const Array<FontAsset*>& fontAssets, float size) {
        Array<Font*> fonts;
        fonts.Resize(fontAssets.Count());
        for (int32 i = 0; i < fontAssets.Count(); i++)
        {
            fonts[i] = fontAssets[i]->CreateFont(size);
        }

        return New<MultiFont>(fonts);
    }

    API_PROPERTY() FORCE_INLINE Array<Font*>& GetFonts() {
        return _fonts;
    }

    API_PROPERTY() FORCE_INLINE void SetFonts(const Array<Font*>& val) {
        _fonts = val;
    }

    API_PROPERTY() FORCE_INLINE int32 GetMaxHeight() {
        int32 maxHeight = 0;
        for (int32 i = 0; i < _fonts.Count(); i++)
        {
            if (_fonts[i]) {
                maxHeight = Math::Max(maxHeight, _fonts[i]->GetHeight());
            }
        }

        return maxHeight;
    }

    API_PROPERTY() FORCE_INLINE int32 GetMaxAscender() {
        int32 maxAsc = 0;
        for (int32 i = 0; i < _fonts.Count(); i++)
        {
            if (_fonts[i]) {
                maxAsc = Math::Max(maxAsc, _fonts[i]->GetAscender());
            }
        }

        return maxAsc;
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="layout">The layout properties.</param>
    /// <param name="outputLines">The output lines list.</param>
    void ProcessText(const StringView& text, Array<MultiFontLineCache>& outputLines, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() Array<MultiFontLineCache> ProcessText(const StringView& text, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        Array<MultiFontLineCache> lines;
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
    API_FUNCTION() Array<MultiFontLineCache> ProcessText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        Array<MultiFontLineCache> lines;
        ProcessText(textRange.Substring(text), lines, layout);
        return lines;
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() FORCE_INLINE Array<MultiFontLineCache> ProcessText(const StringView& text)
    {
        return ProcessText(text, TextLayoutOptions());
    }

    /// <summary>
    /// Processes text to get cached lines for rendering.
    /// </summary>
    /// <param name="text">The input text.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <returns>The output lines list.</returns>
    API_FUNCTION() FORCE_INLINE Array<MultiFontLineCache> ProcessText(const StringView& text, API_PARAM(Ref) const TextRange& textRange)
    {
        return ProcessText(textRange.Substring(text), TextLayoutOptions());
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The minimum size for that text and fot to render properly.</returns>
    API_FUNCTION() Float2 MeasureText(const StringView& text, API_PARAM(Ref) const TextLayoutOptions& layout);

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text.
    /// </summary>
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="layout">The layout properties.</param>
    /// <returns>The minimum size for that text and fot to render properly.</returns>
    API_FUNCTION() Float2 MeasureText(const StringView& text, API_PARAM(Ref) const TextRange& textRange, API_PARAM(Ref) const TextLayoutOptions& layout)
    {
        return MeasureText(textRange.Substring(text), layout);
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text
    /// </summary>.
    /// <param name="text">The input text to test.</param>
    /// <returns>The minimum size for that text and fot to render properly.</returns>
    API_FUNCTION() FORCE_INLINE Float2 MeasureText(const StringView& text)
    {
        return MeasureText(text, TextLayoutOptions());
    }

    /// <summary>
    /// Measures minimum size of the rectangle that will be needed to draw given text
    /// </summary>.
    /// <param name="text">The input text to test.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <returns>The minimum size for that text and fot to render properly.</returns>
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
    /// Gets the index of the font that should be used to render the char
    /// </summary>
    /// <param name="fonts">The font list.</param>
    /// <param name="c">The char.</param>
    /// <param name="missing">Number to return if char cannot be found.</param>
    /// <returns></returns>
    API_FUNCTION() FORCE_INLINE int32 GetCharFontIndex(Char c, int32 missing = -1) {
        int32 fontIndex = 0;
        while (fontIndex < _fonts.Count() && _fonts[fontIndex] && !_fonts[fontIndex]->ContainsChar(c))
        {
            fontIndex++;
        }

        if (fontIndex == _fonts.Count()) {
            return missing;
        }

        return fontIndex;
    }

    API_FUNCTION() FORCE_INLINE bool Verify() {
        for (int32 i = 0; i < _fonts.Count(); i++)
        {
            if (!_fonts[i]) {
                return false;
            }
        }

        return true;
    }
};
