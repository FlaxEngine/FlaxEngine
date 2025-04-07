// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Particles/ParticleEmitter.h"

/// <summary>
/// Creating particle emitter asset utility.
/// </summary>
class CreateParticleEmitter
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
        IMPORT_SETUP(ParticleEmitter, 20);
        context.SkipMetadata = true;

        // Set Custom Data with Header
        ShaderStorage::Header20 shaderHeader;
        Platform::MemoryClear(&shaderHeader, sizeof(shaderHeader));
        context.Data.CustomData.Copy(&shaderHeader);

        // Create default layer
        if (context.AllocateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
            return CreateAssetResult::CannotAllocateChunk;
        ParticleEmitterGraphCPU graph;
        graph.CreateDefault();
        MemoryWriteStream stream(512);
        graph.Save(&stream, false);
        context.Data.Header.Chunks[SHADER_FILE_CHUNK_VISJECT_SURFACE]->Data.Copy(ToSpan(stream));

        return CreateAssetResult::Ok;
    }
};

#endif
