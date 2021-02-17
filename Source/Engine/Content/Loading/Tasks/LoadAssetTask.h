// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../ContentLoadTask.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Profiler/ProfilerCPU.h"

/// <summary>
/// Asset loading task object.
/// </summary>
class LoadAssetTask : public ContentLoadTask
{
private:

    WeakAssetReference<Asset> _asset;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="LoadAssetTask"/> class.
    /// </summary>
    /// <param name="asset">The asset to load.</param>
    LoadAssetTask(Asset* asset)
        : ContentLoadTask(Type::LoadAsset)
        , _asset(asset)
    {
    }

public:

    /// <summary>
    /// Gets the asset.
    /// </summary>
    /// <returns>The asset.</returns>
    FORCE_INLINE Asset* GetAsset() const
    {
        return _asset.Get();
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
        PROFILE_CPU();

        AssetReference<Asset> ref = _asset.Get();
        if (ref == nullptr)
            return Result::MissingReferences;

        // Call loading
        if (ref->onLoad(this))
            return Result::AssetLoadError;

        return Result::Ok;
    }

    void OnEnd() override
    {
        _asset = nullptr;

        // Base
        ContentLoadTask::OnEnd();
    }
};
