// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

struct DrawCall;
class GPUBuffer;
class MaterialBase;

/// <summary>
/// Terrain service used to unify and provide data sharing for various terrain instances and related logic.
/// </summary>
class TerrainManager
{
public:

    /// <summary>
    /// Gets the default terrain material to be used for rendering.
    /// </summary>
    /// <returns>The default material for terrain.</returns>
    static MaterialBase* GetDefaultTerrainMaterial();

    /// <summary>
    /// Gets the chunk geometry buffers for the given chunk size (after LOD reduction).
    /// </summary>
    /// <param name="drawCall">The draw call to setup (sets the geometry data such as vertex and index buffers to use).</param>
    /// <param name="chunkSize">The chunk size (on chunk edge) for the LOD0 chunk. Must be equal or higher than 3.</param>
    /// <param name="lodIndex">The chunk LOD.</param>
    /// <returns>True if failed to get it, otherwise false.</returns>
    static bool GetChunkGeometry(DrawCall& drawCall, int32 chunkSize, int32 lodIndex);
};
