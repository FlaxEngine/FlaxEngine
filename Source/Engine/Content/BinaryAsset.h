// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Asset.h"
#include "Engine/Core/Types/Pair.h"
#include "Storage/FlaxStorageReference.h"

// Helper define
#define DECLARE_BINARY_ASSET_HEADER(type, serializedVersion) \
	DECLARE_ASSET_HEADER(type); \
	static const uint32 SerializedVersion = serializedVersion; \
	uint32 GetSerializedVersion() const override { return SerializedVersion; }

#define REGISTER_BINARY_ASSET_ABSTRACT(type, typeName) \
	const String type::TypeName = TEXT(typeName)

/// <summary>
/// Base class for all binary assets.
/// </summary>
/// <seealso cref="Asset" />
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API BinaryAsset : public Asset
{
    DECLARE_ASSET_HEADER(BinaryAsset);
protected:
    AssetHeader _header;
    FlaxStorageReference _storageRef; // Allow asset to have missing storage reference but only before asset is loaded or if it's virtual
    bool _isSaving;
    Array<BinaryAsset*> _dependantAssets;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="BinaryAsset"/> class.
    /// </summary>
    ~BinaryAsset();

public:
    /// <summary>
    /// The asset storage container.
    /// </summary>
    FlaxStorage* Storage;

#if USE_EDITOR
    /// <summary>
    /// The asset metadata information. Stored in a Json format.
    /// </summary>
    BytesContainer Metadata;

    /// <summary>
    /// Asset dependencies list used by the asset for tracking (eg. material functions used by material asset). The pair of asset ID and cached file edit time (for tracking modification).
    /// </summary>
    Array<Pair<Guid, DateTime>> Dependencies;
#endif

public:
    /// <summary>
    /// Gets the asset serialized version.
    /// </summary>
    virtual uint32 GetSerializedVersion() const = 0;

    /// <summary>
    /// Initializes the specified asset with asset header and storage only. Asset should be initialized later using will AssetInitData.
    /// </summary>
    /// <param name="storage">The storage container.</param>
    /// <param name="header">The asset header.</param>
    bool Init(const FlaxStorageReference& storage, AssetHeader& header);

    /// <summary>
    /// Initializes the specified asset.
    /// </summary>
    /// <param name="initData">The initialize data.</param>
    /// <returns>True if cannot init, otherwise false</returns>
    bool Init(AssetInitData& initData);

    /// <summary>
    /// Initializes the specified asset (as virtual).
    /// </summary>
    /// <param name="initData">The initialize data.</param>
    /// <returns>True if cannot init, otherwise false</returns>
    bool InitVirtual(AssetInitData& initData);

public:
#if USE_EDITOR
#if COMPILE_WITH_ASSETS_IMPORTER
    /// <summary>
    /// Reimports asset from the source file.
    /// </summary>
    API_FUNCTION() void Reimport() const;
#endif

    /// <summary>
    /// Returns imported file metadata.
    /// </summary>
    /// <param name="path">The import path.</param>
    /// <param name="username">The import username.</param>
    void GetImportMetadata(String& path, String& username) const;

    /// <summary>
    /// Gets the imported file path from the asset metadata (can be empty if not available).
    /// </summary>
    API_PROPERTY() String GetImportPath() const;

    /// <summary>
    /// Clears the asset dependencies list and unregisters from tracking their changes.
    /// </summary>
    void ClearDependencies();

    /// <summary>
    /// Adds the dependency to the given asset.
    /// </summary>
    /// <param name="asset">The asset to depend on.</param>
    void AddDependency(BinaryAsset* asset);

    /// <summary>
    /// Determines whether any of the dependency assets was modified after last modification time of this asset (last file write time check).
    /// </summary>
    bool HasDependenciesModified() const;

protected:
    /// <summary>
    /// Called when one of the asset dependencies gets modified (it was saved or reloaded or reimported).
    /// </summary>
    /// <param name="asset">The modified asset that this asset depends on.</param>
    virtual void OnDependencyModified(BinaryAsset* asset)
    {
    }
#endif

protected:
    /// <summary>
    /// Initializes the specified asset.
    /// </summary>
    /// <param name="initData">The initialize data.</param>
    /// <returns>True if cannot init, otherwise false</returns>
    virtual bool init(AssetInitData& initData)
    {
        return false;
    }

    /// <summary>
    /// Gets packed chunks indices to preload before asset loading action
    /// </summary>
    /// <returns>Chunks to load flags</returns>
    virtual AssetChunksFlag getChunksToPreload() const
    {
        return 0;
    }

public:
    /// <summary>
    /// Gets the asset chunk.
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>Flax Chunk object or null if is missing</returns>
    FlaxChunk* GetChunk(int32 index) const
    {
        ASSERT(index >= 0 && index < ASSET_FILE_DATA_CHUNKS);
        auto chunk = _header.Chunks[index];
        if (chunk)
            chunk->RegisterUsage();
        return chunk;
    }

    /// <summary>
    /// Gets or creates the asset chunk.
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>Flax Chunk object (may be null if asset storage doesn't allow to add new chunks).</returns>
    FlaxChunk* GetOrCreateChunk(int32 index) const;

    /// <summary>
    /// Determines whether the specified chunk exists.
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>True if the specified chunk exists, otherwise false.</returns>
    bool HasChunk(int32 index) const
    {
        ASSERT(index >= 0 && index < ASSET_FILE_DATA_CHUNKS);
        return _header.Chunks[index] != nullptr;
    }

    /// <summary>
    /// Gets the chunk size (in bytes).
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>Chunk size (in bytes)</returns>
    uint32 GetChunkSize(int32 index) const
    {
        ASSERT(index >= 0 && index < ASSET_FILE_DATA_CHUNKS);
        return _header.Chunks[index] != nullptr ? _header.Chunks[index]->LocationInFile.Size : 0;
    }

    /// <summary>
    /// Determines whether the specified chunk exists and is loaded.
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>True if the specified chunk exists and is loaded, otherwise false.</returns>
    bool HasChunkLoaded(int32 index) const
    {
        ASSERT(index >= 0 && index < ASSET_FILE_DATA_CHUNKS);
        return _header.Chunks[index] != nullptr && _header.Chunks[index]->IsLoaded();
    }

    /// <summary>
    /// Sets the chunk data (creates new chunk if is missing).
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <param name="data">The chunk data to set.</param>
    void SetChunk(int32 index, const Span<byte>& data);

    /// <summary>
    /// Releases all the chunks data.
    /// </summary>
    void ReleaseChunks() const;

    /// <summary>
    /// Releases the chunk data (if loaded).
    /// </summary>
    /// <param name="index">The chunk index.</param>
    void ReleaseChunk(int32 index) const;

    /// <summary>
    /// Requests the chunk data asynchronously (creates task that will gather chunk data or null if already here).
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <returns>Task that will gather chunk data or null if already here.</returns>
    ContentLoadTask* RequestChunkDataAsync(int32 index);

    /// <summary>
    /// Gets chunk data. May fail if not data requested. See RequestChunkDataAsync.
    /// </summary>
    /// <param name="index">The chunk index.</param>
    /// <param name="data">Result data (linked chunk data, not copied).</param>
    void GetChunkData(int32 index, BytesContainer& data) const;

    /// <summary>
    /// Loads the chunk (synchronous, blocks current thread).
    /// </summary>
    /// <param name="chunkIndex">The chunk index to load.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadChunk(int32 chunkIndex) const;

    /// <summary>
    /// Loads the chunks (synchronous, blocks current thread).
    /// </summary>
    /// <param name="chunks">The chunks to load (packed in a flag).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadChunks(AssetChunksFlag chunks) const;

#if USE_EDITOR
    /// <summary>
    /// Saves this asset to the storage container.
    /// </summary>
    /// <param name="data">Asset data.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool SaveAsset(AssetInitData& data, bool silentMode = false) const;

    /// <summary>
    /// Saves this asset to the file.
    /// </summary>
    /// <param name="data">Asset data.</param>
    /// <param name="path">Asset path (will be used to override the asset or create a new one).</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool SaveAsset(const StringView& path, AssetInitData& data, bool silentMode = false) const;

    /// <summary>
    /// Saves asset data to the storage container. Asset unique ID is handled by auto.
    /// </summary>
    /// <param name="path">Asset path (will be used to override the asset or create a new one).</param>
    /// <param name="data">Asset data.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SaveToAsset(const StringView& path, AssetInitData& data, bool silentMode = false);
#endif

protected:
    /// <summary>
    /// Load data from the chunks
    /// </summary>
    /// <returns>Loading result</returns>
    virtual LoadResult load() = 0;

private:
#if USE_EDITOR
    void OnStorageReloaded(FlaxStorage* storage, bool failed);
#endif

public:
    // [Asset]
#if USE_EDITOR
    void OnDeleteObject() override;
#endif
    const String& GetPath() const final override;
    uint64 GetMemoryUsage() const override;

protected:
    // [Asset]
    ContentLoadTask* createLoadingTask() override;
    LoadResult loadAsset() override;
    void releaseStorage() final override;
#if USE_EDITOR
    void onRename(const StringView& newPath) override;
#endif
};
