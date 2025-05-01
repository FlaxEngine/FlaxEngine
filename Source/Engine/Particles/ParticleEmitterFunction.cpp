// Copyright (c) Wojciech Figat. All rights reserved.

#include "ParticleEmitterFunction.h"
#include "Particles.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#endif
#include "Engine/Content/Factories/BinaryAssetFactory.h"

void InitParticleEmitterFunctionCall(const Guid& assetId, AssetReference<Asset>& asset, bool& usesParticleData, ParticleLayout& layout)
{
    const auto function = Content::Load<ParticleEmitterFunction>(assetId);
    asset = function;
    if (function)
    {
        // Insert any used particle data into the calling graph
        for (const ParticleAttribute& e : function->Graph.Layout.Attributes)
        {
            if (layout.FindAttribute(e.Name, e.ValueType) == -1)
                layout.AddAttribute(e.Name, e.ValueType);
        }

        // Detect if function needs to be evaluated per-particle
        for (int32 i = 0; i < function->Outputs.Count() && !usesParticleData; i++)
        {
            usesParticleData = function->Graph.Nodes[function->Outputs.Get()[i]].UsesParticleData;
        }
    }
}

REGISTER_BINARY_ASSET(ParticleEmitterFunction, "FlaxEngine.ParticleEmitterFunction", false);

ParticleEmitterFunction::ParticleEmitterFunction(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Asset::LoadResult ParticleEmitterFunction::load()
{
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);

    // Load graph
    const auto surfaceChunk = GetChunk(0);
    if (!surfaceChunk || !surfaceChunk->IsLoaded())
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());
    if (Graph.Load(&stream, USE_EDITOR))
        return LoadResult::Failed;
    for (int32 i = 0; i < Graph.Nodes.Count(); i++)
    {
        // Initialize all used nodes (starting from function output as roots)
        if (Graph.Nodes[i].Type == GRAPH_NODE_MAKE_TYPE(16, 2))
        {
            Graph.InitializeNode(&Graph.Nodes[i]);
        }
    }
#if COMPILE_WITH_PARTICLE_GPU_GRAPH
    stream.SetPosition(0);
    if (GraphGPU.Load(&stream, false))
        return LoadResult::Failed;
#endif

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

    return LoadResult::Ok;
}

void ParticleEmitterFunction::unload(bool isReloading)
{
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);
    Graph.Clear();
#if COMPILE_WITH_PARTICLE_GPU_GRAPH
    GraphGPU.Clear();
#endif
    Inputs.Clear();
    Outputs.Clear();
}

AssetChunksFlag ParticleEmitterFunction::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}

bool ParticleEmitterFunction::LoadSurface(ParticleEmitterGraphCPU& graph, bool loadMeta)
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

#if USE_EDITOR

BytesContainer ParticleEmitterFunction::LoadSurface()
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

#if COMPILE_WITH_PARTICLE_GPU_GRAPH

bool ParticleEmitterFunction::LoadSurface(ParticleEmitterGraphGPU& graph) const
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
            return graph.Load(&stream, false);
        }
    }
    return true;
}

#endif

void ParticleEmitterFunction::GetSignature(Array<StringView, FixedAllocation<32>>& types, Array<StringView, FixedAllocation<32>>& names)
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

bool ParticleEmitterFunction::SaveSurface(const BytesContainer& data) const
{
    if (OnCheckSave())
        return true;
    ConcurrentSystemLocker::WriteScope systemScope(Particles::SystemLocker);
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

bool ParticleEmitterFunction::Save(const StringView& path)
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
