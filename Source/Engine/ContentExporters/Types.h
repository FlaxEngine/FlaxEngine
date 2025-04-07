// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Engine/Content/Config.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Enums.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/String.h"

class JsonWriter;
class ExportAssetContext;

/// <summary>
/// Export asset callback result
/// </summary>
DECLARE_ENUM_6(ExportAssetResult, Ok, Abort, Error, CannotLoadAsset, MissingInputFile, CannotLoadData);

/// <summary>
/// Create/Import new asset callback function
/// </summary>
typedef Function<ExportAssetResult(ExportAssetContext&)> ExportAssetFunction;

/// <summary>
/// Exporting asset context structure
/// </summary>
class FLAXENGINE_API ExportAssetContext : public NonCopyable
{
public:
    /// <summary>
    /// The asset reference (prepared by the context to be used by callback).
    /// </summary>
    AssetReference<Asset> Asset;

    /// <summary>
    /// Path of the input file
    /// </summary>
    String InputPath;

    /// <summary>
    /// Recommended output filename
    /// </summary>
    String OutputFilename;

    /// <summary>
    /// Output file directory
    /// </summary>
    String OutputFolder;

    /// <summary>
    /// Custom argument for the importing function
    /// </summary>
    void* CustomArg;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ExportAssetContext"/> class.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The custom argument.</param>
    ExportAssetContext(const String& inputPath, const String& outputPath, void* arg);

    /// <summary>
    /// Finalizes an instance of the <see cref="ExportAssetContext"/> class.
    /// </summary>
    ~ExportAssetContext()
    {
    }

public:
    /// <summary>
    /// Runs the specified callback.
    /// </summary>
    /// <param name="callback">The export asset callback.</param>
    /// <returns>Operation result.</returns>
    ExportAssetResult Run(const ExportAssetFunction& callback);
};

#define GET_OUTPUT_PATH(context, extension) (context.OutputFolder / context.OutputFilename + TEXT("." extension))

#endif
