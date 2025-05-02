// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"

/// <summary>
/// Raw bytes container asset.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API RawDataAsset : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(RawDataAsset, 1);
public:
    /// <summary>
    /// The bytes array stored in the asset.
    /// </summary>
    API_FIELD() Array<byte> Data;

public:
    // [BinaryAsset]
#if USE_EDITOR
    bool Save(const StringView& path = StringView::Empty) override;
#endif
    uint64 GetMemoryUsage() const override;

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
