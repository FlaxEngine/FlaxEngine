// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/TextureBase.h"

/// <summary>
/// Texture asset contains an image that is usually stored on a GPU and is used during rendering graphics.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Texture : public TextureBase
{
    DECLARE_BINARY_ASSET_HEADER(Texture, TexturesSerializedVersion);

    /// <summary>
    /// Gets the texture format type.
    /// </summary>
    TextureFormatType GetFormatType() const;

    /// <summary>
    /// Returns true if texture is a normal map.
    /// </summary>
    API_PROPERTY() bool IsNormalMap() const;

public:
#if USE_EDITOR
    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <param name="customData">The custom texture data container. Can be used to override the data stored in the asset. Use null to ignore this argument.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    bool Save(const StringView& path, const InitData* customData);
#endif

    /// <summary>
    /// Loads the texture from the image file. Supported file formats depend on a runtime platform. All platform support loading PNG, BMP, TGA, HDR and JPEG files.
    /// </summary>
    /// <remarks>Valid only for virtual assets.</remarks>
    /// <param name="path">The source image file path.</param>
    /// <param name="generateMips">True if generate mipmaps for the imported texture.</param>
    /// <returns>True if fails, otherwise false.</returns>
    API_FUNCTION() bool LoadFile(const StringView& path, bool generateMips = false);

    /// <summary>
    /// Loads the texture from the image file and creates the virtual texture asset for it. Supported file formats depend on a runtime platform. All platform support loading PNG, BMP, TGA, HDR and JPEG files.
    /// </summary>
    /// <param name="path">The source image file path.</param>
    /// <param name="generateMips">True if generate mipmaps for the imported texture.</param>
    /// <returns>The loaded texture (virtual asset) or null if fails.</returns>
    API_FUNCTION() static Texture* FromFile(const StringView& path, bool generateMips = false);

public:
    // [TextureBase]
#if USE_EDITOR
    bool Save(const StringView& path = StringView::Empty) override;
#endif
};
