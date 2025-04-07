// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Particles/ParticleSystem.h"

/// <summary>
/// Creating particle system asset utility.
/// </summary>
class CreateParticleSystem
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
        IMPORT_SETUP(ParticleSystem, 1);

        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        MemoryWriteStream stream(64);
        {
            // Create empty system
            stream.WriteInt32(2);
            stream.WriteFloat(60.0f); // FramesPerSecond
            stream.WriteInt32(5 * 60); // DurationFrames
            stream.WriteInt32(0); // Emitters Count
            stream.WriteInt32(0); // Tracks Count
        }
        context.Data.Header.Chunks[0]->Data.Copy(ToSpan(stream));

        return CreateAssetResult::Ok;
    }
};

#endif
