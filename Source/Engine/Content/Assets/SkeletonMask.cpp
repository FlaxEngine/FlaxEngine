// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SkeletonMask.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/SkeletonMaskUpgrader.h"

REGISTER_BINARY_ASSET(SkeletonMask, "FlaxEngine.SkeletonMask", ::New<SkeletonMaskUpgrader>(), true);

SkeletonMask::SkeletonMask(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
    Skeleton.Unload.Bind<SkeletonMask, &SkeletonMask::OnSkeletonUnload>(this);
}

Asset::LoadResult SkeletonMask::load()
{
    const auto dataChunk = GetChunk(0);
    if (dataChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(dataChunk->Get(), dataChunk->Size());

    Guid skeletonId;
    stream.Read(&skeletonId);
    int32 maskedNodesCount;
    stream.ReadInt32(&maskedNodesCount);
    _maskedNodes.Resize(maskedNodesCount);
    for (auto& e : _maskedNodes)
    {
        stream.ReadString(&e, -13);
    }
    Skeleton = skeletonId;

    return LoadResult::Ok;
}

void SkeletonMask::unload(bool isReloading)
{
    Skeleton = nullptr;
    _maskedNodes.Resize(0);
    _mask.Resize(0);
}

AssetChunksFlag SkeletonMask::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

const BitArray<>& SkeletonMask::GetNodesMask()
{
    if (_mask.IsEmpty() && Skeleton && !Skeleton->WaitForLoaded())
    {
        _mask.Resize(Skeleton->Skeleton.Nodes.Count());
        for (int32 i = 0; i < _mask.Count(); i++)
        {
            _mask.Set(i, _maskedNodes.Contains(Skeleton->Skeleton.Nodes[i].Name));
        }
    }

    return _mask;
}

#if USE_EDITOR

bool SkeletonMask::Save(const StringView& path)
{
    // Validate state
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

    ScopeLock lock(Locker);

    // Write data
    MemoryWriteStream stream(4096);
    const auto skeletonId = Skeleton.GetID();
    stream.Write(&skeletonId);
    stream.WriteInt32(_maskedNodes.Count());
    for (auto& e : _maskedNodes)
    {
        stream.WriteString(e, -13);
    }

    // Save
    bool result;
    if (IsVirtual())
    {
        FlaxChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
        Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
        FlaxChunk chunk;
        tmpChunks[0] = &chunk;
        tmpChunks[0]->Data.Link(stream.GetHandle(), stream.GetPosition());

        AssetInitData initData;
        initData.SerializedVersion = SerializedVersion;
        Platform::MemoryCopy(_header.Chunks, tmpChunks, sizeof(_header.Chunks));
        result = path.HasChars() ? SaveAsset(path, initData) : SaveAsset(initData, true);
        Platform::MemoryClear(_header.Chunks, sizeof(_header.Chunks));
    }
    else
    {
        auto chunk0 = GetChunk(0);
        chunk0->Data.Copy(stream.GetHandle(), stream.GetPosition());

        AssetInitData initData;
        initData.SerializedVersion = SerializedVersion;
        result = path.HasChars() ? SaveAsset(path, initData) : SaveAsset(initData, true);

        chunk0->Data.Unlink();
    }

    return result;
}

#endif

void SkeletonMask::OnSkeletonUnload()
{
    // Clear cached mask
    _mask.Resize(0);
}
