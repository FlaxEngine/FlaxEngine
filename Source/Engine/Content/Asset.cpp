// Copyright (c) Wojciech Figat. All rights reserved.

#include "Asset.h"
#include "Content.h"
#include "Deprecated.h"
#include "SoftAssetReference.h"
#include "Cache/AssetsCache.h"
#include "Loading/Tasks/LoadAssetTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Threading/ThreadLocal.h"

#if USE_EDITOR

ThreadLocal<bool> ContentDeprecatedFlags;

void ContentDeprecated::Mark()
{
    ContentDeprecatedFlags.Set(true);
}

bool ContentDeprecated::Clear(bool newValue)
{
    auto& flag = ContentDeprecatedFlags.Get();
    bool result = flag;
    flag = newValue;
    return result;
}

#endif

AssetReferenceBase::~AssetReferenceBase()
{
    Asset* asset = _asset;
    if (asset)
    {
        _asset = nullptr;
        asset->OnLoaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnLoaded>(this);
        asset->OnUnloaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnUnloaded>(this);
        asset->RemoveReference();
    }
}

String AssetReferenceBase::ToString() const
{
    return _asset ? _asset->ToString() : TEXT("<null>");
}

void AssetReferenceBase::OnSet(Asset* asset)
{
    auto e = _asset;
    if (e != asset)
    {
        if (e)
        {
            e->OnLoaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnLoaded>(this);
            e->OnUnloaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnUnloaded>(this);
            e->RemoveReference();
        }
        _asset = e = asset;
        if (e)
        {
            e->AddReference();
            e->OnLoaded.Bind<AssetReferenceBase, &AssetReferenceBase::OnLoaded>(this);
            e->OnUnloaded.Bind<AssetReferenceBase, &AssetReferenceBase::OnUnloaded>(this);
        }
        Changed();
        if (e && e->IsLoaded())
            Loaded();
    }
}

void AssetReferenceBase::OnLoaded(Asset* asset)
{
    if (_asset != asset)
        return;
    Loaded();
}

void AssetReferenceBase::OnUnloaded(Asset* asset)
{
    if (_asset != asset)
        return;
    Unload();
    OnSet(nullptr);
}

WeakAssetReferenceBase::~WeakAssetReferenceBase()
{
    Asset* asset = _asset;
    if (asset)
    {
        _asset = nullptr;
        asset->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnUnloaded>(this);
    }
}

String WeakAssetReferenceBase::ToString() const
{
    return _asset ? _asset->ToString() : TEXT("<null>");
}

void WeakAssetReferenceBase::OnSet(Asset* asset)
{
    auto e = _asset;
    if (e != asset)
    {
        if (e)
            e->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnUnloaded>(this);
        _asset = e = asset;
        if (e)
            e->OnUnloaded.Bind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnUnloaded>(this);
    }
}

void WeakAssetReferenceBase::OnUnloaded(Asset* asset)
{
    if (_asset != asset)
        return;
    Unload();
    asset->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnUnloaded>(this);
    _asset = nullptr;
}

SoftAssetReferenceBase::~SoftAssetReferenceBase()
{
    Asset* asset = _asset;
    if (asset)
    {
        _asset = nullptr;
        asset->OnUnloaded.Unbind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
        asset->RemoveReference();
    }
#if !BUILD_RELEASE
    _id = Guid::Empty;
#endif
}

String SoftAssetReferenceBase::ToString() const
{
    return _asset ? _asset->ToString() : (_id.IsValid() ? _id.ToString() : TEXT("<null>"));
}

void SoftAssetReferenceBase::OnSet(Asset* asset)
{
    if (_asset == asset)
        return;
    if (_asset)
    {
        _asset->OnUnloaded.Unbind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
        _asset->RemoveReference();
    }
    _asset = asset;
    _id = asset ? asset->GetID() : Guid::Empty;
    if (asset)
    {
        asset->AddReference();
        asset->OnUnloaded.Bind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
    }
    Changed();
}

void SoftAssetReferenceBase::OnSet(const Guid& id)
{
    if (_id == id)
        return;
    if (_asset)
    {
        _asset->OnUnloaded.Unbind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
        _asset->RemoveReference();
    }
    _asset = nullptr;
    _id = id;
    Changed();
}

void SoftAssetReferenceBase::OnResolve(const ScriptingTypeHandle& type)
{
    ASSERT(!_asset);
    _asset = ::LoadAsset(_id, type);
    if (_asset)
    {
        _asset->OnUnloaded.Bind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
        _asset->AddReference();
    }
}

void SoftAssetReferenceBase::OnUnloaded(Asset* asset)
{
    if (_asset != asset)
        return;
    _asset->RemoveReference();
    _asset->OnUnloaded.Unbind<SoftAssetReferenceBase, &SoftAssetReferenceBase::OnUnloaded>(this);
    _asset = nullptr;
    _id = Guid::Empty;
    Changed();
}

Asset::Asset(const SpawnParams& params, const AssetInfo* info)
    : ManagedScriptingObject(params)
    , _refCount(0)
    , _loadState(0)
    , _loadingTask(0)
    , _deleteFileOnUnload(false)
    , _isVirtual(false)
{
}

int32 Asset::GetReferencesCount() const
{
    return (int32)Platform::AtomicRead(const_cast<int64 volatile*>(&_refCount));
}

String Asset::ToString() const
{
    return String::Format(TEXT("{0}, {1}, {2}"), GetTypeName(), GetID(), GetPath());
}

void Asset::OnDeleteObject()
{
    ASSERT(IsInMainThread());

    // Send event to the gameplay so it can release handle to this asset
    // This may happen when asset gets removed but sth is still referencing it (eg. in managed code)
    if (!IsInternalType())
        Content::AssetDisposing(this);

    const bool wasMarkedToDelete = _deleteFileOnUnload != 0;
#if USE_EDITOR
    const String path = wasMarkedToDelete ? GetPath() : String::Empty;
#endif
    const Guid id = GetID();

    // Fire unload event (every object referencing this asset or it's data should release reference so later actions are safe)
    onUnload_MainThread();

    // Remove from pool
    Content::onAssetUnload(this);

    // Unload asset data (in a safe way to protect asset data)
    Locker.Lock();
    if (IsLoaded())
    {
        unload(false);
        Platform::AtomicStore(&_loadState, (int64)LoadState::Unloaded);
    }
    Locker.Unlock();

#if BUILD_DEBUG
    // Ensure no object is referencing it (except managed instance if exists)
    //ASSERT(_refCount == (HasManagedInstance() ? 1 : 0));
#endif

    // Base (after it `this` is invalid)
    ManagedScriptingObject::OnDeleteObject();

#if USE_EDITOR
    if (wasMarkedToDelete)
    {
        LOG(Info, "Deleting asset '{0}':{1}.", path, id.ToString());

        // Remove from registry
        Content::GetRegistry()->DeleteAsset(id, nullptr);

        // Delete file
        if (!IsVirtual())
            Content::deleteFileSafety(path, id);
    }
#endif
}

bool Asset::CreateManaged()
{
    // Base
    if (ManagedScriptingObject::CreateManaged())
        return true;

    // Managed objects holds a reference to this asset until it will be removed by GC
    AddReference();

    return false;
}

void Asset::DestroyManaged()
{
    // Clear reference
    if (HasManagedInstance())
    {
        RemoveReference();
    }

    // Base
    ManagedScriptingObject::DestroyManaged();
}

void Asset::OnManagedInstanceDeleted()
{
    // Clear reference
    RemoveReference();

    // Cleanup
    if (_gcHandle)
    {
        MCore::GCHandle::Free(_gcHandle);
        _gcHandle = 0;
    }

    // But do not delete itself
}

void Asset::OnScriptingDispose()
{
    // Delete C# object
    if (IsRegistered())
        UnregisterObject();
    DestroyManaged();

    // Don't delete C++ object
}

void Asset::ChangeID(const Guid& newId)
{
    // Only virtual asset can change ID
    if (!IsVirtual())
        CRASH;

    // ID has to be unique
    if (Content::GetAsset(newId) != nullptr)
        CRASH;

    const Guid oldId = _id;
    ManagedScriptingObject::ChangeID(newId);
    Content::onAssetChangeId(this, oldId, newId);
}

#if USE_EDITOR

bool Asset::ShouldDeleteFileOnUnload() const
{
    return _deleteFileOnUnload != 0;
}

#endif

uint64 Asset::GetMemoryUsage() const
{
    uint64 result = sizeof(Asset);
    Locker.Lock();
    if (Platform::AtomicRead(&_loadingTask))
        result += sizeof(ContentLoadTask);
    result += (OnLoaded.Capacity() + OnReloading.Capacity() + OnUnloaded.Capacity()) * sizeof(EventType::FunctionType);
    Locker.Unlock();
    return result;
}

void Asset::Reload()
{
    // Virtual assets are memory-only so reloading them makes no sense
    if (IsVirtual())
        return;
    PROFILE_CPU_NAMED("Asset.Reload");

    // It's better to call it from the main thread
    if (IsInMainThread())
    {
        LOG(Info, "Reloading asset {0}", ToString());

        WaitForLoaded();

        // Fire event
        if (!IsInternalType())
            Content::AssetReloading(this);
        OnReloading(this);

        ScopeLock lock(Locker);

        if (IsLoaded())
        {
            // Unload current data
            unload(true);
            Platform::AtomicStore(&_loadState, (int64)LoadState::Unloaded);
        }

        // Start reloading process
        startLoading();
    }
    else
    {
        Function<void()> action;
        action.Bind<Asset, &Asset::Reload>(this);
        Task::StartNew(New<MainThreadActionTask>(action, this));
    }
}

bool Asset::WaitForLoaded(double timeoutInMilliseconds) const
{
    // This function is used many time when some parts of the engine need to wait for asset loading end (it may fail but has to end).
    // But it cannot be just a simple active-wait loop.
    // Imagine following situation:
    // Content Pool has 2 loading threads.
    // Both start to load layered materials which need to be recompiled (Material Generator work).
    // Every of these materials is made of a few layers.
    // To load child layer Material Generator is requesting Content Pool to load it fully.
    // However this cannot be done because Pool has limited threads and all of them may requesting more loads to be done.
    //
    // To solve that problem we have different solutions:
    // 1) add more loading threads (bad idea)
    // 2) content loading could use thread pool to enqueue single tasks (still risky due to many tasks queued and limited Thread Pool size (need to build graph of dependencies?))
    // 3) content loading could detect asset load calls from content loader threads and load requested asset without stalls
    // 4) every asset could expose dependencies and content system could load required dependencies earlier
    //
    // 3) and 4) are good solutions. 4) would require more work but will be needed in future for building system to gather assets for game packages.
    // But WaitForLoaded could detect if is called from the Loading Thead and manually load dependent asset. It's fairly easy to do and will work out.

    // Early out if asset has been already loaded
    if (IsLoaded())
    {
        // If running on a main thread we can flush asset `Loaded` event
        if (IsInMainThread())
        {
            Content::tryCallOnLoaded((Asset*)this);
        }

        return false;
    }

    // Check if loading failed
    Platform::MemoryBarrier();
    if (LastLoadFailed())
        return true;

    // Check if has missing loading task
    Platform::MemoryBarrier();
    const auto loadingTask = (ContentLoadTask*)Platform::AtomicRead(&_loadingTask);
    if (loadingTask == nullptr)
    {
        LOG(Warning, "WaitForLoaded asset \'{0}\' failed. No loading task attached and asset is not loaded.", ToString());
        return true;
    }

    PROFILE_CPU();

    Content::WaitForTask(loadingTask, timeoutInMilliseconds);

    // If running on a main thread we can flush asset `Loaded` event
    if (IsInMainThread() && IsLoaded())
    {
        Content::tryCallOnLoaded((Asset*)this);
    }

    return !IsLoaded();
}

void Asset::InitAsVirtual()
{
    // Be a virtual thing
    _isVirtual = true;

    // Be a loaded thing
    Platform::AtomicStore(&_loadState, (int64)LoadState::Loaded);
}

void Asset::CancelStreaming()
{
    // Cancel loading task but go over asset locker to prevent case if other load threads still loads asset while it's reimported on other thread
    Locker.Lock();
    auto loadingTask = (ContentLoadTask*)Platform::AtomicRead(&_loadingTask);
    Locker.Unlock();
    if (loadingTask)
    {
        Platform::AtomicStore(&_loadingTask, 0);
        LOG(Warning, "Cancel loading task for \'{0}\'", ToString());
        loadingTask->Cancel();
    }
}

#if USE_EDITOR

void Asset::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    // Fallback to the old API
PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    GetReferences(assets);
PRAGMA_ENABLE_DEPRECATION_WARNINGS;
}

void Asset::GetReferences(Array<Guid>& output) const
{
    // No refs by default
}

Array<Guid> Asset::GetReferences() const
{
    Array<Guid> result;
    Array<String> files;
    GetReferences(result, files);
    return result;
}

bool Asset::Save(const StringView& path)
{
    LOG(Warning, "Asset type '{}' does not support saving.", GetTypeName());
    return true;
}

#endif

void Asset::DeleteManaged()
{
    if (HasManagedInstance())
    {
        if (IsRegistered())
            UnregisterObject();
        DestroyManaged();
    }
}

ContentLoadTask* Asset::createLoadingTask()
{
    // Spawn new task and cache pointer
    return New<LoadAssetTask>(this);
}

void Asset::startLoading()
{
    ASSERT(!IsLoaded());
    ASSERT(Platform::AtomicRead(&_loadingTask) == 0);
    auto loadingTask = createLoadingTask();
    ASSERT(loadingTask != nullptr);
    Platform::AtomicStore(&_loadingTask, (intptr)loadingTask);
    loadingTask->Start();
}

void Asset::releaseStorage()
{
}

bool Asset::IsInternalType() const
{
    return false;
}

bool Asset::onLoad(LoadAssetTask* task)
{
    // It may fail when task is cancelled and new one was created later (don't crash but just end with an error)
    if (task->Asset.Get() != this || Platform::AtomicRead(&_loadingTask) == 0)
        return true;
    LogContextScope logContext(GetID());

    Locker.Lock();

    // Load asset
    LoadResult result;
#if USE_EDITOR
    auto& deprecatedFlag = ContentDeprecatedFlags.Get();
    bool prevDeprecated = deprecatedFlag;
    deprecatedFlag = false;
#endif
    {
        PROFILE_CPU_ASSET(this);
        result = loadAsset();
    }
    const bool isLoaded = result == LoadResult::Ok;
    const bool failed = !isLoaded;
#if USE_EDITOR
    const bool isDeprecated = deprecatedFlag;
    deprecatedFlag = prevDeprecated;
#endif
    Platform::AtomicStore(&_loadState, (int64)(isLoaded ? LoadState::Loaded : LoadState::LoadFailed));
    if (failed)
    {
        LOG(Error, "Loading asset \'{0}\' result: {1}.", ToString(), ToString(result));
    }

    // Unlink task
    Platform::AtomicStore(&_loadingTask, 0);
    ASSERT(failed || IsLoaded() == isLoaded);

    Locker.Unlock();

    // Send event
    if (isLoaded)
    {
        // Register event `OnLoaded` invoke on main thread
        // We don't want to fire it here because current thread is content loader.
        // This allows to reduce mutexes and locks (max one frame delay isn't hurting but provides more safety)
        Content::onAssetLoaded(this);
    }
    
#if USE_EDITOR
    // Auto-save deprecated assets to get rid of data in an old format
    if (isDeprecated && isLoaded)
    {
        PROFILE_CPU_NAMED("Asset.Save");
        LOG(Info, "Resaving asset '{}' that uses deprecated data format", ToString());
        if (Save())
        {
            LOG(Error, "Failed to resave asset '{}'", ToString());
        }
    }
#endif

    return failed;
}

void Asset::onLoaded()
{
    if (IsInMainThread())
    {
        onLoaded_MainThread();
    }
    else if (OnLoaded.IsBinded())
    {
        Function<void()> action;
        action.Bind<Asset, &Asset::onLoaded>(this);
        Task::StartNew(New<MainThreadActionTask>(action, this));
    }
}

void Asset::onLoaded_MainThread()
{
    ASSERT(IsInMainThread());

    // Send event
    OnLoaded(this);
}

void Asset::onUnload_MainThread()
{
    // Note: asset should not be locked now (Locker should be free) so other thread won't be locked.

    ASSERT(IsInMainThread());

    // Cancel any streaming before calling OnUnloaded event
    CancelStreaming();

    // Send event
    OnUnloaded(this);
}

#if USE_EDITOR

bool Asset::OnCheckSave(const StringView& path) const
{
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
        if (path.IsEmpty())
            return false;
    }
    if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    if (IsVirtual() && path.IsEmpty())
    {
        LOG(Error, "To save virtual asset asset you need to specify the target asset path location.");
        return true;
    }
    return false;
}

#endif
