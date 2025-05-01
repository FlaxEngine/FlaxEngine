// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Types.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Assets Importing service allows to import or create new assets
/// </summary>
class FLAXENGINE_API AssetsExportingManager
{
public:
    /// <summary>
    /// The asset exporting callbacks. Identified by the asset typename.
    /// </summary>
    static Dictionary<String, ExportAssetFunction> Exporters;

public:
    /// <summary>
    /// Gets the asset export for thee given asset typename.
    /// </summary>
    /// <param name="typeName">The asset typename.</param>
    /// <returns>Exporter or null if not found.</returns>
    static const ExportAssetFunction* GetExporter(const String& typeName);

    /// <summary>
    /// Checks if the asset at the given location can be exports.
    /// </summary>
    /// <param name="inputPath">The input asset path.</param>
    /// <returns>True if can export it, otherwise false.</returns>
    static bool CanExport(const String& inputPath);

public:
    /// <summary>
    /// Exports the asset.
    /// </summary>
    /// <param name="inputPath">The input asset path.</param>
    /// <param name="outputFolder">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Export(const String& inputPath, const String& outputFolder, void* arg = nullptr);

    /// <summary>
    /// Exports the asset.
    /// </summary>
    /// <param name="callback">The custom callback.</param>
    /// <param name="inputPath">The input asset path.</param>
    /// <param name="outputFolder">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Export(const ExportAssetFunction& callback, const String& inputPath, const String& outputFolder, void* arg = nullptr);
};

#endif
