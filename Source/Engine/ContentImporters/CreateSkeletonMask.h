// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Content/Assets/SkeletonMask.h"

/// <summary>
/// Creating skeleton mask asset utility.
/// </summary>
class CreateSkeletonMask
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
        IMPORT_SETUP(SkeletonMask, 2);

        // Chunk 0
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        struct Empty
        {
            Guid SkeletonId = Guid::Empty;
            int32 Size = 0;
        };
        Empty emptyData;
        context.Data.Header.Chunks[0]->Data.Copy(&emptyData);

        return CreateAssetResult::Ok;
    }
};

#endif
