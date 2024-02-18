#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Font.h"
#include "FontAsset.h"

struct TextRange;
class Font;
class FontAsset;

/// <summary>
/// Defines a list of fonts that can be used as a fallback, ordered by priority. 
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API FontFallbackList : public ManagedScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(FontFallbackList);
private:
    Array<FontAsset*> _fontAssets;

    // Cache fallback fonts of various sizes
    Dictionary<float, Array<Font*>*> _cache;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="FontFallbackList"/> class.
    /// </summary>
    /// <param name="fonts">The fallback font assets.</param>
    FontFallbackList(const Array<FontAsset*>& fonts);

    /// <summary>
    /// Initializes a new instance of the <see cref="FontFallbackList"/> class, exposed for C#.
    /// </summary>
    /// <param name="fonts">The fallback font assets.</param>
    /// <returns>The new instance.</returns>
    API_FUNCTION() FORCE_INLINE static FontFallbackList* Create(const Array<FontAsset*>& fonts) {
        return New<FontFallbackList>(fonts);
    }

    /// <summary>
    /// Get the parent assets of fallback fonts.
    /// </summary>
    /// <returns>The font assets.</returns>
    API_PROPERTY() FORCE_INLINE Array<FontAsset*>& GetFonts() {
        return _fontAssets;
    }

    /// <summary>
    /// Set the fallback fonts.
    /// </summary>
    /// <param name="val">The parent assets of the new fonts.</param>
    API_PROPERTY() FORCE_INLINE void SetFonts(const Array<FontAsset*>& val) {
        _fontAssets = val;
    }

    /// <summary>
    /// Gets the fallback fonts with the given size.
    /// </summary>
    /// <param name="size">The size.</param>
    /// <returns>The generated fonts.</returns>
    API_FUNCTION() FORCE_INLINE Array<Font*>& GetFontList(float size) {
        Array<Font*>* result;
        if (_cache.TryGet(size, result)) {
            return *result;
        }

        result = New<Array<Font*>>(_fontAssets.Count());
        auto& arr = *result;
        for (int32 i = 0; i < _fontAssets.Count(); i++)
        {
            arr.Add(_fontAssets[i]->CreateFont(size));
        }

        _cache[size] = result;
        return *result;
    }


    /// <summary>
    /// Gets the index of the fallback font that should be used to render the char
    /// </summary>
    /// <param name="c">The char.</param>
    /// <param name="primaryFont">The primary font.</param>
    /// <param name="missing">The number to return if none of the fonts can render.</param>
    /// <returns>-1 if char can be rendered with primary font, index if it matches a fallback font. </returns>
    API_FUNCTION() FORCE_INLINE int32 GetCharFallbackIndex(Char c, Font* primaryFont = nullptr, int32 missing = -1) {
        if (primaryFont && primaryFont->GetAsset()->ContainsChar(c)) {
            return -1;
        }

        int32 fontIndex = 0;
        while (fontIndex < _fontAssets.Count() && _fontAssets[fontIndex] && !_fontAssets[fontIndex]->ContainsChar(c))
        {
            fontIndex++;
        }

        if (fontIndex < _fontAssets.Count()) {
            return fontIndex;

        }

        return missing;
    }

    /// <summary>
    /// Checks if every font is properly loaded.
    /// </summary>
    /// <returns>True if every font asset is non-null, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE bool Verify() {
        for (int32 i = 0; i < _fontAssets.Count(); i++)
        {
            if (!_fontAssets[i]) {
                return false;
            }
        }

        return true;
    }
};
