// Copyright (c) Wojciech Figat. All rights reserved.

#include "Content.h"
#include "JsonAsset.h"
#include "SceneReference.h"
#include "Cache/AssetsCache.h"
#include "Storage/ContentStorageManager.h"
#include "Storage/JsonStorageProxy.h"
#include "Factories/IAssetFactory.h"
#include "Loading/LoadingThread.h"
#include "Loading/ContentLoadTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Threading/ConcurrentTaskQueue.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Level/Types.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/Scripting.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#endif
#if ENABLE_ASSETS_DISCOVERY
#include "Engine/Core/Collections/HashSet.h"
#endif
#if USE_EDITOR && PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include <propidlbase.h>
#endif

TimeSpan Content::AssetsUpdateInterval = TimeSpan::FromMilliseconds(500);
TimeSpan Content::AssetsUnloadInterval = TimeSpan::FromSeconds(10);
Delegate<Asset*> Content::AssetDisposing;
Delegate<Asset*> Content::AssetReloading;

String AssetInfo::ToString() const
{
    return String::Format(TEXT("ID: {0}, TypeName: {1}, Path: \'{2}\'"), ID, TypeName, Path);
}

void FLAXENGINE_API Serialization::Serialize(ISerializable::SerializeStream& stream, const SceneReference& v, const void* otherObj)
{
    Serialize(stream, v.ID, otherObj);
}

void FLAXENGINE_API Serialization::Deserialize(ISerializable::DeserializeStream& stream, SceneReference& v, ISerializeModifier* modifier)
{
    Deserialize(stream, v.ID, modifier);
}

namespace
{
    // Assets
    CriticalSection AssetsLocker;
    Dictionary<Guid, Asset*> Assets(2048);
    Array<Guid> LoadCallAssets(PLATFORM_THREADS_LIMIT);
    CriticalSection LoadedAssetsToInvokeLocker;
    Array<Asset*> LoadedAssetsToInvoke(64);
    Array<Asset*> ToUnload;

    // Assets Registry Stuff
    AssetsCache Cache;

    // Loading assets
    THREADLOCAL LoadingThread* ThisLoadThread = nullptr;
    LoadingThread* MainLoadThread = nullptr;
    Array<LoadingThread*> LoadThreads;
    ConcurrentTaskQueue<ContentLoadTask> LoadTasks;
    ConditionVariable LoadTasksSignal;
    CriticalSection LoadTasksMutex;

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
    void BeforeExit() override;
    void Dispose() override;
};

ContentService ContentServiceInstance;

bool ContentService::Init()
{
    // Load assets registry
    Cache.Init();

    // Create loading threads
    const CPUInfo cpuInfo = Platform::GetCPUInfo();
    const int32 count = Math::Clamp(Math::CeilToInt(LOADING_THREAD_PER_LOGICAL_CORE * (float)cpuInfo.LogicalProcessorCount), 1, 12);
    LOG(Info, "Creating {0} content loading threads...", count);
    MainLoadThread = New<LoadingThread>();
    ThisLoadThread = MainLoadThread;
    LoadThreads.EnsureCapacity(count);
    for (int32 i = 0; i < count; i++)
    {
        auto thread = New<LoadingThread>();
        if (thread->Start(String::Format(TEXT("Load Thread {0}"), i)))
        {
            LOG(Fatal, "Cannot spawn content thread {0}/{1}", i, count);
            Delete(thread);
            return true;
        }
        LoadThreads.Add(thread);
    }

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
    AssetsLocker.Lock();

    // Verify all assets
    for (auto i = Assets.Begin(); i.IsNotEnd(); ++i)
    {
        Asset* asset = i->Value;

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
        Asset* asset = ToUnload[i];

        // Check if has no references
        if (asset->GetReferencesCount() <= 0)
        {
            Content::UnloadAsset(asset);
        }

        // Remove from unload queue
        UnloadQueue.Remove(asset);
    }

    AssetsLocker.Unlock();

    // Update cache (for longer sessions it will help to reduce cache misses)
    Cache.Save();
}

void ContentService::BeforeExit()
{
    // Signal threads to end work soon
    for (auto thread : LoadThreads)
        thread->NotifyExit();
    LoadTasksSignal.NotifyAll();
}

void ContentService::Dispose()
{
    IsExiting = true;

    // Save assets registry before engine closing
    Cache.Save();

    // Flush objects (some asset-related objects/references may be pending to delete)
    ObjectsRemovalService::Flush();

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

    // Exit all load threads
    for (auto thread : LoadThreads)
        thread->NotifyExit();
    LoadTasksSignal.NotifyAll();
    for (auto thread : LoadThreads)
        thread->Join();
    LoadThreads.ClearDelete();
    Delete(MainLoadThread);
    MainLoadThread = nullptr;
    ThisLoadThread = nullptr;

    // Cancel all remaining tasks (no chance to execute them)
    LoadTasks.CancelAll();
}

IAssetFactory::Collection& IAssetFactory::Get()
{
    static Collection Factories(1024);
    return Factories;
}

LoadingThread::LoadingThread()
    : _exitFlag(false)
    , _thread(nullptr)
    , _totalTasksDoneCount(0)
{
}

LoadingThread::~LoadingThread()
{
    if (_thread != nullptr)
    {
        _thread->Kill(true);
        Delete(_thread);
    }
}

void LoadingThread::NotifyExit()
{
    Platform::InterlockedIncrement(&_exitFlag);
}

void LoadingThread::Join()
{
    auto thread = _thread;
    if (thread)
        thread->Join();
}

bool LoadingThread::Start(const String& name)
{
    ASSERT(_thread == nullptr && name.HasChars());

    // Create new thread
    auto thread = Thread::Create(this, name, ThreadPriority::Normal);
    if (thread == nullptr)
        return true;

    _thread = thread;

    return false;
}

void LoadingThread::Run(ContentLoadTask* job)
{
    ASSERT(job);

    job->Execute();
    _totalTasksDoneCount++;
}

String LoadingThread::ToString() const
{
    return String::Format(TEXT("Loading Thread {0}"), _thread ? _thread->GetID() : 0);
}

int32 LoadingThread::Run()
{
#if USE_EDITOR && PLATFORM_WINDOWS
    // Initialize COM
    // TODO: maybe add sth to Thread::Create to indicate that thread will use COM stuff
    const auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result))
    {
        LOG(Error, "Failed to init COM for WIC texture importing! Result: {0:x}", static_cast<uint32>(result));
        return -1;
    }
#endif

    ContentLoadTask* task;
    ThisLoadThread = this;

    while (Platform::AtomicRead(&_exitFlag) == 0)
    {
        if (LoadTasks.try_dequeue(task))
        {
            Run(task);
        }
        else
        {
            LoadTasksMutex.Lock();
            LoadTasksSignal.Wait(LoadTasksMutex);
            LoadTasksMutex.Unlock();
        }
    }

    ThisLoadThread = nullptr;
    return 0;
}

void LoadingThread::Exit()
{
    LOG(Info, "Content thread '{0}' exited. Load calls: {1}", _thread->GetName(), _totalTasksDoneCount);
}

String ContentLoadTask::ToString() const
{
    return String::Format(TEXT("Content Load Task ({})"), (int32)GetState());
}

void ContentLoadTask::Enqueue()
{
    LoadTasks.Add(this);
    LoadTasksSignal.NotifyOne();
}

bool ContentLoadTask::Run()
{
    const auto result = run();
    const bool failed = result != Result::Ok;
    if (failed)
    {
        LOG(Warning, "\'{0}\' failed with result: {1}", ToString(), ToString(result));
    }
    return failed;
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
    if (!id.IsValid())
        return false;

#if ENABLE_ASSETS_DISCOVERY
    // Find asset in registry
    if (Cache.FindAsset(id, info))
        return true;
    PROFILE_CPU();

    // Locking injects some stalls but we need to make it safe (only one thread can pass though it at once)
    ScopeLock lock(WorkspaceDiscoveryLocker);

    // Check if we can search workspace
    // Note: we want to limit searching frequency due to high I/O usage and thread stall
    // We also perform full workspace discovery so all new assets will be found
    auto now = DateTime::NowUTC();
    auto diff = now - LastWorkspaceDiscovery;
    if (diff <= TimeSpan::FromSeconds(5))
    {
        //LOG(Warning, "Cannot perform workspace scan for '{1}'. Too often call. Time diff: {0} ms", static_cast<int32>(diff.GetTotalMilliseconds()), id);
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

    //LOG(Warning, "Cannot find {0}.", id);
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
    if (!FileSystem::FileExists(path))
        return false;
    PROFILE_CPU();

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
#if BUILD_DEBUG || FLAX_TESTS
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

String Content::GetEditorAssetPath(const Guid& id)
{
    return Cache.GetEditorAssetPath(id);
}

Array<Guid> Content::GetAllAssets()
{
    Array<Guid> result;
    Cache.GetAll(result);
    return result;
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

ContentStats Content::GetStats()
{
    ContentStats stats;
    AssetsLocker.Lock();
    stats.AssetsCount = Assets.Count();
    int32 loadFailedCount = 0;
    for (const auto& e : Assets)
    {
        if (e.Value->IsLoaded())
            stats.LoadedAssetsCount++;
        else if (e.Value->LastLoadFailed())
            loadFailedCount++;
        if (e.Value->IsVirtual())
            stats.VirtualAssetsCount++;
    }
    stats.LoadingAssetsCount = stats.AssetsCount - loadFailedCount - stats.LoadedAssetsCount;
    AssetsLocker.Unlock();
    return stats;
}

Asset* Content::LoadAsyncInternal(const StringView& internalPath, const MClass* type)
{
    CHECK_RETURN(type, nullptr);
    const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
    if (scriptingType)
        return LoadAsyncInternal(internalPath, scriptingType);
    LOG(Error, "Failed to find asset type '{0}'.", String(type->GetFullName()));
    return nullptr;
}

Asset* Content::LoadAsyncInternal(const StringView& internalPath, const ScriptingTypeHandle& type)
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

Asset* Content::LoadAsyncInternal(const Char* internalPath, const ScriptingTypeHandle& type)
{
    return LoadAsyncInternal(StringView(internalPath), type);
}

FLAXENGINE_API Asset* LoadAsset(const Guid& id, const ScriptingTypeHandle& type)
{
    return Content::LoadAsync(id, type);
}

Asset* Content::LoadAsync(const StringView& path, const MClass* type)
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
    // Ensure path is in a valid format
    String pathNorm(path);
    ContentStorageManager::FormatPath(pathNorm);
    const StringView filePath = pathNorm;

#if USE_EDITOR
    if (!FileSystem::FileExists(filePath))
    {
        LOG(Error, "Missing file \'{0}\'", filePath);
        return nullptr;
    }
#endif

    AssetInfo assetInfo;
    if (GetAssetInfo(filePath, assetInfo))
    {
        return LoadAsync(assetInfo.ID, type);
    }

    return nullptr;
}

Array<Asset*> Content::GetAssets()
{
    Array<Asset*> assets;
    AssetsLocker.Lock();
    Assets.GetValues(assets);
    AssetsLocker.Unlock();
    return assets;
}

const Dictionary<Guid, Asset*>& Content::GetAssetsRaw()
{
    AssetsLocker.Lock();
    AssetsLocker.Unlock();
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
    if (asset == nullptr || asset->_deleteFileOnUnload)
        return;

    LOG(Info, "Deleting asset {0}...", asset->ToString());

    // Ensure that asset is loaded (easier than cancel in-flight loading)
    asset->WaitForLoaded();

    // Mark asset for delete queue (delete it after auto unload)
    asset->_deleteFileOnUnload = true;

    // Unload
    asset->DeleteObject();
}

void Content::DeleteAsset(const StringView& path)
{
    PROFILE_CPU();

    // Try to delete already loaded asset
    Asset* asset = GetAsset(path);
    if (asset != nullptr)
    {
        DeleteAsset(asset);
        return;
    }

    ScopeLock locker(AssetsLocker);

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
    if (!id.IsValid())
    {
        LOG(Warning, "Cannot remove file \'{0}\'. Given ID is invalid.", path);
        return;
    }
    PROFILE_CPU();

    // Ensure that file has the same ID (prevent from deleting different assets)
    auto storage = ContentStorageManager::TryGetStorage(path);
    if (storage)
    {
        storage->CloseFileHandles(); // Close file handle to allow removing it
        if (!storage->HasAsset(id))
        {
            LOG(Warning, "Cannot remove file \'{0}\'. It doesn\'t contain asset {1}.", path, id);
            return;
        }
    }

#if PLATFORM_WINDOWS || PLATFORM_LINUX
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

class CloneAssetFileTask : public MainThreadTask
{
public:
    StringView dstPath;
    StringView srcPath;
    Guid dstId;
    bool* output;

protected:
    bool Run() override
    {
        *output = Content::CloneAssetFile(dstPath, srcPath, dstId);
        return false;
    }
};

bool Content::CloneAssetFile(const StringView& dstPath, const StringView& srcPath, const Guid& dstId)
{
    // Best to run this on the main thread to avoid clone conflicts.
    if (IsInMainThread())
    {
        PROFILE_CPU();
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
                if (!storage)
                {
                    LOG(Warning, "Cannot change asset ID.");
                    return true;
                }
                FlaxStorage::Entry e;
                storage->GetEntry(0, e);
                if (storage->ChangeAssetID(e, dstId))
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
            if (auto storage = ContentStorageManager::GetStorage(dstPath))
            {
                storage->Reload();
            }
        }
    }
    else
    {
        CloneAssetFileTask* task = New<CloneAssetFileTask>();
        task->dstId = dstId;
        task->dstPath = dstPath;
        task->srcPath = srcPath;

        bool result = false;
        task->output = &result;
        task->Start();
        task->Wait();

        return result;
    }

    return false;
}

#endif

void Content::UnloadAsset(Asset* asset)
{
    if (asset == nullptr)
        return;
    asset->DeleteObject();
}

Asset* Content::CreateVirtualAsset(const MClass* type)
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
    PROFILE_CPU();
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
    asset->RegisterObject();

    // Call initializer function
    asset->InitAsVirtual();

    // Register asset
    AssetsLocker.Lock();
    ASSERT(!Assets.ContainsKey(asset->GetID()));
    Assets.Add(asset->GetID(), asset);
    AssetsLocker.Unlock();

    return asset;
}

void Content::WaitForTask(ContentLoadTask* loadingTask, double timeoutInMilliseconds)
{
    // Check if call is made from the Loading Thread and task has not been taken yet
    auto thread = ThisLoadThread;
    if (thread != nullptr)
    {
        // Note: to reproduce this case just include material into material (use layering).
        // So during loading first material it will wait for child materials loaded calling this function

        const double timeoutInSeconds = timeoutInMilliseconds * 0.001;
        const double startTime = Platform::GetTimeSeconds();
        Task* task = loadingTask;
        Array<ContentLoadTask*, InlinedAllocation<64>> localQueue;
#define CHECK_CONDITIONS() (!Engine::ShouldExit() && (timeoutInSeconds <= 0.0 || Platform::GetTimeSeconds() - startTime < timeoutInSeconds))
        do
        {
            // Try to execute content tasks
            while (task->IsQueued() && CHECK_CONDITIONS())
            {
                // Dequeue task from the loading queue
                ContentLoadTask* tmp;
                if (LoadTasks.try_dequeue(tmp))
                {
                    if (tmp == task)
                    {
                        if (localQueue.Count() != 0)
                        {
                            // Put back queued tasks
                            LoadTasks.enqueue_bulk(localQueue.Get(), localQueue.Count());
                            localQueue.Clear();
                        }

                        thread->Run(tmp);
                    }
                    else
                    {
                        localQueue.Add(tmp);
                    }
                }
                else
                {
                    // No task in queue but it's queued so other thread could have stolen it into own local queue
                    break;
                }
            }
            if (localQueue.Count() != 0)
            {
                // Put back queued tasks
                LoadTasks.enqueue_bulk(localQueue.Get(), localQueue.Count());
                localQueue.Clear();
            }

            // Check if task is done
            if (task->IsEnded())
            {
                // If was fine then wait for the next task
                if (task->IsFinished())
                {
                    task = task->GetContinueWithTask();
                    if (!task)
                        break;
                }
                else
                {
                    // Failed or cancelled so this wait also fails
                    break;
                }
            }
        } while (CHECK_CONDITIONS());
#undef CHECK_CONDITIONS
    }
    else
    {
        // Wait for task end
        loadingTask->Wait(timeoutInMilliseconds);
    }
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

void Content::onAssetChangeId(Asset* asset, const Guid& oldId, const Guid& newId)
{
    ScopeLock locker(AssetsLocker);
    Assets.Remove(oldId);
    Assets.Add(newId, asset);
}

bool Content::IsAssetTypeIdInvalid(const ScriptingTypeHandle& type, const ScriptingTypeHandle& assetType)
{
    // Skip if no restrictions for the type
    if (!type || !assetType)
        return false;

#if BUILD_DEBUG || FLAX_TESTS
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
    if (!id.IsValid())
        return nullptr;

    // Check if asset has been already loaded
    Asset* result = nullptr;
    AssetsLocker.Lock();
    Assets.TryGet(id, result);
    if (result)
    {
        AssetsLocker.Unlock();

        // Validate type
        if (IsAssetTypeIdInvalid(type, result->GetTypeHandle()) && !result->Is(type))
        {
            LOG(Warning, "Different loaded asset type! Asset: \'{0}\'. Expected type: {1}", result->ToString(), type.ToString());
            LogContext::Print(LogType::Warning);
            return nullptr;
        }
        return result;
    }

    // Check if that asset is during loading
    if (LoadCallAssets.Contains(id))
    {
        AssetsLocker.Unlock();

        // Wait for loading end by other thread
        bool contains = true;
        while (contains)
        {
            Platform::Sleep(1);
            AssetsLocker.Lock();
            contains = LoadCallAssets.Contains(id);
            AssetsLocker.Unlock();
        }
        Assets.TryGet(id, result);
        return result;
    }

    // Mark asset as loading and release lock so other threads can load other assets
    LoadCallAssets.Add(id);
    AssetsLocker.Unlock();

#define LOAD_FAILED() AssetsLocker.Lock(); LoadCallAssets.Remove(id); AssetsLocker.Unlock(); return nullptr

    // Get cached asset info (from registry)
    AssetInfo assetInfo;
    if (!GetAssetInfo(id, assetInfo))
    {
        LOG(Warning, "Invalid or missing asset ({0}, {1}).", id, type.ToString());
        LogContext::Print(LogType::Warning);
        LOAD_FAILED();
    }
#if ASSETS_LOADING_EXTRA_VERIFICATION
    if (!FileSystem::FileExists(assetInfo.Path))
    {
        LOG(Error, "Cannot find file '{0}'", assetInfo.Path);
        LOAD_FAILED();
    }
#endif

    // Find asset factory based in its type
    auto factory = GetAssetFactory(assetInfo);
    if (factory == nullptr)
    {
        LOG(Error, "Cannot find asset factory. Info: {0}", assetInfo.ToString());
        LOAD_FAILED();
    }

    // Create asset object
    result = factory->New(assetInfo);
    if (result == nullptr)
    {
        LOG(Error, "Cannot create asset object. Info: {0}", assetInfo.ToString());
        LOAD_FAILED();
    }
    ASSERT(result->GetID() == id);
#if ASSETS_LOADING_EXTRA_VERIFICATION
    if (IsAssetTypeIdInvalid(type, result->GetTypeHandle()) && !result->Is(type))
    {
        LOG(Warning, "Different loaded asset type! Asset: '{0}'. Expected type: {1}", assetInfo.ToString(), type.ToString());
        result->DeleteObject();
        LOAD_FAILED();
    }
#endif
    if (!result->IsInternalType())
        result->RegisterObject();

    // Register asset
    AssetsLocker.Lock();
#if ASSETS_LOADING_EXTRA_VERIFICATION
    ASSERT(!Assets.ContainsKey(id));
#endif
    Assets.Add(id, result);

    // Start asset loading
    result->startLoading();

    // Remove from the loading queue and release lock
    LoadCallAssets.Remove(id);
    AssetsLocker.Unlock();

#undef LOAD_FAILED

    return result;
}

#if ENABLE_ASSETS_DISCOVERY

bool findAsset(const Guid& id, const String& directory, Array<String>& tmpCache, AssetInfo& info)
{
    // Get all asset files
    tmpCache.Clear();
    if (FileSystem::DirectoryGetFiles(tmpCache, directory))
    {
        if (FileSystem::DirectoryExists(directory))
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
