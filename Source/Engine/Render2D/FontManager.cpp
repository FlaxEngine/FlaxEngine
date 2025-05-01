// Copyright (c) Wojciech Figat. All rights reserved.

#include "FontManager.h"
#include "FontTextureAtlas.h"
#include "FontAsset.h"
#include "Font.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "IncludeFreeType.h"
#include <ThirdParty/freetype/ftsynth.h>
#include <ThirdParty/freetype/ftbitmap.h>
#include <ThirdParty/freetype/internal/ftdrv.h>

namespace FontManagerImpl
{
    FT_Library Library;
    CriticalSection Locker;
    Array<AssetReference<FontTextureAtlas>> Atlases;
    Array<byte> GlyphImageData;
}

using namespace FontManagerImpl;

class FontManagerService : public EngineService
{
public:
    FontManagerService()
        : EngineService(TEXT("Font Manager"), -700)
    {
    }

    bool Init() override;
    void Dispose() override;
};

FontManagerService FontManagerServiceInstance;

float FontManager::FontScale = 1.0f;

FT_Library FontManager::GetLibrary()
{
    return Library;
}

FT_MemoryRec_ FreeTypeMemory;

static void* FreeTypeAlloc(FT_Memory memory, long size)
{
    return Allocator::Allocate(size);
}

void* FreeTypeRealloc(FT_Memory memory, long oldSize, long newSize, void* ptr)
{
    return AllocatorExt::Realloc(ptr, oldSize, newSize);
}

void FreeTypeFree(FT_Memory memory, void* ptr)
{
    return Allocator::Free(ptr);
}

bool FontManagerService::Init()
{
    ASSERT(Library == nullptr);

    // Scale UI fonts to match the monitor DPI
    FontManager::FontScale = (float)Platform::GetDpi() / (float)DefaultDPI; // TODO: Adjust this at runtime

    // Init Free Type
    FreeTypeMemory.user = nullptr;
    FreeTypeMemory.alloc = &FreeTypeAlloc;
    FreeTypeMemory.realloc = &FreeTypeRealloc;
    FreeTypeMemory.free = &FreeTypeFree;
    const FT_Error error = FT_New_Library(&FreeTypeMemory, &Library);
    if (error)
    {
        LOG_FT_ERROR(error);
        return true;
    }
    FT_Add_Default_Modules(Library);

    // Log version info
    FT_Int major, minor, patch;
    FT_Library_Version(Library, &major, &minor, &patch);
    LOG(Info, "FreeType initialized, version: {0}.{1}.{2}", major, minor, patch);

    return false;
}

void FontManagerService::Dispose()
{
    // Release font atlases
    Atlases.Resize(0);

    // Clean library
    if (Library)
    {
        const FT_Error error = FT_Done_Library(Library);
        Library = nullptr;
        if (error)
        {
            LOG_FT_ERROR(error);
        }
    }
}

FontTextureAtlas* FontManager::GetAtlas(int32 index)
{
    return index >= 0 && index < Atlases.Count() ? Atlases.Get()[index].Get() : nullptr;
}

bool FontManager::AddNewEntry(Font* font, Char c, FontCharacterEntry& entry)
{
    ScopeLock lock(Locker);

    const FontAsset* asset = font->GetAsset();
    const FontOptions& options = asset->GetOptions();
    const FT_Face face = asset->GetFTFace();
    ASSERT(face != nullptr);
    font->FlushFaceSize();

    // Set load flags
    uint32 glyphFlags = FT_LOAD_NO_BITMAP;
    const bool useAA = EnumHasAnyFlags(options.Flags, FontFlags::AntiAliasing);
    if (useAA)
    {
        switch (options.Hinting)
        {
        case FontHinting::Auto:
            glyphFlags |= FT_LOAD_FORCE_AUTOHINT;
            break;
        case FontHinting::AutoLight:
            glyphFlags |= FT_LOAD_TARGET_LIGHT;
            break;
        case FontHinting::Monochrome:
            glyphFlags |= FT_LOAD_TARGET_MONO;
            break;
        case FontHinting::None:
            glyphFlags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
            break;
        case FontHinting::Default:
        default:
            glyphFlags |= FT_LOAD_TARGET_NORMAL;
            break;
        }
    }
    else
    {
        glyphFlags |= FT_LOAD_TARGET_MONO | FT_LOAD_FORCE_AUTOHINT;
    }

    // Get the index to the glyph in the font face
    const FT_UInt glyphIndex = FT_Get_Char_Index(face, c);
#if !BUILD_RELEASE
    if (glyphIndex == 0 && c >= '!')
    {
        LOG(Warning, "Font `{}` doesn't contain character `\\u{:x}`, consider choosing another font.", String(face->family_name), c);
    }
#endif

    // Init the character data
    Platform::MemoryClear(&entry, sizeof(entry));
    entry.Character = c;
    entry.Font = font;
    entry.IsValid = false;

    // Load the glyph
    const FT_Error error = FT_Load_Glyph(face, glyphIndex, glyphFlags);
    if (error)
    {
        LOG_FT_ERROR(error);
        return true;
    }

    // Handle special effects
    if (EnumHasAnyFlags(options.Flags, FontFlags::Bold))
    {
        FT_GlyphSlot_Embolden(face->glyph);
    }
    if (EnumHasAnyFlags(options.Flags, FontFlags::Italic))
    {
        FT_GlyphSlot_Oblique(face->glyph);
    }

    // Render glyph to the bitmap
    FT_GlyphSlot glyph = face->glyph;
    FT_Render_Glyph(glyph, useAA ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);

    FT_Bitmap* bitmap = &glyph->bitmap;
    FT_Bitmap tmpBitmap;
    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY)
    {
        // Convert the bitmap to 8bpp grayscale
        FT_Bitmap_New(&tmpBitmap);
        FT_Bitmap_Convert(Library, bitmap, &tmpBitmap, 4);
        bitmap = &tmpBitmap;
    }
    ASSERT(bitmap && bitmap->pixel_mode == FT_PIXEL_MODE_GRAY);

    // Fill the character data
    entry.AdvanceX = Convert26Dot6ToRoundedPixel<int16>(glyph->advance.x);
    entry.OffsetY = glyph->bitmap_top;
    entry.OffsetX = glyph->bitmap_left;
    entry.IsValid = true;
    entry.BearingY = Convert26Dot6ToRoundedPixel<int16>(glyph->metrics.horiBearingY);
    entry.Height = Convert26Dot6ToRoundedPixel<int16>(glyph->metrics.height);

    // Allocate memory
    const int32 glyphWidth = bitmap->width;
    const int32 glyphHeight = bitmap->rows;
    GlyphImageData.Clear();
    GlyphImageData.Resize(glyphWidth * glyphHeight);

    // End for empty glyphs
    if (GlyphImageData.IsEmpty())
    {
        entry.TextureIndex = MAX_uint8;
        if (bitmap == &tmpBitmap)
        {
            FT_Bitmap_Done(Library, bitmap);
            bitmap = nullptr;
        }
        return false;
    }

    // Copy glyph data after rasterization (row by row)
    for (int32 row = 0; row < glyphHeight; row++)
    {
        Platform::MemoryCopy(&GlyphImageData[row * glyphWidth], &bitmap->buffer[row * bitmap->pitch], glyphWidth);
    }

    // Normalize gray scale images not using 256 colors
    if (bitmap->num_grays != 256)
    {
        const int32 scale = 255 / (bitmap->num_grays - 1);
        for (byte& pixel : GlyphImageData)
        {
            pixel *= scale;
        }
    }

    // Free temporary bitmap if used
    if (bitmap == &tmpBitmap)
    {
        FT_Bitmap_Done(Library, bitmap);
        bitmap = nullptr;
    }

    // Find atlas for the character texture
    int32 atlasIndex = 0;
    const FontTextureAtlasSlot* slot = nullptr;
    for (; atlasIndex < Atlases.Count() && slot == nullptr; atlasIndex++)
        slot = Atlases[atlasIndex]->AddEntry(glyphWidth, glyphHeight, GlyphImageData);
    atlasIndex--;

    // Check if there is no atlas for this character
    if (!slot)
    {
        // Create new atlas
        auto atlas = Content::CreateVirtualAsset<FontTextureAtlas>();
        atlas->Setup(PixelFormat::R8_UNorm, FontTextureAtlas::PaddingStyle::PadWithZero);
        Atlases.Add(atlas);
        atlasIndex++;

        // Init atlas
        const int32 fontAtlasSize = 512; // TODO: make it a configuration variable
        atlas->Init(fontAtlasSize, fontAtlasSize);

        // Add the character to the texture
        slot = atlas->AddEntry(glyphWidth, glyphHeight, GlyphImageData);
    }
    if (slot == nullptr)
    {
        LOG(Error, "Cannot find free space in texture atlases for character '{0}' from font {1} {2}. Size: {3}x{4}", c, String(face->family_name), String(face->style_name), glyphWidth, glyphHeight);
        return true;
    }

    // Fill with atlas dependant data
    entry.TextureIndex = (byte)atlasIndex;
    entry.UV.X = (float)slot->X;
    entry.UV.Y = (float)slot->Y;
    entry.UVSize.X = (float)slot->Width;
    entry.UVSize.Y = (float)slot->Height;
    entry.Slot = slot;

    return false;
}

void FontManager::Invalidate(FontCharacterEntry& entry)
{
    if (entry.TextureIndex == MAX_uint8)
        return;
    auto atlas = Atlases[entry.TextureIndex];
    atlas->Invalidate(entry.Slot);
}

void FontManager::Flush()
{
    for (const auto& atlas : Atlases)
    {
        atlas->Flush();
    }
}

void FontManager::EnsureAtlasCreated(int32 index)
{
    Atlases[index]->EnsureTextureCreated();
}

bool FontManager::IsDirty()
{
    for (const auto atlas : Atlases)
    {
        if (atlas->IsDirty())
        {
            return true;
        }
    }
    return false;
}

bool FontManager::HasDataSyncWithGPU()
{
    for (const auto atlas : Atlases)
    {
        if (atlas->HasDataSyncWithGPU() == false)
        {
            return false;
        }
    }
    return true;
}
