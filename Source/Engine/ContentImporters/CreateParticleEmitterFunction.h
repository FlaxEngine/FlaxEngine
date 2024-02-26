// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Particles/ParticleEmitterFunction.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Creating particle graph function asset utility.
/// </summary>
class CreateParticleEmitterFunction
{
public:
    /// <summary>
    /// Creates the asset.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context)
    {
        // Base
        IMPORT_SETUP(ParticleEmitterFunction, 1);

        // Create single output function graph
        ParticleEmitterGraphCPU graph;
        auto& outputNode = graph.Nodes.AddOne();
        outputNode.ID = 1;
        outputNode.Type = GRAPH_NODE_MAKE_TYPE(16, 2);
        outputNode.Values.Resize(2);
        outputNode.Values[0] = TEXT("System.Single");
        outputNode.Values[1] = TEXT("Output");
        auto& outputBox = outputNode.Boxes.AddOne();
        outputBox.Parent = &outputNode;
        outputBox.ID = 0;
        outputBox.Type = VariantType::Float;

        // Save graph to first chunk
        MemoryWriteStream stream(512);
        if (graph.Save(&stream, true))
            return CreateAssetResult::Error;
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());

        return CreateAssetResult::Ok;
    }
};

#endif
