// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        auto asset = Asset.Get();
        if (asset)
        {
            asset->Locker.Lock();
            if (Platform::AtomicRead(&asset->_loadingTask) == (intptr)this)
            {
                Platform::AtomicStore(&asset->_loadState, (int64)Asset::LoadState::LoadFailed);
                Platform::AtomicStore(&asset->_loadingTask, 0);
                LOG(Error, "Loading asset \'{0}\' result: {1}.", ToString(), ToString(Result::TaskFailed));
            }
            asset->Locker.Unlock();
        }
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
        auto asset = Asset.Get();
        if (asset)
        {
            Asset = nullptr;
            asset->Locker.Lock();
            if (Platform::AtomicRead(&asset->_loadingTask) == (intptr)this)
                Platform::AtomicStore(&asset->_loadingTask, 0);
            asset->Locker.Unlock();
        }

        // Base
        ContentLoadTask::OnFail();
    }

    void OnEnd() override
    {
        auto asset = Asset.Get();
        if (asset)
        {
            Asset = nullptr;
            asset->Locker.Lock();
            if (Platform::AtomicRead(&asset->_loadingTask) == (intptr)this)
                Platform::AtomicStore(&asset->_loadingTask, 0);
            asset->Locker.Unlock();
            asset = nullptr;
        }

        // Base
        ContentLoadTask::OnEnd();
    }
};
