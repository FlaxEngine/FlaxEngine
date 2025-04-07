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
    ScopeLock lock(Locker);

    bool result;
    if (IsVirtual())
    {
        FlaxChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
        Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
        FlaxChunk chunk;
        tmpChunks[0] = &chunk;
        tmpChunks[0]->Data.Link(Data);

        AssetInitData initData;
        initData.SerializedVersion = SerializedVersion;
        Platform::MemoryCopy(_header.Chunks, tmpChunks, sizeof(_header.Chunks));
        result = path.HasChars() ? SaveAsset(path, initData) : SaveAsset(initData, true);
        Platform::MemoryClear(_header.Chunks, sizeof(_header.Chunks));
    }
    else
    {
        auto chunk0 = GetChunk(0);
        chunk0->Data.Link(Data);

        AssetInitData initData;
        initData.SerializedVersion = SerializedVersion;
        result = path.HasChars() ? SaveAsset(path, initData) : SaveAsset(initData, true);

        chunk0->Data.Unlink();
    }

    return result;
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
