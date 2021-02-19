// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Content.h"
#include "JsonAsset.h"
#include "Cache/AssetsCache.h"
#include "Storage/ContentStorageManager.h"
#include "Storage/JsonStorageProxy.h"
#include "Factories/IAssetFactory.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Types.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Utilities/StringConverter.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#endif
#if ENABLE_ASSETS_DISCOVERY
#include "Engine/Core/Collections/HashSet.h"
#endif

TimeSpan Content::AssetsUpdateInterval = TimeSpan::FromMilliseconds(500);
TimeSpan Content::AssetsUnloadInterval = TimeSpan::FromSeconds(10);
Delegate<Asset*> Content::AssetDisposing;
Delegate<Asset*> Content::AssetReloading;

namespace
{
    // Assets
    CriticalSection AssetsLocker;
    Dictionary<Guid, Asset*> Assets(2048);
    CriticalSection LoadCallAssetsLocker;
    Array<Guid> LoadCallAssets(64);
    CriticalSection LoadedAssetsToInvokeLocker;
    Array<Asset*> LoadedAssetsToInvoke(64);
    Array<Asset*> ToUnload;

    // Assets Registry Stuff
    AssetsCache Cache;

    // Unloading assets
    Dictionary<Asset*, TimeSpan> UnloadQueue;
    TimeSpan LastUnloadCheckTime(0);
    bool IsExiting = false;

#if ENABLE_ASSETS_DISCOVERY
    DateTime LastWorkspaceDiscovery;
    CriticalSection WorkspaceDiscoveryLocker;
#endif
}

#if ENABLE_ASSETS_DISCOVERY
bool findAsset(const Guid& id, const String& directory, Array<String>& tmpCache, AssetInfo& info);
#endif

class ContentService : public EngineService
{
public:

    ContentService()
        : EngineService(TEXT("Content"), -600)
    {
    }

    bool Init() override;
    void Update() override;
    void LateUpdate() override;
    void Dispose() override;
};

ContentService ContentServiceInstance;

bool ContentService::Init()
{
    // Load assets registry
    Cache.Init();

    return false;
}

void ContentService::Update()
{
    PROFILE_CPU();

    ScopeLock lock(LoadedAssetsToInvokeLocker);

    // Broadcast `OnLoaded` events
    while (LoadedAssetsToInvoke.HasItems())
    {
        auto asset = LoadedAssetsToInvoke.Dequeue();
        asset->onLoaded_MainThread();
    }
}

void ContentService::LateUpdate()
{
    PROFILE_CPU();

    // Check if need to perform an update of unloading assets
    const TimeSpan timeNow = Time::Update.UnscaledTime;
    if (timeNow - LastUnloadCheckTime < Content::AssetsUpdateInterval)
        return;
    LastUnloadCheckTime = timeNow;

    Asset* asset;
    ScopeLock lock(AssetsLocker);

    // TODO: maybe it would be better to link for asset remove ref event and cache only assets with no references - test it with millions of assets?

    // Verify all assets
    for (auto i = Assets.Begin(); i.IsNotEnd(); ++i)
    {
        asset = i->Value;

        // Check if has no references and is not during unloading
        if (asset->GetReferencesCount() <= 0 && !UnloadQueue.ContainsKey(asset))
        {
            // Add to removes
            UnloadQueue.Add(asset, timeNow);
        }
    }

    // Find assets to unload in unload queue
    ToUnload.Clear();
    for (auto i = UnloadQueue.Begin(); i != UnloadQueue.End(); ++i)
    {
        // Check if asset gain any new reference or if need to unload it
        if (i->Key->GetReferencesCount() > 0 || timeNow - i->Value >= Content::AssetsUnloadInterval)
        {
            ToUnload.Add(i->Key);
        }
    }

    // Unload marked assets
    for (int32 i = 0; i < ToUnload.Count(); i++)
    {
        asset = ToUnload[i];

        // Check if has no references
        if (asset->GetReferencesCount() <= 0)
        {
            Content::UnloadAsset(asset);
        }

        // Remove from unload queue
        UnloadQueue.Remove(asset);
    }

    // Update cache (for longer sessions it will help to reduce cache misses)
    Cache.Save();
}

void ContentService::Dispose()
{
    IsExiting = true;

    // Save assets registry before engine closing
    Cache.Save();

    // Unload all remaining assets
    {
        ScopeLock lock(AssetsLocker);

        for (auto i = Assets.Begin(); i.IsNotEnd(); ++i)
        {
            i->Value->DeleteObject();
        }
    }

    // Flush objects (some assets may be pending to delete)
    ObjectsRemovalService::Flush();

    // NOW dispose graphics device - where there is no loaded assets at all
    Graphics::DisposeDevice();
}

AssetsCache* Content::GetRegistry()
{
    return &Cache;
}

#if USE_EDITOR

bool FindAssets(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects, const Guid& id, Array<String>& tmpCache, AssetInfo& info)
{
    if (projects.Contains(project))
        return false;

    projects.Add(project);
    bool found = findAsset(id, project->ProjectFolderPath / TEXT("Content"), tmpCache, info);
    for (const auto& reference : project->References)
    {
        if (reference.Project)
            found |= FindAssets(reference.Project, projects, id, tmpCache, info);
    }

    return found;
}

#endif

bool Content::GetAssetInfo(const Guid& id, AssetInfo& info)
{
    // Validate ID
    if (!id.IsValid())
    {
        LOG(Warning, "Invalid asset ID.");
        return false;
    }

#if ENABLE_ASSETS_DISCOVERY

    // Find asset in registry
    if (Cache.FindAsset(id, info))
        return true;

    // Locking injects some stalls but we need to make it safe (only one thread can pass though it at once)
    ScopeLock lock(WorkspaceDiscoveryLocker);

    // Check if we can search workspace
    // Note: we want to limit searching frequency due to high I/O usage and thread stall
    // We also perform full workspace discovery so all new assets will be found
    auto now = DateTime::NowUTC();
    auto diff = now - LastWorkspaceDiscovery;
    if (diff <= TimeSpan::FromSeconds(5))
    {
        LOG(Warning, "Cannot perform workspace scan for '{1}'. Too often call. Time diff: {0} ms", static_cast<int32>(diff.GetTotalMilliseconds()), id);
        return false;
    }
    LastWorkspaceDiscovery = now;

    // Try to find an asset within the project, engine, plugins workspace folders
    DateTime startTime = now;
    int32 startCount = Cache.Size();
    Array<String> tmpCache(1024);
#if USE_EDITOR
    HashSet<const ProjectInfo*> projects;
    bool found = FindAssets(Editor::Project, projects, id, tmpCache, info);
#else
    bool found = findAsset(id, Globals::ProjectContentFolder, tmpCache, info);
#endif
    if (found)
    {
        LOG(Info, "Workspace searching time: {0} ms, new assets found: {1}", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()), Cache.Size() - startCount);
        return true;
    }

    LOG(Warning, "Cannot find {0}.", id);
    return false;

#else

    // Find asset in registry
    return Cache.FindAsset(id, info);

#endif
}

bool Content::GetAssetInfo(const StringView& path, AssetInfo& info)
{
#if ENABLE_ASSETS_DISCOVERY

    // Find asset in registry
    if (Cache.FindAsset(path, info))
        return true;

    const auto extension = FileSystem::GetExtension(path).ToLower();

    // Check if it's a binary asset
    if (ContentStorageManager::IsFlaxStorageExtension(extension))
    {
        // Skip packages in editor (results in conflicts with build game packages if deployed inside project folder)
#if USE_EDITOR
        if (extension == PACKAGE_FILES_EXTENSION)
            return false;
#endif

        // Open storage
        auto storage = ContentStorageManager::GetStorage(path);
        if (storage)
        {
#if BUILD_DEBUG
            ASSERT(storage->GetPath() == path);
#endif

            // Register assets from the storage container (will handle duplicated IDs)
            Cache.RegisterAssets(storage);
            return Cache.FindAsset(path, info);
        }
    }
        // Check for json resource
    else if (JsonStorageProxy::IsValidExtension(extension))
    {
        // Check Json storage layer
        Guid jsonId;
        String jsonTypeName;
        if (JsonStorageProxy::GetAssetInfo(path, jsonId, jsonTypeName))
        {
            // Register asset
            Cache.RegisterAsset(jsonId, jsonTypeName, path);
            return Cache.FindAsset(path, info);
        }
    }

    return false;

#else

    // Find asset in registry
    return Cache.FindAsset(path, info);

#endif
}

Array<Guid> Content::GetAllAssetsByType(const MClass* type)
{
    Array<Guid> result;
    CHECK_RETURN(type, result);
    Cache.GetAllByTypeName(String(type->GetFullName()), result);
    return result;
}

IAssetFactory* Content::GetAssetFactory(const StringView& typeName)
{
    IAssetFactory* result = nullptr;
    IAssetFactory::Get().TryGet(typeName, result);
    return result;
}

IAssetFactory* Content::GetAssetFactory(const AssetInfo& assetInfo)
{
    IAssetFactory* result = nullptr;
    if (!IAssetFactory::Get().TryGet(assetInfo.TypeName, result))
    {
        // Check if it's a json asset (in editor only)
        // In build game all asset factories are valid and json assets are cooked into binary packages
#if USE_EDITOR
        if (assetInfo.Path.EndsWith(DEFAULT_JSON_EXTENSION_DOT))
#endif
        {
            IAssetFactory::Get().TryGet(JsonAsset::TypeName, result);
        }
    }

    return result;
}

String Content::CreateTemporaryAssetPath()
{
    return Globals::TemporaryFolder / (Guid::New().ToString(Guid::FormatType::N) + ASSET_FILES_EXTENSION_WITH_DOT);
}

Asset* Content::LoadAsyncInternal(const StringView& internalPath, MClass* type)
{
    CHECK_RETURN(type, nullptr);
    const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
    if (scriptingType)
        return LoadAsyncInternal(internalPath.GetText(), scriptingType);
    LOG(Error, "Failed to find asset type '{0}'.", String(type->GetFullName()));
    return nullptr;
}

Asset* Content::LoadAsyncInternal(const Char* internalPath, const ScriptingTypeHandle& type)
{
#if USE_EDITOR
    const String path = Globals::EngineContentFolder / internalPath + ASSET_FILES_EXTENSION_WITH_DOT;
    if (!FileSystem::FileExists(path))
    {
        LOG(Error, "Missing file \'{0}\'", path);
        return nullptr;
    }
#else
    const String path = Globals::ProjectContentFolder / internalPath + ASSET_FILES_EXTENSION_WITH_DOT;
#endif

    const auto asset = LoadAsync(path, type);
    if (asset == nullptr)
    {
        LOG(Error, "Failed to load \'{0}\' (type: {1})", internalPath, type.ToString());
    }

    return asset;
}

FLAXENGINE_API Asset* LoadAsset(const Guid& id, const ScriptingTypeHandle& type)
{
    return Content::LoadAsync(id, type);
}

Asset* Content::LoadAsync(const StringView& path, MClass* type)
{
    CHECK_RETURN(type, nullptr);
    const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
    if (scriptingType)
        return LoadAsync(path, scriptingType);
    LOG(Error, "Failed to find asset type '{0}'.", String(type->GetFullName()));
    return nullptr;
}

Asset* Content::LoadAsync(const StringView& path, const ScriptingTypeHandle& type)
{
#if USE_EDITOR
    if (!FileSystem::FileExists(path))
    {
        LOG(Error, "Missing file \'{0}\'", path);
        return nullptr;
    }
#endif

    AssetInfo assetInfo;
    if (GetAssetInfo(path, assetInfo))
    {
        return LoadAsync(assetInfo.ID, type);
    }

    return nullptr;
}

int32 Content::GetAssetCount()
{
    return Assets.Count();
}

Array<Asset*> Content::GetAssets()
{
    Array<Asset*> assets;
    Assets.GetValues(assets);
    return assets;
}

const Dictionary<Guid, Asset*>& Content::GetAssetsRaw()
{
    return Assets;
}

Asset* Content::LoadAsync(const Guid& id, const MClass* type)
{
    CHECK_RETURN(type, nullptr);
    const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
    if (scriptingType)
        return LoadAsync(id, scriptingType);
    LOG(Error, "Failed to find asset type '{0}'.", String(type->GetFullName()));
    return nullptr;
}

Asset* Content::GetAsset(const StringView& outputPath)
{
    if (outputPath.IsEmpty())
        return nullptr;

    ScopeLock lock(AssetsLocker);
    for (auto i = Assets.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value->GetPath() == outputPath)
        {
            return i->Value;
        }
    }
    return nullptr;
}

Asset* Content::GetAsset(const Guid& id)
{
    Asset* result = nullptr;
    AssetsLocker.Lock();
    Assets.TryGet(id, result);
    AssetsLocker.Unlock();
    return result;
}

void Content::DeleteAsset(Asset* asset)
{
    ScopeLock locker(AssetsLocker);

    // Validate
    if (asset == nullptr || asset->_deleteFileOnUnload)
    {
        // Back
        return;
    }

    LOG(Info, "Deleting asset {0}...", asset->ToString());

    // Mark asset for delete queue (delete it after auto unload)
    asset->_deleteFileOnUnload = true;

    // Unload
    UnloadAsset(asset);
}

void Content::DeleteAsset(const StringView& path)
{
    ScopeLock locker(AssetsLocker);

    // Check if is loaded
    Asset* asset = GetAsset(path);
    if (asset != nullptr)
    {
        // Delete asset
        DeleteAsset(asset);
        return;
    }

    // Remove from registry
    AssetInfo info;
    if (Cache.DeleteAsset(path, &info))
    {
        LOG(Info, "Deleting asset '{0}':{1}({2})", path, info.ID, info.TypeName);
    }
    else
    {
        LOG(Info, "Deleting asset '{0}':{1}({2})", path, TEXT("?"), TEXT("?"));
        info.ID = Guid::Empty;
    }

    // Delete file
    deleteFileSafety(path, info.ID);
}

void Content::deleteFileSafety(const StringView& path, const Guid& id)
{
    // Check if given id is invalid
    if (!id.IsValid())
    {
        // Cancel operation
        LOG(Warning, "Cannot remove file \'{0}\'. Given ID is invalid.", path);
        return;
    }

    // Ensure that file has the same ID (prevent from deleting different assets)
    auto storage = ContentStorageManager::TryGetStorage(path);
    if (storage)
    {
        storage->CloseFileHandles(); // Close file handle to allow removing it
        if (!storage->HasAsset(id))
        {
            // Skip removing
            LOG(Warning, "Cannot remove file \'{0}\'. It doesn\'t contain asset {1}.", path, id);
            return;
        }
    }

#if PLATFORM_WINDOWS
    // Safety way - move file to the recycle bin
    if (FileSystem::MoveFileToRecycleBin(path))
    {
        LOG(Warning, "Failed to move file to Recycle Bin. Path: \'{0}\'", path);
    }
#else
    // Remove file
    if (FileSystem::DeleteFile(path))
    {
        LOG(Warning, "Failed to delete file Path: \'{0}\'", path);
    }
#endif
}

#if USE_EDITOR

bool Content::RenameAsset(const StringView& oldPath, const StringView& newPath)
{
    ASSERT(IsInMainThread());

    // Cache data
    Asset* oldAsset = GetAsset(oldPath);
    Asset* newAsset = GetAsset(newPath);

    // Validate name
    if (newAsset != nullptr && newAsset != oldAsset)
    {
        LOG(Error, "Invalid name '{0}' when trying to rename '{1}'.", newPath, oldPath);
        return true;
    }

    // Ensure asset is ready for renaming
    if (oldAsset)
    {
        // Wait for data loaded
        if (oldAsset->WaitForLoaded())
        {
            LOG(Error, "Failed to load asset '{0}'.", oldAsset->ToString());
            return true;
        }

        // Unload
        // Don't unload asset fully, only release ref to file, don't call OnUnload so managed asset and all refs will remain alive
        oldAsset->releaseStorage();
        //oldAsset->onUnload_MainThread();
        //ScopeLock lock(oldAsset->Locker);
        //oldAsset->unload(true);
    }

    // Ensure to unlock file
    ContentStorageManager::EnsureAccess(oldPath);

    // Move file
    if (FileSystem::MoveFile(newPath, oldPath))
    {
        LOG(Error, "Cannot move file '{0}' to '{1}'", oldPath, newPath);
        return true;
    }

    // Update cache
    Cache.RenameAsset(oldPath, newPath);
    ContentStorageManager::OnRenamed(oldPath, newPath);

    // Check if is loaded
    if (oldAsset)
    {
        // Rename internal call
        oldAsset->onRename(newPath);

        // Load
        //ScopeLock lock(oldAsset->Locker);
        //oldAsset->startLoading();
    }

    return false;
}

bool Content::FastTmpAssetClone(const StringView& path, String& resultPath)
{
    ASSERT(path.HasChars());

    const String dstPath = Globals::TemporaryFolder / Guid::New().ToString(Guid::FormatType::D) + ASSET_FILES_EXTENSION_WITH_DOT;

    if (CloneAssetFile(dstPath, path, Guid::New()))
        return true;

    resultPath = dstPath;
    return false;
}

bool Content::CloneAssetFile(const StringView& dstPath, const StringView& srcPath, const Guid& dstId)
{
    ASSERT(FileSystem::AreFilePathsEqual(srcPath, dstPath) == false && dstId.IsValid());

    LOG(Info, "Cloning asset \'{0}\' to \'{1}\'({2}).", srcPath, dstPath, dstId);

    // Check source file
    if (!FileSystem::FileExists(srcPath))
    {
        LOG(Warning, "Missing source file.");
        return true;
    }

    // Special case for json resources
    if (JsonStorageProxy::IsValidExtension(FileSystem::GetExtension(srcPath).ToLower()))
    {
        if (FileSystem::CopyFile(dstPath, srcPath))
        {
            LOG(Warning, "Cannot copy file to destination.");
            return true;
        }

        if (JsonStorageProxy::ChangeId(dstPath, dstId))
        {
            LOG(Warning, "Cannot change asset ID.");
            return true;
        }

        return false;
    }

    // Check if destination file is missing
    if (!FileSystem::FileExists(dstPath))
    {
        // Use quick file copy
        if (FileSystem::CopyFile(dstPath, srcPath))
        {
            LOG(Warning, "Cannot copy file to destination.");
            return true;
        }

        // Change ID
        auto storage = ContentStorageManager::GetStorage(dstPath);
        FlaxStorage::Entry e;
        storage->GetEntry(0, e);
        if (storage == nullptr || storage->ChangeAssetID(e, dstId))
        {
            LOG(Warning, "Cannot change asset ID.");
            return true;
        }
    }
    else
    {
        // Use temporary file
        String tmpPath = Globals::TemporaryFolder / Guid::New().ToString(Guid::FormatType::D);
        if (FileSystem::CopyFile(tmpPath, srcPath))
        {
            LOG(Warning, "Cannot copy file.");
            return true;
        }

        // Change asset ID
        {
            auto storage = ContentStorageManager::GetStorage(tmpPath);
            FlaxStorage::Entry e;
            storage->GetEntry(0, e);
            if (!storage || storage->ChangeAssetID(e, dstId))
            {
                LOG(Warning, "Cannot change asset ID.");
                return true;
            }
        }

        // Unlock destination file
        ContentStorageManager::EnsureAccess(dstPath);

        // Copy temp file to the destination
        if (FileSystem::CopyFile(dstPath, tmpPath))
        {
            LOG(Warning, "Cannot copy file to destination.");
            return true;
        }

        // Cleanup
        FileSystem::DeleteFile(tmpPath);

        // Reload storage
        {
            auto storage = ContentStorageManager::GetStorage(dstPath);
            if (storage)
            {
                storage->Reload();
            }
        }
    }

    return false;
}

#endif

void Content::UnloadAsset(Asset* asset)
{
    // Check input
    if (asset == nullptr)
        return;

    asset->DeleteObject();
}

Asset* Content::CreateVirtualAsset(MClass* type)
{
    CHECK_RETURN(type, nullptr);
    const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
    if (scriptingType)
        return CreateVirtualAsset(scriptingType);
    LOG(Error, "Failed to find asset type '{0}'.", String(type->GetFullName()));
    return nullptr;
}

Asset* Content::CreateVirtualAsset(const ScriptingTypeHandle& type)
{
    auto& assetType = type.GetType();

    // Init mock asset info
    AssetInfo info;
    info.ID = Guid::New();
    info.TypeName = String(assetType.Fullname.Get(), assetType.Fullname.Length());
    info.Path = CreateTemporaryAssetPath();

    // Find asset factory based in its type
    auto factory = GetAssetFactory(info);
    if (factory == nullptr)
    {
        LOG(Error, "Cannot find virtual asset factory.");
        return nullptr;
    }
    if (!factory->SupportsVirtualAssets())
    {
        LOG(Error, "Cannot create virtual asset of type '{0}'.", info.TypeName);
        return nullptr;
    }

    // Create asset object
    auto asset = factory->NewVirtual(info);
    if (asset == nullptr)
    {
        LOG(Error, "Cannot create virtual asset object.");
        return nullptr;
    }

    // Call initializer function
    asset->InitAsVirtual();

    // Register asset
    AssetsLocker.Lock();
    ASSERT(!Assets.ContainsKey(asset->GetID()));
    Assets.Add(asset->GetID(), asset);
    AssetsLocker.Unlock();

    return asset;
}

void Content::tryCallOnLoaded(Asset* asset)
{
    ScopeLock lock(LoadedAssetsToInvokeLocker);
    const int32 index = LoadedAssetsToInvoke.Find(asset);
    if (index != -1)
    {
        LoadedAssetsToInvoke.RemoveAtKeepOrder(index);
        asset->onLoaded_MainThread();
    }
}

void Content::onAssetLoaded(Asset* asset)
{
    // This is called by the asset on loading end
    ASSERT(asset && asset->IsLoaded());

    ScopeLock locker(LoadedAssetsToInvokeLocker);

    LoadedAssetsToInvoke.Add(asset);
}

void Content::onAssetUnload(Asset* asset)
{
    // This is called by the asset on unloading

    ScopeLock locker(AssetsLocker);

    Assets.Remove(asset->GetID());
    UnloadQueue.Remove(asset);
    LoadedAssetsToInvoke.Remove(asset);
}

bool Content::IsAssetTypeIdInvalid(const ScriptingTypeHandle& type, const ScriptingTypeHandle& assetType)
{
    // Skip if no restrictions for the type
    if (!type || !assetType)
        return false;

#if BUILD_DEBUG
    // Peek types for debugging
    const auto& typeObj = type.GetType();
    const auto& assetTypeObj = assetType.GetType();
#endif

    // Early out if type matches
    if (type == assetType)
        return false;

    // Check if the asset type inherited from the requested type
    ScriptingTypeHandle it = assetType.GetType().GetBaseType();
    while (it)
    {
        if (type == it)
            return false;
        it = it.GetType().GetBaseType();
    }

    return true;
}

Asset* Content::LoadAsync(const Guid& id, const ScriptingTypeHandle& type)
{
    // Early out
    if (!id.IsValid())
    {
        // Back
        return nullptr;
    }

    // Check if asset has been already loaded
    Asset* result = GetAsset(id);
    if (result)
    {
        // Validate type
        if (IsAssetTypeIdInvalid(type, result->GetTypeHandle()) && !result->Is(type))
        {
            LOG(Warning, "Different loaded asset type! Asset: \'{0}\'. Expected type: {1}", result->ToString(), type.ToString());
            return nullptr;
        }

        return result;
    }

    // Check if that asset is during loading
    LoadCallAssetsLocker.Lock();
    if (LoadCallAssets.Contains(id))
    {
        LoadCallAssetsLocker.Unlock();

        // Wait for load end
        // TODO: dont use active waiting and prevent deadlocks if running on a main thread
        //while (!Engine::ShouldExit())
        while (true)
        {
            LoadCallAssetsLocker.Lock();
            const bool contains = LoadCallAssets.Contains(id);
            LoadCallAssetsLocker.Unlock();

            if (!contains)
            {
                return GetAsset(id);
            }

            Platform::Sleep(1);
        }
    }
    else
    {
        // Mark asset as loading
        LoadCallAssets.Add(id);

        LoadCallAssetsLocker.Unlock();
    }

    // Load asset
    AssetInfo assetInfo;
    result = load(id, type, assetInfo);

    // End loading
    LoadCallAssetsLocker.Lock();
    LoadCallAssets.Remove(id);
    LoadCallAssetsLocker.Unlock();

    return result;
}

Asset* Content::load(const Guid& id, const ScriptingTypeHandle& type, AssetInfo& assetInfo)
{
    // Get cached asset info (from registry)
    if (!GetAssetInfo(id, assetInfo))
    {
        LOG(Warning, "Invalid asset ID ({0}).", id.ToString(Guid::FormatType::N));
        return nullptr;
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION

    // Ensure we have valid asset info
    ASSERT(assetInfo.TypeName.HasChars() && assetInfo.Path.HasChars());

    // Check if file exists
    if (!FileSystem::FileExists(assetInfo.Path))
    {
        LOG(Error, "Cannot find file '{0}'", assetInfo.Path);
        return nullptr;
    }

#endif

    // Find asset factory based in its type
    auto factory = GetAssetFactory(assetInfo);
    if (factory == nullptr)
    {
        LOG(Error, "Cannot find asset factory. Info: {0}", assetInfo.ToString());
        return nullptr;
    }

    // Create asset object
    auto result = factory->New(assetInfo);
    if (result == nullptr)
    {
        LOG(Error, "Cannot create asset object. Info: {0}", assetInfo.ToString());
        return nullptr;
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION

    // Validate type
    if (IsAssetTypeIdInvalid(type, result->GetTypeHandle()) && !result->Is(type))
    {
        LOG(Error, "Different loaded asset type! Asset: '{0}'. Expected type: {1}", assetInfo.ToString(), type.ToString());
        result->DeleteObject();
        return nullptr;
    }

#endif

    // Register asset
    {
        AssetsLocker.Lock();

#if ASSETS_LOADING_EXTRA_VERIFICATION

        // Asset id has to be unique
        if (Assets.ContainsKey(id))
        {
            CRASH;
        }

#endif

        // Add to the list
        ASSERT(result->GetID() == id);
        Assets.Add(id, result);

        AssetsLocker.Unlock();
    }

    // Start asset loading
    result->startLoading();

    return result;
}

#if ENABLE_ASSETS_DISCOVERY

bool findAsset(const Guid& id, const String& directory, Array<String>& tmpCache, AssetInfo& info)
{
    // Get all asset files
    tmpCache.Clear();
    if (FileSystem::DirectoryGetFiles(tmpCache, directory, TEXT("*"), DirectorySearchOption::AllDirectories))
    {
        LOG(Error, "Cannot query files in folder '{0}'.", directory);
        return false;
    }

    // Start searching for asset with given ID
    bool result = false;
    LOG(Info, "Start searching asset with ID: {0} in '{1}'. {2} potential files to check...", id, directory, tmpCache.Count());
    for (int32 i = 0; i < tmpCache.Count(); i++)
    {
        String& path = tmpCache[i];

        // Check if not already in registry
        // Note: maybe we could disable this check? it would slow down searching but we will find more workspace problems
        if (!Cache.HasAsset(path))
        {
            auto extension = FileSystem::GetExtension(path).ToLower();

            // Check if it's a binary asset
            if (ContentStorageManager::IsFlaxStorageExtension(extension))
            {
                // Skip packages in editor (results in conflicts with build game packages if deployed inside project folder)
#if USE_EDITOR
                if (extension == PACKAGE_FILES_EXTENSION)
                    continue;
#endif

                // Open storage
                auto storage = ContentStorageManager::GetStorage(path);
                if (storage)
                {
                    // Register assets
                    Cache.RegisterAssets(storage);

                    // Check if that is a missing asset
                    if (storage->HasAsset(id))
                    {
                        // Found
                        result = Cache.FindAsset(id, info);
                        LOG(Info, "Found {1} at '{0}'!", id, path);
                    }
                }
                else
                {
                    LOG(Error, "Cannot open file '{0}' error code: {1}", path, 0);
                }
            }
                // Check for json resource
            else if (JsonStorageProxy::IsValidExtension(extension))
            {
                // Check Json storage layer
                Guid jsonId;
                String jsonTypeName;
                if (JsonStorageProxy::GetAssetInfo(path, jsonId, jsonTypeName))
                {
                    // Register asset
                    Cache.RegisterAsset(jsonId, jsonTypeName, path);

                    // Check if that is a missing asset
                    if (id == jsonId)
                    {
                        // Found
                        result = Cache.FindAsset(id, info);
                        LOG(Info, "Found {1} at '{0}'!", id, path);
                    }
                }
            }
        }
    }

    return result;
}

#endif
