// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../ContentLoadTask.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"

/// <summary>
/// Asset loading task object.
/// </summary>
class LoadAssetTask : public ContentLoadTask
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="LoadAssetTask"/> class.
    /// </summary>
    /// <param name="asset">The asset to load.</param>
    LoadAssetTask(Asset* asset)
        : Asset(asset)
    {
    }

    ~LoadAssetTask()
    {
        DereferenceAsset();
    }

public:
    WeakAssetReference<Asset> Asset;

public:
    // [ContentLoadTask]
    bool HasReference(Object* obj) const override
    {
        return obj == Asset;
    }

protected:
    // [ContentLoadTask]
    Result run() override
    {
        PROFILE_CPU();

        // Keep valid ref to the asset
        AssetReference<::Asset> ref = Asset.Get();
        if (ref == nullptr)
            return Result::MissingReferences;

        // Call loading
        if (ref->onLoad(this))
            return Result::AssetLoadError;
        return Result::Ok;
    }

    void OnFail() override
    {
        DereferenceAsset(true);

        // Base
        ContentLoadTask::OnFail();
    }

    void OnEnd() override
    {
        DereferenceAsset();

        // Base
        ContentLoadTask::OnEnd();
    }

private:
    void DereferenceAsset(bool failed = false)
    {
        auto asset = Asset.Get();
        if (asset)
        {
            asset->Locker.Lock();
            Task* task = (Task*)Platform::AtomicRead(&asset->_loadingTask);
            if (task)
            {
                do
                {
                    if (task == this)
                    {
                        Platform::AtomicStore(&asset->_loadingTask, 0);
                        if (failed)
                        {
                            Platform::AtomicStore(&asset->_loadState, (int64)Asset::LoadState::LoadFailed);
                            LOG(Error, "Loading asset \'{0}\' result: {1}.", ToString(), ToString(Result::TaskFailed));
                        }
                        break;
                    }
                    task = task->GetContinueWithTask();
                } while (task);
            }
            asset->Locker.Unlock();
        }
    }
};
