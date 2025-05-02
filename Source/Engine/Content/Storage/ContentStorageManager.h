// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxStorageReference.h"
#include "Engine/Core/Types/TimeSpan.h"

class FlaxFile;
class FlaxPackage;

/// <summary>
/// Content Storage Manager is responsible for content data management
/// </summary>
class FLAXENGINE_API ContentStorageManager
{
public:
    /// <summary>
    /// Auto-release timeout for unused asset chunks.
    /// </summary>
    static TimeSpan UnusedDataChunksLifetime;

public:
    /// <summary>
    /// Gets the assets data storage container.
    /// </summary>
    /// <param name="path">The path.</param>
    /// <param name="loadIt">True if load container, otherwise false.</param>
    /// <returns>Flax Storage or null if not package at that location</returns>
    static FlaxStorageReference GetStorage(const StringView& path, bool loadIt = true);

    /// <summary>
    /// Tries the assets data storage container if it's created.
    /// </summary>
    /// <param name="path">The path.</param>
    /// <returns>Flax Storage or null in no package at that location has been opened.</returns>
    static FlaxStorageReference TryGetStorage(const StringView& path);

    /// <summary>
    /// Ensures the access to the given file location is free. Closes handles to that file from packages.
    /// </summary>
    /// <param name="path">The path.</param>
    /// <returns>Flax Storage or null in no package at that location has been opened.</returns>
    static FlaxStorageReference EnsureAccess(const StringView& path);

    /// <summary>
    /// Gets total memory used by chunks (in bytes).
    /// </summary>
    /// <returns>Memory usage in bytes.</returns>
    static uint32 GetMemoryUsage();

    /// <summary>
    /// Determines whether the specified asset exists in this container.
    /// </summary>
    /// <param name="id">The asset identifier.</param>
    /// <returns>True if the specified asset exists in this container, otherwise false.</returns>
    static bool HasAsset(const Guid& id);

    /// <summary>
    /// Gets the asset entry in the storage at the given path (if it has one stored item).
    /// </summary>
    /// <param name="path">The path.</param>
    /// <param name="output">The output.</param>
    /// <returns>True if cannot load storage container or it has more than one entry inside.</returns>
    static bool GetAssetEntry(const String& path, FlaxStorage::Entry& output);

    /// <summary>
    /// Called when asset ges renamed.
    /// </summary>
    /// <param name="oldPath">The old path.</param>
    /// <param name="newPath">The new path.</param>
    static void OnRenamed(const StringView& oldPath, const StringView& newPath);

    /// <summary>
    /// Ensures that storage manager is unlocked (by stopping the thread if its locked).
    /// </summary>
    static void EnsureUnlocked();

    // Formats path into valid format used by the storage system (normalized and absolute).
    static void FormatPath(String& path);

public:
    /// <summary>
    /// Determines whether the specified path can be a binary asset file (based on it's extension).
    /// </summary>
    /// <param name="path">The path.</param>
    /// <returns>True if can be a storage path (has a proper extension), otherwise false.</returns>
    static bool IsFlaxStoragePath(const String& path);

    /// <summary>
    /// Determines whether the specified extension can be a binary asset file.
    /// </summary>
    /// <param name="extension">The path.</param>
    /// <returns>True if can be a storage extension, otherwise false.</returns>
    static bool IsFlaxStorageExtension(const String& extension);

public:
    /// <summary>
    /// Gets the packages.
    /// </summary>
    /// <param name="result">The result.</param>
    static void GetPackages(Array<FlaxPackage*>& result);

    /// <summary>
    /// Gets the files.
    /// </summary>
    /// <param name="result">The result.</param>
    static void GetFiles(Array<FlaxFile*>& result);

    /// <summary>
    /// Gets the storage containers (packages and files).
    /// </summary>
    /// <param name="result">The result.</param>
    static void GetStorage(Array<FlaxStorage*>& result);
};
