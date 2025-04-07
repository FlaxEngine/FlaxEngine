// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Types.h"

/// <summary>
/// Assets Importing service allows to import or create new assets
/// </summary>
class FLAXENGINE_API AssetsImportingManager
{
public:
    /// <summary>
    /// The asset importers.
    /// </summary>
    static Array<AssetImporter> Importers;

    /// <summary>
    /// The asset creators.
    /// </summary>
    static Array<AssetCreator> Creators;

    /// <summary>
    /// If true store asset import path relative to the current workspace, otherwise will store absolute path.
    /// </summary>
    static bool UseImportPathRelative;

public:
    /// <summary>
    /// The create texture tag (using internal import pipeline to crate texture asset from custom image source).
    /// </summary>
    static const String CreateTextureTag;

    /// <summary>
    /// The create texture from raw data. Argument must be TextureData*.
    /// </summary>
    static const String CreateTextureAsTextureDataTag;

    /// <summary>
    /// The create texture from raw data. Argument must be TextureBase::InitData*.
    /// </summary>
    static const String CreateTextureAsInitDataTag;

    /// <summary>
    /// The create material tag.
    /// </summary>
    static const String CreateMaterialTag;

    /// <summary>
    /// The create material tag.
    /// </summary>
    static const String CreateMaterialInstanceTag;

    /// <summary>
    /// The create cube texture tag. Argument must be TextureData*.
    /// </summary>
    static const String CreateCubeTextureTag;

    /// <summary>
    /// The create model tag. Argument must be ModelData*.
    /// </summary>
    static const String CreateModelTag;

    /// <summary>
    /// The create raw data asset tag. Argument must be BytesContainer*.
    /// </summary>
    static const String CreateRawDataTag;

    /// <summary>
    /// The create collision data asset tag.
    /// </summary>
    static const String CreateCollisionDataTag;

    /// <summary>
    /// The create animation graph asset tag.
    /// </summary>
    static const String CreateAnimationGraphTag;

    /// <summary>
    /// The create skeleton mask asset tag.
    /// </summary>
    static const String CreateSkeletonMaskTag;

    /// <summary>
    /// The create particle emitter asset tag.
    /// </summary>
    static const String CreateParticleEmitterTag;

    /// <summary>
    /// The create particle system asset tag.
    /// </summary>
    static const String CreateParticleSystemTag;

    /// <summary>
    /// The create scene animation asset tag.
    /// </summary>
    static const String CreateSceneAnimationTag;

    /// <summary>
    /// The create material function asset tag.
    /// </summary>
    static const String CreateMaterialFunctionTag;

    /// <summary>
    /// The create particle graph function asset tag.
    /// </summary>
    static const String CreateParticleEmitterFunctionTag;

    /// <summary>
    /// The create animation graph function asset tag.
    /// </summary>
    static const String CreateAnimationGraphFunctionTag;

    /// <summary>
    /// The create animation asset tag.
    /// </summary>
    static const String CreateAnimationTag;

    /// <summary>
    /// The create Behavior Tree asset tag.
    /// </summary>
    static const String CreateBehaviorTreeTag;

    /// <summary>
    /// The create visual script asset tag.
    /// </summary>
    static const String CreateVisualScriptTag;

public:
    /// <summary>
    /// Gets the asset importer by file extension.
    /// </summary>
    /// <param name="extension">The file extension.</param>
    /// <returns>Importer or null if not found.</returns>
    static const AssetImporter* GetImporter(const String& extension);

    /// <summary>
    /// Gets the asset creator by tag.
    /// </summary>
    /// <param name="tag">The tag.</param>
    /// <returns>Creator or null if not found.</returns>
    static const AssetCreator* GetCreator(const String& tag);

public:
    /// <summary>
    /// Creates new asset.
    /// </summary>
    /// <param name="importFunc">The import function.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="assetId">The asset identifier. If valid then used as new asset id. Set to the actual asset id after import.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Create(const CreateAssetFunction& importFunc, const StringView& outputPath, Guid& assetId, void* arg = nullptr);

    /// <summary>
    /// Creates new asset.
    /// </summary>
    /// <param name="importFunc">The import function.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Create(const CreateAssetFunction& importFunc, const StringView& outputPath, void* arg = nullptr)
    {
        Guid id = Guid::Empty;
        return Create(importFunc, outputPath, id, arg);
    }

    /// <summary>
    /// Creates new asset.
    /// </summary>
    /// <param name="tag">The asset type tag.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="assetId">The asset identifier.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Create(const String& tag, const StringView& outputPath, Guid& assetId, void* arg = nullptr);

    /// <summary>
    /// Creates new asset.
    /// </summary>
    /// <param name="tag">The asset type tag.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Create(const String& tag, const StringView& outputPath, void* arg = nullptr)
    {
        Guid id = Guid::Empty;
        return Create(tag, outputPath, id, arg);
    }

    /// <summary>
    /// Imports file and creates asset. If asset already exists overwrites it's contents.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="assetId">The asset identifier. If valid then used as new asset id. Set to the actual asset id after import.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Import(const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg = nullptr);

    /// <summary>
    /// Imports file and creates asset. If asset already exists overwrites it's contents.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Import(const StringView& inputPath, const StringView& outputPath, void* arg = nullptr)
    {
        Guid id = Guid::Empty;
        return Import(inputPath, outputPath, id, arg);
    }

    /// <summary>
    /// Imports file and creates asset only if source file has been modified. If asset already exists overwrites it's contents.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="assetId">The asset identifier. If valid then used as new asset id. Set to the actual asset id after import.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportIfEdited(const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg = nullptr);

    /// <summary>
    /// Imports file and creates asset only if source file has been modified. If asset already exists overwrites it's contents.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportIfEdited(const StringView& inputPath, const StringView& outputPath, void* arg = nullptr)
    {
        Guid id = Guid::Empty;
        return ImportIfEdited(inputPath, outputPath, id, arg);
    }

    // Converts source files path into the relative format if enabled by the project settings. Result path can be stored in asset for reimports.
    static String GetImportPath(const String& path);

private:
    static bool Create(const CreateAssetFunction& callback, const StringView& inputPath, const StringView& outputPath, Guid& assetId, void* arg);
};

#endif
