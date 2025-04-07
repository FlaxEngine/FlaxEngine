// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Animations/Graph/AnimGraph.h"

/// <summary>
/// Animation Graph function asset that contains reusable part of the anim graph.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API AnimationGraphFunction : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(AnimationGraphFunction, 1);
public:
    /// <summary>
    /// The loaded anim graph function graph data (serialized anim graph).
    /// </summary>
    BytesContainer GraphData;

    struct FunctionParameter
    {
        int32 InputIndex;
        int32 NodeIndex;
#if USE_EDITOR
        String Type;
#endif
        String Name;
    };

    /// <summary>
    /// The input nodes.
    /// </summary>
    Array<FunctionParameter, FixedAllocation<16>> Inputs;

    /// <summary>
    /// The output nodes.
    /// </summary>
    Array<FunctionParameter, FixedAllocation<16>> Outputs;

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadSurface() const;

#if USE_EDITOR

    // Gets the function signature for Visject Surface editor.
    API_FUNCTION() void GetSignature(API_PARAM(Out) Array<StringView, FixedAllocation<32>>& types, API_PARAM(Out) Array<StringView, FixedAllocation<32>>& names);

    /// <summary>
    /// Updates the anim graph surface (save new one, discards cached data, reloads asset).
    /// </summary>
    /// <param name="data">The surface graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data) const;

#endif

private:
    void ProcessGraphForSignature(AnimGraphBase* graph, bool canUseOutputs);

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
