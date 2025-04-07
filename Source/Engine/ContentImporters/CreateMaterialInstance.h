// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Graphics/Materials/MaterialParams.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Creating material instances utility.
/// </summary>
class CreateMaterialInstance
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
        IMPORT_SETUP(MaterialInstance, 4);

        // Chunk 0 - Header
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        MemoryWriteStream stream(256);
        stream.Write(Guid::Empty);
        MaterialParams::Save(&stream, nullptr);
        context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

        return CreateAssetResult::Ok;
    }
};

#endif
