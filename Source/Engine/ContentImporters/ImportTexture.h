// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Graphics/Textures/TextureBase.h"

/// <summary>
/// Enable/disable caching texture import options
/// </summary>
#define IMPORT_TEXTURE_CACHE_OPTIONS 1

/// <summary>
/// Importing textures utility
/// </summary>
class ImportTexture
{
public:
    typedef TextureTool::Options Options;

public:
    /// <summary>
    /// Tries the get texture import options from the target location asset.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="options">The options.</param>
    /// <returns>True if success, otherwise false.</returns>
    static bool TryGetImportOptions(const StringView& path, Options& options);

    /// <summary>
    /// Imports texture, cube texture or sprite atlas.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Import(CreateAssetContext& context);

    /// <summary>
    /// Creates the Texture. Argument must be TextureData*.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportAsTextureData(CreateAssetContext& context);

    /// <summary>
    /// Creates the Texture. Argument must be TextureBase::InitData*.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportAsInitData(CreateAssetContext& context);

    /// <summary>
    /// Creates the Texture asset from the given data.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <param name="textureData">The texture data.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context, TextureData* textureData);

    /// <summary>
    /// Creates the Texture asset from the given data.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <param name="initData">The texture data.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context, TextureBase::InitData* initData);

    /// <summary>
    /// Imports the Cube Texture.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportCube(CreateAssetContext& context);

    /// <summary>
    /// Creates the Cube Texture asset from the given data.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <param name="textureData">The cube texture data.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult CreateCube(CreateAssetContext& context, TextureData* textureData);

    /// <summary>
    /// Imports the IES Profile file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult ImportIES(CreateAssetContext& context);

private:
    static void InitOptions(CreateAssetContext& context, Options& options);
    static CreateAssetResult Create(CreateAssetContext& context, const TextureData& textureData, Options& options);
    static CreateAssetResult Create(CreateAssetContext& context, const TextureBase::InitData& textureData, Options& options);
};

#endif
