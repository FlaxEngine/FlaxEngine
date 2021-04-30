// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "AnimationGraph.h"
#if USE_EDITOR
#include "AnimationGraphFunction.h"
#endif
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"

REGISTER_BINARY_ASSET(AnimationGraph, "FlaxEngine.AnimationGraph", false);

AnimationGraph::AnimationGraph(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , Graph(this)
    , GraphExecutor(Graph)
{
}

Asset::LoadResult AnimationGraph::load()
{
    // Get stream with graph data
    const auto surfaceChunk = GetChunk(0);
    if (surfaceChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());

    // Load graph
    if (Graph.Load(&stream, USE_EDITOR))
    {
        LOG(Warning, "Failed to load animation graph \'{0}\'", ToString());
        return LoadResult::Failed;
    }

#if USE_EDITOR
    // Find asset dependencies to nested anim graph functions
    ClearDependencies();
    FindDependencies(&Graph);
#endif

    return LoadResult::Ok;
}

void AnimationGraph::unload(bool isReloading)
{
    Graph.Clear();
}

AssetChunksFlag AnimationGraph::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

#if USE_EDITOR

void AnimationGraph::OnDependencyModified(BinaryAsset* asset)
{
    BinaryAsset::OnDependencyModified(asset);

    Reload();
}

#endif

BytesContainer AnimationGraph::LoadSurface()
{
    ScopeLock lock(Locker);
    if (!LoadChunks(GET_CHUNK_FLAG(0)))
    {
        const auto data = GetChunk(0);
        BytesContainer result;
        result.Copy(data->Data);
        return result;
    }

    LOG(Warning, "Animation Graph \'{0}\' surface data is missing.", GetPath());
    return BytesContainer();
}

#if USE_EDITOR

bool AnimationGraph::SaveSurface(BytesContainer& data)
{
    // Wait for asset to be loaded or don't if last load failed
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
    }
    else if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }

    ScopeLock lock(Locker);

    // Release all chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);

    // Set Visject Surface data
    auto visjectSurfaceChunk = GetOrCreateChunk(0);
    ASSERT(visjectSurfaceChunk != nullptr);
    visjectSurfaceChunk->Data.Copy(data);

    // Save
    AssetInitData assetData;
    assetData.SerializedVersion = 1;
    if (SaveAsset(assetData))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

void AnimationGraph::FindDependencies(AnimGraphBase* graph)
{
    for (const auto& node : graph->Nodes)
    {
        if (node.Type == GRAPH_NODE_MAKE_TYPE(9, 24))
        {
            const auto function = node.Assets[0].As<AnimationGraphFunction>();
            if (function)
            {
                AddDependency(function);
            }
        }
    }

    for (auto* subGraph : graph->SubGraphs)
    {
        FindDependencies(subGraph);
    }
}

#endif
