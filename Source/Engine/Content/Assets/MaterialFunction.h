// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Tools/MaterialGenerator/Types.h"

/// <summary>
/// Material function graph asset that contains reusable part of the material graph.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API MaterialFunction : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(MaterialFunction, 1);

public:
#if COMPILE_WITH_MATERIAL_GRAPH
    /// <summary>
    /// The loaded material function graph.
    /// </summary>
    MaterialGraph Graph;

    /// <summary>
    /// The input nodes (indices).
    /// </summary>
    Array<int32, FixedAllocation<16>> Inputs;

    /// <summary>
    /// The output nodes (indices).
    /// </summary>
    Array<int32, FixedAllocation<16>> Outputs;

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadSurface();

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <param name="graph">The graph to load.</param>
    /// <param name="loadMeta">True if load metadata.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadSurface(MaterialGraph& graph, bool loadMeta = false);

    // Gets the function signature for Visject Surface editor.
    API_FUNCTION() void GetSignature(API_PARAM(Out) Array<StringView, FixedAllocation<32>>& types, API_PARAM(Out) Array<StringView, FixedAllocation<32>>& names);

#endif

#if USE_EDITOR
    /// <summary>
    /// Updates the material graph surface (save new one, discards cached data, reloads asset).
    /// </summary>
    /// <param name="data">The surface graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data) const;
#endif

public:
    // [BinaryAsset]
#if USE_EDITOR
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
