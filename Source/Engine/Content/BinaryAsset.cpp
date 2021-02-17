// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BinaryAsset.h"
#include "Storage/ContentStorageManager.h"
#include "Loading/Tasks/LoadAssetDataTask.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Content/Content.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Factories/BinaryAssetFactory.h"
#include "Engine/Threading/ThreadPoolTask.h"
#if USE_EDITOR
#include "Engine/Platform/FileSystem.h"
#endif

REGISTER_BINARY_ASSET_ABSTRACT(BinaryAsset, "FlaxEngine.BinaryAsset");

BinaryAsset::BinaryAsset(const SpawnParams& params, const AssetInfo* info)
    : Asset(params, info)
    , _storageRef(nullptr) // We link storage container later
    , _isSaving(false)
    , Storage(nullptr)
{
}

BinaryAsset::~BinaryAsset()
{
#if USE_EDITOR
    if (Storage)
        Storage->OnReloaded.Unbind<BinaryAsset, &BinaryAsset::OnStorageReloaded>(this);
#endif
}

bool BinaryAsset::Init(const FlaxStorageReference& storage, AssetHeader& header)
{
    // We allow to init asset only once like that
    ASSERT(Storage == nullptr && _header.ID.IsValid() == false);

    // Block initialization with a different storage
    bool isChanged = _storageRef != storage;
    if (Storage != nullptr && isChanged)
    {
        LOG(Error, "Asset \'{0}\' has been already initialized.", GetPath());
        return true;
    }

    // Get data
    _storageRef = storage;
    Storage = storage.Get();
    _header = header;

#if USE_EDITOR
    // Link for storage reload event
    if (Storage && isChanged)
        Storage->OnReloaded.Bind<BinaryAsset, &BinaryAsset::OnStorageReloaded>(this);
#endif

    return false;
}

bool BinaryAsset::Init(AssetInitData& initData)
{
    // Validate serialized version
    if (initData.SerializedVersion != GetSerializedVersion())
    {
        LOG(Error, "Asset \'{0}\' is using different serialized version. Loaded: {1}, Runtime: {2}.", GetPath(), initData.SerializedVersion, GetSerializedVersion());
        return true;
    }

    // Get asset data
    _header = initData.Header;
#if USE_EDITOR
    Metadata.Copy(initData.Metadata);
    ClearDependencies();
    Dependencies = initData.Dependencies;
    for (auto& e : Dependencies)
    {
        auto asset = Cast<BinaryAsset>(Content::GetAsset(e.First));
        if (asset)
        {
            asset->_dependantAssets.Add(this);
        }
    }
#endif

    return init(initData);
}

bool BinaryAsset::InitVirtual(AssetInitData& initData)
{
    // Be virtual
    _isVirtual = true;

    return Init(initData);
}

#if USE_EDITOR

#if COMPILE_WITH_ASSETS_IMPORTER

void BinaryAsset::Reimport() const
{
    const String importPath = GetImportPath();
    if (importPath.HasChars())
    {
        AssetsImportingManager::Import(importPath, GetPath());
    }
}

#endif

void BinaryAsset::GetImportMetadata(String& path, String& username) const
{
    if (Metadata.IsInvalid())
    {
        LOG(Warning, "Missing asset metadata.");
        return;
    }

    // Parse metadata and try to get import info
    rapidjson_flax::Document document;
    document.Parse((const char*)Metadata.Get(), Metadata.Length());
    if (document.HasParseError() == false)
    {
        path = JsonTools::GetString(document, "ImportPath");
        username = JsonTools::GetString(document, "ImportUsername");
    }
    else
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), GetPath());
    }
}

void BinaryAsset::ClearDependencies()
{
    for (auto& e : Dependencies)
    {
        auto asset = Cast<BinaryAsset>(Content::GetAsset(e.First));
        if (asset)
        {
            asset->_dependantAssets.Remove(this);
        }
    }
    Dependencies.Clear();
}

void BinaryAsset::AddDependency(BinaryAsset* asset)
{
    ASSERT_LOW_LAYER(asset);
    const Guid id = asset->GetID();
    for (auto& e : Dependencies)
    {
        if (e.First == id)
            return;
    }
    ASSERT(!asset->_dependantAssets.Contains(asset));
    Dependencies.Add(ToPair(id, FileSystem::GetFileLastEditTime(asset->GetPath())));
    asset->_dependantAssets.Add(this);
}

bool BinaryAsset::HasDependenciesModified() const
{
    AssetInfo info;
    for (const auto& e : Dependencies)
    {
        if (Content::GetAssetInfo(e.First, info))
        {
            const auto editTime = FileSystem::GetFileLastEditTime(info.Path);
            if (editTime > e.Second)
            {
                LOG(Info, "Asset {0} was modified - dependency of {1}", info.Path, GetPath());
                return true;
            }
        }
    }
    return false;
}

#endif

FlaxChunk* BinaryAsset::GetOrCreateChunk(int32 index)
{
    ASSERT(Math::IsInRange(index, 0, ASSET_FILE_DATA_CHUNKS - 1));

    // Try get
    auto chunk = _header.Chunks[index];
    if (chunk)
    {
        chunk->RegisterUsage();
        return chunk;
    }

    // Allocate
    ASSERT(Storage);
    _header.Chunks[index] = chunk = Storage->AllocateChunk();
    if (chunk)
        chunk->RegisterUsage();

    return chunk;
}

void BinaryAsset::SetChunk(int32 index, const Span<byte>& data)
{
    auto chunk = GetOrCreateChunk(index);
    if (chunk)
        chunk->Data.Copy(data.Get(), data.Length());
}

void BinaryAsset::ReleaseChunks() const
{
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);
}

void BinaryAsset::ReleaseChunk(int32 index) const
{
    auto chunk = GetChunk(index);
    if (chunk)
        chunk->Data.Release();
}

ContentLoadTask* BinaryAsset::RequestChunkDataAsync(int32 index)
{
    auto chunk = GetChunk(index);
    if (chunk != nullptr && chunk->IsLoaded())
    {
        // Data already here
        chunk->RegisterUsage();
        return nullptr;
    }

    // Spawn loading task
    return New<LoadAssetDataTask>(this, GET_CHUNK_FLAG(index));
}

void BinaryAsset::GetChunkData(int32 index, BytesContainer& data) const
{
    //ScopeLock lock(Locker);

    // Check if has data missing
    if (!HasChunkLoaded(index))
    {
        // Missing data
        data.Release();
        return;
    }

    // Get data
    auto chunk = GetChunk(index);
    data.Link(chunk->Data);
}

bool BinaryAsset::LoadChunk(int32 chunkIndex)
{
    ASSERT(Storage);

    const auto chunk = _header.Chunks[chunkIndex];
    if (chunk != nullptr
        && chunk->IsMissing()
        && chunk->ExistsInFile())
    {
        if (Storage->LoadAssetChunk(chunk))
            return true;
    }

    return false;
}

bool BinaryAsset::LoadChunks(AssetChunksFlag chunks)
{
    ASSERT(Storage);

    // Check if skip loading
    if (chunks == 0)
        return false;

    // Load all missing marked chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
    {
        auto chunk = _header.Chunks[i];
        if (chunk != nullptr
            && chunks & GET_CHUNK_FLAG(i)
            && chunk->IsMissing()
            && chunk->ExistsInFile())
        {
            if (Storage->LoadAssetChunk(chunk))
                return true;
        }
    }

    return false;
}

#if USE_EDITOR

bool BinaryAsset::SaveAsset(const StringView& path, AssetInitData& data, bool silentMode)
{
    data.Header = _header;
    data.Metadata.Link(Metadata);
    data.Dependencies = Dependencies;
    return SaveToAsset(path, data, silentMode);
}

bool BinaryAsset::SaveToAsset(const StringView& path, AssetInitData& data, bool silentMode)
{
    // Find target storage container and the asset
    auto storage = ContentStorageManager::TryGetStorage(path);
    auto asset = Content::GetAsset(path);
    auto binaryAsset = dynamic_cast<BinaryAsset*>(asset);
    if (asset && !binaryAsset)
    {
        LOG(Warning, "Cannot write to the non-binary asset location.");
        return true;
    }

    // Check if can perform write operation to the asset container
    if (storage)
    {
        if (!storage->AllowDataModifications())
        {
            LOG(Warning, "Cannot write to the asset storage container.");
            return true;
        }
    }

    // Initialize data container
    ASSERT(data.SerializedVersion > 0);
    if (binaryAsset)
    {
        // Use the same asset ID
        data.Header.ID = binaryAsset->GetID();
    }
    else
    {
        // Randomize ID
        data.Header.ID = Guid::New();
    }

    // Save (set flag to lock reloads on storage modified)
    if (binaryAsset)
        binaryAsset->_isSaving = true;
    bool result;
    if (storage)
    {
        // HACK: file is locked by some tasks (e.g material asset loaded some data and is updating the asset)
        // Let's hide these locks just for the saving
        const auto locks = storage->_chunksLock;
        storage->_chunksLock = 0;
        result = storage->Save(data, silentMode);
        storage->_chunksLock = locks;
    }
    else
    {
        ASSERT(path.HasChars());
        result = FlaxStorage::Create(path, data, silentMode);
    }
    if (binaryAsset)
        binaryAsset->_isSaving = false;

    return result;
}

void BinaryAsset::OnStorageReloaded(FlaxStorage* storage, bool failed)
{
    ASSERT(Storage != nullptr && Storage == storage);

    // Clear header (prevent from using old chunks)
    auto oldHeader = _header;
    Platform::MemoryClear(_header.Chunks, sizeof(_header.Chunks));

    // Check if reload failed
    if (failed)
    {
        LOG(Error, "Asset storage reloading failed. Asset: \'{0}\'.", ToString());
        return;
    }

    // Gather updated asset init data
    AssetInitData initData;
    if (Storage->LoadAssetHeader(GetID(), initData))
    {
        LOG(Error, "Asset header loading failed. Asset: \'{0}\'.", ToString());
        return;
    }
    if (oldHeader.ID != initData.Header.ID || oldHeader.TypeName != initData.Header.TypeName)
    {
        LOG(Warning, "Asset reloading data mismatch. Old ID:{0},TypeName:{1}, New ID:{2},TypeName:{3}. Asset: \'{4}\'.", oldHeader.ID, oldHeader.TypeName, initData.Header.ID, initData.Header.TypeName, GetPath());

        // Unload asset (file contains different asset data)
        // For eg. texture has been changed into sprite atlas on reimport
        Content::UnloadAsset(this);

        // Delete managed object now because it way fail when we recreate the asset object and want to register the new managed object (IDs will overlap)
        DeleteManaged();

        return;
    }

    // Reinitialize (file may modify some data so it needs to be flushed)
    if (Init(initData))
    {
        LOG(Error, "Asset reloading failed. Asset: \'{0}\'.", ToString());
    }

    // Don't reload on save
    if (_isSaving == false)
    {
        Reload();
    }

    // Inform dependant asset (use cloned version because it might be modified by assets when they got reloaded)
    auto dependantAssets = _dependantAssets;
    for (auto& e : dependantAssets)
    {
        e->OnDependencyModified(this);
    }
}

void BinaryAsset::OnDeleteObject()
{
    // Clear dependencies stuff
    ClearDependencies();
    _dependantAssets.Clear();

    Asset::OnDeleteObject();
}

#endif

const String& BinaryAsset::GetPath() const
{
    return Storage ? Storage->GetPath() : String::Empty;
}

/// <summary>
/// Helper task used to initialize binary asset and upgrade it if need to in background.
/// </summary>
/// <seealso cref="ContentLoadTask" />
class InitAssetTask : public ContentLoadTask
{
private:

    WeakAssetReference<BinaryAsset> _asset;
    FlaxStorage::LockData _dataLock;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="InitAssetTask"/> class.
    /// </summary>
    /// <param name="asset">The asset.</param>
    InitAssetTask(BinaryAsset* asset)
        : ContentLoadTask(Type::Custom)
        , _asset(asset)
        , _dataLock(asset->Storage->Lock())
    {
    }

public:

    // [ContentLoadTask]
    bool HasReference(Object* obj) const override
    {
        return obj == _asset;
    }

protected:

    // [ContentLoadTask]
    Result run() override
    {
        AssetReference<BinaryAsset> ref = _asset.Get();
        if (ref == nullptr)
            return Result::MissingReferences;

        // Prepare
        auto storage = ref->Storage;
        auto factory = (BinaryAssetFactoryBase*)Content::GetAssetFactory(ref->GetTypeName());
        ASSERT(factory);

        // Here we should open storage and extract AssetInitData
        // This would also allow to convert/upgrade data
        if (!storage->IsLoaded() && storage->Load())
            return Result::AssetLoadError;
        if (factory->Init(ref.Get()))
            return Result::AssetLoadError;

        return Result::Ok;
    }

    void OnEnd() override
    {
        _dataLock.Release();
        _asset = nullptr;

        // Base
        ContentLoadTask::OnEnd();
    }
};

ContentLoadTask* BinaryAsset::createLoadingTask()
{
    ContentLoadTask* loadTask = Asset::createLoadingTask();

    // Check if asset need any just to be preloaded
    auto chunksToPreload = getChunksToPreload();
    if (chunksToPreload != 0)
    {
        // Inject loading chunks task
        auto preLoadChunksTask = New<LoadAssetDataTask>(this, chunksToPreload);
        preLoadChunksTask->ContinueWith(loadTask);
        loadTask = preLoadChunksTask;
    }

    // Before asset loading we have to initialize storage
    // TODO: maybe in build game we could do it in place?
    // This step is only for opening asset files in background and upgrading them
    // In build game we have only a few packages which are ready to use
    auto initTask = New<InitAssetTask>(this);
    initTask->ContinueWith(loadTask);
    loadTask = initTask;

    return loadTask;
}

Asset::LoadResult BinaryAsset::loadAsset()
{
    // Ensure that asset has been initialized
    ASSERT(Storage && _header.ID.IsValid() && _header.TypeName.HasChars());

    auto lock = Storage->Lock();
    return load();
}

void BinaryAsset::releaseStorage()
{
#if USE_EDITOR
    // Close file
    if (Storage)
        Storage->CloseFileHandles();
#endif
}

#if USE_EDITOR

void BinaryAsset::onRename(const StringView& newPath)
{
    ScopeLock lock(Locker);

    // We don't support packages now
    ASSERT(!Storage->IsPackage() && Storage->AllowDataModifications() && Storage->GetEntriesCount() == 1);

    // Rename storage
    Storage->OnRename(newPath);
}

#endif
