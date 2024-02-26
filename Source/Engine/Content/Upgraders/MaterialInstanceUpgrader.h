// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Core/Core.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Material Instance Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class MaterialInstanceUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="MaterialInstanceUpgrader"/> class.
    /// </summary>
    MaterialInstanceUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 3, &Upgrade_1_To_3 },
            { 3, 4, &Upgrade_3_To_4 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //                  Version 4:
    // Designed: 5/18/2017
    // Custom Data: not used
    // Chunk 0:
    //   - Parent material: Guid
    //   - Material Params
    // ============================================
    //                  Version 3:
    // Designed: 1/24/2016
    // Custom Data: not used
    // Chunk 0:
    //   - Magic Code: int32 ('MI\0@', 1073760589)
    //   - Version: int32
    //   - Parent material: Guid
    //   - Material Params
    //   - Ending char: 1 byte (~)
    // ============================================

    static bool Upgrade_1_To_3(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 3);

        auto chunk = context.Input.Header.Chunks[0];
        if (chunk == nullptr || chunk->IsMissing())
        {
            LOG(Warning, "Missing material instance data chunk.");
            return true;
        }
        if (context.AllocateChunk(0))
            return true;
        context.Output.Header.Chunks[0]->Data.Copy(chunk->Data);

        return false;
    }

    static bool Upgrade_3_To_4(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 3 && context.Output.SerializedVersion == 4);

        auto chunk = context.Input.Header.Chunks[0];
        if (chunk == nullptr || chunk->IsMissing())
        {
            LOG(Warning, "Missing material instance data chunk.");
            return true;
        }
        if (context.AllocateChunk(0))
            return true;
        MemoryReadStream inStream(chunk->Data.Get(), chunk->Data.Length());
        MemoryWriteStream outStream(chunk->Size());

        // Header
        int32 magicCode;
        inStream.ReadInt32(&magicCode);
        int32 version;
        inStream.ReadInt32(&version);
        if (magicCode != 1073760589 || version != 3)
        {
            LOG(Warning, "Invalid material instance header.");
            return true;
        }

        // Parent material + Params
        // TODO: add tool function to copy stream -> stream
        while (inStream.CanRead())
        {
            byte b = inStream.ReadByte();
            outStream.WriteByte(b);
        }
        context.Output.Header.Chunks[0]->Data.Copy(outStream.GetHandle(), outStream.GetPosition());

        return false;
    }
};

#endif
