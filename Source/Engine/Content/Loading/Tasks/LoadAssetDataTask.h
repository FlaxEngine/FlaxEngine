// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../ContentLoadTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/BinaryAsset.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Profiler/ProfilerCPU.h"

/// <summary>
/// Asset data loading task object.
/// </summary>
class LoadAssetDataTask : public ContentLoadTask
{
private:
    WeakAssetReference<BinaryAsset> _asset; // Don't keep ref to the asset (so it can be unloaded if none using it, task will fail then)
    AssetChunksFlag _chunks;
    FlaxStorage::LockData _dataLock;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="LoadAssetDataTask"/> class.
    /// </summary>
    /// <param name="asset">The asset to load.</param>
    /// <param name="chunks">The chunks to load.</param>
    LoadAssetDataTask(BinaryAsset* asset, AssetChunksFlag chunks)
        : _asset(asset)
        , _chunks(chunks)
        , _dataLock(asset->Storage->Lock())
    {
    }

public:
    // [ContentLoadTask]
    String ToString() const override
    {
        return String::Format(TEXT("Load Asset Data Task ({}, {}, {})"), (int32)GetState(), _chunks, _asset ? _asset->GetPath() : String::Empty);
    }
    bool HasReference(Object* obj) const override
    {
        return obj == _asset;
    }

protected:
    // [ContentLoadTask]
    Result run() override
    {
        if (IsCancelRequested())
            return Result::Ok;
        PROFILE_CPU();

        AssetReference<BinaryAsset> ref = _asset.Get();
        if (ref == nullptr)
            return Result::MissingReferences;
#if TRACY_ENABLE
        const StringView name(ref->GetPath());
#endif

        // Load chunks
        for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        {
            if (GET_CHUNK_FLAG(i) & _chunks)
            {
                const auto chunk = ref->GetChunk(i);
                if (chunk != nullptr)
                {
                    if (IsCancelRequested())
                        return Result::Ok;
#if TRACY_ENABLE
                    ZoneScoped;
                    ZoneName(*name, name.Length());
                    ZoneValue(chunk->LocationInFile.Size / 1024); // Size in kB
#endif
                    if (ref->Storage->LoadAssetChunk(chunk))
                    {
                        LOG(Warning, "Cannot load asset \'{0}\' chunk {1}.", ref->ToString(), i);
                        return Result::LoadDataError;
                    }
                }
            }
        }

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
