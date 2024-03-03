// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Content/Assets/VisualScript.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Creating visual script asset utility
/// </summary>
class CreateVisualScript
{
public:
    /// <summary>
    /// Creates the asset.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context)
    {
        ASSERT(context.CustomArg);
        const auto baseTypename = static_cast<String*>(context.CustomArg);

        // Base
        IMPORT_SETUP(VisualScript, 1);

        // Chunk 0 - Visject Surface
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        {
            const VisualScriptGraph graph;
            MemoryWriteStream stream(64);
            graph.Save(&stream, true);
            context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());
        }

        // Chunk 1 - Visual Script Metadata
        if (context.AllocateChunk(1))
            return CreateAssetResult::CannotAllocateChunk;
        {
            MemoryWriteStream stream(256);
            stream.WriteInt32(1);
            stream.WriteString(*baseTypename, 31);
            stream.WriteInt32((int32)VisualScript::Flags::None);
            context.Data.Header.Chunks[1]->Data.Copy(stream.GetHandle(), stream.GetPosition());
        }

        return CreateAssetResult::Ok;
    }
};

#endif
