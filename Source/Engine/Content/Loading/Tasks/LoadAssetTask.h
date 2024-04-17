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
        : ContentLoadTask(Type::LoadAsset)
        , Asset(asset)
    {
    }

    ~LoadAssetTask()
    {
        if (Asset)
        {
            Asset->Locker.Lock();
            if (Platform::AtomicRead(&Asset->_loadingTask) == (intptr)this)
            {
                Platform::AtomicStore(&Asset->_loadState, (int64)Asset::LoadState::LoadFailed);
                Platform::AtomicStore(&Asset->_loadingTask, 0);
                LOG(Error, "Loading asset \'{0}\' result: {1}.", ToString(), ToString(Result::TaskFailed));
            }
            Asset->Locker.Unlock();
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
        if (Asset)
        {
            Asset->Locker.Lock();
            if (Platform::AtomicRead(&Asset->_loadingTask) == (intptr)this)
                Platform::AtomicStore(&Asset->_loadingTask, 0);
            Asset->Locker.Unlock();
            Asset = nullptr;
        }

        // Base
        ContentLoadTask::OnFail();
    }
    void OnEnd() override
    {
        if (Asset)
        {
            Asset->Locker.Lock();
            if (Platform::AtomicRead(&Asset->_loadingTask) == (intptr)this)
                Platform::AtomicStore(&Asset->_loadingTask, 0);
            Asset->Locker.Unlock();
            Asset = nullptr;
        }

        // Base
        ContentLoadTask::OnEnd();
    }
};
