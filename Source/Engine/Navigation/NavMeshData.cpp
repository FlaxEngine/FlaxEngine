// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavMeshData.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/WriteStream.h"
#include "Engine/Serialization/MemoryReadStream.h"

void NavMeshData::Save(WriteStream& stream)
{
    // Write header
    NavMeshDataHeader header;
    header.Version = 1;
    header.TileSize = TileSize;
    header.TilesCount = Tiles.Count();
    stream.Write(header);

    // Write tiles
    for (int32 tileIndex = 0; tileIndex < Tiles.Count(); tileIndex++)
    {
        auto& tile = Tiles.Get()[tileIndex];

        // Write tile header
        NavMeshTileDataHeader tileHeader;
        tileHeader.PosX = tile.PosX;
        tileHeader.PosY = tile.PosY;
        tileHeader.Layer = tile.Layer;
        tileHeader.DataSize = tile.Data.Length();
        stream.Write(tileHeader);

        // Write tile data
        if (tileHeader.DataSize)
        {
            stream.WriteBytes(tile.Data.Get(), tileHeader.DataSize);
        }
        else
        {
            LOG(Warning, "Empty navmesh tile data.");
        }
    }
}

bool NavMeshData::Load(BytesContainer& data, bool copyData)
{
    if (data.Length() < sizeof(NavMeshDataHeader))
    {
        LOG(Warning, "No valid navmesh data.");
        return true;
    }
    MemoryReadStream stream(data.Get(), data.Length());

    // Read header
    const auto header = stream.Move<NavMeshDataHeader>();
    if (header->Version != 1)
    {
        LOG(Warning, "Invalid valid navmesh data version {0}.", header->Version);
        return true;
    }
    if (header->TilesCount < 0 || header->TileSize < 1)
    {
        LOG(Warning, "Invalid navmesh data.");
        return true;
    }
    TileSize = header->TileSize;
    Tiles.Resize(header->TilesCount);

    // Read tiles
    for (int32 tileIndex = 0; tileIndex < Tiles.Count(); tileIndex++)
    {
        auto& tile = Tiles.Get()[tileIndex];

        // Read tile header
        const auto tileHeader = stream.Move<NavMeshTileDataHeader>();
        if (tileHeader->DataSize <= 0)
        {
            LOG(Warning, "Invalid navmesh tile data.");
            return true;
        }
        tile.PosX = tileHeader->PosX;
        tile.PosY = tileHeader->PosY;
        tile.Layer = tileHeader->Layer;

        // Read tile data
        const auto* tileData = (const byte*)stream.Move(tileHeader->DataSize);
        if (copyData)
            tile.Data.Copy(tileData, tileHeader->DataSize);
        else
            tile.Data.Link(tileData, tileHeader->DataSize);
    }

    return false;
}
