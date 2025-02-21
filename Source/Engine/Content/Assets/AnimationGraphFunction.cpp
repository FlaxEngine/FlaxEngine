// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "AnimationGraphFunction.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Animations/Animations.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Threading/Threading.h"

REGISTER_BINARY_ASSET(AnimationGraphFunction, "FlaxEngine.AnimationGraphFunction", false);

AnimationGraphFunction::AnimationGraphFunction(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Asset::LoadResult AnimationGraphFunction::load()
{
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);

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
    ProcessGraphForSignature(&graph, true);
    if (Inputs.Count() >= 16 || Outputs.Count() >= 16)
    {
        LOG(Error, "Too many function inputs/outputs in '{0}'. The limit is max 16 inputs and max 16 outputs.", ToString());
    }

    return LoadResult::Ok;
}

void AnimationGraphFunction::unload(bool isReloading)
{
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);
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
    if (WaitForLoaded())
        return result;
    ScopeLock lock(Locker);
    result.Link(GraphData);
    return result;
}

#if USE_EDITOR

void AnimationGraphFunction::GetSignature(Array<StringView, FixedAllocation<32>>& types, Array<StringView, FixedAllocation<32>>& names)
{
    ScopeLock lock(Locker);
    types.Resize(32);
    names.Resize(32);
    for (int32 i = 0, j = 0; i < Inputs.Count(); i++)
    {
        auto& input = Inputs[i];
        if (input.InputIndex != i)
            continue;
        types[j] = input.Type;
        names[j] = input.Name;
        j++;
    }
    for (int32 i = 0; i < Outputs.Count(); i++)
    {
        auto& output = Outputs[i];
        types[i + 16] = output.Type;
        names[i + 16] = output.Name;
    }
}

bool AnimationGraphFunction::SaveSurface(const BytesContainer& data)
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

void AnimationGraphFunction::ProcessGraphForSignature(AnimGraphBase* graph, bool canUseOutputs)
{
    ScopeLock lock(Locker);
    for (int32 i = 0; i < graph->Nodes.Count(); i++)
    {
        auto& node = graph->Nodes[i];

        if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 1)) // Function Input
        {
            if (Inputs.Count() < 16)
            {
                // If there already is an input with that name just batch them together
                int32 inputIndex = Inputs.Count();
                auto name = (StringView)node.Values[1];
                for (auto& e : Inputs)
                {
                    if (e.Name == name)
                    {
                        inputIndex = e.InputIndex;
                        break;
                    }
                }

                auto& p = Inputs.AddOne();
                p.InputIndex = inputIndex;
                p.NodeIndex = i;
#if USE_EDITOR
                p.Type = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
#endif
                p.Name = name;
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 2)) // Function Output
        {
            if (Outputs.Count() < 16 && canUseOutputs)
            {
                auto& p = Outputs.AddOne();
                p.InputIndex = i;
                p.NodeIndex = i;
#if USE_EDITOR
                p.Type = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
#endif
                p.Name = (StringView)node.Values[1];
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(9, 18)) // State Machine
        {
            if (node.Data.StateMachine.Graph)
            {
                ProcessGraphForSignature(node.Data.StateMachine.Graph, false);
            }
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(9, 20)) // State
        {
            if (node.Data.State.Graph)
            {
                ProcessGraphForSignature(node.Data.State.Graph, false);
            }
        }
    }
    for (auto& stateTransition : graph->StateTransitions)
    {
        if (stateTransition.RuleGraph)
        {
            ProcessGraphForSignature(stateTransition.RuleGraph, false);
        }
    }
}
