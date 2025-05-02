// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

class Font;
class FontAsset;
class FontTextureAtlas;
struct FontCharacterEntry;
typedef struct FT_LibraryRec_* FT_Library;

/// <summary>
/// Fonts management and character atlases management utility service.
/// </summary>
class FLAXENGINE_API FontManager
{
public:

    /// <summary>
    /// The global characters font scale factor. Used to upscale characters on high-DPI monitors.
    /// </summary>
    static float FontScale;

    /// <summary>
    /// Gets the FreeType library.
    /// </summary>
    /// <returns>The library.</returns>
    static FT_Library GetLibrary();

    /// <summary>
    /// Gets the texture atlas.
    /// </summary>
    /// <param name="index">The atlas index.</param>
    /// <returns>The texture atlas.</returns>
    static FontTextureAtlas* GetAtlas(int32 index);

    /// <summary>
    /// Adds character from given font to the cache.
    /// </summary>
    /// <param name="font">The font to create character entry for it.</param>
    /// <param name="c">The character to add.</param>
    /// <param name="entry">The created character entry.</param>
    /// <returns>True if cannot add new character entry to the font cache, otherwise false.</returns>
    static bool AddNewEntry(Font* font, Char c, FontCharacterEntry& entry);

    /// <summary>
    /// Invalidates the cached dynamic font character. Can be used to reload font characters after changing font asset options.
    /// </summary>
    /// <param name="entry">The font character entry.</param>
    static void Invalidate(FontCharacterEntry& entry);

    /// <summary>
    /// Flushes all font atlases.
    /// </summary>
    static void Flush();

    // Ensure that atlas with given index has been created
    static void EnsureAtlasCreated(int32 index);

    /// <summary>
    /// Determines whether one or more font atlases is dirty.
    /// </summary>
    /// <returns><c>true</c> if one or more font atlases is dirty; otherwise, <c>false</c>.</returns>
    static bool IsDirty();

    /// <summary>
    /// Determines whether all atlases has been synced with the GPU memory and data is up to date.
    /// </summary>
    /// <returns><c>true</c> if all atlases has been synced with the GPU memory and data is up to date; otherwise, <c>false</c>.</returns>
    static bool HasDataSyncWithGPU();
};
