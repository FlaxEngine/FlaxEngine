// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Asset.h"
#include "Content.h"
#include "Cache/AssetsCache.h"
#include "Loading/ContentLoadingManager.h"
#include "Loading/Tasks/LoadAssetTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Threading/ConcurrentTaskQueue.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-gc.h>

Asset::Asset(const SpawnParams& params, const AssetInfo* info)
    : ManagedScriptingObject(params)
    , _refCount(0)
    , _loadingTask(nullptr)
    , _isLoaded(false)
    , _loadFailed(false)
    , _deleteFileOnUnload(false)
    , _isVirtual(false)
{
}

String Asset::ToString() const
{
    return String::Format(TEXT("{0}: {1}, \'{2}\', Refs: {3}"), GetTypeName(), GetID(), GetPath(), GetReferencesCount());
}

void Asset::OnDeleteObject()
{
    ASSERT(IsInMainThread());

    // Send event to the gameplay so it can release handle to this asset
    // This may happen when asset gets removed but sth is still referencing it (eg. in managed code)
    if (!IsInternalType())
        Content::AssetDisposing(this);

    // Cache data
    const bool wasMarkedToDelete = _deleteFileOnUnload != 0;
    const String path = wasMarkedToDelete ? GetPath() : String::Empty;
    const Guid id = GetID();

    // Fire unload event (every object referencing this asset or it's data should release reference so later actions are safe)
    onUnload_MainThread();

    // Remove from pool
    Content::onAssetUnload(this);

    // Unload asset data (in a safe way to protect asset data)
    Locker.Lock();
    if (_isLoaded)
    {
        unload(false);
        _isLoaded = false;
    }
    Locker.Unlock();

#if BUILD_DEBUG
    // Ensure no object is referencing it (except managed instance if exists)
    //ASSERT(_refCount == (HasManagedInstance() ? 1 : 0));
#endif

    // Base (after it `this` is invalid)
    ManagedScriptingObject::OnDeleteObject();

    // Check if asset was marked to delete
    if (wasMarkedToDelete)
    {
        LOG(Info, "Deleting asset '{0}':{1}.", path, id.ToString());

        // Remove from registry
        Content::GetRegistry()->DeleteAsset(id, nullptr);

        // Delete file
        Content::deleteFileSafety(path, id);
    }
}

void Asset::CreateManaged()
{
    // Base
    ManagedScriptingObject::CreateManaged();

    // Managed objects holds a reference to this asset until it will be removed by GC
    AddReference();
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
        mono_gchandle_free(_gcHandle);
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
    // Don't allow to change asset ids
    CRASH;
}

void Asset::Reload()
{
    // It's better to call it from the main thread
    if (IsInMainThread())
    {
        LOG(Info, "Reloading asset {0}", ToString());

        WaitForLoaded();

        // Fire event
        if (!IsInternalType())
            Content::AssetReloading(this);
        OnReloading(this);

        // Fire unload event
        // TODO: maybe just call release storage ref or sth? we cannot call onUnload because managed asset objects gets invalidated
        //onUnload_MainThread();

        ScopeLock lock(Locker);

        // Unload current data
        unload(true);
        _isLoaded = false;

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

namespace ContentLoadingManagerImpl
{
    extern ConcurrentTaskQueue<ContentLoadTask> Tasks;
};

bool Asset::WaitForLoaded(double timeoutInMilliseconds)
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
            Content::tryCallOnLoaded(this);
        }

        return false;
    }

    // Check if has missing loading task
    Platform::MemoryBarrier();
    const auto loadingTask = _loadingTask;
    if (loadingTask == nullptr)
    {
        LOG(Warning, "WaitForLoaded asset \'{0}\' failed. No loading task attached and asset is not loaded.", ToString());
        return true;
    }

    PROFILE_CPU();

    // Check if call is made from the Loading Thread and task has not been taken yet
    auto thread = ContentLoadingManager::GetCurrentLoadThread();
    if (thread != nullptr)
    {
        // Note: to reproduce this case just include material into material (use layering).
        // So during loading first material it will wait for child materials loaded calling this function

        Task* task = loadingTask;
        while (!Engine::ShouldExit())
        {
            // Try to execute content tasks
            while (task->IsQueued())
            {
                // Pick this task from the queue
                ContentLoadTask* tmp;
                if (ContentLoadingManagerImpl::Tasks.try_dequeue(tmp))
                {
                    if (tmp == task)
                    {
                        thread->Run(tmp);
                    }
                    else
                    {
                        ContentLoadingManagerImpl::Tasks.enqueue(tmp);
                    }
                }
            }

            // Wait some time
            //Platform::Sleep(1);

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
        }
    }
    else
    {
        // Wait for task end
        loadingTask->Wait(timeoutInMilliseconds);
    }

    // If running on a main thread we can flush asset `Loaded` event
    if (IsInMainThread() && IsLoaded())
    {
        Content::tryCallOnLoaded(this);
    }

    return IsLoaded() == false;
}

void Asset::InitAsVirtual()
{
    // Be a virtual thing
    _isVirtual = true;

    // Be a loaded thing
    _isLoaded = true;
}

#if USE_EDITOR

void Asset::GetReferences(Array<Guid>& output) const
{
    // No refs by default
}

Array<Guid> Asset::GetReferences() const
{
    Array<Guid> result;
    GetReferences(result);
    return result;
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
    // Check if is already loaded
    if (IsLoaded())
        return;

    // Start loading (using async tasks)
    ASSERT(_loadingTask == nullptr);
    _loadingTask = createLoadingTask();
    ASSERT(_loadingTask != nullptr);
    _loadingTask->Start();
}

bool Asset::onLoad(LoadAssetTask* task)
{
    // It may fail when task is cancelled and new one is created later (don't crash but just end with an error)
    if (task->GetAsset() != this || _loadingTask == nullptr)
        return true;

    Locker.Lock();

    // Load asset
    const LoadResult result = loadAsset();
    const bool isLoaded = result == LoadResult::Ok;
    const bool failed = !isLoaded;
    _loadFailed = failed;
    _isLoaded = !failed;
    if (failed)
    {
        LOG(Error, "Loading asset \'{0}\' result: {1}.", ToString(), ToString(result));
    }

    // Unlink task
    _loadingTask = nullptr;
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

    return failed;
}

void Asset::onLoaded()
{
    if (IsInMainThread())
    {
        onLoaded_MainThread();
    }
    else
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

    // Send event
    OnUnloaded(this);

    // Check if is during loading
    if (_loadingTask != nullptr)
    {
        // Cancel loading
        auto task = _loadingTask;
        _loadingTask = nullptr;
        LOG(Warning, "Cancel loading task for \'{0}\'", ToString());
        task->Cancel();
    }
}
