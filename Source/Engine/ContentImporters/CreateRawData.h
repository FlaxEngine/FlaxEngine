// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Content/Assets/RawDataAsset.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Creating raw data asset utility
/// </summary>
class CreateRawData
{
public:
    /// <summary>
    /// Creates the raw data asset.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context)
    {
        ASSERT(context.CustomArg);
        const auto data = static_cast<BytesContainer*>(context.CustomArg);

        // Base
        IMPORT_SETUP(RawDataAsset, 1);

        // Chunk 0
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(*data);

        return CreateAssetResult::Ok;
    }
};

#endif
