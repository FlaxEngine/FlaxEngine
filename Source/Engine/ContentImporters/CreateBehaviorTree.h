// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/AI/BehaviorTree.h"

/// <summary>
/// Creating animation utility
/// </summary>
class CreateBehaviorTree
{
public:
    static CreateAssetResult Create(CreateAssetContext& context)
    {
        // Base
        IMPORT_SETUP(BehaviorTree, 1);

        // Chunk 0 - Visject Surface
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        {
            const BehaviorTreeGraph graph;
            MemoryWriteStream stream(64);
            graph.Save(&stream, true);
            context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));
        }

        return CreateAssetResult::Ok;
    }
};

#endif
