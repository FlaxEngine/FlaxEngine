// Copyright (c) Wojciech Figat. All rights reserved.

#include "MaterialFunction.h"
#include "Engine/Threading/Threading.h"
#if COMPILE_WITH_MATERIAL_GRAPH
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#endif
#include "Engine/Content/Factories/BinaryAssetFactory.h"

REGISTER_BINARY_ASSET(MaterialFunction, "FlaxEngine.MaterialFunction", false);

MaterialFunction::MaterialFunction(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Asset::LoadResult MaterialFunction::load()
{
#if COMPILE_WITH_MATERIAL_GRAPH
    // Load graph
    const auto surfaceChunk = GetChunk(0);
    if (!surfaceChunk || !surfaceChunk->IsLoaded())
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());
    if (Graph.Load(&stream, USE_EDITOR))
        return LoadResult::Failed;

    // Cache input and output nodes
    bool tooManyInputsOutputs = false;
    for (int32 i = 0; i < Graph.Nodes.Count(); i++)
    {
        auto& node = Graph.Nodes[i];
        if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 1))
        {
            if (Inputs.Count() < 16)
                Inputs.Add(i);
            else
                tooManyInputsOutputs = true;
        }
        else if (node.Type == GRAPH_NODE_MAKE_TYPE(16, 2))
        {
            if (Outputs.Count() < 16)
                Outputs.Add(i);
            else
                tooManyInputsOutputs = true;
        }
    }
    if (tooManyInputsOutputs)
    {
        LOG(Error, "Too many function inputs/outputs in '{0}'. The limit is max 16 inputs and max 16 outputs.", ToString());
    }
#endif

    return LoadResult::Ok;
}

void MaterialFunction::unload(bool isReloading)
{
#if COMPILE_WITH_MATERIAL_GRAPH
    Graph.Clear();
    Inputs.Clear();
    Outputs.Clear();
#endif
}

AssetChunksFlag MaterialFunction::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

#if COMPILE_WITH_MATERIAL_GRAPH

BytesContainer MaterialFunction::LoadSurface()
{
    BytesContainer result;
    if (WaitForLoaded())
        return result;
    ScopeLock lock(Locker);
    if (HasChunk(0))
    {
        if (!LoadChunks(GET_CHUNK_FLAG(0)))
        {
            const auto surfaceChunk = GetChunk(0);
            result.Copy(surfaceChunk->Data);
        }
    }
    return result;
}

bool MaterialFunction::LoadSurface(MaterialGraph& graph, bool loadMeta)
{
    if (WaitForLoaded())
        return true;
    ScopeLock lock(Locker);
    if (HasChunk(0))
    {
        if (!LoadChunks(GET_CHUNK_FLAG(0)))
        {
            const auto surfaceChunk = GetChunk(0);
            MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());
            return graph.Load(&stream, loadMeta);
        }
    }
    return true;
}

void MaterialFunction::GetSignature(Array<StringView, FixedAllocation<32>>& types, Array<StringView, FixedAllocation<32>>& names)
{
    types.Resize(32);
    names.Resize(32);
    for (int32 i = 0; i < Inputs.Count(); i++)
    {
        auto& node = Graph.Nodes[Inputs[i]];
        types[i] = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
        names[i] = (StringView)node.Values[1];
    }
    for (int32 i = 0; i < Outputs.Count(); i++)
    {
        auto& node = Graph.Nodes[Outputs[i]];
        types[i + 16] = GetGraphFunctionTypeName_Deprecated(node.Values[0]);
        names[i + 16] = (StringView)node.Values[1];
    }
}

#endif

#if USE_EDITOR

bool MaterialFunction::SaveSurface(const BytesContainer& data) const
{
    if (OnCheckSave())
        return true;
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

bool MaterialFunction::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);
    MemoryWriteStream writeStream;
    if (Graph.Save(&writeStream, true))
        return true;
    BytesContainer data;
    data.Link(ToSpan(writeStream));
    return SaveSurface(data);
}

#endif
