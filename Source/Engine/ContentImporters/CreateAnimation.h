// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Content/Assets/Animation.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Creating animation utility
/// </summary>
class CreateAnimation
{
public:
    static CreateAssetResult Create(CreateAssetContext& context)
    {
        // Base
        IMPORT_SETUP(Animation, 1);

        // Serialize empty animation data to the stream
        MemoryWriteStream stream(256);
        {
            // Info
            stream.Write(102);
            stream.Write(5 * 60.0);
            stream.Write(60.0);
            stream.Write(false);
            stream.Write(StringView::Empty, 13);

            // Animation channels
            stream.Write(0);

            // Animation events
            stream.Write(0);

            // Nested animations
            stream.Write(0);
        }

        // Copy to asset chunk
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

        return CreateAssetResult::Ok;
    }
};

#endif
