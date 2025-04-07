// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Array.h"

class WriteStream;

PACK_STRUCT(struct NavMeshTileDataHeader
{
    int32 PosX;
    int32 PosY;
    int32 Layer;
    int32 DataSize;
});

struct NavMeshTileData
{
    int32 PosX;
    int32 PosY;
    int32 Layer;
    BytesContainer Data;
};

struct NavMeshDataHeader
{
    int32 Version;
    float TileSize;
    int32 TilesCount;
};

class NavMeshData
{
public:
    /// <summary>
    /// The size of the navmesh tile (in world units).
    /// </summary>
    float TileSize = 0.0f;

    /// <summary>
    /// The all loaded tiles.
    /// </summary>
    Array<NavMeshTileData> Tiles;

public:
    /// <summary>
    /// Saves the navmesh tiles to the specified stream.
    /// </summary>
    /// <param name="stream">The output stream.</param>
    void Save(WriteStream& stream);

    /// <summary>
    /// Loads the navmesh tiles from the specified data source.
    /// </summary>
    /// <param name="data">The data container.</param>
    /// <param name="copyData">True if copy data into this container, otherwise will link the navmesh tiles data to the input bytes to reduce memory allocations and copies Use with caution.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Load(BytesContainer& data, bool copyData);
};
