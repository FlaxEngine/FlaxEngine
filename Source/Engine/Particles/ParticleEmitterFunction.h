// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Graph/CPU/ParticleEmitterGraph.CPU.h"
#if COMPILE_WITH_PARTICLE_GPU_GRAPH
#include "Graph/GPU/ParticleEmitterGraph.GPU.h"
#endif

/// <summary>
/// Particle function graph asset that contains reusable part of the particle emitter graph.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API ParticleEmitterFunction : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(ParticleEmitterFunction, 1);
public:
    /// <summary>
    /// The loaded CPU particle function graph.
    /// </summary>
    ParticleEmitterGraphCPU Graph;

    /// <summary>
    /// The input nodes (indices).
    /// </summary>
    Array<int32, FixedAllocation<16>> Inputs;

    /// <summary>
    /// The output nodes (indices).
    /// </summary>
    Array<int32, FixedAllocation<16>> Outputs;

#if COMPILE_WITH_PARTICLE_GPU_GRAPH
    /// <summary>
    /// The loaded GPU particle function graph.
    /// </summary>
    ParticleEmitterGraphGPU GraphGPU;
#endif

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <param name="graph">The graph to load.</param>
    /// <param name="loadMeta">True if load metadata.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadSurface(ParticleEmitterGraphCPU& graph, bool loadMeta = false);

#if USE_EDITOR
    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadSurface();

#if COMPILE_WITH_PARTICLE_GPU_GRAPH
    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <param name="graph">The graph to load.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadSurface(ParticleEmitterGraphGPU& graph) const;
#endif

    // Gets the function signature for Visject Surface editor.
    API_FUNCTION() void GetSignature(API_PARAM(Out) Array<StringView, FixedAllocation<32>>& types, API_PARAM(Out) Array<StringView, FixedAllocation<32>>& names);

    /// <summary>
    /// Updates the particle graph surface (save new one, discards cached data, reloads asset).
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
