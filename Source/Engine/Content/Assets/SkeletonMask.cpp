// Copyright (c) Wojciech Figat. All rights reserved.

#include "SkeletonMask.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/SkeletonMaskUpgrader.h"
#include "Engine/Threading/Threading.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(SkeletonMask, "FlaxEngine.SkeletonMask", SkeletonMaskUpgrader, true);

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
    stream.Read(skeletonId);
    int32 maskedNodesCount;
    stream.ReadInt32(&maskedNodesCount);
    _maskedNodes.Resize(maskedNodesCount);
    for (auto& e : _maskedNodes)
    {
        stream.Read(e, -13);
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
    if (!Skeleton || Skeleton->WaitForLoaded())
        return _mask;
    ScopeLock lock(Locker);
    if (_mask.IsEmpty())
    {
        _mask.Resize(Skeleton->Skeleton.Nodes.Count());
        for (int32 i = 0; i < _mask.Count(); i++)
            _mask.Set(i, _maskedNodes.Contains(Skeleton->Skeleton.Nodes[i].Name));
    }

    return _mask;
}

#if USE_EDITOR

bool SkeletonMask::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    SaveContext context(this);

    // Write data
    MemoryWriteStream stream(4096);
    const auto skeletonId = Skeleton.GetID();
    stream.Write(skeletonId);
    stream.WriteInt32(_maskedNodes.Count());
    for (auto& e : _maskedNodes)
    {
        stream.Write(e, -13);
    }
    context.LinkChunk(0, ToSpan(stream));

    // Save
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    return context.Save(path, data);
}

#endif

void SkeletonMask::OnSkeletonUnload()
{
    // Clear cached mask
    _mask.Resize(0);
}
