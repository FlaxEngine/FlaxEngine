// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
            stream.WriteInt32(102);
            stream.WriteDouble(5 * 60.0);
            stream.WriteDouble(60.0);
            stream.WriteBool(false);
            stream.WriteString(StringView::Empty, 13);

            // Animation channels
            stream.WriteInt32(0);

            // Animation events
            stream.WriteInt32(0);

            // Nested animations
            stream.WriteInt32(0);
        }

        // Copy to asset chunk
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());

        return CreateAssetResult::Ok;
    }
};

#endif
