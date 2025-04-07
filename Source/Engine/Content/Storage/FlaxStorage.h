// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Threading/ThreadLocal.h"
#include "FlaxChunk.h"
#include "AssetHeader.h"
#include "../AssetInfo.h"

class ContentStorageManager;
struct FlaxStorageReference;

/// <summary>
/// Flax assets storage container.
/// </summary>
class FLAXENGINE_API FlaxStorage : public Object
{
    friend FlaxStorageReference;
    friend class BinaryAssetFactoryBase;
    friend class BinaryAsset;

public:
    /// <summary>
    /// Magic code stored in file header to identify contents.
    /// </summary>
    static const int32 MagicCode;

    /// <summary>
    /// The custom file data.
    /// </summary>
    struct CustomData
    {
        int32 ContentKey;
        int32 NotUsed0;
        int32 NotUsed1;
        int32 NotUsed2;
    };

    /// <summary>
    /// Asset entry info.
    /// </summary>
    struct Entry
    {
        /// <summary>
        /// The asset identifier.
        /// </summary>
        Guid ID;

        /// <summary>
        /// The asset type name identifier (cached in entry for faster per asset info query)
        /// </summary>
        String TypeName;

        /// <summary>
        /// The address of the AssetHeader in the file.
        /// </summary>
        uint32 Address;

        /// <summary>
        /// Initializes a new instance of the <see cref="Entry"/> struct.
        /// </summary>
        Entry()
            : ID(Guid::Empty)
            , Address(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Entry"/> struct.
        /// </summary>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeName">The asset type name identifier.</param>
        /// <param name="address">The address in the file.</param>
        Entry(const Guid& id, const StringView& typeName, uint32 address)
            : ID(id)
            , TypeName(typeName)
            , Address(address)
        {
        }
    };

protected:
    // State
    int64 _refCount = 0;
    int64 _chunksLock = 0;
    int64 _files = 0;
    double _lastRefLostTime;
    CriticalSection _loadLocker;

    // Storage
    ThreadLocal<FileReadStream*> _file;
    Array<FlaxChunk*> _chunks;

    // Metadata
    uint32 _version = 0;
    String _path;

protected:
    explicit FlaxStorage(const StringView& path);

private:
    // Used by FlaxStorageReference:
    void AddRef()
    {
        Platform::InterlockedIncrement(&_refCount);
    }
    void RemoveRef()
    {
        Platform::InterlockedDecrement(&_refCount);
        if (Platform::AtomicRead(&_refCount) == 0)
        {
            _lastRefLostTime = Platform::GetTimeSeconds();
        }
    }

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="FlaxStorage"/> class.
    /// </summary>
    virtual ~FlaxStorage();

public:
    /// <summary>
    /// Locks the storage chunks data to prevent disposing them. Also ensures that file handles won't be closed while chunks are locked.
    /// </summary>
    FORCE_INLINE void LockChunks()
    {
        Platform::InterlockedIncrement(&_chunksLock);
    }

    /// <summary>
    /// Unlocks the storage chunks data.
    /// </summary>
    FORCE_INLINE void UnlockChunks()
    {
        Platform::InterlockedDecrement(&_chunksLock);
    }

    /// <summary>
    /// Locks storage data via LockChunks/UnlockChunks. Prevents from releasing chunks data cache.
    /// </summary>
    struct LockData
    {
        friend FlaxStorage;
        static LockData Invalid;

    private:
        FlaxStorage* _storage;

        LockData(FlaxStorage* storage)
            : _storage(storage)
        {
            if (_storage)
                _storage->LockChunks();
        }

    public:
        LockData(const LockData& other)
            : _storage(other._storage)
        {
            if (_storage)
                _storage->LockChunks();
        }

        LockData(LockData&& other) noexcept
            : _storage(other._storage)
        {
            other._storage = nullptr;
        }

        ~LockData()
        {
            if (_storage)
                _storage->UnlockChunks();
        }

        void Release()
        {
            if (_storage)
            {
                _storage->UnlockChunks();
                _storage = nullptr;
            }
        }

        LockData& operator=(const LockData& other)
        {
            if (this != &other)
            {
                if (_storage)
                    _storage->UnlockChunks();
                _storage = other._storage;
                if (_storage)
                    _storage->LockChunks();
            }
            return *this;
        }

        LockData& operator=(LockData&& other) noexcept
        {
            if (this == &other)
                return *this;
            _storage = other._storage;
            other._storage = nullptr;
            return *this;
        }
    };

    /// <summary>
    /// Locks the storage container chunks within a code block section (using a local variable that keeps a scope lock).
    /// </summary>
    /// <returns>The scope lock.</returns>
    LockData Lock()
    {
        return LockData(this);
    }

    /// <summary>
    /// Locks the storage container chunks within a code block section (using a local variable that keeps a scope lock).
    /// </summary>
    /// <remarks>Lock is done with a safe critical section for a ContentStorageManager that prevents to minor issues with threading. Should be used on a separate threads (eg. GameCooker thread).</remarks>
    /// <returns>The scope lock.</returns>
    LockData LockSafe();

public:
    /// <summary>
    /// Gets the references count.
    /// </summary>
    uint32 GetRefCount() const;

    /// <summary>
    /// Checks if storage container should be disposed (it's not used anymore).
    /// </summary>
    bool ShouldDispose() const;

    /// <summary>
    /// Gets the path.
    /// </summary>
    FORCE_INLINE const String& GetPath() const
    {
        return _path;
    }

    /// <summary>
    /// Determines whether this instance is loaded.
    /// </summary>
    FORCE_INLINE bool IsLoaded() const
    {
        return _version != 0;
    }

    /// <summary>
    /// Determines whether this instance is disposed.
    /// </summary>
    FORCE_INLINE bool IsDisposed() const
    {
        return _version == 0;
    }

    /// <summary>
    /// Gets total memory used by chunks (in bytes).
    /// </summary>
    uint32 GetMemoryUsage() const;

    /// <summary>
    /// Determines whether this storage container is a package.
    /// </summary>
    virtual bool IsPackage() const = 0;

    /// <summary>
    /// Checks whenever storage container allows the data modifications.
    /// </summary>
    virtual bool AllowDataModifications() const = 0;

    /// <summary>
    /// Determines whether the specified asset exists in this container.
    /// </summary>
    /// <param name="id">The asset identifier.</param>
    /// <returns>True if the specified asset exists in this container, otherwise false.</returns>
    virtual bool HasAsset(const Guid& id) const = 0;

    /// <summary>
    /// Determines whether the specified asset exists in this container.
    /// </summary>
    /// <param name="info">The asset info.</param>
    /// <returns>True if the specified asset exists in this container, otherwise false.</returns>
    virtual bool HasAsset(const AssetInfo& info) const = 0;

    /// <summary>
    /// Gets amount of entries in the storage.
    /// </summary>
    virtual int32 GetEntriesCount() const = 0;

    /// <summary>
    /// Gets the asset entry at given index.
    /// </summary>
    /// <param name="index">The asset index.</param>
    /// <returns>The output.</returns>
    Entry GetEntry(int32 index) const
    {
        Entry result;
        GetEntry(index, result);
        return result;
    }

    /// <summary>
    /// Gets the asset entry at given index.
    /// </summary>
    /// <param name="index">The asset index.</param>
    /// <param name="value">The result.</param>
    virtual void GetEntry(int32 index, Entry& value) const = 0;

#if USE_EDITOR
    /// <summary>
    /// Sets the asset entry at given index.
    /// </summary>
    /// <param name="index">The asset index.</param>
    /// <param name="value">The input value.</param>
    virtual void SetEntry(int32 index, const Entry& value) = 0;
#endif

    /// <summary>
    /// Gets all the entries in the storage.
    /// </summary>
    /// <param name="output">The output.</param>
    virtual void GetEntries(Array<Entry>& output) const = 0;

    /// <summary>
    /// Gets all the chunks in the storage.
    /// </summary>
    /// <param name="output">The output.</param>
    FORCE_INLINE void GetChunks(Array<FlaxChunk*>& output) const
    {
        output.Add(_chunks);
    }

public:
    /// <summary>
    /// Loads package from the file.
    /// </summary>
    /// <returns>True if cannot load, otherwise false</returns>
    bool Load();

#if USE_EDITOR

    /// <summary>
    /// Action fired on storage reloading.
    /// </summary>
    Delegate<FlaxStorage*> OnReloading;

    /// <summary>
    /// Action fired on storage reloading. bool param means 'wasReloadFail'
    /// </summary>
    Delegate<FlaxStorage*, bool> OnReloaded;

    /// <summary>
    /// Reloads this storage container.
    /// </summary>
    /// <returns>True if cannot load, otherwise false</returns>
    bool Reload();

#endif

    /// <summary>
    /// Loads the asset header.
    /// </summary>
    /// <param name="index">The asset index.</param>
    /// <param name="data">The data.</param>
    /// <returns>True if cannot load data, otherwise false</returns>
    bool LoadAssetHeader(const int32 index, AssetInitData& data)
    {
        return LoadAssetHeader(GetEntry(index), data);
    }

    /// <summary>
    /// Loads the asset header.
    /// </summary>
    /// <param name="id">The asset identifier.</param>
    /// <param name="data">The data.</param>
    /// <returns>True if cannot load data, otherwise false</returns>
    bool LoadAssetHeader(const Guid& id, AssetInitData& data);

    /// <summary>
    /// Loads the asset chunk.
    /// </summary>
    /// <param name="chunk">The chunk.</param>
    /// <returns>True if cannot load data, otherwise false</returns>
    bool LoadAssetChunk(FlaxChunk* chunk);

#if USE_EDITOR

    /// <summary>
    /// Changes the asset identifier. Warning! Changes only ID in storage layer, not dependencies resolving at all!
    /// </summary>
    /// <param name="e">The asset entry.</param>
    /// <param name="newId">The new identifier.</param>
    /// <returns>True if cannot change data, otherwise false</returns>
    bool ChangeAssetID(Entry& e, const Guid& newId);

#endif

    /// <summary>
    /// Allocates the new chunk within a storage container (if it's possible).
    /// </summary>
    /// <returns>Allocated chunk or null if cannot.</returns>
    FlaxChunk* AllocateChunk();

    /// <summary>
    /// Closes the file handles (it can be modified from the outside).
    /// </summary>
    bool CloseFileHandles();

    /// <summary>
	/// Releases storage resources and closes handle to the file.
	/// </summary>
    virtual void Dispose();

public:
    /// <summary>
    /// Ticks this instance.
    /// </summary>
    void Tick(double time);

#if USE_EDITOR
    void OnRename(const StringView& newPath);
#endif

public:
#if USE_EDITOR
    /// <summary>
    /// Saves the specified asset data to the storage container.
    /// </summary>
    /// <param name="data">The data to save.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <returns>True if cannot save, otherwise false</returns>
    bool Save(const AssetInitData& data, bool silentMode = false);

    /// <summary>
    /// Creates new FlaxFile using specified asset data.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="asset">The asset data to write.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <param name="customData">Custom options.</param>
    /// <returns>True if cannot create package, otherwise false</returns>
    FORCE_INLINE static bool Create(const StringView& path, const AssetInitData& asset, bool silentMode = false, const CustomData* customData = nullptr)
    {
        return Create(path, ToSpan(&asset, 1), silentMode, customData);
    }

    /// <summary>
    /// Creates new FlaxFile using specified assets data.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="assets">The assets data to write.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <param name="customData">Custom options.</param>
    /// <returns>True if cannot create package, otherwise false</returns>
    FORCE_INLINE static bool Create(const StringView& path, const Array<AssetInitData>& assets, bool silentMode = false, const CustomData* customData = nullptr)
    {
        return Create(path, ToSpan(assets), silentMode, customData);
    }

    /// <summary>
    /// Creates new FlaxFile using specified assets data.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="assets">The assets data to write.</param>
    /// <param name="silentMode">In silent mode don't reload opened storage container that is using target file.</param>
    /// <param name="customData">Custom options.</param>
    /// <returns>True if cannot create package, otherwise false</returns>
    static bool Create(const StringView& path, Span<AssetInitData> assets, bool silentMode = false, const CustomData* customData = nullptr);

    /// <summary>
    /// Creates new FlaxFile using specified assets data.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    /// <param name="assets">The assets data to write.</param>
    /// <param name="customData">Custom options.</param>
    /// <returns>True if cannot create package, otherwise false</returns>
    static bool Create(WriteStream* stream, Span<AssetInitData> assets, const CustomData* customData = nullptr);
#endif

protected:
    bool LoadAssetHeader(const Entry& e, AssetInitData& data);
    void AddChunk(FlaxChunk* chunk);
    virtual void AddEntry(Entry& e) = 0;
    FileReadStream* OpenFile();
    virtual bool GetEntry(const Guid& id, Entry& e) = 0;
    bool ReloadSilent();
};
