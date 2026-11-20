// Copyright (c) Wojciech Figat. All rights reserved.

#include "RawDataAsset.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Platform/FileSystem.h"
#if USE_EDITOR
#include "Engine/Threading/Threading.h"
#endif

REGISTER_BINARY_ASSET(RawDataAsset, "FlaxEngine.RawDataAsset", true);

RawDataAsset::RawDataAsset(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

#if USE_EDITOR

bool RawDataAsset::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    SaveContext context(this);
    context.LinkChunk(0, ToSpan(Data));
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    return context.Save(path, data);
}

#endif

uint64 RawDataAsset::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = BinaryAsset::GetMemoryUsage();
    result += sizeof(RawDataAsset) - sizeof(BinaryAsset);
    result += Data.Capacity();
    Locker.Unlock();
    return result;
}

Asset::LoadResult RawDataAsset::load()
{
    auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;

    // TODO: swap memory alloc pointer to optimize this asset
    Data.Set(chunk0->Data.Get(), chunk0->Data.Length());

    return LoadResult::Ok;
}

void RawDataAsset::unload(bool isReloading)
{
    Data.Resize(0);
}

AssetChunksFlag RawDataAsset::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
