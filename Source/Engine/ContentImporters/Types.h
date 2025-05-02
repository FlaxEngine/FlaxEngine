// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Config.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Enums.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Content/Storage/FlaxFile.h"

class JsonWriter;
class CreateAssetContext;

/// <summary>
/// Create/Import new asset callback result
/// </summary>
DECLARE_ENUM_8(CreateAssetResult, Ok, Abort, Error, CannotSaveFile, InvalidPath, CannotAllocateChunk, InvalidTypeID, Skip);

/// <summary>
/// Create/Import new asset callback function
/// </summary>
typedef Function<CreateAssetResult(CreateAssetContext&)> CreateAssetFunction;

/// <summary>
/// Importing/creating asset context structure
/// </summary>
class FLAXENGINE_API CreateAssetContext : public NonCopyable
{
private:
    CreateAssetResult _applyChangesResult;

public:
    /// <summary>
    /// Path of the input file (may be empty if creating new asset)
    /// </summary>
    String InputPath;

    /// <summary>
    /// Output file path
    /// </summary>
    String OutputPath;

    /// <summary>
    /// Target asset path (may be different than OutputPath)
    /// </summary>
    String TargetAssetPath;

    /// <summary>
    /// Asset file data container
    /// </summary>
    AssetInitData Data;

    /// <summary>
    /// True if skip the default asset import metadata added by the importer. May generate unwanted version control diffs.
    /// </summary>
    bool SkipMetadata;

    /// <summary>
    /// Custom argument for the importing function
    /// </summary>
    void* CustomArg;

    // TODO: add Progress(float progress) to notify operation progress
    // TODO: add cancellation feature - so progress can be aborted on demand

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CreateAssetContext"/> class.
    /// </summary>
    /// <param name="inputPath">The input path.</param>
    /// <param name="outputPath">The output path.</param>
    /// <param name="id">The identifier.</param>
    /// <param name="arg">The custom argument.</param>
    CreateAssetContext(const StringView& inputPath, const StringView& outputPath, const Guid& id, void* arg);

    /// <summary>
    /// Finalizes an instance of the <see cref="CreateAssetContext"/> class.
    /// </summary>
    ~CreateAssetContext()
    {
    }

public:
    /// <summary>
    /// Runs the specified callback.
    /// </summary>
    /// <param name="callback">The import/create asset callback.</param>
    /// <returns>Operation result.</returns>
    CreateAssetResult Run(const CreateAssetFunction& callback);

public:
    /// <summary>
    /// Allocates the chunk in the output data so upgrader can write to it.
    /// </summary>
    /// <param name="index">The index of the chunk.</param>
    /// <returns>True if cannot allocate it.</returns>
    bool AllocateChunk(int32 index);

    /// <summary>
    /// Adds the meta to the writer.
    /// </summary>
    /// <param name="writer">The json metadata writer.</param>
    void AddMeta(JsonWriter& writer) const;

private:
    void ApplyChanges();
};

/// <summary>
/// Asset importer entry
/// </summary>
struct FLAXENGINE_API AssetImporter
{
public:
    /// <summary>
    /// Extension of the file to import with that importer (without leading dot).
    /// </summary>
    String FileExtension;

    /// <summary>
    /// Extension of the output file as output with that importer (without leading dot).
    /// </summary>
    String ResultExtension;

    /// <summary>
    /// Callback for the asset importing process.
    /// </summary>
    CreateAssetFunction Callback;
};

/// <summary>
/// Asset creator entry
/// </summary>
struct FLAXENGINE_API AssetCreator
{
public:
    /// <summary>
    /// Asset creators are identifiable by tag
    /// </summary>
    String Tag;

    /// <summary>
    /// Call asset creating process
    /// </summary>
    CreateAssetFunction Callback;
};

#define IMPORT_SETUP(type, serializedVersion) context.Data.Header.TypeName = type::TypeName; context.Data.SerializedVersion = serializedVersion;

#endif
