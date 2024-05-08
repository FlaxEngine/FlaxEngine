// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Core/Collections/BitArray.h"

class MemoryReadStream;
class MemoryWriteStream;

/// <summary>
/// The skinned model skeleton bones boolean masking data.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkeletonMask : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(SkeletonMask, 2);
private:
    Array<String> _maskedNodes;
    BitArray<> _mask;

public:
    /// <summary>
    /// The referenced skinned model skeleton that defines the masked nodes hierarchy.
    /// </summary>
    API_FIELD() AssetReference<SkinnedModel> Skeleton;

    /// <summary>
    /// Gets the per-skeleton node mask (by name).
    /// </summary>
    /// <returns>The masked nodes names list.</returns>
    API_PROPERTY() const Array<String>& GetMaskedNodes() const
    {
        return _maskedNodes;
    }

    /// <summary>
    /// Sets the per-skeleton node mask (by name).
    /// </summary>
    /// <param name="value">The masked nodes names list.</param>
    API_PROPERTY() void SetMaskedNodes(const Array<String>& value)
    {
        _maskedNodes = value;
        _mask.Resize(0);
    }

public:
    /// <summary>
    /// Gets the per-skeleton-node boolean mask (read-only).
    /// </summary>
    /// <returns>The constant reference to the skeleton nodes mask.</returns>
    API_PROPERTY() const BitArray<>& GetNodesMask();

#if USE_EDITOR

    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(const StringView& path = StringView::Empty);

#endif

private:
    void OnSkeletonUnload();

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override
    {
        BinaryAsset::GetReferences(assets, files);
        assets.Add(Skeleton.GetID());
    }
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
