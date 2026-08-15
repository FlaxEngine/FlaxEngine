// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Math/Vector2.h"

class Font;
class FontManager;
struct FontTextureAtlasSlot;
typedef struct FT_FaceRec_* FT_Face;

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
/// The font hinting used when rendering characters.
/// </summary>
API_ENUM() enum class FontHinting : byte
{
    /// <summary>
    /// Use the default hinting specified in the font.
    /// </summary>
    Default,

    /// <summary>
    /// Force the use of an automatic hinting algorithm (over the fonts native hinter).
    /// </summary>
    Auto,

    /// <summary>
    /// Force the use of an automatic light hinting algorithm, optimized for non-monochrome displays.
    /// </summary>
    AutoLight,

    /// <summary>
    /// Force the use of an automatic hinting algorithm optimized for monochrome displays.
    /// </summary>
    Monochrome,

    /// <summary>
    /// Do not use hinting. This generally generates 'blurrier' bitmap glyphs when the glyphs are rendered in any of the anti-aliased modes.
    /// </summary>
    None,
};

/// <summary>
/// The font flags used when rendering characters.
/// </summary>
API_ENUM(Attributes="Flags") enum class FontFlags : byte
{
    /// <summary>
    /// No options.
    /// </summary>
    None = 0,

    /// <summary>
    /// Enables using anti-aliasing for font characters. Otherwise font will use the monochrome data.
    /// </summary>
    AntiAliasing = 1,

    /// <summary>
    /// Enables artificial embolden effect.
    /// </summary>
    Bold = 2,

    /// <summary>
    /// Enables slant effect, emulating italic style.
    /// </summary>
    Italic = 4,
};

/// <summary>
/// The font rasterization mode.
/// </summary>
API_ENUM() enum class FontRasterMode : byte
{
    /// <summary>
    /// Use the default FreeType rasterizer to render font atlases.
    /// </summary>
    Bitmap,

    /// <summary>
    /// Use the Multi-channel Signed Distance Field (MSDF) generator to render font atlases. Need to be rendered with a compatible material.
    /// </summary>
    MSDF,
};

DECLARE_ENUM_OPERATORS(FontFlags);

/// <summary>
/// The font asset options.
/// </summary>
API_STRUCT() struct FontOptions
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(FontOptions);

    /// <summary>
    /// The font hinting used when rendering characters.
    /// </summary>
    API_FIELD() FontHinting Hinting;

    /// <summary>
    /// The flags.
    /// </summary>
    API_FIELD() FontFlags Flags;

    /// <summary>
    /// The font rasterization mode.
    /// </summary>
    API_FIELD() FontRasterMode RasterMode;

    /// <summary>
    /// The font size used when generating MSDF font atlases.
    /// </summary>
    API_FIELD() float MSDFSize;
};

/// <summary>
/// Font asset contains glyph collection and cached data used to render text.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API FontAsset : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(FontAsset, 5);
    friend Font;

private:
    FT_Face _face;
    FontOptions _options;
    BytesContainer _fontFile;
    Array<Font*, InlinedAllocation<32>> _fonts;
    Dictionary<Pair<float, Char>, FontCharacterEntry> _characterCache;
    AssetReference<FontAsset> _virtualBold;
    AssetReference<FontAsset> _virtualItalic;
    AssetReference<FontAsset> _virtualRasterMode;

public:
    /// <summary>
    /// Gets the font family name.
    /// </summary>
    API_PROPERTY() String GetFamilyName() const;

    /// <summary>
    /// Gets the font style name.
    /// </summary>
    API_PROPERTY() String GetStyleName() const;

    /// <summary>
    /// Gets FreeType face handle.
    /// </summary>
    FORCE_INLINE FT_Face GetFTFace() const
    {
        return _face;
    }

    /// <summary>
    /// Gets the font options.
    /// </summary>
    API_PROPERTY() const FontOptions& GetOptions() const
    {
        return _options;
    }

    /// <summary>
    /// Gets the font style flags.
    /// </summary>
    API_PROPERTY() FontFlags GetStyle() const;

    /// <summary>
    /// Sets the font options.
    /// </summary>
    API_PROPERTY() void SetOptions(const FontOptions& value);

public:
    /// <summary>
    /// Creates the font object of given characters size.
    /// </summary>
    /// <param name="size">The font characters size.</param>
    /// <returns>The created font object.</returns>
    API_FUNCTION() Font* CreateFont(float size);

    /// <summary>
    /// Gets the font with bold style. Returns itself or creates a new virtual font asset using this font but with bold option enabled.
    /// </summary>
    /// <returns>The virtual font or this.</returns>
    API_FUNCTION() FontAsset* GetBold();

    /// <summary>
    /// Gets the font with italic style. Returns itself or creates a new virtual font asset using this font but with italic option enabled.
    /// </summary>
    /// <returns>The virtual font or this.</returns>
    API_FUNCTION() FontAsset* GetItalic();

    /// <summary>
    /// Gets the different rasterization mode of the font. Returns itself or creates a new virtual font asset using this font but rasterized with the specified mode.
    /// </summary>
    /// <returns>The virtual font or this.</returns>
    API_FUNCTION() FontAsset* GetRasterMode(FontRasterMode rasterMode);

    /// <summary>
    /// Initializes the font with a custom font file data.
    /// </summary>
    /// <param name="fontFile">Raw bytes with font file data.</param>
    /// <returns>True if cannot init, otherwise false.</returns>
    API_FUNCTION() bool Init(const BytesContainer& fontFile);

    /// <summary>
    /// Check if the font contains the glyph of a char.
    /// </summary>
    /// <param name="c">The char to test.</param>
    /// <returns>True if the font contains the glyph of the char, otherwise false.</returns>
    API_FUNCTION() bool ContainsChar(Char c) const;

    /// <summary>
    /// Invalidates all cached dynamic font atlases using this font. Can be used to reload font characters after changing font asset options.
    /// </summary>
    API_FUNCTION() void Invalidate();

public:
    // [BinaryAsset]
    uint64 GetMemoryUsage() const override;
#if USE_EDITOR
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [BinaryAsset]
    bool init(AssetInitData& initData) override;
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;

private:
    bool Init();
};
