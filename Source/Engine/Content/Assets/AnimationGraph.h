// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Animations/Graph/AnimGraph.h"

/// <summary>
/// The Animation Graph is used to evaluate a final pose for the animated model for the current frame.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API AnimationGraph : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(AnimationGraph, 1);
public:
    /// <summary>
    /// The animation graph.
    /// </summary>
    AnimGraph Graph;

    /// <summary>
    /// The animation graph runtime executor.
    /// </summary>
    AnimGraphExecutor GraphExecutor;

public:
    /// <summary>
    /// Gets the base model asset used for the animation preview and the skeleton layout source.
    /// </summary>
    API_PROPERTY() SkinnedModel* GetBaseModel() const
    {
        return Graph.BaseModel.Get();
    }

    /// <summary>
    /// Initializes virtual Anim Graph to play a single animation.
    /// </summary>
    /// <param name="baseModel">The base model asset.</param>
    /// <param name="anim">The animation to play.</param>
    /// <param name="loop">True if play animation in a loop.</param>
    /// <param name="rootMotion">True if apply root motion. Otherwise it will be ignored.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool InitAsAnimation(SkinnedModel* baseModel, Animation* anim, bool loop = true, bool rootMotion = false);

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The surface data or empty if failed to load it.</returns>
    API_FUNCTION() BytesContainer LoadSurface() const;

#if USE_EDITOR

    /// <summary>
    /// Updates the animation graph surface (save new one, discard cached data, reload asset).
    /// </summary>
    /// <param name="data">Stream with graph data.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data);

private:
    void FindDependencies(AnimGraphBase* graph);

#endif

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
#if USE_EDITOR
    void OnDependencyModified(BinaryAsset* asset) override;
#endif
};
