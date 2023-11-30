#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Font.h"
#include "FontAsset.h"

struct TextRange;
class Font;
class FontAsset;

API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API FallbackFonts : public ManagedScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(FallbackFonts);
private:
    /// <summary>
    /// The list of fallback fonts, ordered by priority. 
    /// The first element is reserved for the primary font, fallback fonts starts from the second element.
    /// </summary>
    Array<FontAsset*> _fontAssets;

    Dictionary<float, Array<Font*>*> _cache;

public:
    FallbackFonts(const Array<FontAsset*>& fonts);

    API_FUNCTION() FORCE_INLINE static FallbackFonts* Create(const Array<FontAsset*>& fonts) {
        return New<FallbackFonts>(fonts);
    }

    API_PROPERTY() FORCE_INLINE Array<FontAsset*>& GetFonts() {
        return _fontAssets;
    }

    API_PROPERTY() FORCE_INLINE void SetFonts(const Array<FontAsset*>& val) {
        _fontAssets = val;
    }

    /// <summary>
    /// Combine the primary fonts with the fallback fonts to get a font list
    /// </summary>
    API_PROPERTY() FORCE_INLINE Array<Font*>& GetFontList(float size) {
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
