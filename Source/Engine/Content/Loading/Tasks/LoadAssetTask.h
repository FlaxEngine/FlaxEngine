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

    void OnEnd() override
    {
        Asset = nullptr;

        // Base
        ContentLoadTask::OnEnd();
    }
};
