// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "AnimationGraph.h"
#if USE_EDITOR
#include "AnimationGraphFunction.h"
#endif
#include "SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Animations/Animations.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"

REGISTER_BINARY_ASSET(AnimationGraph, "FlaxEngine.AnimationGraph", true);

AnimationGraph::AnimationGraph(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , Graph(this)
    , GraphExecutor(Graph)
{
}

Asset::LoadResult AnimationGraph::load()
{
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);

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
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);
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

bool AnimationGraph::InitAsAnimation(SkinnedModel* baseModel, Animation* anim, bool loop, bool rootMotion)
{
    if (!IsVirtual())
    {
        LOG(Warning, "Only virtual Anim Graph can be modified.");
        return true;
    }
    if (!baseModel || !anim)
    {
        Log::ArgumentNullException();
        return true;
    }
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);

    // Create Graph data
    MemoryWriteStream writeStream(512);
    {
        AnimGraph graph(nullptr);
        graph.Nodes.Resize(2);
        auto& rootNode = graph.Nodes[0];
        rootNode.Type = GRAPH_NODE_MAKE_TYPE(9, 1);
        rootNode.ID = 1;
        rootNode.Values.Resize(1);
        rootNode.Values[0] = (int32)(rootMotion ? RootMotionExtraction::Enable : RootMotionExtraction::Ignore);
        rootNode.Boxes.Resize(1);
        rootNode.Boxes[0] = AnimGraphBox(&rootNode, 0, VariantType::Void);
        auto& animNode = graph.Nodes[1];
        animNode.Type = GRAPH_NODE_MAKE_TYPE(9, 2);
        animNode.ID = 2;
        animNode.Values.Resize(4);
        animNode.Values[0] = anim->GetID();
        animNode.Values[1] = 1.0f;
        animNode.Values[2] = loop;
        animNode.Values[3] = 0.0f;
        animNode.Boxes.Resize(8);
        animNode.Boxes[0] = AnimGraphBox(&animNode, 0, VariantType::Void);
        animNode.Boxes[0].Connections.Add(&rootNode.Boxes[0]);
        rootNode.Boxes[0].Connections.Add(&animNode.Boxes[0]);
        animNode.Boxes[1] = AnimGraphBox(&animNode, 1, VariantType::Void);
        animNode.Boxes[2] = AnimGraphBox(&animNode, 2, VariantType::Void);
        animNode.Boxes[3] = AnimGraphBox(&animNode, 3, VariantType::Void);
        animNode.Boxes[4] = AnimGraphBox(&animNode, 4, VariantType::Void);
        animNode.Boxes[5] = AnimGraphBox(&animNode, 5, VariantType::Void);
        animNode.Boxes[6] = AnimGraphBox(&animNode, 6, VariantType::Void);
        animNode.Boxes[7] = AnimGraphBox(&animNode, 7, VariantType::Void);
        graph.Parameters.Resize(1);
        AnimGraphParameter& baseModelParam = graph.Parameters[0];
        baseModelParam.Identifier = ANIM_GRAPH_PARAM_BASE_MODEL_ID;
        baseModelParam.Type = VariantType::Asset;
        baseModelParam.IsPublic = false;
        baseModelParam.Value = baseModel->GetID();
        if (graph.Save(&writeStream, USE_EDITOR))
            return true;
    }

    // Load Graph data (with initialization)
    ScopeLock lock(Locker);
    MemoryReadStream readStream(writeStream.GetHandle(), writeStream.GetPosition());
    return Graph.Load(&readStream, USE_EDITOR);
}

BytesContainer AnimationGraph::LoadSurface()
{
    if (!IsVirtual() && WaitForLoaded())
        return BytesContainer();
    ScopeLock lock(Locker);

    if (IsVirtual())
    {
        // Serialize runtime graph
        MemoryWriteStream stream(512);
        if (!Graph.Save(&stream, USE_EDITOR))
        {
            BytesContainer result;
            result.Copy(stream.GetHandle(), stream.GetPosition());
            return result;
        }
    }

    // Load data from asset
    if (!LoadChunks(GET_CHUNK_FLAG(0)))
    {
        const auto data = GetChunk(0);
        BytesContainer result;
        result.Copy(data->Data);
        return result;
    }

    LOG(Warning, "Animation Graph \'{0}\' surface data is missing.", ToString());
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
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);
    ScopeLock lock(Locker);

    if (IsVirtual())
    {
        MemoryReadStream readStream(data.Get(), data.Length());
        return Graph.Load(&readStream, USE_EDITOR);
    }

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

void AnimationGraph::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    BinaryAsset::GetReferences(assets, files);
    Graph.GetReferences(assets);
}

#endif
