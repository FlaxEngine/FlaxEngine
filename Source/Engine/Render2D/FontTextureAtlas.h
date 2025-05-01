// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Utilities/RectPack.h"

/// <summary>
/// Contains information about single texture atlas slot.
/// </summary>
struct FontTextureAtlasSlot : RectPackNode<>
{
    FontTextureAtlasSlot(Size x, Size y, Size width, Size height)
        : RectPackNode(x, y, width, height)
    {
    }

    void OnInsert()
    {
    }
};

/// <summary>
/// Texture resource that contains an atlas of cached font glyphs.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API FontTextureAtlas : public Texture
{
DECLARE_BINARY_ASSET_HEADER(FontTextureAtlas, TexturesSerializedVersion);
private:

    struct RowData
    {
        const byte* SrcData;
        uint8* DstData;
        int32 SrcRow;
        int32 DstRow;
        int32 SrcWidth;
        int32 DstWidth;
        uint32 Padding;
    };

public:

    /// <summary>
    /// Describes how to handle texture atlas padding
    /// </summary>
    enum PaddingStyle
    {
        /// <summary>
        /// Don't pad the atlas.
        /// </summary>
        NoPadding,

        /// <summary>
        /// Dilate the texture by one pixel to pad the atlas.
        /// </summary>
        DilateBorder,

        /// <summary>
        /// One pixel uniform padding border filled with zeros.
        /// </summary>
        PadWithZero,
    };

private:

    Array<byte> _data;
    uint32 _width;
    uint32 _height;
    PixelFormat _format;
    uint32 _bytesPerPixel;
    PaddingStyle _paddingStyle;
    bool _isDirty;
    RectPackAtlas<FontTextureAtlasSlot> _atlas;
    Array<FontTextureAtlasSlot*> _freeSlots;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="FontTextureAtlas"/> class.
    /// </summary>
    /// <param name="format">The texture pixels format.</param>
    /// <param name="paddingStyle">The texture entries padding style.</param>
    /// <param name="index">The atlas index.</param>
    FontTextureAtlas(PixelFormat format, PaddingStyle paddingStyle, int32 index);

public:

    /// <summary>
    /// Gets the atlas width.
    /// </summary>
    FORCE_INLINE uint32 GetWidth() const
    {
        return _width;
    }

    /// <summary>
    /// Gets the atlas height.
    /// </summary>
    FORCE_INLINE uint32 GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Gets the atlas size.
    /// </summary>
    FORCE_INLINE Float2 GetSize() const
    {
        return Float2(static_cast<float>(_width), static_cast<float>(_height));
    }

    /// <summary>
    /// Determines whether this atlas is dirty and data need to be flushed.
    /// </summary>
    FORCE_INLINE bool IsDirty() const
    {
        return _isDirty;
    }

    /// <summary>
    /// Gets padding style for textures in the atlas.
    /// </summary>
    FORCE_INLINE PaddingStyle GetPaddingStyle() const
    {
        return _paddingStyle;
    }

    /// <summary>
    /// Gets amount of pixels to pad textures inside an atlas.
    /// </summary>
    uint32 GetPaddingAmount() const;

public:

    /// <summary>
    /// Setups the atlas after creation.
    /// </summary>
    /// <param name="format">The pixel format.</param>
    /// <param name="paddingStyle">The padding style.</param>
    void Setup(PixelFormat format, PaddingStyle paddingStyle);

    /// <summary>
    /// Initializes the atlas.
    /// </summary>
    /// <param name="width">The width.</param>
    /// <param name="height">The height.</param>
    void Init(uint32 width, uint32 height);

    /// <summary>
    /// Adds the new entry to the atlas
    /// </summary>
    /// <param name="width">Width of the entry.</param>
    /// <param name="height">Height of the entry.</param>
    /// <param name="data">The data.</param>
    /// <returns>The atlas slot occupied by the new entry.</returns>
    FontTextureAtlasSlot* AddEntry(uint32 width, uint32 height, const Array<byte>& data);

    /// <summary>
    /// Invalidates the cached dynamic entry from the atlas.
    /// </summary>
    /// <param name="slot">The slot to invalidate.</param>
    /// <returns>True if slot has been freed, otherwise false.</returns>
    bool Invalidate(const FontTextureAtlasSlot* slot);

    /// <summary>
    /// Invalidates the cached dynamic entry from the atlas.
    /// </summary>
    /// <param name="x">The slot location (X coordinate in atlas pixels).</param>
    /// <param name="y">The slot location (Y coordinate in atlas pixels).</param>
    /// <param name="width">The slot width (size in atlas pixels).</param>
    /// <param name="height">The slot height (size in atlas pixels).</param>
    /// <returns>True if slot has been freed, otherwise false.</returns>
    bool Invalidate(uint32 x, uint32 y, uint32 width, uint32 height);

    /// <summary>
    /// Copies the data into the slot.
    /// </summary>
    /// <param name="slot">The slot.</param>
    /// <param name="data">The data.</param>
    void CopyDataIntoSlot(const FontTextureAtlasSlot* slot, const Array<byte>& data);

    /// <summary>
    /// Returns glyph's bitmap data of the slot.
    /// </summary>
	/// <param name="slot">The slot in atlas.</param>
	/// <param name="width">The width of the slot.</param>
    /// <param name="height">The height of the slot.</param>
    /// <param name="stride">The stride of the slot.</param>
    /// <returns>The pointer to the bitmap data of the given slot.</returns>
    byte* GetSlotData(const FontTextureAtlasSlot* slot, uint32& width, uint32& height, uint32& stride);

    /// <summary>
    /// Clears this atlas entries data (doesn't change size/texture etc.).
    /// </summary>
    void Clear();

    /// <summary>
    /// Flushes this atlas data to the GPU
    /// </summary>
    void Flush();

    /// <summary>
    /// Ensures that texture has been created for that atlas.
    /// </summary>
    void EnsureTextureCreated() const;

    /// <summary>
    /// Determines whether atlas has data synchronized with the GPU.
    /// </summary>
    /// <returns><c>true</c> if atlas has data synchronized with the GPU; otherwise, <c>false</c>.</returns>
    bool HasDataSyncWithGPU() const;

private:

    void markAsDirty();
    void copyRow(const RowData& copyRowData) const;
    void zeroRow(const RowData& copyRowData) const;

protected:

    // [Texture]
    void unload(bool isReloading) override;
};
