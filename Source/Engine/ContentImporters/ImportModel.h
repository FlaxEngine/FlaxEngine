// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Tools/ModelTool/ModelTool.h"

/// <summary>
/// Enable/disable caching model import options
/// </summary>
#define IMPORT_MODEL_CACHE_OPTIONS 1

/// <summary>
/// Importing models utility
/// </summary>
class ImportModelFile
{
public:
    typedef ModelTool::Options Options;

public:
    /// <summary>
    /// Tries the get model import options from the target location asset.
    /// </summary>
    /// <param name="path">The asset path.</param>
    /// <param name="options">The options.</param>
    /// <returns>True if success, otherwise false.</returns>
    static bool TryGetImportOptions(String path, Options& options);

    /// <summary>
    /// Imports the model file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Import(CreateAssetContext& context);

    /// <summary>
    /// Creates the model asset from the ModelData storage (input argument should be pointer to ModelData).
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context);

private:
    static CreateAssetResult ImportModel(CreateAssetContext& context, ModelData& modelData, const Options* options = nullptr);
    static CreateAssetResult ImportSkinnedModel(CreateAssetContext& context, ModelData& modelData, const Options* options = nullptr);
    static CreateAssetResult ImportAnimation(CreateAssetContext& context, ModelData& modelData, const Options* options = nullptr);
};

#endif
