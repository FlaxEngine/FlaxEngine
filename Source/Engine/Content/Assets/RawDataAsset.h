// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
#if USE_EDITOR

    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool Save(const StringView& path = StringView::Empty);

#endif

public:
    // [BinaryAsset]
    uint64 GetMemoryUsage() const override;

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
