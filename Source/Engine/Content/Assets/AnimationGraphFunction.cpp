// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "AnimationGraphFunction.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"

REGISTER_BINARY_ASSET(AnimationGraphFunction, "FlaxEngine.AnimationGraphFunction", nullptr, false);

AnimationGraphFunction::AnimationGraphFunction(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Asset::LoadResult AnimationGraphFunction::load()
{
    // Get graph data from chunk
    const auto surfaceChunk = GetChunk(0);
    if (!surfaceChunk || !surfaceChunk->IsLoaded())
        return LoadResult::MissingDataChunk;
    GraphData.Swap(surfaceChunk->Data);

    // Load graph
    MemoryReadStream stream(GraphData.Get(), GraphData.Length());
    AnimGraph graph(this, true);
    if (graph.Load(&stream, false))
        return LoadResult::Failed;

    // Load function signature
    // Note: search also the nested state machines graphs (state output and transition rule)
    ProcessGraphForSignature(&graph, true, Array<int32, FixedAllocation<8>>());
    if (Inputs.Count() >= 16 || Outputs.Count() >= 16)
    {
        LOG(Error, "Too many function inputs/outputs in '{0}'. The limit is max 16 inputs and max 16 outputs.", ToString());
    }

    return LoadResult::Ok;
}

void AnimationGraphFunction::unload(bool isReloading)
{
    GraphData.Release();
    Inputs.Clear();
    Outputs.Clear();
}

AssetChunksFlag AnimationGraphFunction::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

BytesContainer AnimationGraphFunction::LoadSurface() const
{
    BytesContainer result;
    ScopeLock lock(Locker);
    result.Link(GraphData);
    return result;
}

#if USE_EDITOR

void AnimationGraphFunction::GetSignature(Array<StringView, FixedAllocation<32>>& types, Array<StringView, FixedAllocation<32>>& names)
{
    types.Resize(32);
    names.Resize(32);
    for (int32 i = 0; i < Inputs.Count(); i++)
    {
        auto& input = Inputs[i];
        types[i] = input.Type;
        names[i] = input.Name;
    }
    for (int32 i = 0; i < Outputs.Count(); i++)
    {
        auto& output = Outputs[i];
        types[i + 16] = output.Type;
        names[i + 16] = output.Name;
    }
}

bool AnimationGraphFunction::SaveSurface(BytesContainer& data)
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

    // Set Visject Surface data
    auto surfaceChunk = GetOrCreateChunk(0);
    ASSERT(surfaceChunk != nullptr);
    surfaceChunk->Data.Copy(data);

    // Save asset
    AssetInitData initData;
    initData.SerializedVersion = 1;
    if (SaveAsset(initData))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

void AnimationGraphFunction::ProcessGraphForSignature(AnimGraphBase* graph, bool canUseOutputs, const Array<int32, FixedAllocation<8>>& graphIndices)
{
    for (int32 i = 0; i < graph->Nodes.Count(); i++)
    {
        auto& node = graph->Nodes[i];

        if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 1)) // Function Input
        {
            if (Inputs.Count() < 16)
            {
                auto& p = Inputs.AddOne();
                p.NodeIndex = i;
                p.GraphIndices = graphIndices;
#if USE_EDITOR
                p.Type = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
                p.Name = (StringView)node.Values[1];
#endif
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 2)) // Function Output
        {
            if (Outputs.Count() < 16 && canUseOutputs)
            {
                auto& p = Outputs.AddOne();
                p.NodeIndex = i;
                p.GraphIndices = graphIndices;
#if USE_EDITOR
                p.Type = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
                p.Name = (StringView)node.Values[1];
#endif
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(9, 18)) // State Machine
        {
            if (node.Data.StateMachine.Graph)
            {
                auto subGraph = graphIndices;
                subGraph.Add(graph->SubGraphs.Find(node.Data.StateMachine.Graph));
                ASSERT(subGraph.Last() != -1);
                ProcessGraphForSignature(node.Data.StateMachine.Graph, false, subGraph);
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(9, 20)) // State
        {
            if (node.Data.State.Graph)
            {
                auto subGraph = graphIndices;
                subGraph.Add(graph->SubGraphs.Find(node.Data.State.Graph));
                ASSERT(subGraph.Last() != -1);
                ProcessGraphForSignature(node.Data.State.Graph, false, subGraph);
            }
        }
    }
    for (auto& stateTransition : graph->StateTransitions)
    {
        if (stateTransition.RuleGraph)
        {
            auto subGraph = graphIndices;
            subGraph.Add(graph->SubGraphs.Find(stateTransition.RuleGraph));
            ASSERT(subGraph.Last() != -1);
            ProcessGraphForSignature(stateTransition.RuleGraph, false, subGraph);
        }
    }
}
